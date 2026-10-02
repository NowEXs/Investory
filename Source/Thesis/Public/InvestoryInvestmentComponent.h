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
struct FInvestoryOrderSubmissionAttempt
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName StockId = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    EInvestoryOrderSide Side = EInvestoryOrderSide::Buy;

    UPROPERTY(BlueprintReadOnly)
    int32 Quantity = 0;

    UPROPERTY(BlueprintReadOnly)
    float SubmittedPrice = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    bool bAccepted = false;

    UPROPERTY(BlueprintReadOnly)
    FText Reason;
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

USTRUCT(BlueprintType)
struct FInvestoryPortfolioInsight
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FInvestoryStockSnapshot Snapshot;
    UPROPERTY(BlueprintReadOnly) float SpendableCash = 0.0f;
    UPROPERTY(BlueprintReadOnly) float SelectedPositionAllocationPercent = 0.0f;
    UPROPERTY(BlueprintReadOnly) bool bLiquidityWarning = false;
    UPROPERTY(BlueprintReadOnly) bool bShowAverageCost = false;
    UPROPERTY(BlueprintReadOnly) bool bShowUnrealizedProfitLoss = false;
    UPROPERTY(BlueprintReadOnly) bool bShowBreakEvenPrice = false;
    UPROPERTY(BlueprintReadOnly) FText PrimaryFeedback;
    UPROPERTY(BlueprintReadOnly) FText LearningTip;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInvestoryOrderChanged, FInvestoryPendingOrder, Order);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInvestoryOrderExecuted, FInvestoryExecutionResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInvestoryOrderSubmissionAttempt, FInvestoryOrderSubmissionAttempt, Attempt);

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


    // Knowledge makes the portfolio screen progressively more informative instead of merely adding score.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Portfolio Learning", meta=(ClampMin="0"))
    int32 KnowledgeForAverageCost = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Portfolio Learning", meta=(ClampMin="0"))
    int32 KnowledgeForUnrealizedPL = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Investory|Portfolio Learning", meta=(ClampMin="0"))
    int32 KnowledgeForBreakEven = 15;

    UPROPERTY(BlueprintAssignable, Category="Investory|Trading")
    FOnInvestoryOrderChanged OnOrderChanged;

    UPROPERTY(BlueprintAssignable, Category="Investory|Trading")
    FOnInvestoryOrderExecuted OnOrderExecuted;

    UPROPERTY(BlueprintAssignable, Category="Investory|Trading")
    FOnInvestoryOrderSubmissionAttempt OnOrderSubmissionAttempt;

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

    /** Store the latest market price known by gameplay/UI. Useful for end-game net-worth evaluation. */
    UFUNCTION(BlueprintCallable, Category="Investory|Trading|Market Data")
    void UpdateMarketPrice(FName StockId, float CurrentPrice);

    /** Bulk helper for Blueprint. Extra ids/prices are ignored when the arrays have different sizes. */
    UFUNCTION(BlueprintCallable, Category="Investory|Trading|Market Data")
    void UpdateMarketPrices(const TArray<FName>& StockIds, const TArray<float>& CurrentPrices);

    UFUNCTION(BlueprintPure, Category="Investory|Trading|Market Data")
    float GetLastKnownPrice(FName StockId) const;

    /** Total current value of all owned positions using the latest known prices. Falls back to average cost when no price has been observed yet. */
    UFUNCTION(BlueprintPure, Category="Investory|Trading|Market Data")
    float GetTotalPortfolioMarketValue() const;

    /** Cash + current portfolio value. Pending buy cash remains cash until execution; pending sell shares remain portfolio assets. */
    UFUNCTION(BlueprintPure, Category="Investory|Trading|Market Data")
    float GetEstimatedNetWorth(float CurrentCash) const;

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
     * Portfolio screen helper. It never changes money/shares and can be opened outside the Market Phase.
     * RequiredCashReserve should normally be the next living-expense/rest safety amount you want the player to keep.
     */
    UFUNCTION(BlueprintPure, Category="Investory|Portfolio Learning")
    FInvestoryPortfolioInsight GetPortfolioInsight(FName StockId, float CurrentPrice, float CurrentMoney,
        int32 Knowledge, float RequiredCashReserve) const;

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

    // Runtime-only cache. Prices are supplied by existing Blueprint/GameState flows whenever a stock is viewed,
    // submitted or executed. It intentionally does not own the market simulation itself.
    mutable TMap<FName, float> LastKnownPrices;

    void CacheMarketPrice(FName StockId, float CurrentPrice) const;
    FInvestoryStockPosition& FindOrAddPosition(FName StockId);
    FInvestoryExecutionResult AdvanceOrderInternal(FName StockId, float CurrentPrice, bool bValidateCash, float CurrentCash);
    FInvestoryExecutionResult ExecuteOrder(FInvestoryPendingOrder& Order, float CurrentPrice);
};
