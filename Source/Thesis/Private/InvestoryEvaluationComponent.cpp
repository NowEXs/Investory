#include "InvestoryEvaluationComponent.h"
#include "InvestoryInvestmentComponent.h"
#include "InvestoryStatusComponent.h"
#include "InvestoryTurnFlowComponent.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "InvestoryEvaluation"

namespace InvestoryEvaluationCsv
{
    static FString Escape(const FString& In)
    {
        FString S = In;
        S.ReplaceInline(TEXT("\""), TEXT("\"\""));
        if (S.Contains(TEXT(",")) || S.Contains(TEXT("\n")) || S.Contains(TEXT("\r")) || S.Contains(TEXT("\"")))
        {
            return FString::Printf(TEXT("\"%s\""), *S);
        }
        return S;
    }

    static FString Bool(bool bValue)
    {
        return bValue ? TEXT("1") : TEXT("0");
    }

    static FString Side(EInvestoryOrderSide SideValue)
    {
        return SideValue == EInvestoryOrderSide::Buy ? TEXT("Buy") : TEXT("Sell");
    }

    static FString ExperienceType(EInvestoryExperienceType Type)
    {
        if (const UEnum* Enum = StaticEnum<EInvestoryExperienceType>())
        {
            return Enum->GetNameStringByValue(static_cast<int64>(Type));
        }
        return TEXT("Unknown");
    }

    static FString ScamQuality(EInvestoryScamAnswerQuality Quality)
    {
        if (const UEnum* Enum = StaticEnum<EInvestoryScamAnswerQuality>())
        {
            return Enum->GetNameStringByValue(static_cast<int64>(Quality));
        }
        return TEXT("Unknown");
    }


    static FString KnowledgeTopic(EInvestoryKnowledgeTopic Topic)
    {
        if (const UEnum* Enum = StaticEnum<EInvestoryKnowledgeTopic>())
        {
            return Enum->GetNameStringByValue(static_cast<int64>(Topic));
        }
        return TEXT("Unknown");
    }
}

UInvestoryEvaluationComponent::UInvestoryEvaluationComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UInvestoryEvaluationComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AActor* Owner = GetOwner())
    {
        if (UInvestoryInvestmentComponent* Investment = Owner->FindComponentByClass<UInvestoryInvestmentComponent>())
        {
            Investment->OnOrderChanged.AddDynamic(this, &UInvestoryEvaluationComponent::HandleOrderChanged);
            Investment->OnOrderExecuted.AddDynamic(this, &UInvestoryEvaluationComponent::HandleOrderExecuted);
            Investment->OnOrderSubmissionAttempt.AddDynamic(this, &UInvestoryEvaluationComponent::HandleOrderSubmissionAttempt);
        }
    }
}

void UInvestoryEvaluationComponent::BeginSession(float InitialMoney, int32 InitialHappiness, int32 InitialKnowledge, FString SessionLabel)
{
    SequenceCounter = 0;
    LastNewsTurn = INDEX_NONE;
    ExperienceLog.Reset();
    ScamLog.Reset();
    LearningExposureLog.Reset();
    KnowledgeExamLog.Reset();
    InvestmentLog.Reset();
    StatusHistory.Reset();
    ExposedConceptIds.Reset();
    PendingSubmissionCache.Reset();
    Progress = FInvestoryPlayerProgress();

    SessionLabel.TrimStartAndEndInline();
    if (SessionLabel.IsEmpty())
    {
        SessionLabel = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    }

    Progress.SessionId = SessionLabel;
    Progress.StartTimeIso = FDateTime::UtcNow().ToIso8601();
    Progress.InitialMoney = InitialMoney;
    Progress.InitialHappiness = InitialHappiness;
    Progress.InitialKnowledge = InitialKnowledge;
    Progress.FinalMoney = InitialMoney;
    Progress.FinalPortfolioMarketValue = 0.0f;
    Progress.FinalNetWorth = FMath::Max(0.0f, InitialMoney);
    Progress.FinalHappiness = InitialHappiness;
    Progress.FinalKnowledge = InitialKnowledge;
    Progress.CurrentYear = 1;
    bSessionStarted = true;

    AddSnapshot(TEXT("SessionStart"), InitialMoney, InitialHappiness, InitialKnowledge);
}

void UInvestoryEvaluationComponent::EnsureSessionStarted()
{
    if (bSessionStarted)
    {
        return;
    }

    float Money = 0.0f;
    int32 Happiness = 0;
    int32 Knowledge = 0;

    if (AActor* Owner = GetOwner())
    {
        if (const UInvestoryStatusComponent* Status = Owner->FindComponentByClass<UInvestoryStatusComponent>())
        {
            Money = Status->Money;
            Happiness = Status->Happiness;
            Knowledge = Status->Knowledge;
        }
    }

    BeginSession(Money, Happiness, Knowledge, FString());
}

void UInvestoryEvaluationComponent::SetTurnContext(int32 TurnNumber, int32 Year)
{
    EnsureSessionStarted();
    Progress.CurrentTurn = FMath::Max(0, TurnNumber);
    Progress.CurrentYear = FMath::Max(1, Year);
}

void UInvestoryEvaluationComponent::RecordTurnStarted(int32 TurnNumber, float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge)
{
    EnsureSessionStarted();
    Progress.CurrentTurn = FMath::Max(Progress.CurrentTurn, TurnNumber);
    AddExperience(EInvestoryExperienceType::TurnStarted, TEXT("Turn"), 0, FText::GetEmpty(), NAME_None,
        0.0f, 0, 0, true, FText::Format(LOCTEXT("TurnStarted", "เริ่มเทิร์น {0}"), FText::AsNumber(TurnNumber)));
    AddSnapshot(TEXT("TurnStart"), CurrentMoney, CurrentHappiness, CurrentKnowledge);
}

void UInvestoryEvaluationComponent::RecordEventChoice(FName EventId, int32 ChoiceIndex, FText ChoiceLabel,
    float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta,
    float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge)
{
    EnsureSessionStarted();
    ++Progress.EventChoices;
    AddExperience(EInvestoryExperienceType::EventChoice, EventId, ChoiceIndex, ChoiceLabel, NAME_None,
        MoneyDelta, HappinessDelta, KnowledgeDelta, true, FText::GetEmpty());
    AddSnapshot(TEXT("EventChoice"), CurrentMoney, CurrentHappiness, CurrentKnowledge);
}

void UInvestoryEvaluationComponent::RecordScamAnswer(FName QuestionId, int32 ChoiceIndex, EInvestoryScamAnswerQuality Quality,
    float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta,
    float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge)
{
    RecordScamAnswerDetailed(QuestionId, ChoiceIndex, FText::GetEmpty(), Quality,
        FText::GetEmpty(), FText::GetEmpty(), MoneyDelta, HappinessDelta, KnowledgeDelta,
        CurrentMoney, CurrentHappiness, CurrentKnowledge);
}

FInvestoryScamFeedback UInvestoryEvaluationComponent::RecordScamAnswerDetailed(
    FName QuestionId, int32 ChoiceIndex, FText ChoiceLabel,
    EInvestoryScamAnswerQuality Quality, FText Explanation, FText WarningSign,
    float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta,
    float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge)
{
    EnsureSessionStarted();

    ++Progress.ScamAnswered;
    switch (Quality)
    {
    case EInvestoryScamAnswerQuality::Best: ++Progress.ScamBest; break;
    case EInvestoryScamAnswerQuality::Good: ++Progress.ScamGood; break;
    default: ++Progress.ScamWrong; break;
    }

    FInvestoryScamRecord Record;
    Record.Sequence = ++SequenceCounter;
    Record.TurnNumber = Progress.CurrentTurn;
    Record.Year = Progress.CurrentYear;
    Record.QuestionId = QuestionId;
    Record.ChoiceIndex = ChoiceIndex;
    Record.ChoiceLabel = ChoiceLabel;
    Record.Quality = Quality;
    Record.Explanation = Explanation;
    Record.WarningSign = WarningSign;
    Record.MoneyDelta = MoneyDelta;
    Record.HappinessDelta = HappinessDelta;
    Record.KnowledgeDelta = KnowledgeDelta;
    ScamLog.Add(Record);

    FInvestoryScamFeedback Feedback;
    Feedback.Quality = Quality;
    Feedback.Explanation = Explanation;
    Feedback.WarningSign = WarningSign;
    Feedback.MoneyDelta = MoneyDelta;
    Feedback.HappinessDelta = HappinessDelta;
    Feedback.KnowledgeDelta = KnowledgeDelta;

    switch (Quality)
    {
    case EInvestoryScamAnswerQuality::Best:
        Feedback.ResultLabel = LOCTEXT("ScamFeedbackBest", "เลือกได้เหมาะสมที่สุด");
        break;
    case EInvestoryScamAnswerQuality::Good:
        Feedback.ResultLabel = LOCTEXT("ScamFeedbackGood", "เป็นทางเลือกที่พอใช้ แต่ยังมีความเสี่ยง");
        break;
    default:
        Feedback.ResultLabel = LOCTEXT("ScamFeedbackWrong", "ทางเลือกนี้มีความเสี่ยงสูง");
        break;
    }

    AddExperience(EInvestoryExperienceType::ScamAnswer, QuestionId, ChoiceIndex, ChoiceLabel, NAME_None,
        MoneyDelta, HappinessDelta, KnowledgeDelta, Quality != EInvestoryScamAnswerQuality::Wrong,
        Explanation.IsEmpty() ? Feedback.ResultLabel : Explanation);
    AddSnapshot(TEXT("ScamAnswer"), CurrentMoney, CurrentHappiness, CurrentKnowledge);
    return Feedback;
}

FInvestoryKnowledgeExamFeedback UInvestoryEvaluationComponent::RecordKnowledgeExamAnswer(
    FName QuestionId, EInvestoryKnowledgeTopic Topic, int32 ChoiceIndex, FText ChoiceLabel,
    bool bCorrect, float PointsEarned, float MaxPoints, FText Explanation, FText LearningPoint,
    float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge)
{
    EnsureSessionStarted();

    const float SafeMax = FMath::Max(0.0f, MaxPoints);
    const float SafeEarned = FMath::Clamp(PointsEarned, 0.0f, SafeMax);
    ++Progress.FinalExamAnswered;
    Progress.FinalExamCorrect += bCorrect ? 1 : 0;
    Progress.FinalExamPointsEarned += SafeEarned;
    Progress.FinalExamMaxPoints += SafeMax;

    FInvestoryKnowledgeExamRecord Record;
    Record.Sequence = ++SequenceCounter;
    Record.TurnNumber = Progress.CurrentTurn;
    Record.Year = Progress.CurrentYear;
    Record.QuestionId = QuestionId;
    Record.Topic = Topic;
    Record.ChoiceIndex = ChoiceIndex;
    Record.ChoiceLabel = ChoiceLabel;
    Record.bCorrect = bCorrect;
    Record.PointsEarned = SafeEarned;
    Record.MaxPoints = SafeMax;
    Record.Explanation = Explanation;
    Record.LearningPoint = LearningPoint;
    KnowledgeExamLog.Add(Record);

    AddExperience(EInvestoryExperienceType::KnowledgeExamAnswer, QuestionId, ChoiceIndex, ChoiceLabel, NAME_None,
        0.0f, 0, 0, bCorrect, Explanation);
    AddSnapshot(TEXT("KnowledgeExam"), CurrentMoney, CurrentHappiness, CurrentKnowledge);

    FInvestoryKnowledgeExamFeedback Feedback;
    Feedback.bCorrect = bCorrect;
    Feedback.PointsEarned = SafeEarned;
    Feedback.MaxPoints = SafeMax;
    Feedback.CurrentExamPercent = CalculateFinalExamScore();
    Feedback.ResultLabel = bCorrect
        ? LOCTEXT("ExamCorrect", "ตอบถูก")
        : LOCTEXT("ExamIncorrect", "ยังไม่ถูก");
    Feedback.Explanation = Explanation;
    Feedback.LearningPoint = LearningPoint;
    return Feedback;
}

float UInvestoryEvaluationComponent::GetFinalExamScore() const
{
    return CalculateFinalExamScore();
}

void UInvestoryEvaluationComponent::RecordKnowledgeExamReward(float MoneyReward,
    float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge)
{
    EnsureSessionStarted();
    const float SafeReward = FMath::Max(0.0f, MoneyReward);
    Progress.FinalExamRewardMoney += SafeReward;
    AddExperience(EInvestoryExperienceType::KnowledgeExamReward, TEXT("FinalExamReward"), 0,
        FText::GetEmpty(), NAME_None, SafeReward, 0, 0, true,
        FText::Format(LOCTEXT("ExamRewardLog", "ได้รับทุนจากแบบประเมินความรู้ {0} บาท"), FText::AsNumber(SafeReward)));
    AddSnapshot(TEXT("KnowledgeExamReward"), CurrentMoney, CurrentHappiness, CurrentKnowledge);
}

void UInvestoryEvaluationComponent::RecordLearningConceptExposure(
    FName ConceptId,
    FName SourceId,
    FText Note)
{
    EnsureSessionStarted();

    if (ConceptId.IsNone())
    {
        return;
    }

    // เคยเจอ concept นี้แล้ว ไม่เพิ่ม unique ซ้ำ
    if (ExposedConceptIds.Contains(ConceptId))
    {
        return;
    }

    ExposedConceptIds.Add(ConceptId);
    ++Progress.UniqueLearningConceptsExposed;

    FInvestoryLearningExposureRecord Record;
    Record.Sequence = ++SequenceCounter;
    Record.TurnNumber = Progress.CurrentTurn;
    Record.Year = Progress.CurrentYear;
    Record.ConceptId = ConceptId;
    Record.SourceId = SourceId;
    Record.Note = Note;
    LearningExposureLog.Add(Record);

    AddExperience(EInvestoryExperienceType::LearningConceptExposed,
        SourceId.IsNone() ? ConceptId : SourceId,
        0,
        FText::FromString(ConceptId.ToString()),
        NAME_None,
        0.0f,
        0,
        0,
        true,
        Note);
}

void UInvestoryEvaluationComponent::RecordNewsSeen(FName NewsId, FName TargetStockId, bool bBigNews,
    float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge)
{
    EnsureSessionStarted();
    LastNewsTurn = Progress.CurrentTurn;
    if (bBigNews) ++Progress.BigNewsSeen; else ++Progress.NewsSeen;

    AddExperience(bBigNews ? EInvestoryExperienceType::BigNewsSeen : EInvestoryExperienceType::NewsSeen,
        NewsId, 0, FText::GetEmpty(), TargetStockId, 0.0f, 0, 0, true, FText::GetEmpty());
    AddSnapshot(bBigNews ? TEXT("BigNews") : TEXT("News"), CurrentMoney, CurrentHappiness, CurrentKnowledge);
}

void UInvestoryEvaluationComponent::RecordShopPurchase(FName ItemId, float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta,
    float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge)
{
    EnsureSessionStarted();
    ++Progress.ShopPurchases;
    AddExperience(EInvestoryExperienceType::ShopPurchase, ItemId, 0, FText::GetEmpty(), NAME_None,
        MoneyDelta, HappinessDelta, KnowledgeDelta, true, FText::GetEmpty());
    AddSnapshot(TEXT("ShopPurchase"), CurrentMoney, CurrentHappiness, CurrentKnowledge);
}

void UInvestoryEvaluationComponent::RecordPortfolioViewed(FName StockId, int32 Shares, float UnrealizedProfitLoss,
    float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge)
{
    // Legacy compatibility: the old node assumed all portfolio details were visible.
    RecordPortfolioViewedDetailed(StockId, Shares, UnrealizedProfitLoss,
        Shares > 0, Shares > 0, Shares > 0,
        CurrentMoney, CurrentHappiness, CurrentKnowledge);
}

void UInvestoryEvaluationComponent::RecordPortfolioViewedDetailed(FName StockId, int32 Shares, float UnrealizedProfitLoss,
    bool bAverageCostVisible, bool bUnrealizedPLVisible, bool bBreakEvenVisible,
    float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge)
{
    EnsureSessionStarted();
    ++Progress.PortfolioViews;
    if (Shares > 0)
    {
        ++Progress.MeaningfulPortfolioViews;
        Progress.bExperiencedAverageCost |= bAverageCostVisible;
        Progress.bExperiencedUnrealizedPL |= bUnrealizedPLVisible;
        Progress.bExperiencedBreakEven |= bBreakEvenVisible;
    }

    AddExperience(EInvestoryExperienceType::PortfolioViewed, TEXT("Portfolio"), 0, FText::GetEmpty(), StockId,
        0.0f, 0, 0, true,
        FText::Format(LOCTEXT("PortfolioViewedDetailed", "ถือ {0} หุ้น | AvgCost {1} | Unrealized P/L {2} | Break-even {3}"),
            FText::AsNumber(Shares),
            bAverageCostVisible ? LOCTEXT("VisibleYes1", "เห็น") : LOCTEXT("VisibleNo1", "ยังไม่ปลด"),
            bUnrealizedPLVisible ? LOCTEXT("VisibleYes2", "เห็น") : LOCTEXT("VisibleNo2", "ยังไม่ปลด"),
            bBreakEvenVisible ? LOCTEXT("VisibleYes3", "เห็น") : LOCTEXT("VisibleNo3", "ยังไม่ปลด")));
    AddSnapshot(TEXT("PortfolioViewed"), CurrentMoney, CurrentHappiness, CurrentKnowledge);
}

void UInvestoryEvaluationComponent::RecordLivingExpense(bool bPaid, float Amount,
    float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge)
{
    EnsureSessionStarted();
    if (bPaid)
    {
        ++Progress.LivingExpensePaidCount;
        Progress.TotalLivingExpensesPaid += FMath::Max(0.0f, Amount);
    }
    else
    {
        ++Progress.LivingExpenseUnpaidCount;
    }

    AddExperience(EInvestoryExperienceType::LivingExpense, TEXT("LivingExpense"), 0, FText::GetEmpty(), NAME_None,
        bPaid ? -FMath::Abs(Amount) : 0.0f, 0, 0, bPaid,
        bPaid ? LOCTEXT("LivingPaid", "จ่ายค่าครองชีพสำเร็จ") : LOCTEXT("LivingUnpaid", "เงินไม่พอจ่ายค่าครองชีพ"));
    AddSnapshot(TEXT("LivingExpense"), CurrentMoney, CurrentHappiness, CurrentKnowledge);
}

void UInvestoryEvaluationComponent::RecordBurnoutEncounter(float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge)
{
    EnsureSessionStarted();
    ++Progress.BurnoutEncounters;
    AddExperience(EInvestoryExperienceType::BurnoutEncounter, TEXT("Burnout"), 0, FText::GetEmpty(), NAME_None,
        0.0f, 0, 0, false, LOCTEXT("BurnoutEncounter", "ผู้เล่นเข้าสู่ภาวะ Burnout"));
    AddSnapshot(TEXT("Burnout"), CurrentMoney, CurrentHappiness, CurrentKnowledge);
}

void UInvestoryEvaluationComponent::RecordRest(float MoneySpent, int32 HappinessGained,
    float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge)
{
    EnsureSessionStarted();
    ++Progress.RestCount;
    AddExperience(EInvestoryExperienceType::Rest, TEXT("Rest"), 0, FText::GetEmpty(), NAME_None,
        -FMath::Abs(MoneySpent), HappinessGained, 0, true, LOCTEXT("RestRecord", "เลือกพักเพื่อฟื้นความสุข"));
    AddSnapshot(TEXT("Rest"), CurrentMoney, CurrentHappiness, CurrentKnowledge);
}

void UInvestoryEvaluationComponent::RecordForcedBurnout(int32 HappinessSpent,
    float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge)
{
    EnsureSessionStarted();
    ++Progress.ForceThroughBurnoutCount;
    AddExperience(EInvestoryExperienceType::ForcedBurnout, TEXT("Burnout"), 0, FText::GetEmpty(), NAME_None,
        0.0f, -FMath::Abs(HappinessSpent), 0, true, LOCTEXT("ForcedBurnoutRecord", "เลือกฝืนเล่นต่อขณะ Burnout"));
    AddSnapshot(TEXT("ForcedBurnout"), CurrentMoney, CurrentHappiness, CurrentKnowledge);
}

void UInvestoryEvaluationComponent::HandleOrderSubmissionAttempt(FInvestoryOrderSubmissionAttempt Attempt)
{
    EnsureSessionStarted();
    if (Attempt.bAccepted)
    {
        return; // Accepted orders are recorded from OnOrderChanged so we can also cache Pending details.
    }

    ++Progress.OrdersRejected;

    FInvestoryInvestmentRecord Record;
    Record.Sequence = ++SequenceCounter;
    Record.TurnNumber = Progress.CurrentTurn;
    Record.Year = Progress.CurrentYear;
    Record.StockId = Attempt.StockId;
    Record.Side = Attempt.Side;
    Record.Quantity = Attempt.Quantity;
    Record.SubmittedPrice = Attempt.SubmittedPrice;
    Record.bSuccess = false;
    Record.bAfterRecentNews = IsAfterRecentNews();
    Record.Message = Attempt.Reason;
    InvestmentLog.Add(Record);

    AddExperience(EInvestoryExperienceType::InvestmentRejected, TEXT("InvestmentSubmit"), 0, FText::GetEmpty(), Attempt.StockId,
        0.0f, 0, 0, false, Attempt.Reason);
}

void UInvestoryEvaluationComponent::HandleOrderChanged(FInvestoryPendingOrder Order)
{
    EnsureSessionStarted();

    if (Order.Status == EInvestoryOrderStatus::Pending)
    {
        // OnOrderChanged also fires while TurnsRemaining counts down. Only the first sight of this stock/order is submission.
        if (!PendingSubmissionCache.Contains(Order.StockId))
        {
            PendingSubmissionCache.Add(Order.StockId, Order);
            if (Order.Side == EInvestoryOrderSide::Buy)
            {
                ++Progress.BuyOrdersSubmitted;
            }
            else
            {
                ++Progress.SellOrdersSubmitted;
            }

            if (IsAfterRecentNews())
            {
                Progress.bTradedAfterNews = true;
            }

            AddExperience(EInvestoryExperienceType::InvestmentSubmitted, TEXT("Investment"), 0, FText::GetEmpty(), Order.StockId,
                0.0f, 0, 0, true,
                FText::Format(LOCTEXT("OrderSubmitted", "ส่งคำสั่ง {0} จำนวน {1} ที่ราคา {2}"),
                    Order.Side == EInvestoryOrderSide::Buy ? LOCTEXT("Buy", "ซื้อ") : LOCTEXT("Sell", "ขาย"),
                    FText::AsNumber(Order.Quantity), FText::AsNumber(Order.SubmittedPrice)));
        }
        return;
    }

    if (Order.Status == EInvestoryOrderStatus::Cancelled)
    {
        PendingSubmissionCache.Remove(Order.StockId);
        return;
    }

    if (Order.Status == EInvestoryOrderStatus::Rejected)
    {
        ++Progress.OrdersRejected;
        const FInvestoryPendingOrder* Original = PendingSubmissionCache.Find(Order.StockId);

        FInvestoryInvestmentRecord Record;
        Record.Sequence = ++SequenceCounter;
        Record.TurnNumber = Progress.CurrentTurn;
        Record.Year = Progress.CurrentYear;
        Record.StockId = Order.StockId;
        Record.Side = Order.Side;
        Record.Quantity = Order.Quantity;
        Record.SubmittedPrice = Original ? Original->SubmittedPrice : Order.SubmittedPrice;
        Record.ExecutedPrice = Order.ExecutedPrice;
        Record.Fee = Order.EstimatedFee;
        Record.bSuccess = false;
        Record.bPriceChangedWhilePending = !FMath::IsNearlyEqual(Record.SubmittedPrice, Record.ExecutedPrice, 0.01f);
        Record.bAfterRecentNews = IsAfterRecentNews();
        Record.Message = LOCTEXT("OrderRejected", "คำสั่งถูกปฏิเสธตอนดำเนินการ");
        InvestmentLog.Add(Record);

        AddExperience(EInvestoryExperienceType::InvestmentRejected, TEXT("Investment"), 0, FText::GetEmpty(), Order.StockId,
            0.0f, 0, 0, false, Record.Message);
        PendingSubmissionCache.Remove(Order.StockId);
    }
}

void UInvestoryEvaluationComponent::HandleOrderExecuted(FInvestoryExecutionResult Result)
{
    EnsureSessionStarted();
    ++Progress.OrdersExecuted;

    if (Result.Side == EInvestoryOrderSide::Buy)
    {
        Progress.bHasBoughtStock = true;
    }
    else
    {
        Progress.bHasSoldStock = true;
        Progress.bExperiencedRealizedPL = true;
        Progress.TotalRealizedProfitLoss += Result.RealizedProfitLoss;
    }

    if (Result.Fee > KINDA_SMALL_NUMBER)
    {
        Progress.bExperiencedFee = true;
        Progress.TotalFeesPaid += Result.Fee;
    }

    const FInvestoryPendingOrder* Original = PendingSubmissionCache.Find(Result.StockId);
    const float SubmittedPrice = Original ? Original->SubmittedPrice : Result.ExecutedPrice;
    const bool bPriceChanged = !FMath::IsNearlyEqual(SubmittedPrice, Result.ExecutedPrice, 0.01f);
    Progress.bExperiencedPendingPriceChange |= bPriceChanged;

    const bool bAfterNews = IsAfterRecentNews();
    Progress.bTradedAfterNews |= bAfterNews;

    FInvestoryInvestmentRecord Record;
    Record.Sequence = ++SequenceCounter;
    Record.TurnNumber = Progress.CurrentTurn;
    Record.Year = Progress.CurrentYear;
    Record.StockId = Result.StockId;
    Record.Side = Result.Side;
    Record.Quantity = Result.Quantity;
    Record.SubmittedPrice = SubmittedPrice;
    Record.ExecutedPrice = Result.ExecutedPrice;
    Record.Fee = Result.Fee;
    Record.CashDelta = Result.CashDelta;
    Record.RealizedProfitLoss = Result.RealizedProfitLoss;
    Record.bSuccess = Result.bSuccess;
    Record.bPriceChangedWhilePending = bPriceChanged;
    Record.bAfterRecentNews = bAfterNews;
    Record.Message = Result.Message;
    InvestmentLog.Add(Record);

    AddExperience(EInvestoryExperienceType::InvestmentExecuted, TEXT("Investment"), 0, FText::GetEmpty(), Result.StockId,
        Result.CashDelta, 0, 0, Result.bSuccess, Result.Message);
    PendingSubmissionCache.Remove(Result.StockId);
}

void UInvestoryEvaluationComponent::AddExperience(EInvestoryExperienceType Type, FName SourceId, int32 ChoiceIndex,
    const FText& ChoiceLabel, FName StockId, float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta,
    bool bSuccess, const FText& Note)
{
    FInvestoryExperienceLogEntry Entry;
    Entry.Sequence = ++SequenceCounter;
    Entry.TurnNumber = Progress.CurrentTurn;
    Entry.Year = Progress.CurrentYear;
    Entry.Type = Type;
    Entry.SourceId = SourceId;
    Entry.ChoiceIndex = ChoiceIndex;
    Entry.ChoiceLabel = ChoiceLabel;
    Entry.StockId = StockId;
    Entry.MoneyDelta = MoneyDelta;
    Entry.HappinessDelta = HappinessDelta;
    Entry.KnowledgeDelta = KnowledgeDelta;
    Entry.bSuccess = bSuccess;
    Entry.Note = Note;
    ExperienceLog.Add(Entry);
}

void UInvestoryEvaluationComponent::AddSnapshot(FName Reason, float Money, int32 Happiness, int32 Knowledge)
{
    FInvestoryStatusSnapshot Snapshot;
    Snapshot.Sequence = ++SequenceCounter;
    Snapshot.TurnNumber = Progress.CurrentTurn;
    Snapshot.Year = Progress.CurrentYear;
    Snapshot.Money = Money;
    Snapshot.Happiness = Happiness;
    Snapshot.Knowledge = Knowledge;
    Snapshot.Reason = Reason;
    StatusHistory.Add(Snapshot);

    Progress.FinalMoney = Money;
    Progress.FinalHappiness = Happiness;
    Progress.FinalKnowledge = Knowledge;
}

bool UInvestoryEvaluationComponent::IsAfterRecentNews() const
{
    if (LastNewsTurn == INDEX_NONE)
    {
        return false;
    }
    return Progress.CurrentTurn - LastNewsTurn <= FMath::Max(0, NewsDecisionWindowTurns);
}

int32 UInvestoryEvaluationComponent::CountInvestmentMilestones() const
{
    int32 Count = 0;
    Count += Progress.bHasBoughtStock ? 1 : 0;
    Count += Progress.bHasSoldStock ? 1 : 0;
    Count += Progress.bExperiencedFee ? 1 : 0;
    Count += Progress.bExperiencedPendingPriceChange ? 1 : 0;
    Count += Progress.bExperiencedAverageCost ? 1 : 0;
    Count += Progress.bExperiencedUnrealizedPL ? 1 : 0;
    Count += Progress.bExperiencedBreakEven ? 1 : 0;
    Count += Progress.bExperiencedRealizedPL ? 1 : 0;
    Count += Progress.bTradedAfterNews ? 1 : 0;
    return Count;
}

float UInvestoryEvaluationComponent::CalculateScamKnowledgeScore() const
{
    if (Progress.ScamAnswered <= 0)
    {
        return 0.0f;
    }

    const float Earned = static_cast<float>(Progress.ScamBest) * FMath::Clamp(ScamBestCredit, 0.0f, 1.0f)
        + static_cast<float>(Progress.ScamGood) * FMath::Clamp(ScamGoodCredit, 0.0f, 1.0f);
    return FMath::Clamp((Earned / static_cast<float>(Progress.ScamAnswered)) * 100.0f, 0.0f, 100.0f);
}


float UInvestoryEvaluationComponent::CalculateFinalExamScore() const
{
    if (Progress.FinalExamMaxPoints <= KINDA_SMALL_NUMBER)
    {
        return 0.0f;
    }
    return FMath::Clamp((Progress.FinalExamPointsEarned / Progress.FinalExamMaxPoints) * 100.0f, 0.0f, 100.0f);
}

FString UInvestoryEvaluationComponent::GradeFromScore(float Score) const
{
    if (Score >= GradeAThreshold) return TEXT("A");
    if (Score >= GradeBThreshold) return TEXT("B");
    if (Score >= GradeCThreshold) return TEXT("C");
    if (Score >= GradeDThreshold) return TEXT("D");
    return TEXT("F");
}

FInvestoryEvaluationResult UInvestoryEvaluationComponent::CalculateEvaluation(
    float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge) const
{
    return CalculateEvaluationWithPortfolio(CurrentMoney, 0.0f, CurrentHappiness, CurrentKnowledge);
}

FInvestoryEvaluationResult UInvestoryEvaluationComponent::CalculateEvaluationWithPortfolio(
    float CurrentMoney, float CurrentPortfolioMarketValue, int32 CurrentHappiness, int32 CurrentKnowledge) const
{
    FInvestoryEvaluationResult Result;

    Result.PortfolioMarketValue = FMath::Max(0.0f, CurrentPortfolioMarketValue);
    Result.NetWorth = FMath::Max(0.0f, CurrentMoney) + Result.PortfolioMarketValue;

    const float InitialMoney = FMath::Max(1.0f, Progress.InitialMoney);
    const float MoneyRatio = Result.NetWorth / InitialMoney;
    const float StartScore = FMath::Clamp(FinancialScoreAtStartingMoney, 0.0f, 100.0f);
    const float TargetRatio = FMath::Max(1.0001f, FinancialTargetMoneyMultiplier);
    const float SafeKnowledgeMax = FMath::Max(1.0f, KnowledgeScoreMax);
    const float SafeHappinessMax = FMath::Max(1.0f, HappinessScoreMax);
    if (MoneyRatio <= 1.0f)
    {
        Result.FinancialScore = FMath::Clamp(MoneyRatio * StartScore, 0.0f, StartScore);
    }
    else
    {
        const float GrowthAlpha = FMath::Clamp((MoneyRatio - 1.0f) / (TargetRatio - 1.0f), 0.0f, 1.0f);
        Result.FinancialScore = FMath::Lerp(StartScore, 100.0f, GrowthAlpha);
    }
    Result.FinancialScore = FMath::Clamp(
        Result.FinancialScore - static_cast<float>(Progress.LivingExpenseUnpaidCount) * FMath::Max(0.0f, UnpaidLivingExpensePenalty),
        0.0f, 100.0f);

    const float KnowledgeStatusScore =
    FMath::Clamp(
        (static_cast<float>(CurrentKnowledge) / SafeKnowledgeMax) * 100.0f,
        0.0f,
        100.0f
    );
    Result.ScamKnowledgeScore = CalculateScamKnowledgeScore();
    Result.FinalExamScore = CalculateFinalExamScore();

    float WeightedKnowledge = 0.0f;
    float KnowledgeWeightSum = 0.0f;
    const float KWeight = FMath::Max(0.0f, KnowledgeStatusContribution);
    WeightedKnowledge += KnowledgeStatusScore * KWeight;
    KnowledgeWeightSum += KWeight;

    if (Progress.ScamAnswered > 0)
    {
        const float SWeight = FMath::Max(0.0f, ScamContribution);
        WeightedKnowledge += Result.ScamKnowledgeScore * SWeight;
        KnowledgeWeightSum += SWeight;
    }

    if (Progress.FinalExamMaxPoints > KINDA_SMALL_NUMBER)
    {
        const float EWeight = FMath::Max(0.0f, FinalExamContribution);
        WeightedKnowledge += Result.FinalExamScore * EWeight;
        KnowledgeWeightSum += EWeight;
    }

    Result.KnowledgeScore = KnowledgeWeightSum > KINDA_SMALL_NUMBER
        ? WeightedKnowledge / KnowledgeWeightSum
        : KnowledgeStatusScore;

    const float HappinessScore =
    FMath::Clamp(
        (static_cast<float>(CurrentHappiness) / SafeHappinessMax) * 100.0f,
        0.0f,
        100.0f
    );
    const float BurnoutBehaviorScore = FMath::Clamp(100.0f - static_cast<float>(Progress.ForceThroughBurnoutCount) * 12.5f, 0.0f, 100.0f);
    Result.WellbeingScore = HappinessScore * 0.80f + BurnoutBehaviorScore * 0.20f;

    Result.InvestmentMilestonesCompleted = CountInvestmentMilestones();
    Result.InvestmentMilestonesTotal = 9;
    Result.InvestmentLearningScore = CalculateInvestmentLearningScore();

    const float FW = FMath::Max(0.0f, FinancialWeight);
    const float KW = FMath::Max(0.0f, KnowledgeWeight);
    const float WW = FMath::Max(0.0f, WellbeingWeight);
    const float IW = FMath::Max(0.0f, InvestmentLearningWeight);
    const float WeightSum = FW + KW + WW + IW;

    if (WeightSum > KINDA_SMALL_NUMBER)
    {
        Result.OverallScore =
            (Result.FinancialScore * FW + Result.KnowledgeScore * KW +
             Result.WellbeingScore * WW + Result.InvestmentLearningScore * IW) / WeightSum;
    }

    Result.OverallScore = FMath::Clamp(Result.OverallScore, 0.0f, 100.0f);
    Result.Grade = GradeFromScore(Result.OverallScore);
    Result.Summary = FText::Format(
        LOCTEXT("EvaluationSummary", "การเงิน {0} | ความรู้ {1} | สอบไฟนอล {2} | สุขภาวะ {3} | ประสบการณ์ลงทุน {4}/{5}"),
        FText::AsNumber(FMath::RoundToInt(Result.FinancialScore)),
        FText::AsNumber(FMath::RoundToInt(Result.KnowledgeScore)),
        FText::AsNumber(FMath::RoundToInt(Result.FinalExamScore)),
        FText::AsNumber(FMath::RoundToInt(Result.WellbeingScore)),
        FText::AsNumber(Result.InvestmentMilestonesCompleted),
        FText::AsNumber(Result.InvestmentMilestonesTotal));

    return Result;
}

FInvestoryEvaluationResult UInvestoryEvaluationComponent::FinalizeEvaluation(
    float FinalMoney, int32 FinalHappiness, int32 FinalKnowledge)
{
    return FinalizeEvaluationWithPortfolio(FinalMoney, 0.0f, FinalHappiness, FinalKnowledge);
}

FInvestoryEvaluationResult UInvestoryEvaluationComponent::FinalizeEvaluationWithPortfolio(
    float FinalMoney, float FinalPortfolioMarketValue, int32 FinalHappiness, int32 FinalKnowledge)
{
    EnsureSessionStarted();
    Progress.FinalMoney = FMath::Max(0.0f, FinalMoney);
    Progress.FinalPortfolioMarketValue = FMath::Max(0.0f, FinalPortfolioMarketValue);
    Progress.FinalNetWorth = Progress.FinalMoney + Progress.FinalPortfolioMarketValue;
    Progress.FinalHappiness = FinalHappiness;
    Progress.FinalKnowledge = FinalKnowledge;
    Progress.EndTimeIso = FDateTime::UtcNow().ToIso8601();

    AddExperience(EInvestoryExperienceType::SessionEnded, TEXT("GameEnd"), 0, FText::GetEmpty(), NAME_None,
        0.0f, 0, 0, true,
        FText::Format(LOCTEXT("SessionEndedNetWorth", "จบการเล่น | Net Worth {0}"), FText::AsNumber(Progress.FinalNetWorth)));
    AddSnapshot(TEXT("GameEnd"), Progress.FinalMoney, FinalHappiness, FinalKnowledge);

    return CalculateEvaluationWithPortfolio(
        Progress.FinalMoney, Progress.FinalPortfolioMarketValue, FinalHappiness, FinalKnowledge);
}

bool UInvestoryEvaluationComponent::ExportEvaluationCsv(FString FolderLabel, FString& OutDirectory, FText& OutMessage) const
{
    if (!bSessionStarted)
    {
        OutMessage = LOCTEXT("ExportNoSession", "ยังไม่มี Evaluation Session ที่จะส่งออก");
        return false;
    }

    FolderLabel.TrimStartAndEndInline();
    if (FolderLabel.IsEmpty())
    {
        FolderLabel = Progress.SessionId;
    }

    FolderLabel.ReplaceInline(TEXT("/"), TEXT("_"));
    FolderLabel.ReplaceInline(TEXT("\\"), TEXT("_"));
    FolderLabel.ReplaceInline(TEXT(":"), TEXT("_"));

    OutDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("InvestoryEvaluation"), FolderLabel);
    IFileManager::Get().MakeDirectory(*OutDirectory, true);

    bool bAllSaved = true;

    const FInvestoryEvaluationResult Scores = CalculateEvaluationWithPortfolio(
        Progress.FinalMoney, Progress.FinalPortfolioMarketValue, Progress.FinalHappiness, Progress.FinalKnowledge);

    FString SummaryCsv = TEXT("SessionId,StartTimeUtc,EndTimeUtc,Turn,Year,InitialMoney,FinalMoney,FinalPortfolioMarketValue,FinalNetWorth,InitialHappiness,FinalHappiness,InitialKnowledge,FinalKnowledge,ScamAnswered,ScamWrong,ScamGood,ScamBest,FinalExamAnswered,FinalExamCorrect,FinalExamPoints,FinalExamMaxPoints,FinalExamRewardMoney,FinalExamScore,LearningConceptsExposed,PortfolioViews,MeaningfulPortfolioViews,BuySubmitted,SellSubmitted,OrdersExecuted,OrdersRejected,FeesPaid,RealizedPL,HasBought,HasSold,ExperiencedFee,PendingPriceChange,ExperiencedAverageCost,ExperiencedUnrealizedPL,ExperiencedBreakEven,ExperiencedRealizedPL,TradedAfterNews,EventChoices,NewsSeen,BigNewsSeen,ShopPurchases,BurnoutEncounters,RestCount,ForceBurnoutCount,LivingExpensePaid,LivingExpenseUnpaid,LivingExpenseTotal,FinancialScore,KnowledgeScore,ScamScore,WellbeingScore,InvestmentLearningScore,InvestmentMilestones,OverallScore,Grade\n");
    SummaryCsv += FString::Printf(TEXT("%s,%s,%s,%d,%d,%.2f,%.2f,%.2f,%.2f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.2f,%.2f,%.2f,%.2f,%d,%d,%d,%d,%d,%d,%d,%.2f,%.2f,%s,%s,%s,%s,%s,%s,%s,%s,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d/%d,%.2f,%s\n"),
        *InvestoryEvaluationCsv::Escape(Progress.SessionId),
        *InvestoryEvaluationCsv::Escape(Progress.StartTimeIso),
        *InvestoryEvaluationCsv::Escape(Progress.EndTimeIso),
        Progress.CurrentTurn, Progress.CurrentYear,
        Progress.InitialMoney, Progress.FinalMoney,
        Scores.PortfolioMarketValue, Scores.NetWorth,
        Progress.InitialHappiness, Progress.FinalHappiness,
        Progress.InitialKnowledge, Progress.FinalKnowledge,
        Progress.ScamAnswered, Progress.ScamWrong, Progress.ScamGood, Progress.ScamBest,
        Progress.FinalExamAnswered, Progress.FinalExamCorrect, Progress.FinalExamPointsEarned, Progress.FinalExamMaxPoints,
        Progress.FinalExamRewardMoney, Scores.FinalExamScore, Progress.UniqueLearningConceptsExposed, Progress.PortfolioViews, Progress.MeaningfulPortfolioViews,
        Progress.BuyOrdersSubmitted, Progress.SellOrdersSubmitted, Progress.OrdersExecuted, Progress.OrdersRejected,
        Progress.TotalFeesPaid, Progress.TotalRealizedProfitLoss,
        *InvestoryEvaluationCsv::Bool(Progress.bHasBoughtStock),
        *InvestoryEvaluationCsv::Bool(Progress.bHasSoldStock),
        *InvestoryEvaluationCsv::Bool(Progress.bExperiencedFee),
        *InvestoryEvaluationCsv::Bool(Progress.bExperiencedPendingPriceChange),
        *InvestoryEvaluationCsv::Bool(Progress.bExperiencedAverageCost),
        *InvestoryEvaluationCsv::Bool(Progress.bExperiencedUnrealizedPL),
        *InvestoryEvaluationCsv::Bool(Progress.bExperiencedBreakEven),
        *InvestoryEvaluationCsv::Bool(Progress.bExperiencedRealizedPL),
        *InvestoryEvaluationCsv::Bool(Progress.bTradedAfterNews),
        Progress.EventChoices, Progress.NewsSeen, Progress.BigNewsSeen, Progress.ShopPurchases,
        Progress.BurnoutEncounters, Progress.RestCount, Progress.ForceThroughBurnoutCount,
        Progress.LivingExpensePaidCount, Progress.LivingExpenseUnpaidCount, Progress.TotalLivingExpensesPaid,
        Scores.FinancialScore, Scores.KnowledgeScore, Scores.ScamKnowledgeScore, Scores.WellbeingScore,
        Scores.InvestmentLearningScore, Scores.InvestmentMilestonesCompleted, Scores.InvestmentMilestonesTotal,
        Scores.OverallScore, *Scores.Grade);
    bAllSaved &= FFileHelper::SaveStringToFile(SummaryCsv, *FPaths::Combine(OutDirectory, TEXT("summary.csv")), FFileHelper::EEncodingOptions::ForceUTF8);

    FString ExperienceCsv = TEXT("Sequence,Turn,Year,Type,SourceId,ChoiceIndex,ChoiceLabel,StockId,MoneyDelta,HappinessDelta,KnowledgeDelta,Success,Note\n");
    for (const FInvestoryExperienceLogEntry& E : ExperienceLog)
    {
        ExperienceCsv += FString::Printf(TEXT("%d,%d,%d,%s,%s,%d,%s,%s,%.2f,%d,%d,%s,%s\n"),
            E.Sequence, E.TurnNumber, E.Year,
            *InvestoryEvaluationCsv::Escape(InvestoryEvaluationCsv::ExperienceType(E.Type)),
            *InvestoryEvaluationCsv::Escape(E.SourceId.ToString()), E.ChoiceIndex,
            *InvestoryEvaluationCsv::Escape(E.ChoiceLabel.ToString()),
            *InvestoryEvaluationCsv::Escape(E.StockId.ToString()), E.MoneyDelta, E.HappinessDelta, E.KnowledgeDelta,
            *InvestoryEvaluationCsv::Bool(E.bSuccess), *InvestoryEvaluationCsv::Escape(E.Note.ToString()));
    }
    bAllSaved &= FFileHelper::SaveStringToFile(ExperienceCsv, *FPaths::Combine(OutDirectory, TEXT("experience_log.csv")), FFileHelper::EEncodingOptions::ForceUTF8);

    FString ScamCsv = TEXT("Sequence,Turn,Year,QuestionId,ChoiceIndex,ChoiceLabel,Quality,Explanation,WarningSign,MoneyDelta,HappinessDelta,KnowledgeDelta\n");
    for (const FInvestoryScamRecord& S : ScamLog)
    {
        ScamCsv += FString::Printf(TEXT("%d,%d,%d,%s,%d,%s,%s,%s,%s,%.2f,%d,%d\n"),
            S.Sequence, S.TurnNumber, S.Year, *InvestoryEvaluationCsv::Escape(S.QuestionId.ToString()), S.ChoiceIndex,
            *InvestoryEvaluationCsv::Escape(S.ChoiceLabel.ToString()),
            *InvestoryEvaluationCsv::Escape(InvestoryEvaluationCsv::ScamQuality(S.Quality)),
            *InvestoryEvaluationCsv::Escape(S.Explanation.ToString()),
            *InvestoryEvaluationCsv::Escape(S.WarningSign.ToString()),
            S.MoneyDelta, S.HappinessDelta, S.KnowledgeDelta);
    }
    bAllSaved &= FFileHelper::SaveStringToFile(ScamCsv, *FPaths::Combine(OutDirectory, TEXT("scam_log.csv")), FFileHelper::EEncodingOptions::ForceUTF8);

    FString LearningCsv = TEXT("Sequence,Turn,Year,ConceptId,SourceId,Note\n");
    for (const FInvestoryLearningExposureRecord& L : LearningExposureLog)
    {
        LearningCsv += FString::Printf(TEXT("%d,%d,%d,%s,%s,%s\n"),
            L.Sequence, L.TurnNumber, L.Year,
            *InvestoryEvaluationCsv::Escape(L.ConceptId.ToString()),
            *InvestoryEvaluationCsv::Escape(L.SourceId.ToString()),
            *InvestoryEvaluationCsv::Escape(L.Note.ToString()));
    }
    bAllSaved &= FFileHelper::SaveStringToFile(LearningCsv, *FPaths::Combine(OutDirectory, TEXT("learning_exposure_log.csv")), FFileHelper::EEncodingOptions::ForceUTF8);

    FString ExamCsv = TEXT("Sequence,Turn,Year,QuestionId,Topic,ChoiceIndex,ChoiceLabel,Correct,PointsEarned,MaxPoints,Explanation,LearningPoint\n");
    for (const FInvestoryKnowledgeExamRecord& E : KnowledgeExamLog)
    {
        ExamCsv += FString::Printf(TEXT("%d,%d,%d,%s,%s,%d,%s,%s,%.2f,%.2f,%s,%s\n"),
            E.Sequence, E.TurnNumber, E.Year,
            *InvestoryEvaluationCsv::Escape(E.QuestionId.ToString()),
            *InvestoryEvaluationCsv::Escape(InvestoryEvaluationCsv::KnowledgeTopic(E.Topic)),
            E.ChoiceIndex, *InvestoryEvaluationCsv::Escape(E.ChoiceLabel.ToString()),
            *InvestoryEvaluationCsv::Bool(E.bCorrect), E.PointsEarned, E.MaxPoints,
            *InvestoryEvaluationCsv::Escape(E.Explanation.ToString()),
            *InvestoryEvaluationCsv::Escape(E.LearningPoint.ToString()));
    }
    bAllSaved &= FFileHelper::SaveStringToFile(ExamCsv, *FPaths::Combine(OutDirectory, TEXT("knowledge_exam_log.csv")), FFileHelper::EEncodingOptions::ForceUTF8);

    FString InvestmentCsv = TEXT("Sequence,Turn,Year,StockId,Side,Quantity,SubmittedPrice,ExecutedPrice,Fee,CashDelta,RealizedPL,Success,PriceChangedWhilePending,AfterRecentNews,Message\n");
    for (const FInvestoryInvestmentRecord& I : InvestmentLog)
    {
        InvestmentCsv += FString::Printf(TEXT("%d,%d,%d,%s,%s,%d,%.4f,%.4f,%.4f,%.4f,%.4f,%s,%s,%s,%s\n"),
            I.Sequence, I.TurnNumber, I.Year,
            *InvestoryEvaluationCsv::Escape(I.StockId.ToString()), *InvestoryEvaluationCsv::Escape(InvestoryEvaluationCsv::Side(I.Side)),
            I.Quantity, I.SubmittedPrice, I.ExecutedPrice, I.Fee, I.CashDelta, I.RealizedProfitLoss,
            *InvestoryEvaluationCsv::Bool(I.bSuccess), *InvestoryEvaluationCsv::Bool(I.bPriceChangedWhilePending),
            *InvestoryEvaluationCsv::Bool(I.bAfterRecentNews), *InvestoryEvaluationCsv::Escape(I.Message.ToString()));
    }
    bAllSaved &= FFileHelper::SaveStringToFile(InvestmentCsv, *FPaths::Combine(OutDirectory, TEXT("investment_log.csv")), FFileHelper::EEncodingOptions::ForceUTF8);

    FString StatusCsv = TEXT("Sequence,Turn,Year,Money,Happiness,Knowledge,Reason\n");
    for (const FInvestoryStatusSnapshot& S : StatusHistory)
    {
        StatusCsv += FString::Printf(TEXT("%d,%d,%d,%.2f,%d,%d,%s\n"),
            S.Sequence, S.TurnNumber, S.Year, S.Money, S.Happiness, S.Knowledge,
            *InvestoryEvaluationCsv::Escape(S.Reason.ToString()));
    }
    bAllSaved &= FFileHelper::SaveStringToFile(StatusCsv, *FPaths::Combine(OutDirectory, TEXT("status_history.csv")), FFileHelper::EEncodingOptions::ForceUTF8);

    OutMessage = bAllSaved
        ? FText::Format(LOCTEXT("ExportSuccess", "ส่งออกข้อมูล Evaluation แล้ว: {0}"), FText::FromString(OutDirectory))
        : LOCTEXT("ExportFailed", "ส่งออกข้อมูลบางไฟล์ไม่สำเร็จ");
    return bAllSaved;
}

float UInvestoryEvaluationComponent::CalculateInvestmentLearningScore() const
{
    float Score = 0.0f;

    // Core learning = 80 คะแนน
    Score += Progress.bHasBoughtStock              ? 10.0f : 0.0f;
    Score += Progress.bHasSoldStock               ? 10.0f : 0.0f;
    Score += Progress.bExperiencedFee             ? 10.0f : 0.0f;
    Score += Progress.bExperiencedAverageCost     ? 12.5f : 0.0f;
    Score += Progress.bExperiencedUnrealizedPL    ? 12.5f : 0.0f;
    Score += Progress.bExperiencedBreakEven       ? 12.5f : 0.0f;
    Score += Progress.bExperiencedRealizedPL      ? 12.5f : 0.0f;

    // Bonus experience = 20 คะแนน
    Score += Progress.bExperiencedPendingPriceChange ? 10.0f : 0.0f;
    Score += Progress.bTradedAfterNews                ? 10.0f : 0.0f;

    return FMath::Clamp(Score, 0.0f, 100.0f);
}

#undef LOCTEXT_NAMESPACE
