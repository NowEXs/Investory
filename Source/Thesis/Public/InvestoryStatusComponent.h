#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InvestoryStatusComponent.generated.h"

UENUM(BlueprintType)
enum class EInvestoryKnowledgeTier : uint8
{
    Beginner UMETA(DisplayName = "Beginner"),
    Informed UMETA(DisplayName = "Informed"),
    Insight UMETA(DisplayName = "Insight")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FOnInvestoryStatusChanged,
    float, Money,
    int32, Happiness,
    int32, Knowledge);

UCLASS(ClassGroup=(Investory), meta=(BlueprintSpawnableComponent))
class THESIS_API UInvestoryStatusComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInvestoryStatusComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Status")
    float Money = 10000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Status", meta=(ClampMin="0", ClampMax="100"))
    int32 Happiness = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Status", meta=(ClampMin="0", ClampMax="100"))
    int32 Knowledge = 50;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Thresholds")
    float FinancialStressThreshold = 3000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Thresholds", meta=(ClampMin="0", ClampMax="100"))
    int32 BurnoutThreshold = 25;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Thresholds", meta=(ClampMin="0", ClampMax="100"))
    int32 InformedKnowledgeThreshold = 40;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Thresholds", meta=(ClampMin="0", ClampMax="100"))
    int32 InsightKnowledgeThreshold = 70;

    UPROPERTY(BlueprintAssignable, Category="Investory|Status")
    FOnInvestoryStatusChanged OnStatusChanged;

    UFUNCTION(BlueprintCallable, Category="Investory|Status")
    void InitializeStatus(float NewMoney, int32 NewHappiness, int32 NewKnowledge);

    UFUNCTION(BlueprintCallable, Category="Investory|Status")
    void ChangeMoney(float Amount);

    UFUNCTION(BlueprintCallable, Category="Investory|Status")
    void ChangeHappiness(int32 Amount);

    UFUNCTION(BlueprintCallable, Category="Investory|Status")
    void ChangeKnowledge(int32 Amount);

    UFUNCTION(BlueprintPure, Category="Investory|Status")
    bool IsFinancialStress() const;

    UFUNCTION(BlueprintPure, Category="Investory|Status")
    bool IsBurnout() const;

    UFUNCTION(BlueprintPure, Category="Investory|Status")
    EInvestoryKnowledgeTier GetKnowledgeTier() const;

    UFUNCTION(BlueprintPure, Category="Investory|Status")
    float GetSpendableMoney(float ReservedMoney) const;

private:
    void BroadcastStatus();
};
