#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InvestoryInvestmentComponent.h"
#include "InvestoryEvaluationComponent.generated.h"

UENUM(BlueprintType)
enum class EInvestoryExperienceType : uint8
{
    TurnStarted UMETA(DisplayName="Turn Started"),
    EventChoice UMETA(DisplayName="Event Choice"),
    ScamAnswer UMETA(DisplayName="Scam Answer"),
    NewsSeen UMETA(DisplayName="News Seen"),
    BigNewsSeen UMETA(DisplayName="Big News Seen"),
    ShopPurchase UMETA(DisplayName="Shop Purchase"),
    LivingExpense UMETA(DisplayName="Living Expense"),
    BurnoutEncounter UMETA(DisplayName="Burnout Encounter"),
    Rest UMETA(DisplayName="Rest"),
    ForcedBurnout UMETA(DisplayName="Forced Through Burnout"),
    InvestmentSubmitted UMETA(DisplayName="Investment Submitted"),
    InvestmentExecuted UMETA(DisplayName="Investment Executed"),
    InvestmentRejected UMETA(DisplayName="Investment Rejected"),
    PortfolioViewed UMETA(DisplayName="Portfolio Viewed"),
    LearningConceptExposed UMETA(DisplayName="Learning Concept Exposed"),
    KnowledgeExamAnswer UMETA(DisplayName="Knowledge Exam Answer"),
    KnowledgeExamReward UMETA(DisplayName="Knowledge Exam Reward"),
    SessionEnded UMETA(DisplayName="Session Ended")
};

UENUM(BlueprintType)
enum class EInvestoryScamAnswerQuality : uint8
{
    Wrong UMETA(DisplayName="Wrong"),
    Good UMETA(DisplayName="Good"),
    Best UMETA(DisplayName="Best")
};

UENUM(BlueprintType)
enum class EInvestoryKnowledgeTopic : uint8
{
    General UMETA(DisplayName="General"),
    Budgeting UMETA(DisplayName="Budgeting"),
    ScamAwareness UMETA(DisplayName="Scam Awareness"),
    InvestingBasics UMETA(DisplayName="Investing Basics"),
    FeesAndBreakEven UMETA(DisplayName="Fees and Break-even"),
    ProfitAndLoss UMETA(DisplayName="Profit and Loss"),
    NewsAndRisk UMETA(DisplayName="News and Risk"),
    Liquidity UMETA(DisplayName="Liquidity")
};

USTRUCT(BlueprintType)
struct FInvestoryScamFeedback
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) EInvestoryScamAnswerQuality Quality = EInvestoryScamAnswerQuality::Wrong;
    UPROPERTY(BlueprintReadOnly) FText ResultLabel;
    UPROPERTY(BlueprintReadOnly) FText Explanation;
    UPROPERTY(BlueprintReadOnly) FText WarningSign;
    UPROPERTY(BlueprintReadOnly) float MoneyDelta = 0.0f;
    UPROPERTY(BlueprintReadOnly) int32 HappinessDelta = 0;
    UPROPERTY(BlueprintReadOnly) int32 KnowledgeDelta = 0;
};

USTRUCT(BlueprintType)
struct FInvestoryKnowledgeExamFeedback
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) bool bCorrect = false;
    UPROPERTY(BlueprintReadOnly) float PointsEarned = 0.0f;
    UPROPERTY(BlueprintReadOnly) float MaxPoints = 0.0f;
    UPROPERTY(BlueprintReadOnly) float CurrentExamPercent = 0.0f;
    UPROPERTY(BlueprintReadOnly) FText ResultLabel;
    UPROPERTY(BlueprintReadOnly) FText Explanation;
    UPROPERTY(BlueprintReadOnly) FText LearningPoint;
};

USTRUCT(BlueprintType)
struct FInvestoryStatusSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) int32 Sequence = 0;
    UPROPERTY(BlueprintReadOnly) int32 TurnNumber = 0;
    UPROPERTY(BlueprintReadOnly) int32 Year = 1;
    UPROPERTY(BlueprintReadOnly) float Money = 0.0f;
    UPROPERTY(BlueprintReadOnly) int32 Happiness = 0;
    UPROPERTY(BlueprintReadOnly) int32 Knowledge = 0;
    UPROPERTY(BlueprintReadOnly) FName Reason = NAME_None;
};

USTRUCT(BlueprintType)
struct FInvestoryExperienceLogEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) int32 Sequence = 0;
    UPROPERTY(BlueprintReadOnly) int32 TurnNumber = 0;
    UPROPERTY(BlueprintReadOnly) int32 Year = 1;
    UPROPERTY(BlueprintReadOnly) EInvestoryExperienceType Type = EInvestoryExperienceType::EventChoice;
    UPROPERTY(BlueprintReadOnly) FName SourceId = NAME_None;
    UPROPERTY(BlueprintReadOnly) int32 ChoiceIndex = 0;
    UPROPERTY(BlueprintReadOnly) FText ChoiceLabel;
    UPROPERTY(BlueprintReadOnly) FName StockId = NAME_None;
    UPROPERTY(BlueprintReadOnly) float MoneyDelta = 0.0f;
    UPROPERTY(BlueprintReadOnly) int32 HappinessDelta = 0;
    UPROPERTY(BlueprintReadOnly) int32 KnowledgeDelta = 0;
    UPROPERTY(BlueprintReadOnly) bool bSuccess = true;
    UPROPERTY(BlueprintReadOnly) FText Note;
};

USTRUCT(BlueprintType)
struct FInvestoryScamRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) int32 Sequence = 0;
    UPROPERTY(BlueprintReadOnly) int32 TurnNumber = 0;
    UPROPERTY(BlueprintReadOnly) int32 Year = 1;
    UPROPERTY(BlueprintReadOnly) FName QuestionId = NAME_None;
    UPROPERTY(BlueprintReadOnly) int32 ChoiceIndex = 0;
    UPROPERTY(BlueprintReadOnly) FText ChoiceLabel;
    UPROPERTY(BlueprintReadOnly) EInvestoryScamAnswerQuality Quality = EInvestoryScamAnswerQuality::Wrong;
    UPROPERTY(BlueprintReadOnly) FText Explanation;
    UPROPERTY(BlueprintReadOnly) FText WarningSign;
    UPROPERTY(BlueprintReadOnly) float MoneyDelta = 0.0f;
    UPROPERTY(BlueprintReadOnly) int32 HappinessDelta = 0;
    UPROPERTY(BlueprintReadOnly) int32 KnowledgeDelta = 0;
};

USTRUCT(BlueprintType)
struct FInvestoryLearningExposureRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) int32 Sequence = 0;
    UPROPERTY(BlueprintReadOnly) int32 TurnNumber = 0;
    UPROPERTY(BlueprintReadOnly) int32 Year = 1;
    UPROPERTY(BlueprintReadOnly) FName ConceptId = NAME_None;
    UPROPERTY(BlueprintReadOnly) FName SourceId = NAME_None;
    UPROPERTY(BlueprintReadOnly) FText Note;
};

USTRUCT(BlueprintType)
struct FInvestoryKnowledgeExamRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) int32 Sequence = 0;
    UPROPERTY(BlueprintReadOnly) int32 TurnNumber = 0;
    UPROPERTY(BlueprintReadOnly) int32 Year = 1;
    UPROPERTY(BlueprintReadOnly) FName QuestionId = NAME_None;
    UPROPERTY(BlueprintReadOnly) EInvestoryKnowledgeTopic Topic = EInvestoryKnowledgeTopic::General;
    UPROPERTY(BlueprintReadOnly) int32 ChoiceIndex = 0;
    UPROPERTY(BlueprintReadOnly) FText ChoiceLabel;
    UPROPERTY(BlueprintReadOnly) bool bCorrect = false;
    UPROPERTY(BlueprintReadOnly) float PointsEarned = 0.0f;
    UPROPERTY(BlueprintReadOnly) float MaxPoints = 1.0f;
    UPROPERTY(BlueprintReadOnly) FText Explanation;
    UPROPERTY(BlueprintReadOnly) FText LearningPoint;
};

USTRUCT(BlueprintType)
struct FInvestoryInvestmentRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) int32 Sequence = 0;
    UPROPERTY(BlueprintReadOnly) int32 TurnNumber = 0;
    UPROPERTY(BlueprintReadOnly) int32 Year = 1;
    UPROPERTY(BlueprintReadOnly) FName StockId = NAME_None;
    UPROPERTY(BlueprintReadOnly) EInvestoryOrderSide Side = EInvestoryOrderSide::Buy;
    UPROPERTY(BlueprintReadOnly) int32 Quantity = 0;
    UPROPERTY(BlueprintReadOnly) float SubmittedPrice = 0.0f;
    UPROPERTY(BlueprintReadOnly) float ExecutedPrice = 0.0f;
    UPROPERTY(BlueprintReadOnly) float Fee = 0.0f;
    UPROPERTY(BlueprintReadOnly) float CashDelta = 0.0f;
    UPROPERTY(BlueprintReadOnly) float RealizedProfitLoss = 0.0f;
    UPROPERTY(BlueprintReadOnly) bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly) bool bPriceChangedWhilePending = false;
    UPROPERTY(BlueprintReadOnly) bool bAfterRecentNews = false;
    UPROPERTY(BlueprintReadOnly) FText Message;
};

USTRUCT(BlueprintType)
struct FInvestoryPlayerProgress
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Session") FString SessionId;
    UPROPERTY(BlueprintReadOnly, Category="Session") FString StartTimeIso;
    UPROPERTY(BlueprintReadOnly, Category="Session") FString EndTimeIso;
    UPROPERTY(BlueprintReadOnly, Category="Session") int32 CurrentTurn = 0;
    UPROPERTY(BlueprintReadOnly, Category="Session") int32 CurrentYear = 1;

    UPROPERTY(BlueprintReadOnly, Category="Status") float InitialMoney = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="Status") int32 InitialHappiness = 0;
    UPROPERTY(BlueprintReadOnly, Category="Status") int32 InitialKnowledge = 0;
    UPROPERTY(BlueprintReadOnly, Category="Status") float FinalMoney = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="Status") int32 FinalHappiness = 0;
    UPROPERTY(BlueprintReadOnly, Category="Status") int32 FinalKnowledge = 0;

    UPROPERTY(BlueprintReadOnly, Category="Scam") int32 ScamAnswered = 0;
    UPROPERTY(BlueprintReadOnly, Category="Scam") int32 ScamWrong = 0;
    UPROPERTY(BlueprintReadOnly, Category="Scam") int32 ScamGood = 0;
    UPROPERTY(BlueprintReadOnly, Category="Scam") int32 ScamBest = 0;

    UPROPERTY(BlueprintReadOnly, Category="Knowledge Exam") int32 FinalExamAnswered = 0;
    UPROPERTY(BlueprintReadOnly, Category="Knowledge Exam") int32 FinalExamCorrect = 0;
    UPROPERTY(BlueprintReadOnly, Category="Knowledge Exam") float FinalExamPointsEarned = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="Knowledge Exam") float FinalExamMaxPoints = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="Knowledge Exam") float FinalExamRewardMoney = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="Learning") int32 UniqueLearningConceptsExposed = 0;

    UPROPERTY(BlueprintReadOnly, Category="Investment") int32 PortfolioViews = 0;
    UPROPERTY(BlueprintReadOnly, Category="Investment") int32 MeaningfulPortfolioViews = 0;
    UPROPERTY(BlueprintReadOnly, Category="Investment") int32 BuyOrdersSubmitted = 0;
    UPROPERTY(BlueprintReadOnly, Category="Investment") int32 SellOrdersSubmitted = 0;
    UPROPERTY(BlueprintReadOnly, Category="Investment") int32 OrdersExecuted = 0;
    UPROPERTY(BlueprintReadOnly, Category="Investment") int32 OrdersRejected = 0;
    UPROPERTY(BlueprintReadOnly, Category="Investment") float TotalFeesPaid = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="Investment") float TotalRealizedProfitLoss = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="Investment") bool bExperiencedFee = false;
    UPROPERTY(BlueprintReadOnly, Category="Investment") bool bExperiencedPendingPriceChange = false;
    UPROPERTY(BlueprintReadOnly, Category="Investment") bool bExperiencedAverageCost = false;
    UPROPERTY(BlueprintReadOnly, Category="Investment") bool bExperiencedUnrealizedPL = false;
    UPROPERTY(BlueprintReadOnly, Category="Investment") bool bExperiencedBreakEven = false;
    UPROPERTY(BlueprintReadOnly, Category="Investment") bool bExperiencedRealizedPL = false;
    UPROPERTY(BlueprintReadOnly, Category="Investment") bool bTradedAfterNews = false;
    UPROPERTY(BlueprintReadOnly, Category="Investment") bool bHasBoughtStock = false;
    UPROPERTY(BlueprintReadOnly, Category="Investment") bool bHasSoldStock = false;

    UPROPERTY(BlueprintReadOnly, Category="Choices") int32 EventChoices = 0;
    UPROPERTY(BlueprintReadOnly, Category="Choices") int32 NewsSeen = 0;
    UPROPERTY(BlueprintReadOnly, Category="Choices") int32 BigNewsSeen = 0;
    UPROPERTY(BlueprintReadOnly, Category="Choices") int32 ShopPurchases = 0;
    UPROPERTY(BlueprintReadOnly, Category="Choices") int32 BurnoutEncounters = 0;
    UPROPERTY(BlueprintReadOnly, Category="Choices") int32 RestCount = 0;
    UPROPERTY(BlueprintReadOnly, Category="Choices") int32 ForceThroughBurnoutCount = 0;
    UPROPERTY(BlueprintReadOnly, Category="Choices") int32 LivingExpensePaidCount = 0;
    UPROPERTY(BlueprintReadOnly, Category="Choices") int32 LivingExpenseUnpaidCount = 0;
    UPROPERTY(BlueprintReadOnly, Category="Choices") float TotalLivingExpensesPaid = 0.0f;
};

USTRUCT(BlueprintType)
struct FInvestoryEvaluationResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) float FinancialScore = 0.0f;
    UPROPERTY(BlueprintReadOnly) float KnowledgeScore = 0.0f;
    UPROPERTY(BlueprintReadOnly) float ScamKnowledgeScore = 0.0f;
    UPROPERTY(BlueprintReadOnly) float FinalExamScore = 0.0f;
    UPROPERTY(BlueprintReadOnly) float WellbeingScore = 0.0f;
    UPROPERTY(BlueprintReadOnly) float InvestmentLearningScore = 0.0f;
    UPROPERTY(BlueprintReadOnly) float OverallScore = 0.0f;
    UPROPERTY(BlueprintReadOnly) int32 InvestmentMilestonesCompleted = 0;
    UPROPERTY(BlueprintReadOnly) int32 InvestmentMilestonesTotal = 9;
    UPROPERTY(BlueprintReadOnly) FString Grade = TEXT("F");
    UPROPERTY(BlueprintReadOnly) FText Summary;
};

/**
 * Central research/evaluation recorder for INVESTORY.
 * It stores detailed play-experience logs separately from the aggregate values used by the result screen.
 * Investment submission/execution is observed automatically from UInvestoryInvestmentComponent delegates.
 */
UCLASS(ClassGroup=(Investory), meta=(BlueprintSpawnableComponent))
class THESIS_API UInvestoryEvaluationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInvestoryEvaluationComponent();

    // Result weights. They are editable so the thesis rubric can be tuned without recompiling.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Evaluation|Weights", meta=(ClampMin="0.0"))
    float FinancialWeight = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Evaluation|Weights", meta=(ClampMin="0.0"))
    float KnowledgeWeight = 0.30f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Evaluation|Weights", meta=(ClampMin="0.0"))
    float WellbeingWeight = 0.20f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Evaluation|Weights", meta=(ClampMin="0.0"))
    float InvestmentLearningWeight = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Evaluation|Knowledge", meta=(ClampMin="0.0", ClampMax="1.0"))
    float KnowledgeStatusContribution = 0.20f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Evaluation|Knowledge", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ScamContribution = 0.30f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Evaluation|Knowledge", meta=(ClampMin="0.0", ClampMax="1.0"))
    float FinalExamContribution = 0.50f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Evaluation|Scam", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ScamGoodCredit = 0.60f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Evaluation|Scam", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ScamBestCredit = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Evaluation|News", meta=(ClampMin="0"))
    int32 NewsDecisionWindowTurns = 1;

    /** Ending with the same money as the start is intentionally not a perfect financial score. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Evaluation|Financial", meta=(ClampMin="0.0", ClampMax="100.0"))
    float FinancialScoreAtStartingMoney = 70.0f;

    /** Money target that maps to 100 financial points, expressed as a multiple of initial money. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Evaluation|Financial", meta=(ClampMin="1.0"))
    float FinancialTargetMoneyMultiplier = 1.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Evaluation|Financial", meta=(ClampMin="0.0", ClampMax="100.0"))
    float UnpaidLivingExpensePenalty = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Evaluation|Grade", meta=(ClampMin="0.0", ClampMax="100.0"))
    float GradeAThreshold = 85.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Evaluation|Grade", meta=(ClampMin="0.0", ClampMax="100.0"))
    float GradeBThreshold = 70.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Evaluation|Grade", meta=(ClampMin="0.0", ClampMax="100.0"))
    float GradeCThreshold = 55.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Evaluation|Grade", meta=(ClampMin="0.0", ClampMax="100.0"))
    float GradeDThreshold = 40.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Evaluation|Status")
    float KnowledgeScoreMax = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Evaluation|Status")
    float HappinessScoreMax = 30.0f;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Evaluation")
    FInvestoryPlayerProgress Progress;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Evaluation")
    TArray<FInvestoryExperienceLogEntry> ExperienceLog;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Evaluation")
    TArray<FInvestoryScamRecord> ScamLog;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Evaluation")
    TArray<FInvestoryLearningExposureRecord> LearningExposureLog;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Evaluation")
    TArray<FInvestoryKnowledgeExamRecord> KnowledgeExamLog;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Evaluation")
    TArray<FInvestoryInvestmentRecord> InvestmentLog;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Evaluation")
    TArray<FInvestoryStatusSnapshot> StatusHistory;

    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation")
    void BeginSession(float InitialMoney, int32 InitialHappiness, int32 InitialKnowledge, FString SessionLabel);

    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation")
    void SetTurnContext(int32 TurnNumber, int32 Year);

    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation")
    void RecordTurnStarted(int32 TurnNumber, float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge);

    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|Event")
    void RecordEventChoice(FName EventId, int32 ChoiceIndex, FText ChoiceLabel,
        float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta,
        float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge);

    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|Scam")
    void RecordScamAnswer(FName QuestionId, int32 ChoiceIndex, EInvestoryScamAnswerQuality Quality,
        float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta,
        float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge);

    /** Records the scam answer and returns a ready-to-display feedback payload for WBP_Scam feedback. */
    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|Scam")
    FInvestoryScamFeedback RecordScamAnswerDetailed(FName QuestionId, int32 ChoiceIndex, FText ChoiceLabel,
        EInvestoryScamAnswerQuality Quality, FText Explanation, FText WarningSign,
        float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta,
        float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge);

    /** Fixed/final-exam question recorder. This is independent from the random Scam encounters. */
    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|Knowledge Exam")
    FInvestoryKnowledgeExamFeedback RecordKnowledgeExamAnswer(FName QuestionId, EInvestoryKnowledgeTopic Topic,
        int32 ChoiceIndex, FText ChoiceLabel, bool bCorrect, float PointsEarned, float MaxPoints,
        FText Explanation, FText LearningPoint,
        float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge);

    /** Called only on first exposure to a concept by UInvestoryLearningComponent. */
    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|Learning")
    void RecordLearningConceptExposure(FName ConceptId, FName SourceId, FText Note);

    UFUNCTION(BlueprintPure, Category="Investory|Evaluation|Knowledge Exam")
    float GetFinalExamScore() const;


    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|Knowledge Exam")
    void RecordKnowledgeExamReward(float MoneyReward, float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge);

    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|News")
    void RecordNewsSeen(FName NewsId, FName TargetStockId, bool bBigNews,
        float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge);

    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|Shop")
    void RecordShopPurchase(FName ItemId, float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta,
        float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge);

    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|Investment")
    void RecordPortfolioViewed(FName StockId, int32 Shares, float UnrealizedProfitLoss,
        float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge);


    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|Investment")
    void RecordPortfolioViewedDetailed(FName StockId, int32 Shares, float UnrealizedProfitLoss,
        bool bAverageCostVisible, bool bUnrealizedPLVisible, bool bBreakEvenVisible,
        float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge);

    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|Turn")
    void RecordLivingExpense(bool bPaid, float Amount, float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge);

    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|Turn")
    void RecordBurnoutEncounter(float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge);

    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|Turn")
    void RecordRest(float MoneySpent, int32 HappinessGained, float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge);

    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|Turn")
    void RecordForcedBurnout(int32 HappinessSpent, float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge);

    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation")
    FInvestoryEvaluationResult FinalizeEvaluation(float FinalMoney, int32 FinalHappiness, int32 FinalKnowledge);

    UFUNCTION(BlueprintPure, Category="Investory|Evaluation")
    FInvestoryEvaluationResult CalculateEvaluation(float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge) const;

    UFUNCTION(BlueprintPure, Category="Investory|Evaluation")
    FInvestoryPlayerProgress GetProgress() const { return Progress; }

    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|Export")
    bool ExportEvaluationCsv(FString FolderLabel, FString& OutDirectory, FText& OutMessage) const;

protected:
    virtual void BeginPlay() override;

private:
    int32 SequenceCounter = 0;
    int32 LastNewsTurn = INDEX_NONE;
    bool bSessionStarted = false;
    TMap<FName, FInvestoryPendingOrder> PendingSubmissionCache;
    TSet<FName> ExposedConceptIds;
    float CalculateInvestmentLearningScore() const;
    
    UFUNCTION()
    void HandleOrderChanged(FInvestoryPendingOrder Order);

    UFUNCTION()
    void HandleOrderExecuted(FInvestoryExecutionResult Result);

    UFUNCTION()
    void HandleOrderSubmissionAttempt(FInvestoryOrderSubmissionAttempt Attempt);

    void EnsureSessionStarted();
    void AddExperience(EInvestoryExperienceType Type, FName SourceId, int32 ChoiceIndex, const FText& ChoiceLabel,
        FName StockId, float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta, bool bSuccess, const FText& Note);
    void AddSnapshot(FName Reason, float Money, int32 Happiness, int32 Knowledge);
    bool IsAfterRecentNews() const;
    int32 CountInvestmentMilestones() const;
    float CalculateScamKnowledgeScore() const;
    float CalculateFinalExamScore() const;
    FString GradeFromScore(float Score) const;
};
