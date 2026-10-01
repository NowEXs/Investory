#include "InvestoryInvestmentComponent.h"

#define LOCTEXT_NAMESPACE "InvestoryInvestment"

UInvestoryInvestmentComponent::UInvestoryInvestmentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UInvestoryInvestmentComponent::PlaceBuyOrder(
    FName StockId,
    int32 Quantity,
    float SubmittedPrice,
    float AvailableCash,
    FText& OutReason)
{
    if (StockId.IsNone() || Quantity <= 0 || SubmittedPrice <= 0.0f)
    {
        OutReason = LOCTEXT("InvalidBuy", "ข้อมูลคำสั่งซื้อไม่ถูกต้อง");
        return false;
    }

    if (HasPendingOrder(StockId))
    {
        OutReason = LOCTEXT("PendingExists", "หุ้นนี้มีคำสั่งที่กำลังรอดำเนินการอยู่");
        return false;
    }

    const float Gross = SubmittedPrice * static_cast<float>(Quantity);
    const float Fee = EstimateFee(Gross);
    const float RequiredCash = Gross + Fee;

    if (AvailableCash < RequiredCash)
    {
        OutReason = LOCTEXT("NotEnoughCash", "เงินที่ใช้ได้ไม่เพียงพอสำหรับคำสั่งซื้อนี้");
        return false;
    }

    FInvestoryPendingOrder Order;
    Order.StockId = StockId;
    Order.Side = EInvestoryOrderSide::Buy;
    Order.Status = EInvestoryOrderStatus::Pending;
    Order.Quantity = Quantity;
    Order.SubmittedPrice = SubmittedPrice;
    Order.EstimatedFee = Fee;
    Order.TurnsRemaining = FMath::Max(1, DefaultPendingTurns);

    PendingOrders.Add(StockId, Order);
    ReservedCash += RequiredCash;
    OnOrderChanged.Broadcast(Order);

    OutReason = LOCTEXT("BuySubmitted", "ส่งคำสั่งซื้อแล้ว");
    return true;
}

bool UInvestoryInvestmentComponent::PlaceSellOrder(
    FName StockId,
    int32 Quantity,
    float SubmittedPrice,
    FText& OutReason)
{
    if (StockId.IsNone() || Quantity <= 0 || SubmittedPrice <= 0.0f)
    {
        OutReason = LOCTEXT("InvalidSell", "ข้อมูลคำสั่งขายไม่ถูกต้อง");
        return false;
    }

    if (HasPendingOrder(StockId))
    {
        OutReason = LOCTEXT("PendingExistsSell", "หุ้นนี้มีคำสั่งที่กำลังรอดำเนินการอยู่");
        return false;
    }

    const FInvestoryStockPosition Position = GetPosition(StockId);
    const int32 AvailableShares = Position.Shares - GetReservedShares(StockId);
    if (AvailableShares < Quantity)
    {
        OutReason = LOCTEXT("NotEnoughShares", "จำนวนหุ้นที่พร้อมขายไม่เพียงพอ");
        return false;
    }

    FInvestoryPendingOrder Order;
    Order.StockId = StockId;
    Order.Side = EInvestoryOrderSide::Sell;
    Order.Status = EInvestoryOrderStatus::Pending;
    Order.Quantity = Quantity;
    Order.SubmittedPrice = SubmittedPrice;
    Order.EstimatedFee = EstimateFee(SubmittedPrice * static_cast<float>(Quantity));
    Order.TurnsRemaining = FMath::Max(1, DefaultPendingTurns);

    PendingOrders.Add(StockId, Order);
    OnOrderChanged.Broadcast(Order);

    OutReason = LOCTEXT("SellSubmitted", "ส่งคำสั่งขายแล้ว");
    return true;
}

bool UInvestoryInvestmentComponent::CancelOrder(FName StockId, FText& OutReason)
{
    FInvestoryPendingOrder* Order = PendingOrders.Find(StockId);
    if (!Order || Order->Status != EInvestoryOrderStatus::Pending)
    {
        OutReason = LOCTEXT("NoPendingToCancel", "ไม่พบคำสั่งที่สามารถยกเลิกได้");
        return false;
    }

    if (Order->Side == EInvestoryOrderSide::Buy)
    {
        const float Gross = Order->SubmittedPrice * static_cast<float>(Order->Quantity);
        ReservedCash = FMath::Max(0.0f, ReservedCash - (Gross + Order->EstimatedFee));
    }

    Order->Status = EInvestoryOrderStatus::Cancelled;
    OnOrderChanged.Broadcast(*Order);
    PendingOrders.Remove(StockId);

    OutReason = LOCTEXT("Cancelled", "ยกเลิกคำสั่งแล้ว");
    return true;
}

FInvestoryExecutionResult UInvestoryInvestmentComponent::AdvanceOrder(FName StockId, float CurrentPrice)
{
    return AdvanceOrderInternal(StockId, CurrentPrice, false, 0.0f);
}

bool UInvestoryInvestmentComponent::HasPendingOrder(FName StockId) const
{
    const FInvestoryPendingOrder* Order = PendingOrders.Find(StockId);
    return Order && Order->Status == EInvestoryOrderStatus::Pending;
}

bool UInvestoryInvestmentComponent::GetPendingOrder(FName StockId, FInvestoryPendingOrder& OutOrder) const
{
    const FInvestoryPendingOrder* Order = PendingOrders.Find(StockId);
    if (!Order)
    {
        return false;
    }

    OutOrder = *Order;
    return true;
}

FInvestoryStockPosition UInvestoryInvestmentComponent::GetPosition(FName StockId) const
{
    const FInvestoryStockPosition* Position = Positions.Find(StockId);
    if (Position)
    {
        return *Position;
    }

    FInvestoryStockPosition Empty;
    Empty.StockId = StockId;
    return Empty;
}

int32 UInvestoryInvestmentComponent::GetOwnedShares(FName StockId) const
{
    return GetPosition(StockId).Shares;
}

float UInvestoryInvestmentComponent::GetAverageCost(FName StockId) const
{
    return GetPosition(StockId).GetAverageCost();
}

float UInvestoryInvestmentComponent::GetUnrealizedProfitLoss(FName StockId, float CurrentPrice) const
{
    const FInvestoryStockPosition Position = GetPosition(StockId);
    return (CurrentPrice * static_cast<float>(Position.Shares)) - Position.TotalCost;
}

float UInvestoryInvestmentComponent::GetMarketValue(FName StockId, float CurrentPrice) const
{
    return FMath::Max(0.0f, CurrentPrice) * static_cast<float>(GetOwnedShares(StockId));
}

float UInvestoryInvestmentComponent::GetReservedCash() const
{
    return ReservedCash;
}

int32 UInvestoryInvestmentComponent::GetReservedShares(FName StockId) const
{
    const FInvestoryPendingOrder* Order = PendingOrders.Find(StockId);
    if (Order && Order->Status == EInvestoryOrderStatus::Pending && Order->Side == EInvestoryOrderSide::Sell)
    {
        return Order->Quantity;
    }

    return 0;
}

float UInvestoryInvestmentComponent::EstimateFee(float GrossValue) const
{
    return FMath::Max(0.0f, GrossValue) * FMath::Max(0.0f, TransactionFeeRate);
}

float UInvestoryInvestmentComponent::GetAvailableCash(float CurrentMoney) const
{
    return FMath::Max(0.0f, CurrentMoney - FMath::Max(0.0f, ReservedCash));
}

int32 UInvestoryInvestmentComponent::GetAvailableShares(FName StockId) const
{
    return FMath::Max(0, GetOwnedShares(StockId) - GetReservedShares(StockId));
}

int32 UInvestoryInvestmentComponent::GetPendingOrderCount() const
{
    int32 Count = 0;
    for (const TPair<FName, FInvestoryPendingOrder>& Pair : PendingOrders)
    {
        if (Pair.Value.Status == EInvestoryOrderStatus::Pending)
        {
            ++Count;
        }
    }
    return Count;
}

TArray<FName> UInvestoryInvestmentComponent::GetPendingStockIds() const
{
    TArray<FName> Result;
    Result.Reserve(PendingOrders.Num());

    for (const TPair<FName, FInvestoryPendingOrder>& Pair : PendingOrders)
    {
        if (Pair.Value.Status == EInvestoryOrderStatus::Pending)
        {
            Result.Add(Pair.Key);
        }
    }

    return Result;
}

float UInvestoryInvestmentComponent::GetBreakEvenPrice(FName StockId) const
{
    const float AverageCost = GetAverageCost(StockId);
    const float SellMultiplier = 1.0f - FMath::Clamp(TransactionFeeRate, 0.0f, 0.9999f);
    return AverageCost > 0.0f ? AverageCost / SellMultiplier : 0.0f;
}

FInvestoryStockSnapshot UInvestoryInvestmentComponent::GetStockSnapshot(FName StockId, float CurrentPrice) const
{
    FInvestoryStockSnapshot Snapshot;
    Snapshot.StockId = StockId;
    Snapshot.Shares = GetOwnedShares(StockId);
    Snapshot.AvailableShares = GetAvailableShares(StockId);
    Snapshot.AverageCost = GetAverageCost(StockId);
    Snapshot.CurrentPrice = FMath::Max(0.0f, CurrentPrice);
    Snapshot.MarketValue = GetMarketValue(StockId, CurrentPrice);
    Snapshot.UnrealizedProfitLoss = GetUnrealizedProfitLoss(StockId, CurrentPrice);
    Snapshot.BreakEvenPrice = GetBreakEvenPrice(StockId);

    const FInvestoryStockPosition Position = GetPosition(StockId);
    if (Position.TotalCost > KINDA_SMALL_NUMBER)
    {
        Snapshot.UnrealizedProfitLossPercent = (Snapshot.UnrealizedProfitLoss / Position.TotalCost) * 100.0f;
    }

    Snapshot.bHasPendingOrder = GetPendingOrder(StockId, Snapshot.PendingOrder);
    return Snapshot;
}

FInvestoryExecutionResult UInvestoryInvestmentComponent::AdvanceOrderWithCash(
    FName StockId,
    float CurrentPrice,
    float CurrentCash)
{
    return AdvanceOrderInternal(StockId, CurrentPrice, true, CurrentCash);
}

FInvestoryExecutionResult UInvestoryInvestmentComponent::AdvanceOrderInternal(
    FName StockId,
    float CurrentPrice,
    bool bValidateCash,
    float CurrentCash)
{
    FInvestoryExecutionResult Result;
    FInvestoryPendingOrder* Order = PendingOrders.Find(StockId);
    if (!Order || Order->Status != EInvestoryOrderStatus::Pending || CurrentPrice <= 0.0f)
    {
        return Result;
    }

    Order->TurnsRemaining = FMath::Max(0, Order->TurnsRemaining - 1);
    OnOrderChanged.Broadcast(*Order);

    if (Order->TurnsRemaining > 0)
    {
        return Result;
    }

    if (bValidateCash && Order->Side == EInvestoryOrderSide::Buy)
    {
        const float ThisOrderReservation =
            (Order->SubmittedPrice * static_cast<float>(Order->Quantity)) + Order->EstimatedFee;
        const float CashReservedForOtherOrders = FMath::Max(0.0f, ReservedCash - ThisOrderReservation);
        const float CashAvailableForThisOrder = FMath::Max(0.0f, CurrentCash - CashReservedForOtherOrders);
        const float Gross = CurrentPrice * static_cast<float>(Order->Quantity);
        const float ActualRequiredCash = Gross + EstimateFee(Gross);

        if (CashAvailableForThisOrder + KINDA_SMALL_NUMBER < ActualRequiredCash)
        {
            ReservedCash = FMath::Max(0.0f, ReservedCash - ThisOrderReservation);
            Order->Status = EInvestoryOrderStatus::Rejected;
            Order->ExecutedPrice = CurrentPrice;
            OnOrderChanged.Broadcast(*Order);

            Result.StockId = Order->StockId;
            Result.Side = Order->Side;
            Result.Quantity = Order->Quantity;
            Result.ExecutedPrice = CurrentPrice;
            Result.Fee = EstimateFee(Gross);
            Result.Message = LOCTEXT("BuyRejectedPriceMoved", "คำสั่งซื้อไม่สำเร็จ: เงินสดปัจจุบันไม่พอสำหรับราคาที่ดำเนินการ");

            PendingOrders.Remove(StockId);
            return Result;
        }
    }

    Result = ExecuteOrder(*Order, CurrentPrice);
    PendingOrders.Remove(StockId);

    if (Result.bSuccess)
    {
        OnOrderExecuted.Broadcast(Result);
    }

    return Result;
}

FInvestoryStockPosition& UInvestoryInvestmentComponent::FindOrAddPosition(FName StockId)
{
    if (FInvestoryStockPosition* Existing = Positions.Find(StockId))
    {
        return *Existing;
    }

    FInvestoryStockPosition NewPosition;
    NewPosition.StockId = StockId;
    Positions.Add(StockId, NewPosition);
    return *Positions.Find(StockId);
}

FInvestoryExecutionResult UInvestoryInvestmentComponent::ExecuteOrder(FInvestoryPendingOrder& Order, float CurrentPrice)
{
    FInvestoryExecutionResult Result;
    Result.StockId = Order.StockId;
    Result.Side = Order.Side;
    Result.Quantity = Order.Quantity;
    Result.ExecutedPrice = CurrentPrice;

    const float Gross = CurrentPrice * static_cast<float>(Order.Quantity);
    const float Fee = EstimateFee(Gross);
    Result.Fee = Fee;

    FInvestoryStockPosition& Position = FindOrAddPosition(Order.StockId);

    if (Order.Side == EInvestoryOrderSide::Buy)
    {
        const float SubmittedGross = Order.SubmittedPrice * static_cast<float>(Order.Quantity);
        ReservedCash = FMath::Max(0.0f, ReservedCash - (SubmittedGross + Order.EstimatedFee));

        Position.Shares += Order.Quantity;
        Position.TotalCost += Gross + Fee;

        Result.CashDelta = -(Gross + Fee);
        Result.bSuccess = true;
        Result.Message = LOCTEXT("BuyExecuted", "คำสั่งซื้อดำเนินการสำเร็จ");
    }
    else
    {
        if (Position.Shares < Order.Quantity)
        {
            Result.Message = LOCTEXT("SellRejectedAtExecution", "คำสั่งขายไม่สำเร็จเนื่องจากจำนวนหุ้นไม่เพียงพอ");
            return Result;
        }

        const float AverageCost = Position.GetAverageCost();
        const float CostBasisSold = AverageCost * static_cast<float>(Order.Quantity);
        const float NetProceeds = Gross - Fee;
        const float RealizedPL = NetProceeds - CostBasisSold;

        Position.Shares -= Order.Quantity;
        Position.TotalCost = FMath::Max(0.0f, Position.TotalCost - CostBasisSold);
        Position.RealizedProfitLoss += RealizedPL;

        if (Position.Shares == 0)
        {
            Position.TotalCost = 0.0f;
        }

        Result.CashDelta = NetProceeds;
        Result.RealizedProfitLoss = RealizedPL;
        Result.bSuccess = true;
        Result.Message = LOCTEXT("SellExecuted", "คำสั่งขายดำเนินการสำเร็จ");
    }

    Order.Status = EInvestoryOrderStatus::Executed;
    Order.ExecutedPrice = CurrentPrice;
    OnOrderChanged.Broadcast(Order);
    return Result;
}

#undef LOCTEXT_NAMESPACE
