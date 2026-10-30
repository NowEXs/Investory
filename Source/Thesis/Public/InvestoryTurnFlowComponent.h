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
 *   StartTurn -> Roll / resolve tile -> TryShowMarketOpeningEvent ->
 *   (market prompt only every N completed moves, or on investment tile) ->
 *   OpenMarketPhase / trade or skip -> EndTurn.
 *
 * A rest/burnout turn does not count as a completed board move.
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
    int32 IncomeEveryTurns = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Turn Flow|Income", meta=(ClampMin="0.0"))
    float IncomeAmount = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Turn Flow|Market", meta=(ClampMin="0"))
    int32 NormalMarketActions = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Turn Flow|Market", meta=(ClampMin="0"))
    int32 InvestmentTileMarketActions = 2;

    /** Open the market prompt after this many completed board moves, starting at 3. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Turn Flow|Market", meta=(ClampMin="1", UIMin="1"))
    int32 MarketOpeningEveryCompletedMoves = 3;

    /**
     * Keep false while migrating the existing Blueprint flow. Turn it on after Blueprint calls
     * StartGameplayTurn -> resolve tile -> OpenMarketPhase -> EndGameplayTurn.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Turn Flow")
    bool bEnforcePhaseGating = false;

    /** Starts a new turn, or safely returns the current turn if one is already active. */
    UFUNCTION(BlueprintCallable, Category="Investory|Turn Flow")
    FInvestoryGameplayTurnResult StartTurn();

    /**
     * Call ONCE from the existing BP_TopDownCharacter "Finish Tile Resolution" event,
     * after the player has acknowledged Event/News/Scam/Shop feedback.
     * True: show WBP_FInishTile (the market-opening prompt).
     * False: the prompt was already shown or the current turn cannot show it.
     * This only announces the opportunity; it does NOT consume/open Market Phase.
     */
    UFUNCTION(BlueprintCallable, Category="Investory|Turn Flow|Market Event")
    bool TryShowMarketOpeningEvent(bool bInvestmentTile, int32& OutAvailableActions);

    /** True after the market prompt was accepted for this turn. */
    UFUNCTION(BlueprintPure, Category="Investory|Turn Flow|Market Event")
    bool HasShownMarketOpeningEvent() const { return bMarketOpeningEventShown; }

    /** Completed moves since the most recent market prompt (investment tile also resets it). */
    UFUNCTION(BlueprintPure, Category="Investory|Turn Flow|Market Event")
    int32 GetCompletedMovesSinceMarketOpening() const { return CompletedMovesSinceMarketOpening; }

    /** 1..N while counting; 0 if a market prompt is already due. */
    UFUNCTION(BlueprintPure, Category="Investory|Turn Flow|Market Event")
    int32 GetMovesUntilMarketOpening() const;

    /**
     * Call ONLY from the market-open button in WBP_FInishTile.
     * Normal tile = 1 action, investment tile = 2 actions by default.
     * Requires TryShowMarketOpeningEvent to have approved the market for THIS turn.
     * Repeated calls NEVER refill actions.
     */
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

    // How many successfully resolved board moves occurred after the last market event.
    UPROPERTY(VisibleAnywhere, Category="Investory|Turn Flow|Market Event")
    int32 CompletedMovesSinceMarketOpening = 0;

    // Ensure resolving one tile more than once does not advance the interval counter.
    UPROPERTY(VisibleAnywhere, Category="Investory|Turn Flow|Market Event")
    bool bMarketOpeningEventCheckedThisTurn = false;

    // One popup per turn; never reset this when WBP_Invest is reopened.
    UPROPERTY(VisibleAnywhere, Category="Investory|Turn Flow|Market Event")
    bool bMarketOpeningEventShown = false;

    // Snapshot of the landing tile at popup creation (not a mutable widget default).
    UPROPERTY(VisibleAnywhere, Category="Investory|Turn Flow|Market Event")
    bool bMarketOpeningEventInvestmentTile = false;
};
