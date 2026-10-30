#include "InvestoryTurnFlowComponent.h"

#define LOCTEXT_NAMESPACE "InvestoryTurnFlow"

UInvestoryTurnFlowComponent::UInvestoryTurnFlowComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

FInvestoryGameplayTurnResult UInvestoryTurnFlowComponent::StartTurn()
{
    FInvestoryGameplayTurnResult Result;

    // A forced Burnout retry may enter the roll flow a second time. Do not create another turn.
    if (bTurnActive)
    {
        Result.bStartedNewTurn = false;
        Result.TurnNumber = TurnNumber;
        Result.Phase = Phase;
        Result.Message = LOCTEXT("TurnAlreadyActive", "เทิร์นปัจจุบันกำลังดำเนินอยู่");
        return Result;
    }

    bTurnActive = true;
    ++TurnNumber;
    MarketActionsRemaining = 0;
    Phase = EInvestoryTurnPhase::ReadyToRoll;
    bMarketOpeningEventCheckedThisTurn = false;
    bMarketOpeningEventShown = false;
    bMarketOpeningEventInvestmentTile = false;

    Result.bStartedNewTurn = true;
    Result.TurnNumber = TurnNumber;
    Result.Phase = Phase;

    if (IncomeEveryTurns > 0 && IncomeAmount > 0.0f && (TurnNumber % IncomeEveryTurns == 0))
    {
        Result.bIncomeGranted = true;
        Result.IncomeGranted = IncomeAmount;
        Result.Message = FText::Format(
            LOCTEXT("PeriodicIncome", "ได้รับรายรับประจำ {0} บาท"),
            FText::AsNumber(IncomeAmount));
    }
    else
    {
        Result.Message = LOCTEXT("TurnStarted", "เริ่มเทิร์นใหม่");
    }

    return Result;
}

int32 UInvestoryTurnFlowComponent::GetMovesUntilMarketOpening() const
{
    const int32 Interval = FMath::Max(1, MarketOpeningEveryCompletedMoves);
    return FMath::Max(0, Interval - CompletedMovesSinceMarketOpening);
}

bool UInvestoryTurnFlowComponent::TryShowMarketOpeningEvent(bool bInvestmentTile, int32& OutAvailableActions)
{
    OutAvailableActions = 0;

    // Call from BP_TopDownCharacter -> Finish Tile Resolution, after all tile
    // choices and feedback have completed. This is not called on a rest turn.
    // The checked flag avoids counting the same move twice (duplicate callbacks).
    if (!bTurnActive || Phase != EInvestoryTurnPhase::ResolvingBoard || bMarketOpeningEventCheckedThisTurn)
    {
        return false;
    }

    bMarketOpeningEventCheckedThisTurn = true;
    const int32 Interval = FMath::Max(1, MarketOpeningEveryCompletedMoves);
    CompletedMovesSinceMarketOpening = FMath::Min(Interval, CompletedMovesSinceMarketOpening + 1);

    // An investment tile ALWAYS opens the market. Either type of market
    // opening resets the counter, so the next resolved move starts at 1.
    if (!bInvestmentTile && CompletedMovesSinceMarketOpening < Interval)
    {
        // No market this move: finish the turn rather than leaving it active.
        // Blueprint's false branch must also avoid creating WBP_FInishTile.
        EndTurn();
        return false;
    }

    CompletedMovesSinceMarketOpening = 0;
    bMarketOpeningEventShown = true;
    bMarketOpeningEventInvestmentTile = bInvestmentTile;
    OutAvailableActions = FMath::Max(0,
        bInvestmentTile ? InvestmentTileMarketActions : NormalMarketActions);
    return true;
}

int32 UInvestoryTurnFlowComponent::OpenMarketPhase(bool bInvestmentTile)
{
    if (!bTurnActive)
    {
        return 0;
    }

    // Opening the portfolio or constructing a widget must not start trading
    // before the dice has resolved a board tile.
    if (Phase != EInvestoryTurnPhase::ResolvingBoard && Phase != EInvestoryTurnPhase::Market)
    {
        return 0;
    }

    // Reopening WBP_Invest does not grant a fresh action allowance.
    if (Phase == EInvestoryTurnPhase::Market)
    {
        return MarketActionsRemaining;
    }

    // Never allow WBP_Invest (including its Construct event) to open a market
    // on a non-market turn. The popup must have been approved after the tile.
    if (!bMarketOpeningEventShown)
    {
        return 0;
    }

    // Snapshot from the approved tile is authoritative; widget defaults
    // must not erase the investment-tile bonus.
    Phase = EInvestoryTurnPhase::Market;
    MarketActionsRemaining = FMath::Max(
        0,
        bMarketOpeningEventInvestmentTile ? InvestmentTileMarketActions : NormalMarketActions);

    return MarketActionsRemaining;
}

bool UInvestoryTurnFlowComponent::TryConsumeMarketAction()
{
    if (!CanTrade())
    {
        return false;
    }

    --MarketActionsRemaining;
    return true;
}

void UInvestoryTurnFlowComponent::EndTurn()
{
    MarketActionsRemaining = 0;
    Phase = EInvestoryTurnPhase::Finished;
    bTurnActive = false;
}

void UInvestoryTurnFlowComponent::MarkResolvingBoard()
{
    // Do not regress to "Resolving Board" after the market has opened or
    // when a skipped/finished turn receives a late asynchronous widget callback.
    if (bTurnActive && (Phase == EInvestoryTurnPhase::ReadyToRoll ||
                        Phase == EInvestoryTurnPhase::ResolvingBoard))
    {
        Phase = EInvestoryTurnPhase::ResolvingBoard;
    }
}

bool UInvestoryTurnFlowComponent::CanTrade() const
{
    return bTurnActive && Phase == EInvestoryTurnPhase::Market && MarketActionsRemaining > 0;
}

#undef LOCTEXT_NAMESPACE
