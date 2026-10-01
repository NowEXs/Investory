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

int32 UInvestoryTurnFlowComponent::OpenMarketPhase(bool bInvestmentTile)
{
    if (!bTurnActive)
    {
        return 0;
    }

    // Do not allow reopening the same phase to refill already-consumed actions.
    if (Phase == EInvestoryTurnPhase::Market)
    {
        return MarketActionsRemaining;
    }

    Phase = EInvestoryTurnPhase::Market;
    MarketActionsRemaining = FMath::Max(
        0,
        bInvestmentTile ? InvestmentTileMarketActions : NormalMarketActions);

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
    if (bTurnActive && Phase != EInvestoryTurnPhase::Market)
    {
        Phase = EInvestoryTurnPhase::ResolvingBoard;
    }
}

bool UInvestoryTurnFlowComponent::CanTrade() const
{
    return bTurnActive && Phase == EInvestoryTurnPhase::Market && MarketActionsRemaining > 0;
}

#undef LOCTEXT_NAMESPACE
