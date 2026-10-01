#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InvestoryInvestmentComponent.generated.h"

UENUM(BlueprintType)
enum class EInvestoryOrderSide : uint8
{
    Buy UMETA(DisplayName = "Buy"),
    Sell UMETA(DisplayName = "Sell")
};

UENUM(BlueprintType)
enum class EInvestoryOrderStatus : uint8
{
    Pending UMETA(DisplayName = "Pending"),
    Executed UMETA(DisplayName = "Executed"),
    Cancelled UMETA(DisplayName = "Cancelled"),
    Rejected UMETA(DisplayName = "Rejected")
};

USTRUCT(BlueprintType)
struct FInvestoryStockPosition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName StockId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Shares = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float TotalCost = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RealizedProfitLoss = 0.0f;

    float GetAverageCost() const
    {
        return Shares > 0 ? TotalCost / static_cast<float>(Shares) : 0.0f;
    }
};

USTRUCT(BlueprintType)
struct FInvestoryPendingOrder
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName StockId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EInvestoryOrderSide Side = EInvestoryOrderSide::Buy;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EInvestoryOrderStatus Status = EInvestoryOrderStatus::Pending;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Quantity = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SubmittedPrice = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ExecutedPrice = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float EstimatedFee = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TurnsRemaining = 1;
};

USTRUCT(BlueprintType)
struct FInvestoryExecutionResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bSuccess = false;

    UPROPERTY(BlueprintReadOnly)
    FName StockId = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    EInvestoryOrderSide Side = EInvestoryOrderSide::Buy;

    UPROPERTY(BlueprintReadOnly)
    int32 Quantity = 0;

    UPROPERTY(BlueprintReadOnly)
    float ExecutedPrice = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float Fee = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float CashDelta = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float RealizedProfitLoss = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    FText Message;
};


USTRUCT(BlueprintType)
struct FInvestoryStockSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName StockId = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    int32 Shares = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 AvailableShares = 0;

    UPROPERTY(BlueprintReadOnly)
    float AverageCost = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float CurrentPrice = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float MarketValue = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float UnrealizedProfitLoss = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float UnrealizedProfitLossPercent = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float BreakEvenPrice = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    bool bHasPendingOrder = false;

    UPROPERTY(BlueprintReadOnly)
    FInvestoryPendingOrder PendingOrder;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInvestoryOrderChanged, FInvestoryPendingOrder, Order);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInvestoryOrderExecuted, FInvestoryExecutionResult, Result);

UCLASS(ClassGroup=(Investory), meta=(BlueprintSpawnableComponent))
class THESIS_API UInvestoryInvestmentComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UInvestoryInvestmentComponent();

    // Simulation setting. Configure this from Blueprint/DataTable later to match the design/research basis.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Trading", meta=(ClampMin="0.0", ClampMax="1.0"))
    float TransactionFeeRate = 0.001f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Trading", meta=(ClampMin="0"))
    int32 DefaultPendingTurns = 1;

    UPROPERTY(BlueprintAssignable, Category="Investory|Trading")
    FOnInvestoryOrderChanged OnOrderChanged;

    UPROPERTY(BlueprintAssignable, Category="Investory|Trading")
    FOnInvestoryOrderExecuted OnOrderExecuted;

    UFUNCTION(BlueprintCallable, Category="Investory|Trading")
    bool PlaceBuyOrder(FName StockId, int32 Quantity, float SubmittedPrice, float AvailableCash, FText& OutReason);

    UFUNCTION(BlueprintCallable, Category="Investory|Trading")
    bool PlaceSellOrder(FName StockId, int32 Quantity, float SubmittedPrice, FText& OutReason);

    UFUNCTION(BlueprintCallable, Category="Investory|Trading")
    bool CancelOrder(FName StockId, FText& OutReason);

    // Call once after the player's next gameplay action/event. If the order reaches zero turns,
    // CurrentPrice is used as the simulated execution price.
    UFUNCTION(BlueprintCallable, Category="Investory|Trading")
    FInvestoryExecutionResult AdvanceOrder(FName StockId, float CurrentPrice);

    UFUNCTION(BlueprintPure, Category="Investory|Trading")
    bool HasPendingOrder(FName StockId) const;

    UFUNCTION(BlueprintPure, Category="Investory|Trading")
    bool GetPendingOrder(FName StockId, FInvestoryPendingOrder& OutOrder) const;

    UFUNCTION(BlueprintPure, Category="Investory|Trading")
    FInvestoryStockPosition GetPosition(FName StockId) const;

    UFUNCTION(BlueprintPure, Category="Investory|Trading")
    int32 GetOwnedShares(FName StockId) const;

    UFUNCTION(BlueprintPure, Category="Investory|Trading")
    float GetAverageCost(FName StockId) const;

    UFUNCTION(BlueprintPure, Category="Investory|Trading")
    float GetUnrealizedProfitLoss(FName StockId, float CurrentPrice) const;

    UFUNCTION(BlueprintPure, Category="Investory|Trading")
    float GetMarketValue(FName StockId, float CurrentPrice) const;

    UFUNCTION(BlueprintPure, Category="Investory|Trading")
    float GetReservedCash() const;

    UFUNCTION(BlueprintPure, Category="Investory|Trading")
    int32 GetReservedShares(FName StockId) const;

    UFUNCTION(BlueprintPure, Category="Investory|Trading")
    float EstimateFee(float GrossValue) const;

    UFUNCTION(BlueprintPure, Category="Investory|Trading")
    float GetAvailableCash(float CurrentMoney) const;

    UFUNCTION(BlueprintPure, Category="Investory|Trading")
    int32 GetAvailableShares(FName StockId) const;

    UFUNCTION(BlueprintPure, Category="Investory|Trading")
    int32 GetPendingOrderCount() const;

    /** Returns only currently pending stock ids. Useful for generic Blueprint loops later. */
    UFUNCTION(BlueprintPure, Category="Investory|Trading")
    TArray<FName> GetPendingStockIds() const;

    UFUNCTION(BlueprintPure, Category="Investory|Trading")
    float GetBreakEvenPrice(FName StockId) const;

    UFUNCTION(BlueprintPure, Category="Investory|Trading")
    FInvestoryStockSnapshot GetStockSnapshot(FName StockId, float CurrentPrice) const;

    /**
     * Safer version of AdvanceOrder for gameplay use. It validates the actual execution cost
     * against current cash while preserving cash reserved by other pending buy orders.
     */
    UFUNCTION(BlueprintCallable, Category="Investory|Trading")
    FInvestoryExecutionResult AdvanceOrderWithCash(FName StockId, float CurrentPrice, float CurrentCash);

private:
    UPROPERTY()
    TMap<FName, FInvestoryStockPosition> Positions;

    UPROPERTY()
    TMap<FName, FInvestoryPendingOrder> PendingOrders;

    float ReservedCash = 0.0f;

    FInvestoryStockPosition& FindOrAddPosition(FName StockId);
    FInvestoryExecutionResult AdvanceOrderInternal(FName StockId, float CurrentPrice, bool bValidateCash, float CurrentCash);
    FInvestoryExecutionResult ExecuteOrder(FInvestoryPendingOrder& Order, float CurrentPrice);
};
