#include "InvestoryStatusComponent.h"

UInvestoryStatusComponent::UInvestoryStatusComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UInvestoryStatusComponent::InitializeStatus(float NewMoney, int32 NewHappiness, int32 NewKnowledge)
{
    Money = FMath::Max(0.0f, NewMoney);
    Happiness = FMath::Clamp(NewHappiness, 0, 100);
    Knowledge = FMath::Clamp(NewKnowledge, 0, 100);
    BroadcastStatus();
}

void UInvestoryStatusComponent::ChangeMoney(float Amount)
{
    Money = FMath::Max(0.0f, Money + Amount);
    BroadcastStatus();
}

void UInvestoryStatusComponent::ChangeHappiness(int32 Amount)
{
    Happiness = FMath::Clamp(Happiness + Amount, 0, 100);
    BroadcastStatus();
}

void UInvestoryStatusComponent::ChangeKnowledge(int32 Amount)
{
    Knowledge = FMath::Clamp(Knowledge + Amount, 0, 100);
    BroadcastStatus();
}

bool UInvestoryStatusComponent::IsFinancialStress() const
{
    return Money < FinancialStressThreshold;
}

bool UInvestoryStatusComponent::IsBurnout() const
{
    return Happiness <= BurnoutThreshold;
}

EInvestoryKnowledgeTier UInvestoryStatusComponent::GetKnowledgeTier() const
{
    if (Knowledge >= InsightKnowledgeThreshold)
    {
        return EInvestoryKnowledgeTier::Insight;
    }

    if (Knowledge >= InformedKnowledgeThreshold)
    {
        return EInvestoryKnowledgeTier::Informed;
    }

    return EInvestoryKnowledgeTier::Beginner;
}

float UInvestoryStatusComponent::GetSpendableMoney(float ReservedMoney) const
{
    return FMath::Max(0.0f, Money - FMath::Max(0.0f, ReservedMoney));
}

void UInvestoryStatusComponent::BroadcastStatus()
{
    OnStatusChanged.Broadcast(Money, Happiness, Knowledge);
}
