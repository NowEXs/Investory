#include "InvestoryLearningComponent.h"
#include "InvestoryStatusComponent.h"

#define LOCTEXT_NAMESPACE "InvestoryLearning"

UInvestoryLearningComponent::UInvestoryLearningComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UInvestoryLearningComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AActor* Owner = GetOwner())
    {
        if (UInvestoryInvestmentComponent* Investment = Owner->FindComponentByClass<UInvestoryInvestmentComponent>())
        {
            Investment->OnOrderChanged.AddDynamic(this, &UInvestoryLearningComponent::HandleOrderChanged);
            Investment->OnOrderExecuted.AddDynamic(this, &UInvestoryLearningComponent::HandleOrderExecuted);
        }
    }
}

void UInvestoryLearningComponent::ResetLearningProgress()
{
    ExposedConcepts.Reset();
    ExposureLog.Reset();
    PendingForLearning.Reset();
    ResetFinalExam();
}

bool UInvestoryLearningComponent::ExposeConcept(EInvestoryLearningConcept Concept, FName SourceId, FText Note)
{
    if (Concept == EInvestoryLearningConcept::General || ExposedConcepts.Contains(Concept))
    {
        return false;
    }

    ExposedConcepts.Add(Concept);

    FInvestoryConceptExposureRecord Record;
    Record.Concept = Concept;
    Record.SourceId = SourceId;
    Record.Note = Note;

    if (AActor* Owner = GetOwner())
    {
        if (const UInvestoryEvaluationComponent* Eval = Owner->FindComponentByClass<UInvestoryEvaluationComponent>())
        {
            const FInvestoryPlayerProgress Progress = Eval->GetProgress();
            Record.TurnNumber = Progress.CurrentTurn;
            Record.Year = Progress.CurrentYear;
        }
    }

    ExposureLog.Add(Record);
    NotifyEvaluationExposure(Concept, SourceId, Note);
    return true;
}

bool UInvestoryLearningComponent::HasExperiencedConcept(EInvestoryLearningConcept Concept) const
{
    return Concept == EInvestoryLearningConcept::General || ExposedConcepts.Contains(Concept);
}

void UInvestoryLearningComponent::RecordPortfolioInsight(FName StockId, const FInvestoryPortfolioInsight& Insight)
{
    if (Insight.Snapshot.Shares > 0 && Insight.bShowAverageCost)
    {
        ExposeConcept(EInvestoryLearningConcept::AverageCost, StockId,
            LOCTEXT("LearnAverageCost", "พอร์ตแสดงต้นทุนเฉลี่ยของหุ้นที่ถือ"));
    }

    if (Insight.Snapshot.Shares > 0 && Insight.bShowUnrealizedProfitLoss)
    {
        ExposeConcept(EInvestoryLearningConcept::UnrealizedProfitLoss, StockId,
            LOCTEXT("LearnUnrealized", "พอร์ตแสดงกำไร/ขาดทุนที่ยังไม่ขาย (Unrealized P/L)"));
    }

    if (Insight.Snapshot.Shares > 0 && Insight.bShowBreakEvenPrice)
    {
        ExposeConcept(EInvestoryLearningConcept::BreakEven, StockId,
            LOCTEXT("LearnBreakEven", "พอร์ตแสดงจุดคุ้มทุนหลังค่าธรรมเนียม"));
    }

    if (Insight.bLiquidityWarning)
    {
        ExposeConcept(EInvestoryLearningConcept::Liquidity, StockId,
            LOCTEXT("LearnLiquidity", "ระบบเตือนว่าเงินสดคงเหลือต่ำกว่าเงินสำรองที่แนะนำ"));
    }
}

FInvestoryScamLearningFeedback UInvestoryLearningComponent::GetScamFeedbackFromTable(
    UDataTable* FeedbackTable, FText QuestionTitle, EInvestoryScamAnswerQuality Quality) const
{
    FInvestoryScamLearningFeedback Result;
    if (!FeedbackTable || FeedbackTable->GetRowStruct() != FInvestoryScamFeedbackRow::StaticStruct())
    {
        return Result;
    }

    const FString Target = QuestionTitle.ToString().TrimStartAndEnd();
    for (const TPair<FName, uint8*>& Pair : FeedbackTable->GetRowMap())
    {
        const FInvestoryScamFeedbackRow* Row = reinterpret_cast<const FInvestoryScamFeedbackRow*>(Pair.Value);
        if (!Row || !Row->QuestionTitle.ToString().TrimStartAndEnd().Equals(Target, ESearchCase::IgnoreCase))
        {
            continue;
        }

        Result.bFound = true;
        Result.Concept = Row->Concept;
        Result.LearningPoint = Row->LearningPoint;

        switch (Quality)
        {
        case EInvestoryScamAnswerQuality::Best:
            Result.Explanation = Row->BestExplanation;
            Result.WarningSign = Row->BestWarningSign;
            break;
        case EInvestoryScamAnswerQuality::Good:
            Result.Explanation = Row->GoodExplanation;
            Result.WarningSign = Row->GoodWarningSign;
            break;
        default:
            Result.Explanation = Row->WrongExplanation;
            Result.WarningSign = Row->WrongWarningSign;
            break;
        }
        return Result;
    }

    // Generic fallback so every Scam question still gives useful feedback even before its dedicated row is authored.
    Result.bFound = true;
    Result.Concept = EInvestoryLearningConcept::General;
    Result.WarningSign = LOCTEXT("ScamGenericWarning", "สัญญาณเตือน: ความเร่งด่วน การขอข้อมูลสำคัญ หรือช่องทางที่ยืนยันตัวตนไม่ได้");
    Result.LearningPoint = LOCTEXT("ScamGenericLearning", "หยุด ตรวจสอบแหล่งที่มาผ่านช่องทางทางการ และอย่าเปิดเผยข้อมูลสำคัญเพราะแรงกดดัน");
    switch (Quality)
    {
    case EInvestoryScamAnswerQuality::Best:
        Result.Explanation = LOCTEXT("ScamGenericBest", "ทางเลือกนี้ลดความเสี่ยงได้ดี เพราะให้ความสำคัญกับการตรวจสอบก่อนดำเนินการ");
        break;
    case EInvestoryScamAnswerQuality::Good:
        Result.Explanation = LOCTEXT("ScamGenericGood", "ทางเลือกนี้ช่วยลดความเสี่ยงบางส่วน แต่ยังควรยืนยันข้อมูลผ่านช่องทางทางการให้ชัดเจน");
        break;
    default:
        Result.Explanation = LOCTEXT("ScamGenericWrong", "ทางเลือกนี้เปิดโอกาสให้มิจฉาชีพใช้แรงกดดันหรือข้อมูลปลอมเพื่อให้คุณตัดสินใจเร็วเกินไป");
        break;
    }
    return Result;
}

int32 UInvestoryLearningComponent::StartFinalExam(UDataTable* QuestionTable, int32 DesiredQuestionCount,
    int32 CurrentKnowledge, int32 RandomSeed, FText& OutMessage)
{
    ResetFinalExam();

    if (!QuestionTable || QuestionTable->GetRowStruct() != FInvestoryKnowledgeQuestionRow::StaticStruct())
    {
        OutMessage = LOCTEXT("InvalidExamTable", "DataTable ข้อสอบไม่ถูกต้องหรือใช้ Row Struct ผิดประเภท");
        return 0;
    }

    TArray<FName> EligibleRows;
    for (const TPair<FName, uint8*>& Pair : QuestionTable->GetRowMap())
    {
        const FInvestoryKnowledgeQuestionRow* Row = reinterpret_cast<const FInvestoryKnowledgeQuestionRow*>(Pair.Value);
        if (!Row)
        {
            continue;
        }

        if (CurrentKnowledge < FMath::Max(0, Row->MinKnowledge))
        {
            continue;
        }

        if (Row->bRequireExposure && !HasExperiencedConcept(Row->Concept))
        {
            continue;
        }

        EligibleRows.Add(Pair.Key);
    }

    if (EligibleRows.IsEmpty())
    {
        OutMessage = LOCTEXT("NoEligibleExamQuestions", "ยังไม่มีหัวข้อความรู้ที่ผู้เล่นได้เรียนรู้เพียงพอสำหรับข้อสอบ");
        return 0;
    }

    FRandomStream Stream(RandomSeed == 0 ? FMath::Rand() : RandomSeed);
    for (int32 Index = EligibleRows.Num() - 1; Index > 0; --Index)
    {
        const int32 SwapIndex = Stream.RandRange(0, Index);
        EligibleRows.Swap(Index, SwapIndex);
    }

    const int32 Requested = DesiredQuestionCount <= 0 ? EligibleRows.Num() : DesiredQuestionCount;
    const int32 ActualCount = FMath::Min(Requested, EligibleRows.Num());
    ActiveQuestionRows.Append(EligibleRows.GetData(), ActualCount);

    bExamActive = ActualCount > 0;
    bExamFinished = false;
    CurrentExamIndex = 0;
    CurrentExamKnowledge = FMath::Max(0, CurrentKnowledge);
    ExamCorrectAnswers = 0;
    ExamPointsEarned = 0.0f;
    ExamMaxPoints = 0.0f;

    for (const FName RowName : ActiveQuestionRows)
    {
        if (const FInvestoryKnowledgeQuestionRow* Row = FindQuestion(QuestionTable, RowName))
        {
            ExamMaxPoints += FMath::Max(0.0f, Row->Points);
        }
    }

    OutMessage = FText::Format(
        LOCTEXT("ExamStarted", "เริ่มแบบประเมินความรู้ {0} ข้อ จากหัวข้อที่ผู้เล่นเคยเรียนรู้ในเกม"),
        FText::AsNumber(ActualCount));
    return ActualCount;
}

bool UInvestoryLearningComponent::GetCurrentExamQuestion(UDataTable* QuestionTable, FInvestoryExamQuestionView& OutQuestion) const
{
    OutQuestion = FInvestoryExamQuestionView();
    if (!bExamActive || bExamFinished || !ActiveQuestionRows.IsValidIndex(CurrentExamIndex))
    {
        return false;
    }

    const FName RowName = ActiveQuestionRows[CurrentExamIndex];
    const FInvestoryKnowledgeQuestionRow* Row = FindQuestion(QuestionTable, RowName);
    if (!Row)
    {
        return false;
    }

    OutQuestion.RowName = RowName;
    OutQuestion.QuestionId = Row->QuestionId.IsNone() ? RowName : Row->QuestionId;
    OutQuestion.Topic = Row->Topic;
    OutQuestion.Concept = Row->Concept;
    OutQuestion.Difficulty = Row->Difficulty;
    OutQuestion.Question = Row->Question;
    OutQuestion.Choices = { Row->ChoiceA, Row->ChoiceB, Row->ChoiceC, Row->ChoiceD };
    OutQuestion.bHintUnlocked = CurrentExamKnowledge >= FMath::Max(0, KnowledgeForExamHint) && !Row->Hint.IsEmpty();
    OutQuestion.Hint = OutQuestion.bHintUnlocked ? Row->Hint : FText::GetEmpty();
    OutQuestion.bEliminateOneWrongChoice = CurrentExamKnowledge >= FMath::Max(0, KnowledgeForExamEliminateChoice);
    if (OutQuestion.bEliminateOneWrongChoice)
    {
        const int32 CorrectIndex = FMath::Clamp(Row->CorrectChoiceIndex, 0, 3);
        OutQuestion.EliminatedChoiceIndex = (CorrectIndex + 1 + CurrentExamIndex) % 4;
        if (OutQuestion.EliminatedChoiceIndex == CorrectIndex)
        {
            OutQuestion.EliminatedChoiceIndex = (CorrectIndex + 1) % 4;
        }
    }
    OutQuestion.QuestionNumber = CurrentExamIndex + 1;
    OutQuestion.TotalQuestions = ActiveQuestionRows.Num();
    return true;
}

FInvestoryExamSubmitResult UInvestoryLearningComponent::SubmitCurrentExamAnswer(UDataTable* QuestionTable,
    int32 ChoiceIndex, float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge)
{
    FInvestoryExamSubmitResult Result;
    if (!bExamActive || bExamFinished || !ActiveQuestionRows.IsValidIndex(CurrentExamIndex))
    {
        Result.Message = LOCTEXT("ExamNotActive", "ไม่มีข้อสอบที่กำลังทำอยู่");
        return Result;
    }

    if (ChoiceIndex < 0 || ChoiceIndex > 3)
    {
        Result.Message = LOCTEXT("InvalidExamChoice", "ตัวเลือกคำตอบไม่ถูกต้อง");
        return Result;
    }

    const FName RowName = ActiveQuestionRows[CurrentExamIndex];
    const FInvestoryKnowledgeQuestionRow* Row = FindQuestion(QuestionTable, RowName);
    if (!Row)
    {
        Result.Message = LOCTEXT("ExamRowMissing", "ไม่พบข้อมูลคำถามใน DataTable");
        return Result;
    }

    const int32 CorrectIndex = FMath::Clamp(Row->CorrectChoiceIndex, 0, 3);
    const bool bCorrect = ChoiceIndex == CorrectIndex;
    const float MaxPoints = FMath::Max(0.0f, Row->Points);
    const float Earned = bCorrect ? MaxPoints : 0.0f;

    Result.bAccepted = true;
    Result.bCorrect = bCorrect;
    Result.SelectedChoiceIndex = ChoiceIndex;
    Result.CorrectChoiceIndex = CorrectIndex;
    Result.SelectedChoice = ChoiceText(*Row, ChoiceIndex);
    Result.CorrectChoice = ChoiceText(*Row, CorrectIndex);

    if (bCorrect)
    {
        ++ExamCorrectAnswers;
        ExamPointsEarned += Earned;
    }

    if (AActor* Owner = GetOwner())
    {
        if (UInvestoryEvaluationComponent* Eval = Owner->FindComponentByClass<UInvestoryEvaluationComponent>())
        {
            Result.Feedback = Eval->RecordKnowledgeExamAnswer(
                Row->QuestionId.IsNone() ? RowName : Row->QuestionId,
                Row->Topic,
                ChoiceIndex,
                Result.SelectedChoice,
                bCorrect,
                Earned,
                MaxPoints,
                Row->Explanation,
                Row->LearningPoint,
                CurrentMoney,
                CurrentHappiness,
                CurrentKnowledge);
        }
    }

    // The explanation itself is learning content, but the question list was already frozen at exam start.
    ExposeConcept(Row->Concept, RowName, Row->LearningPoint);

    ++CurrentExamIndex;
    if (CurrentExamIndex >= ActiveQuestionRows.Num())
    {
        bExamActive = false;
        bExamFinished = true;
    }

    Result.bExamFinished = bExamFinished;
    Result.Message = bCorrect ? LOCTEXT("ExamAnswerCorrect", "ตอบถูก") : LOCTEXT("ExamAnswerWrong", "ยังไม่ถูก");
    return Result;
}

FInvestoryExamSessionState UInvestoryLearningComponent::GetExamState() const
{
    FInvestoryExamSessionState State;
    State.bActive = bExamActive;
    State.bFinished = bExamFinished;
    State.TotalQuestions = ActiveQuestionRows.Num();
    State.CurrentQuestionNumber = bExamFinished ? State.TotalQuestions : FMath::Min(CurrentExamIndex + 1, State.TotalQuestions);
    State.CorrectAnswers = ExamCorrectAnswers;
    State.PointsEarned = ExamPointsEarned;
    State.MaxPoints = ExamMaxPoints;
    State.Percent = ExamMaxPoints > KINDA_SMALL_NUMBER
        ? FMath::Clamp((ExamPointsEarned / ExamMaxPoints) * 100.0f, 0.0f, 100.0f)
        : 0.0f;
    return State;
}

FInvestoryExamRewardResult UInvestoryLearningComponent::ClaimExamReward()
{
    FInvestoryExamRewardResult Result;
    const FInvestoryExamSessionState State = GetExamState();
    Result.ExamPercent = State.Percent;

    if (!bExamFinished)
    {
        Result.Message = LOCTEXT("ExamRewardNotFinished", "ต้องทำแบบประเมินความรู้ให้ครบก่อนรับผลประโยชน์");
        return Result;
    }

    if (bExamRewardClaimed)
    {
        Result.bAlreadyClaimed = true;
        Result.Message = LOCTEXT("ExamRewardAlreadyClaimed", "รับผลประโยชน์จากการสอบครั้งนี้ไปแล้ว");
        return Result;
    }

    const float HighThreshold = FMath::Clamp(ExamHighRewardThreshold, 0.0f, 100.0f);
    const float MidThreshold = FMath::Clamp(ExamMidRewardThreshold, 0.0f, HighThreshold);
    if (State.Percent >= HighThreshold)
    {
        Result.MoneyReward = FMath::Max(0.0f, ExamHighMoneyReward);
    }
    else if (State.Percent >= MidThreshold)
    {
        Result.MoneyReward = FMath::Max(0.0f, ExamMidMoneyReward);
    }

    bExamRewardClaimed = true;
    Result.bSuccess = true;
    Result.Message = Result.MoneyReward > 0.0f
        ? FText::Format(LOCTEXT("ExamRewardGranted", "ผลการประเมิน {0}% ได้รับทุนความรู้ {1} บาท"),
            FText::AsNumber(FMath::RoundToInt(State.Percent)), FText::AsNumber(Result.MoneyReward))
        : FText::Format(LOCTEXT("ExamRewardNoMoney", "ผลการประเมิน {0}% ยังไม่ได้รับทุน แต่คำอธิบายหลังตอบช่วยทบทวนความรู้ได้"),
            FText::AsNumber(FMath::RoundToInt(State.Percent)));
    return Result;
}

void UInvestoryLearningComponent::ResetFinalExam()
{
    ActiveQuestionRows.Reset();
    CurrentExamIndex = 0;
    CurrentExamKnowledge = 0;
    ExamCorrectAnswers = 0;
    ExamPointsEarned = 0.0f;
    ExamMaxPoints = 0.0f;
    bExamActive = false;
    bExamFinished = false;
    bExamRewardClaimed = false;
}

void UInvestoryLearningComponent::HandleOrderChanged(FInvestoryPendingOrder Order)
{
    if (Order.Status == EInvestoryOrderStatus::Pending)
    {
        if (!PendingForLearning.Contains(Order.StockId))
        {
            PendingForLearning.Add(Order.StockId, Order);
        }
        return;
    }

    if (Order.Status == EInvestoryOrderStatus::Cancelled || Order.Status == EInvestoryOrderStatus::Rejected)
    {
        PendingForLearning.Remove(Order.StockId);
    }
}

void UInvestoryLearningComponent::HandleOrderExecuted(FInvestoryExecutionResult Result)
{
    if (!Result.bSuccess)
    {
        PendingForLearning.Remove(Result.StockId);
        return;
    }

    if (Result.Fee > KINDA_SMALL_NUMBER)
    {
        ExposeConcept(EInvestoryLearningConcept::TransactionFee, Result.StockId,
            LOCTEXT("ExecutionFeeLearn", "คำสั่งซื้อขายมีค่าธรรมเนียมและค่าธรรมเนียมส่งผลต่อต้นทุน/ผลตอบแทนจริง"));
    }

    if (const FInvestoryPendingOrder* Pending = PendingForLearning.Find(Result.StockId))
    {
        if (!FMath::IsNearlyEqual(Pending->SubmittedPrice, Result.ExecutedPrice, 0.01f))
        {
            ExposeConcept(EInvestoryLearningConcept::PendingExecution, Result.StockId,
                LOCTEXT("ExecutionPriceLearn", "ราคาตอนส่งคำสั่งอาจต่างจากราคาที่ดำเนินการจริงเมื่อคำสั่งรอ 1 เทิร์น"));
        }
    }

    if (Result.Side == EInvestoryOrderSide::Sell)
    {
        ExposeConcept(EInvestoryLearningConcept::RealizedProfitLoss, Result.StockId,
            LOCTEXT("RealizedLearn", "เมื่อขายหุ้น กำไร/ขาดทุนของสถานะนั้นจึงกลายเป็น Realized P/L"));
    }

    PendingForLearning.Remove(Result.StockId);
}

const FInvestoryKnowledgeQuestionRow* UInvestoryLearningComponent::FindQuestion(UDataTable* QuestionTable, FName RowName) const
{
    if (!QuestionTable || QuestionTable->GetRowStruct() != FInvestoryKnowledgeQuestionRow::StaticStruct())
    {
        return nullptr;
    }
    return QuestionTable->FindRow<FInvestoryKnowledgeQuestionRow>(RowName, TEXT("InvestoryFinalExam"), false);
}

FText UInvestoryLearningComponent::ChoiceText(const FInvestoryKnowledgeQuestionRow& Row, int32 ChoiceIndex) const
{
    switch (ChoiceIndex)
    {
    case 0: return Row.ChoiceA;
    case 1: return Row.ChoiceB;
    case 2: return Row.ChoiceC;
    case 3: return Row.ChoiceD;
    default: return FText::GetEmpty();
    }
}

void UInvestoryLearningComponent::NotifyEvaluationExposure(EInvestoryLearningConcept Concept, FName SourceId, const FText& Note)
{
    if (AActor* Owner = GetOwner())
    {
        if (UInvestoryEvaluationComponent* Eval = Owner->FindComponentByClass<UInvestoryEvaluationComponent>())
        {
            const UEnum* Enum = StaticEnum<EInvestoryLearningConcept>();
            const FName ConceptName = Enum
                ? FName(*Enum->GetNameStringByValue(static_cast<int64>(Concept)))
                : NAME_None;
            Eval->RecordLearningConceptExposure(ConceptName, SourceId, Note);
        }
    }
}

#undef LOCTEXT_NAMESPACE
