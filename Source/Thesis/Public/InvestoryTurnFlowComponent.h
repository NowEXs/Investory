#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InvestoryTurnFlowComponent.generated.h"

UENUM(BlueprintType)
enum class EInvestoryTurnPhase : uint8
{
    ReadyToRoll UMETA(DisplayName = "Ready To Roll"),
    ResolvingBoard UMETA(DisplayName = "Resolving Board"),
    Market UMETA(DisplayName = "Market"),
    Finished UMETA(DisplayName = "Finished")
};

USTRUCT(BlueprintType)
struct FInvestoryGameplayTurnResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Investory|Turn Flow")
    bool bStartedNewTurn = false;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Turn Flow")
    int32 TurnNumber = 0;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Turn Flow")
    bool bIncomeGranted = false;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Turn Flow")
    float IncomeGranted = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Turn Flow")
    EInvestoryTurnPhase Phase = EInvestoryTurnPhase::ReadyToRoll;

    UPROPERTY(BlueprintReadOnly, Category="Investory|Turn Flow")
    FText Message;
};

/**
 * Owns the high-level turn loop only. It intentionally does not know about board movement,
 * widgets, news, or stock prices. Blueprint keeps those presentation/gameplay details.
 *
 * Intended flow:
 *   StartTurn -> Roll/resolve tile -> OpenMarketPhase -> Trade or Skip -> EndTurn
 *
 * StartTurn is idempotent while a turn is active, which prevents a Burnout retry from
 * accidentally incrementing the turn or granting periodic income twice.
 */
UCLASS(ClassGroup=(Investory), meta=(BlueprintSpawnableComponent))
class THESIS_API UInvestoryTurnFlowComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInvestoryTurnFlowComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Turn Flow|Income", meta=(ClampMin="0"))
    int32 IncomeEveryTurns = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Turn Flow|Income", meta=(ClampMin="0.0"))
    float IncomeAmount = 1500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Turn Flow|Market", meta=(ClampMin="0"))
    int32 NormalMarketActions = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Turn Flow|Market", meta=(ClampMin="0"))
    int32 InvestmentTileMarketActions = 2;

    /**
     * Keep false while migrating the existing Blueprint flow. Turn it on after Blueprint calls
     * StartGameplayTurn -> resolve tile -> OpenMarketPhase -> EndGameplayTurn.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Turn Flow")
    bool bEnforcePhaseGating = false;

    /** Starts a new turn, or safely returns the current turn if one is already active. */
    UFUNCTION(BlueprintCallable, Category="Investory|Turn Flow")
    FInvestoryGameplayTurnResult StartTurn();

    /** Call after the tile/event has been resolved. Repeated calls do not refill actions. */
    UFUNCTION(BlueprintCallable, Category="Investory|Turn Flow")
    int32 OpenMarketPhase(bool bInvestmentTile);

    /** Consumes exactly one market action. Returns false when trading is not currently allowed. */
    UFUNCTION(BlueprintCallable, Category="Investory|Turn Flow")
    bool TryConsumeMarketAction();

    /** Skip the market phase / finish this turn. */
    UFUNCTION(BlueprintCallable, Category="Investory|Turn Flow")
    void EndTurn();

    UFUNCTION(BlueprintCallable, Category="Investory|Turn Flow")
    void MarkResolvingBoard();

    UFUNCTION(BlueprintPure, Category="Investory|Turn Flow")
    bool CanTrade() const;

    UFUNCTION(BlueprintPure, Category="Investory|Turn Flow")
    bool IsTurnActive() const { return bTurnActive; }

    UFUNCTION(BlueprintPure, Category="Investory|Turn Flow")
    int32 GetTurnNumber() const { return TurnNumber; }

    UFUNCTION(BlueprintPure, Category="Investory|Turn Flow")
    int32 GetMarketActionsRemaining() const { return MarketActionsRemaining; }

    UFUNCTION(BlueprintPure, Category="Investory|Turn Flow")
    EInvestoryTurnPhase GetTurnPhase() const { return Phase; }

private:
    UPROPERTY(VisibleAnywhere, Category="Investory|Turn Flow")
    int32 TurnNumber = 0;

    UPROPERTY(VisibleAnywhere, Category="Investory|Turn Flow")
    int32 MarketActionsRemaining = 0;

    UPROPERTY(VisibleAnywhere, Category="Investory|Turn Flow")
    EInvestoryTurnPhase Phase = EInvestoryTurnPhase::Finished;

    UPROPERTY(VisibleAnywhere, Category="Investory|Turn Flow")
    bool bTurnActive = false;
};
