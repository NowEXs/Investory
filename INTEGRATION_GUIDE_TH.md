# INVESTORY – Turn Flow / Market Phase Integration

ไฟล์ชุดนี้ปรับเฉพาะ C++ และไม่แก้ `.uasset` Blueprint โดยตรง เพื่อไม่ทำให้ Graph เดิมเสียหาย

## สิ่งที่เพิ่ม

- `UInvestoryTurnFlowComponent`
  - Turn number
  - รายรับประจำทุก N เทิร์น
  - Market Phase หลังจบ Tile
  - 1 Market Action ปกติ / 2 Actions เมื่อเป็น Investment Tile
  - ป้องกันการ Start Turn ซ้ำตอนกด “ฝืนเดินต่อ” หลัง Burnout
- `TryRestInsteadOfRoll`
  - ค่าเริ่มต้น: เสียเงิน 1,000 / ฟื้น Happiness +10
  - ถ้าเงินไม่พอจะ Rest ไม่สำเร็จและส่ง Message กลับมา
  - Rest สำเร็จจะจบเทิร์นให้อัตโนมัติ
- `PlaceBuyOrderForTurn` / `PlaceSellOrderForTurn`
  - Wrapper สำหรับระบบซื้อขายเดิม
  - เช็ก Market Action และหัก Action เมื่อส่งคำสั่งสำเร็จ
- Investment optimization
  - รวม logic ที่ซ้ำกันของ `AdvanceOrder` และ `AdvanceOrderWithCash`
  - เพิ่ม `GetPendingStockIds()` เผื่อใช้ทำหุ้นหลายตัวแบบ generic ภายหลัง

## ค่า Default ใหม่

- IncomeEveryTurns = 4
- IncomeAmount = 1500
- NormalMarketActions = 1
- InvestmentTileMarketActions = 2
- RestMoneyCost = 1000
- RestHappinessGain = 10
- bEnforcePhaseGating = false (เปิดทีหลังเมื่อ Blueprint flow เชื่อมครบ)

---

# 1) BP_Dice – RollDice

ลำดับที่แนะนำ:

```
RollDice(ForceBurnout)
 -> Cast BP_TopDownCharacter
 -> SyncCharacterToStatus
 -> StartGameplayTurn
 -> TryStartRoll(ForceBurnout)
 -> HandleTurnResult
 -> SyncStatusToCharacter
 -> Branch(ReturnValue)
      True  -> ระบบทอย/เดินเดิม
      False -> WBP_Burnout
```

`StartGameplayTurn` เรียกซ้ำได้อย่างปลอดภัยในเทิร์นเดิม ดังนั้นตอน `ForceContinue -> RollDice(true)` จะไม่เพิ่ม Turn และไม่ให้รายรับซ้ำ

### Optimize SyncCharacterToStatus
แทนการ Set Money/Happiness/Knowledge ของ InvestoryStatus 3 node แยกกัน สามารถใช้ node:

```
InvestoryStatus -> Initialize Status
    NewMoney     = Character.Money
    NewHappiness = Character.Happiness
    NewKnowledge = Character.Knowledge
```

แล้วหลัง C++ คำนวณค่อย `SyncStatusToCharacter` เหมือนเดิม

---

# 2) WBP_Burnout

## ฝืนเดินต่อ
ของเดิมใช้ได้:

```
keepgo
 -> Call OnForceContinue
 -> RemoveFromParent
```

ใน BP_Dice:

```
OnForceContinue
 -> RollDice(true)
```

## พัก
แนะนำเปลี่ยนจาก Set Happiness เอง เป็น C++:

```
rest
 -> Get Player Character
 -> Cast BP_TopDownCharacter
 -> (Sync Character -> Status ก่อน)
 -> TryRestInsteadOfRoll
 -> Branch(ReturnValue)
      True:
        Sync Status -> Character
        RemoveFromParent
      False:
        แสดง RestResult.Message
```

ผล Default:
- Money -1000
- Happiness +10
- จบเทิร์นอัตโนมัติ

---

# 3) หลังจบ Tile/Event

หลัง Event / Scam / News / Shop / FixEvent แสดง feedback เสร็จแล้ว ให้เปิด Market Phase

Tile ปกติ:

```
OpenMarketPhase(false)
```

Investment Tile:

```
OpenMarketPhase(true)
```

ผล:
- ปกติ = 1 Market Action
- Investment Tile = 2 Market Actions

แนะนำใช้ `WBP_PreTurn` เดิมเป็นเมนูหลัง Tile:

```
[ตลาดหุ้น]  -> เปิด WBP_Invest
[จบเทิร์น] -> EndGameplayTurn -> กลับหน้า Dice
```

เมื่อระบบนี้เชื่อมครบแล้ว ค่อยเปิดใน `InvestoryTurnFlow`:

```
bEnforcePhaseGating = true
```

เพื่อป้องกันผู้เล่นกด Roll ข้าม Market Phase

---

# 4) WBP_Invest

ใน `HandleOrderConfirmed` เปลี่ยน node เดิม:

```
InvestmentComponent -> Place Buy Order
InvestmentComponent -> Place Sell Order
```

เป็น node จาก Character:

```
BP_TopDownCharacter -> Place Buy Order For Turn
BP_TopDownCharacter -> Place Sell Order For Turn
```

Input เดิมใช้เหมือนเดิม:
- StockId
- Quantity
- SubmittedPrice
- AvailableCash (Buy)

เมื่อคำสั่งสำเร็จ C++ จะหัก Market Action ให้อัตโนมัติ

ใช้:

```
GetMarketActionsRemaining
```

เพื่อแสดง UI เช่น `สิทธิ์ซื้อขายคงเหลือ: 1` และ Disable Buy/Sell เมื่อเหลือ 0

> Pending Order / Fee / Avg Cost / Execute 1 Turn / OrderResult ของเดิมยังใช้ต่อทั้งหมด

---

# 5) รายรับประจำ

`StartGameplayTurn()` จะให้รายรับทุก 4 เทิร์นตาม Default และเพิ่มเข้า InvestoryStatus ให้อัตโนมัติ

หลังเรียก StartGameplayTurn และ TryStartRoll ให้ `SyncStatusToCharacter` แล้ว UI/ระบบเก่าที่อ่าน Character.Money จะเห็นค่าใหม่

ค่าทั้งหมดปรับได้จาก inherited `InvestoryTurnFlow` component ใน Class Defaults

---

# 6) สิ่งที่ยังไม่ควรทำทันที

- ยังไม่ต้องเปิด `bEnforcePhaseGating` จนกว่า Market Phase + EndGameplayTurn จะเชื่อมครบ
- ยังไม่ต้องลบตัวแปร legacy `Money/Happiness/Knowledge` ใน BP_TopDownCharacter
- ยังไม่ต้องทำหุ้น 5 ตัวพร้อมกัน ให้ TECH ผ่าน loop ใหม่ก่อน
- อย่าสร้าง InvestoryStatus / InvestoryInvestment / InvestoryTurnFlow ซ้ำใน Blueprint ถ้ามี inherited component อยู่แล้ว

