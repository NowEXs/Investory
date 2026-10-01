#include "InvestoryCharacterBase.h"
#include "InvestoryStatusComponent.h"
#include "InvestoryInvestmentComponent.h"
#include "InvestoryTurnFlowComponent.h"

#define LOCTEXT_NAMESPACE "InvestoryCharacterBase"

AInvestoryCharacterBase::AInvestoryCharacterBase()
{
    PrimaryActorTick.bCanEverTick = false;

    StatusComponent = CreateDefaultSubobject<UInvestoryStatusComponent>(TEXT("InvestoryStatus"));
    InvestmentComponent = CreateDefaultSubobject<UInvestoryInvestmentComponent>(TEXT("InvestoryInvestment"));
    TurnFlowComponent = CreateDefaultSubobject<UInvestoryTurnFlowComponent>(TEXT("InvestoryTurnFlow"));
}

FInvestoryGameplayTurnResult AInvestoryCharacterBase::StartGameplayTurn()
{
    if (!TurnFlowComponent)
    {
        FInvestoryGameplayTurnResult Missing;
        Missing.Message = LOCTEXT("MissingTurnFlow", "ไม่พบระบบ Turn Flow");
        return Missing;
    }

    FInvestoryGameplayTurnResult Result = TurnFlowComponent->StartTurn();

    // Income is applied exactly once because StartTurn is idempotent while a turn is active.
    if (Result.bStartedNewTurn && Result.bIncomeGranted && StatusComponent)
    {
        StatusComponent->ChangeMoney(Result.IncomeGranted);
    }

    return Result;
}

void AInvestoryCharacterBase::MarkResolvingBoard()
{
    if (TurnFlowComponent)
    {
        TurnFlowComponent->MarkResolvingBoard();
    }
}

int32 AInvestoryCharacterBase::OpenMarketPhase(bool bInvestmentTile)
{
    return TurnFlowComponent ? TurnFlowComponent->OpenMarketPhase(bInvestmentTile) : 0;
}

void AInvestoryCharacterBase::EndGameplayTurn()
{
    if (TurnFlowComponent)
    {
        TurnFlowComponent->EndTurn();
    }
}

bool AInvestoryCharacterBase::CanTradeThisTurn() const
{
    return TurnFlowComponent && TurnFlowComponent->CanTrade();
}

int32 AInvestoryCharacterBase::GetMarketActionsRemaining() const
{
    return TurnFlowComponent ? TurnFlowComponent->GetMarketActionsRemaining() : 0;
}

bool AInvestoryCharacterBase::TryStartRoll(bool bForceThroughBurnout, FInvestoryTurnResult& OutResult)
{
    OutResult = FInvestoryTurnResult();
    OutResult.RollNumber = RollCount + 1;

    if (!StatusComponent || !InvestmentComponent)
    {
        OutResult.Message = LOCTEXT("MissingComponents", "ไม่พบระบบสถานะหรือระบบลงทุนของผู้เล่น");
        return false;
    }

    if (TurnFlowComponent && TurnFlowComponent->bEnforcePhaseGating && TurnFlowComponent->IsTurnActive() &&
        TurnFlowComponent->GetTurnPhase() != EInvestoryTurnPhase::ReadyToRoll)
    {
        OutResult.Message = LOCTEXT("TurnPhaseBlocksRoll", "ยังไม่จบขั้นตอนของเทิร์นปัจจุบัน");
        return false;
    }

    const bool bBurnout = StatusComponent->IsBurnout();
    if (bBurnout && !bForceThroughBurnout)
    {
        OutResult.bCanRoll = false;
        OutResult.Message = LOCTEXT("BurnoutBlocksRoll", "คุณอยู่ในภาวะ Burnout: พักก่อน หรือเลือกฝืนเพื่อทอยต่อ");
        return false;
    }

    ++RollCount;
    OutResult.RollNumber = RollCount;
    OutResult.bCanRoll = true;
    OutResult.bForcedThroughBurnout = bBurnout && bForceThroughBurnout;

    int32 HappinessCost = FMath::Max(0, HappinessCostPerRoll);
    if (OutResult.bForcedThroughBurnout)
    {
        HappinessCost += FMath::Max(0, ExtraHappinessCostWhenForcingBurnout);
    }

    if (HappinessCost > 0)
    {
        StatusComponent->ChangeHappiness(-HappinessCost);
        OutResult.HappinessSpent += HappinessCost;
    }

    const bool bExpenseCycleEnabled = LivingExpenseEveryRolls > 0 && LivingExpenseAmount > 0.0f;
    if (bExpenseCycleEnabled && (RollCount % LivingExpenseEveryRolls == 0))
    {
        OutResult.bLivingExpenseDue = true;
        const float SpendableCash = GetSpendableMoney();

        if (SpendableCash + KINDA_SMALL_NUMBER >= LivingExpenseAmount)
        {
            StatusComponent->ChangeMoney(-LivingExpenseAmount);
            OutResult.bLivingExpensePaid = true;
            OutResult.MoneySpent = LivingExpenseAmount;
        }
        else
        {
            OutResult.bLivingExpenseUnpaid = true;
            const int32 StressPenalty = FMath::Max(0, UnpaidExpenseHappinessPenalty);
            if (StressPenalty > 0)
            {
                StatusComponent->ChangeHappiness(-StressPenalty);
                OutResult.HappinessSpent += StressPenalty;
            }
        }
    }

    if (OutResult.bLivingExpenseUnpaid)
    {
        OutResult.Message = LOCTEXT("ExpenseUnpaid", "เงินสดที่ใช้ได้ไม่พอสำหรับค่าครองชีพ ทำให้เกิดความกดดันทางการเงิน");
    }
    else if (OutResult.bForcedThroughBurnout)
    {
        OutResult.Message = LOCTEXT("ForcedRoll", "คุณฝืนเล่นต่อทั้งที่ Burnout จึงเสีย Happiness เพิ่ม");
    }
    else
    {
        OutResult.Message = LOCTEXT("RollReady", "พร้อมทอย");
    }

    if (TurnFlowComponent && TurnFlowComponent->IsTurnActive())
    {
        TurnFlowComponent->MarkResolvingBoard();
    }

    return true;
}

bool AInvestoryCharacterBase::TryRestInsteadOfRoll(FInvestoryRestResult& OutResult)
{
    OutResult = FInvestoryRestResult();

    if (!StatusComponent)
    {
        OutResult.Message = LOCTEXT("RestMissingStatus", "ไม่พบระบบสถานะของผู้เล่น");
        return false;
    }

    const float Cost = FMath::Max(0.0f, RestMoneyCost);
    const int32 Gain = FMath::Max(0, RestHappinessGain);
    const float SpendableCash = GetSpendableMoney();

    if (SpendableCash + KINDA_SMALL_NUMBER < Cost)
    {
        OutResult.Message = FText::Format(
            LOCTEXT("RestNotEnoughMoney", "เงินสดที่ใช้ได้ไม่พอสำหรับการพัก ต้องใช้ {0} บาท"),
            FText::AsNumber(Cost));
        return false;
    }

    if (Cost > 0.0f)
    {
        StatusComponent->ChangeMoney(-Cost);
        OutResult.MoneySpent = Cost;
    }

    if (Gain > 0)
    {
        StatusComponent->ChangeHappiness(Gain);
        OutResult.HappinessGained = Gain;
    }

    OutResult.bSuccess = true;
    OutResult.Message = FText::Format(
        LOCTEXT("RestSuccess", "พักสำเร็จ: เสียเงิน {0} บาท และฟื้นความสุข {1}"),
        FText::AsNumber(OutResult.MoneySpent),
        FText::AsNumber(OutResult.HappinessGained));

    // Rest is a complete turn by design. This also prevents a free extra roll afterward.
    if (TurnFlowComponent && TurnFlowComponent->IsTurnActive())
    {
        TurnFlowComponent->EndTurn();
    }

    return true;
}

void AInvestoryCharacterBase::RestInsteadOfRoll()
{
    FInvestoryRestResult Ignored;
    TryRestInsteadOfRoll(Ignored);
}

void AInvestoryCharacterBase::ApplyStatusChange(float MoneyDelta, int32 HappinessDelta, int32 KnowledgeDelta)
{
    if (!StatusComponent)
    {
        return;
    }

    if (!FMath::IsNearlyZero(MoneyDelta))
    {
        StatusComponent->ChangeMoney(MoneyDelta);
    }

    if (HappinessDelta != 0)
    {
        StatusComponent->ChangeHappiness(HappinessDelta);
    }

    if (KnowledgeDelta != 0)
    {
        StatusComponent->ChangeKnowledge(KnowledgeDelta);
    }
}

float AInvestoryCharacterBase::GetSpendableMoney() const
{
    if (!StatusComponent)
    {
        return 0.0f;
    }

    const float Reserved = InvestmentComponent ? InvestmentComponent->GetReservedCash() : 0.0f;
    return StatusComponent->GetSpendableMoney(Reserved);
}

bool AInvestoryCharacterBase::CanRollNormally() const
{
    return StatusComponent && !StatusComponent->IsBurnout();
}

bool AInvestoryCharacterBase::PlaceBuyOrderForTurn(
    FName StockId,
    int32 Quantity,
    float SubmittedPrice,
    float AvailableCash,
    FText& OutReason)
{
    if (!InvestmentComponent || !TurnFlowComponent)
    {
        OutReason = LOCTEXT("TradeMissingComponents", "ไม่พบระบบลงทุนหรือระบบ Turn Flow");
        return false;
    }

    if (!TurnFlowComponent->CanTrade())
    {
        OutReason = LOCTEXT("NoMarketActionBuy", "ไม่มีสิทธิ์ส่งคำสั่งซื้อในช่วงนี้ หรือใช้ Market Action หมดแล้ว");
        return false;
    }

    if (!InvestmentComponent->PlaceBuyOrder(StockId, Quantity, SubmittedPrice, AvailableCash, OutReason))
    {
        return false;
    }

    TurnFlowComponent->TryConsumeMarketAction();
    return true;
}

bool AInvestoryCharacterBase::PlaceSellOrderForTurn(
    FName StockId,
    int32 Quantity,
    float SubmittedPrice,
    FText& OutReason)
{
    if (!InvestmentComponent || !TurnFlowComponent)
    {
        OutReason = LOCTEXT("TradeMissingComponentsSell", "ไม่พบระบบลงทุนหรือระบบ Turn Flow");
        return false;
    }

    if (!TurnFlowComponent->CanTrade())
    {
        OutReason = LOCTEXT("NoMarketActionSell", "ไม่มีสิทธิ์ส่งคำสั่งขายในช่วงนี้ หรือใช้ Market Action หมดแล้ว");
        return false;
    }

    if (!InvestmentComponent->PlaceSellOrder(StockId, Quantity, SubmittedPrice, OutReason))
    {
        return false;
    }

    TurnFlowComponent->TryConsumeMarketAction();
    return true;
}

#undef LOCTEXT_NAMESPACE
