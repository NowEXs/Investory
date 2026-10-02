#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InvestoryTurnFlowComponent.h"
#include "InvestoryEvaluationComponent.h"
#include "InvestoryLearningComponent.h"
#include "InvestoryCharacterBase.generated.h"

class UInvestoryStatusComponent;
class UInvestoryInvestmentComponent;

USTRUCT(BlueprintType)
struct FInvestoryTurnResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Investory|Turn")
    bool bCanRoll = false;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Turn")
    bool bForcedThroughBurnout = false;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Turn")
    bool bLivingExpenseDue = false;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Turn")
    bool bLivingExpensePaid = false;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Turn")
    bool bLivingExpenseUnpaid = false;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Turn")
    int32 RollNumber = 0;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Turn")
    int32 HappinessSpent = 0;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Turn")
    float MoneySpent = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Turn")
    FText Message;
};

USTRUCT(BlueprintType)
struct FInvestoryRestResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Investory|Rest")
    bool bSuccess = false;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Rest")
    float MoneySpent = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Rest")
    int32 HappinessGained = 0;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Rest")
    FText Message;
};

/**
 * Lightweight C++ base class for the existing BP_TopDownCharacter.
 * Blueprint keeps movement, tile selection and UI. C++ owns reusable rules/calculations.
 */
UCLASS(Blueprintable)
class THESIS_API AInvestoryCharacterBase : public ACharacter
{
    GENERATED_BODY()

public:
    AInvestoryCharacterBase();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Investory|Components")
    TObjectPtr<UInvestoryStatusComponent> StatusComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Investory|Components")
    TObjectPtr<UInvestoryInvestmentComponent> InvestmentComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Investory|Components")
    TObjectPtr<UInvestoryTurnFlowComponent> TurnFlowComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Investory|Components")
    TObjectPtr<UInvestoryEvaluationComponent> EvaluationComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Investory|Components")
    TObjectPtr<UInvestoryLearningComponent> LearningComponent;

    // ---- Pressure rules ----------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Turn Rules", meta=(ClampMin="0"))
    int32 HappinessCostPerRoll = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Turn Rules", meta=(ClampMin="0"))
    int32 ExtraHappinessCostWhenForcingBurnout = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Turn Rules", meta=(ClampMin="0"))
    int32 LivingExpenseEveryRolls = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Turn Rules", meta=(ClampMin="0.0"))
    float LivingExpenseAmount = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Turn Rules", meta=(ClampMin="0"))
    int32 UnpaidExpenseHappinessPenalty = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Turn Rules", meta=(ClampMin="0"))
    int32 RestHappinessGain = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Turn Rules", meta=(ClampMin="0.0"))
    float RestMoneyCost = 1000.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Investory|Turn")
    int32 RollCount = 0;

    // ---- High-level turn flow ---------------------------------------------

    /**
     * Starts the gameplay turn and applies periodic income once.
     * Safe to call again during the same active turn (e.g. Force Burnout retry):
     * it will not increment TurnNumber or grant income twice.
     */
    UFUNCTION(BlueprintCallable, Category="Investory|Turn Flow")
    FInvestoryGameplayTurnResult StartGameplayTurn();

    UFUNCTION(BlueprintCallable, Category="Investory|Turn Flow")
    void MarkResolvingBoard();

    /** Opens the post-tile market phase. Normal tile = 1 action, Investment tile = bonus actions. */
    UFUNCTION(BlueprintCallable, Category="Investory|Turn Flow")
    int32 OpenMarketPhase(bool bInvestmentTile);

    UFUNCTION(BlueprintCallable, Category="Investory|Turn Flow")
    void EndGameplayTurn();

    UFUNCTION(BlueprintPure, Category="Investory|Turn Flow")
    bool CanTradeThisTurn() const;

    UFUNCTION(BlueprintPure, Category="Investory|Turn Flow")
    int32 GetMarketActionsRemaining() const;

    // ---- Roll / burnout ----------------------------------------------------

    /**
     * Call immediately before the existing dice-roll flow.
     * Normal state: consumes Happiness and allows the roll.
     * Burnout: returns false unless bForceThroughBurnout is true.
     * Every N successful rolls, living expenses are charged from spendable cash.
     */
    UFUNCTION(BlueprintCallable, Category="Investory|Turn")
    bool TryStartRoll(bool bForceThroughBurnout, FInvestoryTurnResult& OutResult);

    /** Paid rest used by the Burnout popup. Fails when spendable money is below RestMoneyCost. */
    UFUNCTION(BlueprintCallable, Category="Investory|Turn")
    bool TryRestInsteadOfRoll(FInvestoryRestResult& OutResult);

    /** Legacy compatibility node. Prefer TryRestInsteadOfRoll so Blueprint can show failure feedback. */
    UFUNCTION(BlueprintCallable, Category="Investory|Turn", meta=(DeprecatedFunction, DeprecationMessage="Use TryRestInsteadOfRoll for paid rest and result feedback."))
    void RestInsteadOfRoll();

    // ---- Status helpers ----------------------------------------------------

    /** Convenience node for Event / Scam / Shop choices. */
    UFUNCTION(BlueprintCallable, Category="Investory|Status")
    void ApplyStatusChange(float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta);

    UFUNCTION(BlueprintPure, Category="Investory|Status")
    float GetSpendableMoney() const;

    UFUNCTION(BlueprintPure, Category="Investory|Turn")
    bool CanRollNormally() const;


    // ---- Research / evaluation --------------------------------------------

    /** Start a clean evaluation session. Call once after your Blueprint status values are initialized. */
    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation")
    void BeginEvaluationSession(float InitialMoney, int32 InitialHappiness, int32 InitialKnowledge, FString SessionLabel);

    /** Keep the research context aligned with the board year/lap. */
    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation")
    void SetEvaluationYear(int32 Year);

    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|Event")
    void RecordEventChoiceForEvaluation(FName EventId, int32 ChoiceIndex, FText ChoiceLabel,
        float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta,
        float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge);

    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|Scam")
    void RecordScamAnswerForEvaluation(FName QuestionId, int32 ChoiceIndex, EInvestoryScamAnswerQuality Quality,
        float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta,
        float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge);


    /** Detailed Scam recorder that also returns a feedback struct ready for the feedback page. */
    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|Scam")
    FInvestoryScamFeedback RecordScamAnswerDetailedForEvaluation(FName QuestionId, int32 ChoiceIndex,
        FText ChoiceLabel, EInvestoryScamAnswerQuality Quality, FText Explanation, FText WarningSign,
        float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta,
        float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge);


    /** One-node Scam flow: lookup feedback by displayed question title, record research log, and expose the learned concept. */
    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|Scam")
    FInvestoryScamFeedback RecordScamAnswerWithLearningForEvaluation(UDataTable* ScamFeedbackTable,
        FName QuestionId, FText QuestionTitle, int32 ChoiceIndex, FText ChoiceLabel,
        EInvestoryScamAnswerQuality Quality,
        float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta,
        float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge,
        FInvestoryScamLearningFeedback& OutLearningFeedback);

    /** Fixed Final Exam / knowledge-check recorder, separate from random Scam encounters. */
    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|Knowledge Exam")
    FInvestoryKnowledgeExamFeedback RecordKnowledgeExamAnswerForEvaluation(FName QuestionId,
        EInvestoryKnowledgeTopic Topic, int32 ChoiceIndex, FText ChoiceLabel, bool bCorrect,
        float PointsEarned, float MaxPoints, FText Explanation, FText LearningPoint,
        float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge);


    UFUNCTION(BlueprintCallable, Category="Investory|Learning")
    bool ExposeLearningConcept(EInvestoryLearningConcept Concept, FName SourceId, FText Note);

    UFUNCTION(BlueprintPure, Category="Investory|Learning")
    TArray<EInvestoryLearningConcept> GetExperiencedLearningConcepts() const;

    /** Starts the Final Exam from questions the player has actually been exposed to. */
    UFUNCTION(BlueprintCallable, Category="Investory|Learning|Final Exam")
    int32 StartFinalKnowledgeExam(UDataTable* QuestionTable, int32 DesiredQuestionCount, int32 CurrentKnowledge,
        int32 RandomSeed, FText& OutMessage);

    UFUNCTION(BlueprintCallable, Category="Investory|Learning|Final Exam")
    bool GetCurrentFinalKnowledgeQuestion(UDataTable* QuestionTable, FInvestoryExamQuestionView& OutQuestion) const;

    UFUNCTION(BlueprintCallable, Category="Investory|Learning|Final Exam")
    FInvestoryExamSubmitResult SubmitFinalKnowledgeExamAnswer(UDataTable* QuestionTable, int32 ChoiceIndex,
        float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge);

    UFUNCTION(BlueprintPure, Category="Investory|Learning|Final Exam")
    FInvestoryExamSessionState GetFinalKnowledgeExamState() const;

    /** Applies the one-time exam scholarship to StatusComponent. Call SyncStatusToCharacter afterward in the current hybrid setup. */
    UFUNCTION(BlueprintCallable, Category="Investory|Learning|Final Exam")
    FInvestoryExamRewardResult ClaimFinalKnowledgeExamReward();

    UFUNCTION(BlueprintCallable, Category="Investory|Learning|Final Exam")
    void ResetFinalKnowledgeExam();

    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|News")
    void RecordNewsForEvaluation(FName NewsId, FName TargetStockId, bool bBigNews,
        float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge);

    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|Shop")
    void RecordShopForEvaluation(FName ItemId, float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta,
        float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge);

    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|Investment")
    void RecordPortfolioViewForEvaluation(FName StockId, int32 Shares, float UnrealizedProfitLoss,
        float CurrentMoney, int32 CurrentHappiness, int32 CurrentKnowledge);


    /** Read-only portfolio helper for an anytime portfolio screen. Does not consume a Market Action. */
    UFUNCTION(BlueprintPure, Category="Investory|Portfolio")
    FInvestoryPortfolioInsight GetPortfolioInsight(FName StockId, float CurrentPrice, float CurrentMoney,
        int32 Knowledge, float RequiredCashReserve) const;


    /** Preferred portfolio node: view anytime, does not consume Market Action, records the view, and unlocks concepts actually shown. */
    UFUNCTION(BlueprintCallable, Category="Investory|Portfolio")
    FInvestoryPortfolioInsight OpenPortfolioForLearning(FName StockId, float CurrentPrice, float CurrentMoney,
        int32 CurrentHappiness, int32 CurrentKnowledge, float RequiredCashReserve);

    /** Final result for WBP_Result. Use the Blueprint character's final values here. */
    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation")
    FInvestoryEvaluationResult FinalizeInvestoryEvaluation(float FinalMoney, int32 FinalHappiness, int32 FinalKnowledge);

    UFUNCTION(BlueprintPure, Category="Investory|Evaluation")
    FInvestoryPlayerProgress GetEvaluationProgress() const;

    /** Writes summary.csv + detailed logs to Saved/InvestoryEvaluation/<FolderLabel>. */
    UFUNCTION(BlueprintCallable, Category="Investory|Evaluation|Export")
    bool ExportInvestoryEvaluationCsv(FString FolderLabel, FString& OutDirectory, FText& OutMessage) const;

    // ---- Market-action-safe order wrappers --------------------------------

    /**
     * Optional replacement for calling InvestmentComponent.PlaceBuyOrder directly.
     * It enforces the current Market Phase action limit and consumes one action on success.
     */
    UFUNCTION(BlueprintCallable, Category="Investory|Trading")
    bool PlaceBuyOrderForTurn(FName StockId, int32 Quantity, float SubmittedPrice, float AvailableCash, FText& OutReason);

    /** Same as PlaceBuyOrderForTurn, for sells. */
    UFUNCTION(BlueprintCallable, Category="Investory|Trading")
    bool PlaceSellOrderForTurn(FName StockId, int32 Quantity, float SubmittedPrice, FText& OutReason);
};
