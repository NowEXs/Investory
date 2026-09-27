#include "InvestoryCharacterBase.h"
#include "InvestoryStatusComponent.h"
#include "InvestoryInvestmentComponent.h"

#define LOCTEXT_NAMESPACE "InvestoryCharacterBase"

AInvestoryCharacterBase::AInvestoryCharacterBase()
{
    PrimaryActorTick.bCanEverTick = false;

    StatusComponent = CreateDefaultSubobject<UInvestoryStatusComponent>(TEXT("InvestoryStatus"));
    InvestmentComponent = CreateDefaultSubobject<UInvestoryInvestmentComponent>(TEXT("InvestoryInvestment"));
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

    const bool bBurnout = StatusComponent->IsBurnout();
    if (bBurnout && !bForceThroughBurnout)
    {
        OutResult.bCanRoll = false;
        OutResult.Message = LOCTEXT("BurnoutBlocksRoll", "คุณอยู่ในภาวะ Burnout: พักก่อน หรือเลือกฝืนเพื่อทอยต่อ");
        return false;
    }

    RollCount++;
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

    return true;
}

void AInvestoryCharacterBase::RestInsteadOfRoll()
{
    if (StatusComponent)
    {
        StatusComponent->ChangeHappiness(FMath::Max(0, RestHappinessGain));
    }
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

#undef LOCTEXT_NAMESPACE
