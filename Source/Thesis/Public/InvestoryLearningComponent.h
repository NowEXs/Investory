#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "InvestoryEvaluationComponent.h"
#include "InvestoryInvestmentComponent.h"
#include "InvestoryLearningComponent.generated.h"

UENUM(BlueprintType)
enum class EInvestoryLearningConcept : uint8
{
    General UMETA(DisplayName="General"),
    Budgeting UMETA(DisplayName="Budgeting"),
    OpportunityCost UMETA(DisplayName="Opportunity Cost"),
    Liquidity UMETA(DisplayName="Liquidity / Cash Reserve"),
    TransactionFee UMETA(DisplayName="Transaction Fee"),
    PendingExecution UMETA(DisplayName="Submitted vs Executed Price"),
    AverageCost UMETA(DisplayName="Average Cost"),
    UnrealizedProfitLoss UMETA(DisplayName="Unrealized P/L"),
    RealizedProfitLoss UMETA(DisplayName="Realized P/L"),
    BreakEven UMETA(DisplayName="Break-even"),
    NewsImpact UMETA(DisplayName="News Impact"),
    ScamGuaranteedReturn UMETA(DisplayName="Scam: Guaranteed Return"),
    ScamUrgency UMETA(DisplayName="Scam: Urgency / Pressure"),
    ScamFakeAuthority UMETA(DisplayName="Scam: Fake Authority"),
    ScamCredentialPhishing UMETA(DisplayName="Scam: Credential Phishing"),
    ScamDangerousPermissions UMETA(DisplayName="Scam: Dangerous App Permissions"),
    ScamRecovery UMETA(DisplayName="Scam: Recovery After Incident"),
    ScamUnsafeNetwork UMETA(DisplayName="Scam: Unsafe Network")
};

UENUM(BlueprintType)
enum class EInvestoryQuestionDifficulty : uint8
{
    Basic UMETA(DisplayName="Basic"),
    Intermediate UMETA(DisplayName="Intermediate"),
    Advanced UMETA(DisplayName="Advanced")
};

/** DataTable row for the Final Knowledge Exam. CorrectChoiceIndex uses 0=A, 1=B, 2=C, 3=D. */
USTRUCT(BlueprintType)
struct FInvestoryKnowledgeQuestionRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName QuestionId = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EInvestoryKnowledgeTopic Topic = EInvestoryKnowledgeTopic::General;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EInvestoryLearningConcept Concept = EInvestoryLearningConcept::General;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EInvestoryQuestionDifficulty Difficulty = EInvestoryQuestionDifficulty::Basic;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Question;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText ChoiceA;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText ChoiceB;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText ChoiceC;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText ChoiceD;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="3")) int32 CorrectChoiceIndex = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Explanation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText LearningPoint;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Hint;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0")) int32 MinKnowledge = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bRequireExposure = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0")) float Points = 1.0f;
};

/** Separate table so the existing DT_Scam / STRUCT_ScamQuestion does not need to be destroyed or migrated. */
USTRUCT(BlueprintType)
struct FInvestoryScamFeedbackRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText QuestionTitle;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EInvestoryLearningConcept Concept = EInvestoryLearningConcept::ScamUrgency;

    /** True for actual scam/cyber-risk questions. False for legitimate opportunities that reuse the same choice/feedback UI. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bCountsAsScam = true;

    /** Explains whether the situation itself is dangerous/safe and why. This is independent of which answer the player picked. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText SituationRiskExplanation;

    /** Optional per-quality labels. When empty, the code falls back to the original Scam labels. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText WrongResultLabel;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText GoodResultLabel;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText BestResultLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText WrongExplanation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText WrongWarningSign;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText GoodExplanation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText GoodWarningSign;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText BestExplanation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText BestWarningSign;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText LearningPoint;
};

USTRUCT(BlueprintType)
struct FInvestoryScamLearningFeedback
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) bool bFound = false;
    UPROPERTY(BlueprintReadOnly) bool bCountsAsScam = true;
    UPROPERTY(BlueprintReadOnly) EInvestoryLearningConcept Concept = EInvestoryLearningConcept::General;
    UPROPERTY(BlueprintReadOnly) FText SituationRiskExplanation;
    UPROPERTY(BlueprintReadOnly) FText ResultLabel;
    UPROPERTY(BlueprintReadOnly) FText Explanation;
    UPROPERTY(BlueprintReadOnly) FText WarningSign;
    UPROPERTY(BlueprintReadOnly) FText LearningPoint;
};

/** Safe question payload for the Widget. The correct answer is intentionally not exposed here. */
USTRUCT(BlueprintType)
struct FInvestoryExamQuestionView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FName RowName = NAME_None;
    UPROPERTY(BlueprintReadOnly) FName QuestionId = NAME_None;
    UPROPERTY(BlueprintReadOnly) EInvestoryKnowledgeTopic Topic = EInvestoryKnowledgeTopic::General;
    UPROPERTY(BlueprintReadOnly) EInvestoryLearningConcept Concept = EInvestoryLearningConcept::General;
    UPROPERTY(BlueprintReadOnly) EInvestoryQuestionDifficulty Difficulty = EInvestoryQuestionDifficulty::Basic;
    UPROPERTY(BlueprintReadOnly) FText Question;
    UPROPERTY(BlueprintReadOnly) TArray<FText> Choices;
    UPROPERTY(BlueprintReadOnly) bool bHintUnlocked = false;
    UPROPERTY(BlueprintReadOnly) FText Hint;
    UPROPERTY(BlueprintReadOnly) bool bEliminateOneWrongChoice = false;
    UPROPERTY(BlueprintReadOnly) int32 EliminatedChoiceIndex = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) int32 QuestionNumber = 0;
    UPROPERTY(BlueprintReadOnly) int32 TotalQuestions = 0;
};

USTRUCT(BlueprintType)
struct FInvestoryExamSessionState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) bool bActive = false;
    UPROPERTY(BlueprintReadOnly) bool bFinished = false;
    UPROPERTY(BlueprintReadOnly) int32 CurrentQuestionNumber = 0;
    UPROPERTY(BlueprintReadOnly) int32 TotalQuestions = 0;
    UPROPERTY(BlueprintReadOnly) int32 CorrectAnswers = 0;
    UPROPERTY(BlueprintReadOnly) float PointsEarned = 0.0f;
    UPROPERTY(BlueprintReadOnly) float MaxPoints = 0.0f;
    UPROPERTY(BlueprintReadOnly) float Percent = 0.0f;
};

USTRUCT(BlueprintType)
struct FInvestoryExamSubmitResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) bool bAccepted = false;
    UPROPERTY(BlueprintReadOnly) bool bCorrect = false;
    UPROPERTY(BlueprintReadOnly) bool bExamFinished = false;
    UPROPERTY(BlueprintReadOnly) int32 SelectedChoiceIndex = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) int32 CorrectChoiceIndex = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) FText SelectedChoice;
    UPROPERTY(BlueprintReadOnly) FText CorrectChoice;
    UPROPERTY(BlueprintReadOnly) FInvestoryKnowledgeExamFeedback Feedback;
    UPROPERTY(BlueprintReadOnly) FText Message;
};

USTRUCT(BlueprintType)
struct FInvestoryExamRewardResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly) bool bAlreadyClaimed = false;
    UPROPERTY(BlueprintReadOnly) float ExamPercent = 0.0f;
    UPROPERTY(BlueprintReadOnly) float MoneyReward = 0.0f;
    UPROPERTY(BlueprintReadOnly) FText Message;
};

USTRUCT(BlueprintType)
struct FInvestoryConceptExposureRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) EInvestoryLearningConcept Concept = EInvestoryLearningConcept::General;
    UPROPERTY(BlueprintReadOnly) FName SourceId = NAME_None;
    UPROPERTY(BlueprintReadOnly) int32 TurnNumber = 0;
    UPROPERTY(BlueprintReadOnly) int32 Year = 1;
    UPROPERTY(BlueprintReadOnly) FText Note;
};

/**
 * Learning / exam subsystem.
 * - Tracks concepts the game has actually taught or demonstrated.
 * - Builds the Final Exam only from eligible/exposed concepts.
 * - Returns feedback payloads to Widgets.
 * - Observes investment execution automatically for fee/slippage/realized-P&L exposure.
 */
UCLASS(ClassGroup=(Investory), meta=(BlueprintSpawnableComponent))
class THESIS_API UInvestoryLearningComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInvestoryLearningComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Learning|Final Exam", meta=(ClampMin="0"))
    int32 KnowledgeForExamHint = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Learning|Final Exam", meta=(ClampMin="0"))
    int32 KnowledgeForExamEliminateChoice = 20;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Learning|Final Exam Reward", meta=(ClampMin="0.0", ClampMax="100.0"))
    float ExamHighRewardThreshold = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Learning|Final Exam Reward", meta=(ClampMin="0.0"))
    float ExamHighMoneyReward = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Learning|Final Exam Reward", meta=(ClampMin="0.0", ClampMax="100.0"))
    float ExamMidRewardThreshold = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Learning|Final Exam Reward", meta=(ClampMin="0.0"))
    float ExamMidMoneyReward = 500.0f;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Learning")
    TArray<EInvestoryLearningConcept> ExposedConcepts;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Learning")
    TArray<FInvestoryConceptExposureRecord> ExposureLog;

    UFUNCTION(BlueprintCallable, Category="Investory|Learning")
    void ResetLearningProgress();

    UFUNCTION(BlueprintCallable, Category="Investory|Learning")
    bool ExposeConcept(EInvestoryLearningConcept Concept, FName SourceId, FText Note);

    UFUNCTION(BlueprintPure, Category="Investory|Learning")
    bool HasExperiencedConcept(EInvestoryLearningConcept Concept) const;

    UFUNCTION(BlueprintPure, Category="Investory|Learning")
    TArray<EInvestoryLearningConcept> GetExperiencedConcepts() const { return ExposedConcepts; }

    /** Call when the player opens a portfolio detail page. This unlocks exam eligibility only for information actually visible. */
    UFUNCTION(BlueprintCallable, Category="Investory|Learning|Portfolio")
    void RecordPortfolioInsight(FName StockId, const FInvestoryPortfolioInsight& Insight);

    /** Helper for the existing Scam widget. Matches by QuestionTitle, so the old DT_Scam row names do not need to change. */
    UFUNCTION(BlueprintPure, Category="Investory|Learning|Scam")
    FInvestoryScamLearningFeedback GetScamFeedbackFromTable(UDataTable* FeedbackTable, FText QuestionTitle,
        EInvestoryScamAnswerQuality Quality) const;

    /** Starts an exam using only rows the player is eligible to answer from their actual play experience. */
    UFUNCTION(BlueprintCallable, Category="Investory|Learning|Final Exam")
    int32 StartFinalExam(UDataTable* QuestionTable, int32 DesiredQuestionCount, int32 CurrentKnowledge,
        int32 RandomSeed, FText& OutMessage);

    UFUNCTION(BlueprintCallable, Category="Investory|Learning|Final Exam")
    bool GetCurrentExamQuestion(UDataTable* QuestionTable, FInvestoryExamQuestionView& OutQuestion) const;

    UFUNCTION(BlueprintCallable, Category="Investory|Learning|Final Exam")
    FInvestoryExamSubmitResult SubmitCurrentExamAnswer(UDataTable* QuestionTable, int32 ChoiceIndex,
        float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge);

    UFUNCTION(BlueprintPure, Category="Investory|Learning|Final Exam")
    FInvestoryExamSessionState GetExamState() const;


    UFUNCTION(BlueprintCallable, Category="Investory|Learning|Final Exam")
    FInvestoryExamRewardResult ClaimExamReward();

    UFUNCTION(BlueprintCallable, Category="Investory|Learning|Final Exam")
    void ResetFinalExam();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY() TArray<FName> ActiveQuestionRows;
    UPROPERTY() int32 CurrentExamIndex = 0;
    UPROPERTY() int32 CurrentExamKnowledge = 0;
    UPROPERTY() int32 ExamCorrectAnswers = 0;
    UPROPERTY() float ExamPointsEarned = 0.0f;
    UPROPERTY() float ExamMaxPoints = 0.0f;
    UPROPERTY() bool bExamActive = false;
    UPROPERTY() bool bExamFinished = false;
    UPROPERTY() bool bExamRewardClaimed = false;
    UPROPERTY() TMap<FName, FInvestoryPendingOrder> PendingForLearning;

    UFUNCTION() void HandleOrderChanged(FInvestoryPendingOrder Order);
    UFUNCTION() void HandleOrderExecuted(FInvestoryExecutionResult Result);

    const FInvestoryKnowledgeQuestionRow* FindQuestion(UDataTable* QuestionTable, FName RowName) const;
    FText ChoiceText(const FInvestoryKnowledgeQuestionRow& Row, int32 ChoiceIndex) const;
    void NotifyEvaluationExposure(EInvestoryLearningConcept Concept, FName SourceId, const FText& Note);
};
