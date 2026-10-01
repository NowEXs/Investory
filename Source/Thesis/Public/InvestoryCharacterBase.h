#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InvestoryTurnFlowComponent.h"
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
