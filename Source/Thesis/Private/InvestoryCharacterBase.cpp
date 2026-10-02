#include "InvestoryCharacterBase.h"
#include "InvestoryStatusComponent.h"
#include "InvestoryInvestmentComponent.h"
#include "InvestoryTurnFlowComponent.h"
#include "InvestoryEvaluationComponent.h"
#include "InvestoryLearningComponent.h"

#define LOCTEXT_NAMESPACE "InvestoryCharacterBase"

AInvestoryCharacterBase::AInvestoryCharacterBase()
{
    PrimaryActorTick.bCanEverTick = false;

    StatusComponent = CreateDefaultSubobject<UInvestoryStatusComponent>(TEXT("InvestoryStatus"));
    InvestmentComponent = CreateDefaultSubobject<UInvestoryInvestmentComponent>(TEXT("InvestoryInvestment"));
    TurnFlowComponent = CreateDefaultSubobject<UInvestoryTurnFlowComponent>(TEXT("InvestoryTurnFlow"));
    EvaluationComponent = CreateDefaultSubobject<UInvestoryEvaluationComponent>(TEXT("InvestoryEvaluation"));
    LearningComponent = CreateDefaultSubobject<UInvestoryLearningComponent>(TEXT("InvestoryLearning"));
}

FInvestoryGameplayTurnResult AInvestoryCharacterBase::StartGameplayTurn()
{
    if (!TurnFlowComponent)
    {
        FInvestoryGameplayTurnResult Missing;
        Missing.Message = LOCTEXT("MissingTurnFlow", "ไม่พบระบบ Turn Flow");
        return Missing;
    }

    FInvestoryGameplayTurnResult Result = TurnFlowComponent->StartTurn();

    if (Result.bStartedNewTurn && EvaluationComponent)
    {
        EvaluationComponent->SetTurnContext(Result.TurnNumber, EvaluationComponent->GetProgress().CurrentYear);
        if (StatusComponent)
        {
            // Recorded before periodic income so the start-of-turn snapshot is not inflated by the grant.
            EvaluationComponent->RecordTurnStarted(Result.TurnNumber, StatusComponent->Money, StatusComponent->Happiness, StatusComponent->Knowledge);
        }
    }

    // Income is applied exactly once because StartTurn is idempotent while a turn is active.
    if (Result.bStartedNewTurn && Result.bIncomeGranted && StatusComponent)
    {
        StatusComponent->ChangeMoney(Result.IncomeGranted);
    }

    return Result;
}

void AInvestoryCharacterBase::MarkResolvingBoard()
{
    if (TurnFlowComponent)
    {
        TurnFlowComponent->MarkResolvingBoard();
    }
}

int32 AInvestoryCharacterBase::OpenMarketPhase(bool bInvestmentTile)
{
    return TurnFlowComponent ? TurnFlowComponent->OpenMarketPhase(bInvestmentTile) : 0;
}

void AInvestoryCharacterBase::EndGameplayTurn()
{
    if (TurnFlowComponent)
    {
        TurnFlowComponent->EndTurn();
    }
}

bool AInvestoryCharacterBase::CanTradeThisTurn() const
{
    return TurnFlowComponent && TurnFlowComponent->CanTrade();
}

int32 AInvestoryCharacterBase::GetMarketActionsRemaining() const
{
    return TurnFlowComponent ? TurnFlowComponent->GetMarketActionsRemaining() : 0;
}

bool AInvestoryCharacterBase::TryStartRoll(bool bForceThroughBurnout, FInvestoryTurnResult& OutResult)
{
    OutResult = FInvestoryTurnResult();
    OutResult.RollNumber = RollCount + 1;

    if (!StatusComponent || !InvestmentComponent)
    {
        OutResult.Message = LOCTEXT("MissingComponents", "ไม่พบระบบสถานะหรือระบบลงทุนของผู้เล่น");
        return false;
    }

    if (TurnFlowComponent && TurnFlowComponent->bEnforcePhaseGating && TurnFlowComponent->IsTurnActive() &&
        TurnFlowComponent->GetTurnPhase() != EInvestoryTurnPhase::ReadyToRoll)
    {
        OutResult.Message = LOCTEXT("TurnPhaseBlocksRoll", "ยังไม่จบขั้นตอนของเทิร์นปัจจุบัน");
        return false;
    }

    const bool bBurnout = StatusComponent->IsBurnout();
    if (bBurnout && !bForceThroughBurnout)
    {
        OutResult.bCanRoll = false;
        OutResult.Message = LOCTEXT("BurnoutBlocksRoll", "คุณอยู่ในภาวะ Burnout: พักก่อน หรือเลือกฝืนเพื่อทอยต่อ");
        if (EvaluationComponent)
        {
            EvaluationComponent->RecordBurnoutEncounter(StatusComponent->Money, StatusComponent->Happiness, StatusComponent->Knowledge);
        }
        return false;
    }

    ++RollCount;
    OutResult.RollNumber = RollCount;
    OutResult.bCanRoll = true;
    OutResult.bForcedThroughBurnout = bBurnout && bForceThroughBurnout;

    int32 HappinessCost = FMath::Max(0, HappinessCostPerRoll);
    if (OutResult.bForcedThroughBurnout)
    {
        HappinessCost += FMath::Max(0, ExtraHappinessCostWhenForcingBurnout);
    }

    if (HappinessCost > 0)
    {
        StatusComponent->ChangeHappiness(-HappinessCost);
        OutResult.HappinessSpent += HappinessCost;
    }

    const bool bExpenseCycleEnabled = LivingExpenseEveryRolls > 0 && LivingExpenseAmount > 0.0f;
    if (bExpenseCycleEnabled && (RollCount % LivingExpenseEveryRolls == 0))
    {
        OutResult.bLivingExpenseDue = true;
        const float SpendableCash = GetSpendableMoney();

        if (SpendableCash + KINDA_SMALL_NUMBER >= LivingExpenseAmount)
        {
            StatusComponent->ChangeMoney(-LivingExpenseAmount);
            OutResult.bLivingExpensePaid = true;
            OutResult.MoneySpent = LivingExpenseAmount;
        }
        else
        {
            OutResult.bLivingExpenseUnpaid = true;
            const int32 StressPenalty = FMath::Max(0, UnpaidExpenseHappinessPenalty);
            if (StressPenalty > 0)
            {
                StatusComponent->ChangeHappiness(-StressPenalty);
                OutResult.HappinessSpent += StressPenalty;
            }
        }
    }

    if (EvaluationComponent && OutResult.bLivingExpenseDue)
    {
        EvaluationComponent->RecordLivingExpense(
            OutResult.bLivingExpensePaid,
            LivingExpenseAmount,
            StatusComponent->Money, StatusComponent->Happiness, StatusComponent->Knowledge);
    }


    if (LearningComponent && OutResult.bLivingExpenseDue)
    {
        LearningComponent->ExposeConcept(EInvestoryLearningConcept::Budgeting, TEXT("LivingExpense"),
            LOCTEXT("BudgetingLearn", "ค่าครองชีพเกิดซ้ำ จึงควรวางแผนเงินสดก่อนใช้จ่ายหรือลงทุน"));
        if (OutResult.bLivingExpenseUnpaid)
        {
            LearningComponent->ExposeConcept(EInvestoryLearningConcept::Liquidity, TEXT("LivingExpense"),
                LOCTEXT("LiquidityLearnExpense", "เงินรวมอาจมี แต่เงินสดที่พร้อมใช้ไม่พอค่าใช้จ่ายจำเป็นได้"));
        }
    }

    if (EvaluationComponent && OutResult.bForcedThroughBurnout)
    {
        EvaluationComponent->RecordForcedBurnout(
            FMath::Max(0, ExtraHappinessCostWhenForcingBurnout),
            StatusComponent->Money, StatusComponent->Happiness, StatusComponent->Knowledge);
    }

    if (OutResult.bLivingExpenseUnpaid)
    {
        OutResult.Message = LOCTEXT("ExpenseUnpaid", "เงินสดที่ใช้ได้ไม่พอสำหรับค่าครองชีพ ทำให้เกิดความกดดันทางการเงิน");
    }
    else if (OutResult.bForcedThroughBurnout)
    {
        OutResult.Message = LOCTEXT("ForcedRoll", "คุณฝืนเล่นต่อทั้งที่ Burnout จึงเสีย Happiness เพิ่ม");
    }
    else
    {
        OutResult.Message = LOCTEXT("RollReady", "พร้อมทอย");
    }

    if (TurnFlowComponent && TurnFlowComponent->IsTurnActive())
    {
        TurnFlowComponent->MarkResolvingBoard();
    }

    return true;
}

bool AInvestoryCharacterBase::TryRestInsteadOfRoll(FInvestoryRestResult& OutResult)
{
    OutResult = FInvestoryRestResult();

    if (!StatusComponent)
    {
        OutResult.Message = LOCTEXT("RestMissingStatus", "ไม่พบระบบสถานะของผู้เล่น");
        return false;
    }

    const float Cost = FMath::Max(0.0f, RestMoneyCost);
    const int32 Gain = FMath::Max(0, RestHappinessGain);
    const float SpendableCash = GetSpendableMoney();

    if (SpendableCash + KINDA_SMALL_NUMBER < Cost)
    {
        OutResult.Message = FText::Format(
            LOCTEXT("RestNotEnoughMoney", "เงินสดที่ใช้ได้ไม่พอสำหรับการพัก ต้องใช้ {0} บาท"),
            FText::AsNumber(Cost));
        return false;
    }

    if (Cost > 0.0f)
    {
        StatusComponent->ChangeMoney(-Cost);
        OutResult.MoneySpent = Cost;
    }

    if (Gain > 0)
    {
        StatusComponent->ChangeHappiness(Gain);
        OutResult.HappinessGained = Gain;
    }

    if (EvaluationComponent)
    {
        EvaluationComponent->RecordRest(OutResult.MoneySpent, OutResult.HappinessGained,
            StatusComponent->Money, StatusComponent->Happiness, StatusComponent->Knowledge);
    }


    if (LearningComponent)
    {
        LearningComponent->ExposeConcept(EInvestoryLearningConcept::OpportunityCost, TEXT("BurnoutRest"),
            LOCTEXT("RestOpportunityCost", "การพักช่วยฟื้นความสุข แต่ต้องแลกกับเงินและโอกาสในการเดินเทิร์นนั้น"));
    }

    OutResult.bSuccess = true;
    OutResult.Message = FText::Format(
        LOCTEXT("RestSuccess", "พักสำเร็จ: เสียเงิน {0} บาท และฟื้นความสุข {1}"),
        FText::AsNumber(OutResult.MoneySpent),
        FText::AsNumber(OutResult.HappinessGained));

    // Rest is a complete turn by design. This also prevents a free extra roll afterward.
    if (TurnFlowComponent && TurnFlowComponent->IsTurnActive())
    {
        TurnFlowComponent->EndTurn();
    }

    return true;
}

void AInvestoryCharacterBase::RestInsteadOfRoll()
{
    FInvestoryRestResult Ignored;
    TryRestInsteadOfRoll(Ignored);
}

void AInvestoryCharacterBase::ApplyStatusChange(float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta)
{
    if (!StatusComponent)
    {
        return;
    }

    if (!FMath::IsNearlyZero(MoneyDelta))
    {
        StatusComponent->ChangeMoney(MoneyDelta);
    }

    if (HappinessDelta != 0)
    {
        StatusComponent->ChangeHappiness(HappinessDelta);
    }

    if (KnowledgeDelta != 0)
    {
        StatusComponent->ChangeKnowledge(KnowledgeDelta);
    }
}

float AInvestoryCharacterBase::GetSpendableMoney() const
{
    if (!StatusComponent)
    {
        return 0.0f;
    }

    const float Reserved = InvestmentComponent ? InvestmentComponent->GetReservedCash() : 0.0f;
    return StatusComponent->GetSpendableMoney(Reserved);
}

bool AInvestoryCharacterBase::CanRollNormally() const
{
    return StatusComponent && !StatusComponent->IsBurnout();
}

void AInvestoryCharacterBase::BeginEvaluationSession(float InitialMoney, int32 InitialHappiness, int32 InitialKnowledge, FString SessionLabel)
{
    if (LearningComponent)
    {
        LearningComponent->ResetLearningProgress();
    }
    if (EvaluationComponent)
    {
        EvaluationComponent->BeginSession(InitialMoney, InitialHappiness, InitialKnowledge, MoveTemp(SessionLabel));
    }
}

void AInvestoryCharacterBase::SetEvaluationYear(int32 Year)
{
    if (EvaluationComponent)
    {
        const int32 Turn = TurnFlowComponent ? TurnFlowComponent->GetTurnNumber() : EvaluationComponent->GetProgress().CurrentTurn;
        EvaluationComponent->SetTurnContext(Turn, Year);
    }
}

void AInvestoryCharacterBase::RecordEventChoiceForEvaluation(FName EventId, int32 ChoiceIndex, FText ChoiceLabel,
    float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta,
    float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge)
{
    if (EvaluationComponent)
    {
        EvaluationComponent->RecordEventChoice(EventId, ChoiceIndex, MoveTemp(ChoiceLabel), MoneyDelta, HappinessDelta, KnowledgeDelta,
            CurrentMoney, CurrentHappiness, CurrentKnowledge);
    }
}

void AInvestoryCharacterBase::RecordScamAnswerForEvaluation(FName QuestionId, int32 ChoiceIndex, EInvestoryScamAnswerQuality Quality,
    float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta,
    float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge)
{
    if (EvaluationComponent)
    {
        EvaluationComponent->RecordScamAnswer(QuestionId, ChoiceIndex, Quality, MoneyDelta, HappinessDelta, KnowledgeDelta,
            CurrentMoney, CurrentHappiness, CurrentKnowledge);
    }
}


FInvestoryScamFeedback AInvestoryCharacterBase::RecordScamAnswerDetailedForEvaluation(
    FName QuestionId, int32 ChoiceIndex, FText ChoiceLabel, EInvestoryScamAnswerQuality Quality,
    FText Explanation, FText WarningSign, float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta,
    float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge)
{
    return EvaluationComponent
        ? EvaluationComponent->RecordScamAnswerDetailed(QuestionId, ChoiceIndex, MoveTemp(ChoiceLabel), Quality,
            MoveTemp(Explanation), MoveTemp(WarningSign), MoneyDelta, HappinessDelta, KnowledgeDelta,
            CurrentMoney, CurrentHappiness, CurrentKnowledge)
        : FInvestoryScamFeedback();
}

FInvestoryScamFeedback AInvestoryCharacterBase::RecordScamAnswerWithLearningForEvaluation(
    UDataTable* ScamFeedbackTable, FName QuestionId, FText QuestionTitle, int32 ChoiceIndex, FText ChoiceLabel,
    EInvestoryScamAnswerQuality Quality, float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta,
    float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge,
    FInvestoryScamLearningFeedback& OutLearningFeedback)
{
    OutLearningFeedback = LearningComponent
        ? LearningComponent->GetScamFeedbackFromTable(ScamFeedbackTable, QuestionTitle, Quality)
        : FInvestoryScamLearningFeedback();

    if (LearningComponent && OutLearningFeedback.bFound)
    {
        LearningComponent->ExposeConcept(
            OutLearningFeedback.Concept,
            QuestionId,
            OutLearningFeedback.LearningPoint.IsEmpty() ? OutLearningFeedback.Explanation : OutLearningFeedback.LearningPoint);
    }

    return EvaluationComponent
        ? EvaluationComponent->RecordScamAnswerDetailed(
            QuestionId,
            ChoiceIndex,
            MoveTemp(ChoiceLabel),
            Quality,
            OutLearningFeedback.Explanation,
            OutLearningFeedback.WarningSign,
            MoneyDelta,
            HappinessDelta,
            KnowledgeDelta,
            CurrentMoney,
            CurrentHappiness,
            CurrentKnowledge)
        : FInvestoryScamFeedback();
}

FInvestoryKnowledgeExamFeedback AInvestoryCharacterBase::RecordKnowledgeExamAnswerForEvaluation(
    FName QuestionId, EInvestoryKnowledgeTopic Topic, int32 ChoiceIndex, FText ChoiceLabel, bool bCorrect,
    float PointsEarned, float MaxPoints, FText Explanation, FText LearningPoint,
    float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge)
{
    return EvaluationComponent
        ? EvaluationComponent->RecordKnowledgeExamAnswer(QuestionId, Topic, ChoiceIndex, MoveTemp(ChoiceLabel), bCorrect,
            PointsEarned, MaxPoints, MoveTemp(Explanation), MoveTemp(LearningPoint),
            CurrentMoney, CurrentHappiness, CurrentKnowledge)
        : FInvestoryKnowledgeExamFeedback();
}

bool AInvestoryCharacterBase::ExposeLearningConcept(EInvestoryLearningConcept Concept, FName SourceId, FText Note)
{
    return LearningComponent && LearningComponent->ExposeConcept(Concept, SourceId, MoveTemp(Note));
}

TArray<EInvestoryLearningConcept> AInvestoryCharacterBase::GetExperiencedLearningConcepts() const
{
    return LearningComponent ? LearningComponent->GetExperiencedConcepts() : TArray<EInvestoryLearningConcept>();
}

int32 AInvestoryCharacterBase::StartFinalKnowledgeExam(UDataTable* QuestionTable, int32 DesiredQuestionCount,
    int32 CurrentKnowledge, int32 RandomSeed, FText& OutMessage)
{
    if (!LearningComponent)
    {
        OutMessage = LOCTEXT("LearningMissingExam", "ไม่พบระบบ Learning");
        return 0;
    }
    return LearningComponent->StartFinalExam(QuestionTable, DesiredQuestionCount, CurrentKnowledge, RandomSeed, OutMessage);
}

bool AInvestoryCharacterBase::GetCurrentFinalKnowledgeQuestion(UDataTable* QuestionTable,
    FInvestoryExamQuestionView& OutQuestion) const
{
    return LearningComponent && LearningComponent->GetCurrentExamQuestion(QuestionTable, OutQuestion);
}

FInvestoryExamSubmitResult AInvestoryCharacterBase::SubmitFinalKnowledgeExamAnswer(UDataTable* QuestionTable,
    int32 ChoiceIndex, float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge)
{
    if (!LearningComponent)
    {
        FInvestoryExamSubmitResult Missing;
        Missing.Message = LOCTEXT("LearningMissingSubmit", "ไม่พบระบบ Learning");
        return Missing;
    }
    return LearningComponent->SubmitCurrentExamAnswer(
        QuestionTable, ChoiceIndex, CurrentMoney, CurrentHappiness, CurrentKnowledge);
}

FInvestoryExamSessionState AInvestoryCharacterBase::GetFinalKnowledgeExamState() const
{
    return LearningComponent ? LearningComponent->GetExamState() : FInvestoryExamSessionState();
}

FInvestoryExamRewardResult AInvestoryCharacterBase::ClaimFinalKnowledgeExamReward()
{
    if (!LearningComponent)
    {
        FInvestoryExamRewardResult Missing;
        Missing.Message = LOCTEXT("LearningMissingReward", "ไม่พบระบบ Learning");
        return Missing;
    }

    FInvestoryExamRewardResult Result = LearningComponent->ClaimExamReward();
    if (Result.bSuccess && Result.MoneyReward > 0.0f && StatusComponent)
    {
        StatusComponent->ChangeMoney(Result.MoneyReward);
        if (EvaluationComponent)
        {
            EvaluationComponent->RecordKnowledgeExamReward(
                Result.MoneyReward,
                StatusComponent->Money,
                StatusComponent->Happiness,
                StatusComponent->Knowledge);
        }
    }
    return Result;
}

void AInvestoryCharacterBase::ResetFinalKnowledgeExam()
{
    if (LearningComponent)
    {
        LearningComponent->ResetFinalExam();
    }
}

void AInvestoryCharacterBase::RecordNewsForEvaluation(FName NewsId, FName TargetStockId, bool bBigNews,
    float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge)
{
    if (EvaluationComponent)
    {
        EvaluationComponent->RecordNewsSeen(NewsId, TargetStockId, bBigNews, CurrentMoney, CurrentHappiness, CurrentKnowledge);
    }
    if (LearningComponent)
    {
        LearningComponent->ExposeConcept(EInvestoryLearningConcept::NewsImpact, NewsId,
            LOCTEXT("NewsImpactLearn", "ข่าวสามารถเปลี่ยนราคาหุ้นและทำให้การตัดสินใจซื้อ/ขายมีผลต่างกัน"));
    }
}

void AInvestoryCharacterBase::RecordShopForEvaluation(FName ItemId, float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta,
    float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge)
{
    if (EvaluationComponent)
    {
        EvaluationComponent->RecordShopPurchase(ItemId, MoneyDelta, HappinessDelta, KnowledgeDelta,
            CurrentMoney, CurrentHappiness, CurrentKnowledge);
    }
}

void AInvestoryCharacterBase::RecordPortfolioViewForEvaluation(FName StockId, int32 Shares, float UnrealizedProfitLoss,
    float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge)
{
    if (EvaluationComponent)
    {
        EvaluationComponent->RecordPortfolioViewed(StockId, Shares, UnrealizedProfitLoss,
            CurrentMoney, CurrentHappiness, CurrentKnowledge);
    }
}


FInvestoryPortfolioInsight AInvestoryCharacterBase::GetPortfolioInsight(
    FName StockId, float CurrentPrice, float CurrentMoney, int32 Knowledge, float RequiredCashReserve) const
{
    return InvestmentComponent
        ? InvestmentComponent->GetPortfolioInsight(StockId, CurrentPrice, CurrentMoney, Knowledge, RequiredCashReserve)
        : FInvestoryPortfolioInsight();
}

FInvestoryPortfolioInsight AInvestoryCharacterBase::OpenPortfolioForLearning(
    FName StockId, float CurrentPrice, float CurrentMoney, int32 CurrentHappiness,
    int32 CurrentKnowledge, float RequiredCashReserve)
{
    const FInvestoryPortfolioInsight Insight = InvestmentComponent
        ? InvestmentComponent->GetPortfolioInsight(StockId, CurrentPrice, CurrentMoney, CurrentKnowledge, RequiredCashReserve)
        : FInvestoryPortfolioInsight();

    if (EvaluationComponent)
    {
        EvaluationComponent->RecordPortfolioViewedDetailed(
            StockId,
            Insight.Snapshot.Shares,
            Insight.Snapshot.UnrealizedProfitLoss,
            Insight.bShowAverageCost,
            Insight.bShowUnrealizedProfitLoss,
            Insight.bShowBreakEvenPrice,
            CurrentMoney,
            CurrentHappiness,
            CurrentKnowledge);
    }

    if (LearningComponent)
    {
        LearningComponent->RecordPortfolioInsight(StockId, Insight);
    }

    return Insight;
}

FInvestoryEvaluationResult AInvestoryCharacterBase::FinalizeInvestoryEvaluation(
    float FinalMoney, int32 FinalHappiness, int32 FinalKnowledge)
{
    if (!EvaluationComponent)
    {
        return FInvestoryEvaluationResult();
    }

    const float PortfolioMarketValue = InvestmentComponent
        ? InvestmentComponent->GetTotalPortfolioMarketValue()
        : 0.0f;

    return EvaluationComponent->FinalizeEvaluationWithPortfolio(
        FinalMoney, PortfolioMarketValue, FinalHappiness, FinalKnowledge);
}

void AInvestoryCharacterBase::RegisterInvestmentMarketPrice(FName StockId, float CurrentPrice)
{
    if (InvestmentComponent)
    {
        InvestmentComponent->UpdateMarketPrice(StockId, CurrentPrice);
    }
}

void AInvestoryCharacterBase::RegisterInvestmentMarketPrices(
    const TArray<FName>& StockIds, const TArray<float>& CurrentPrices)
{
    if (InvestmentComponent)
    {
        InvestmentComponent->UpdateMarketPrices(StockIds, CurrentPrices);
    }
}

float AInvestoryCharacterBase::GetEstimatedNetWorth(float CurrentMoney) const
{
    return InvestmentComponent
        ? InvestmentComponent->GetEstimatedNetWorth(CurrentMoney)
        : FMath::Max(0.0f, CurrentMoney);
}

FInvestoryPlayerProgress AInvestoryCharacterBase::GetEvaluationProgress() const
{
    return EvaluationComponent ? EvaluationComponent->GetProgress() : FInvestoryPlayerProgress();
}

bool AInvestoryCharacterBase::ExportInvestoryEvaluationCsv(
    FString FolderLabel, FString& OutDirectory, FText& OutMessage) const
{
    if (!EvaluationComponent)
    {
        OutMessage = LOCTEXT("EvaluationMissing", "ไม่พบระบบ Evaluation");
        OutDirectory.Reset();
        return false;
    }
    return EvaluationComponent->ExportEvaluationCsv(MoveTemp(FolderLabel), OutDirectory, OutMessage);
}

bool AInvestoryCharacterBase::PlaceBuyOrderForTurn(
    FName StockId,
    int32 Quantity,
    float SubmittedPrice,
    float AvailableCash,
    FText& OutReason)
{
    if (!InvestmentComponent || !TurnFlowComponent)
    {
        OutReason = LOCTEXT("TradeMissingComponents", "ไม่พบระบบลงทุนหรือระบบ Turn Flow");
        return false;
    }

    if (!TurnFlowComponent->CanTrade())
    {
        OutReason = LOCTEXT("NoMarketActionBuy", "ไม่มีสิทธิ์ส่งคำสั่งซื้อในช่วงนี้ หรือใช้ Market Action หมดแล้ว");
        return false;
    }

    if (!InvestmentComponent->PlaceBuyOrder(StockId, Quantity, SubmittedPrice, AvailableCash, OutReason))
    {
        return false;
    }

    TurnFlowComponent->TryConsumeMarketAction();
    return true;
}

bool AInvestoryCharacterBase::PlaceSellOrderForTurn(
    FName StockId,
    int32 Quantity,
    float SubmittedPrice,
    FText& OutReason)
{
    if (!InvestmentComponent || !TurnFlowComponent)
    {
        OutReason = LOCTEXT("TradeMissingComponentsSell", "ไม่พบระบบลงทุนหรือระบบ Turn Flow");
        return false;
    }

    if (!TurnFlowComponent->CanTrade())
    {
        OutReason = LOCTEXT("NoMarketActionSell", "ไม่มีสิทธิ์ส่งคำสั่งขายในช่วงนี้ หรือใช้ Market Action หมดแล้ว");
        return false;
    }

    if (!InvestmentComponent->PlaceSellOrder(StockId, Quantity, SubmittedPrice, OutReason))
    {
        return false;
    }

    TurnFlowComponent->TryConsumeMarketAction();
    return true;
}

#undef LOCTEXT_NAMESPACE
