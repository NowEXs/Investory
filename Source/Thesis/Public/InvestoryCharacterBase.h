#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
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

/**
 * Lightweight C++ base class for the existing BP_TopDownCharacter.
 * Reparent the Blueprint to this class to get the status + investment systems
 * without replacing the existing movement, dice, tile, UI, or animation logic.
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

    // Pressure rules. These are intentionally editable from Blueprint defaults.
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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Investory|Turn")
    int32 RollCount = 0;

    /**
     * Call immediately before the existing dice-roll flow.
     * Normal state: consumes Happiness and allows the roll.
     * Burnout: returns false unless bForceThroughBurnout is true.
     * Every N rolls, living expenses are automatically charged from spendable cash.
     */
    UFUNCTION(BlueprintCallable, Category="Investory|Turn")
    bool TryStartRoll(bool bForceThroughBurnout, FInvestoryTurnResult& OutResult);

    /** Rest instead of rolling. Blueprint decides when the skipped turn is considered complete. */
    UFUNCTION(BlueprintCallable, Category="Investory|Turn")
    void RestInsteadOfRoll();

    /** Convenience node for Event / Scam / Shop choices. */
    UFUNCTION(BlueprintCallable, Category="Investory|Status")
    void ApplyStatusChange(float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta);

    UFUNCTION(BlueprintPure, Category="Investory|Status")
    float GetSpendableMoney() const;

    UFUNCTION(BlueprintPure, Category="Investory|Turn")
    bool CanRollNormally() const;
};
