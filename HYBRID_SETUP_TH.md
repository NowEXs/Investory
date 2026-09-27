# INVESTORY Hybrid C++ + Blueprint Setup

ไฟล์ชุดนี้เพิ่มระบบ C++ โดยไม่ลบ Blueprint เดิม และแก้ปัญหา Git ที่พยายาม commit `Binaries/`, `Intermediate/`, `.idea/` และไฟล์ build ขนาดใหญ่

## 1) แก้ Commit / GitHub ก่อน

ไฟล์ `.gitignore` ในโปรเจกต์นี้จะเก็บเฉพาะสิ่งที่ควรอยู่ใน Git เช่น:

- `Content/`
- `Config/`
- `Source/`
- `Thesis.uproject`

และไม่ commit ของที่ Unreal / Rider สร้างใหม่ได้ เช่น:

- `Binaries/`
- `Intermediate/`
- `DerivedDataCache/`
- `Saved/`
- `.idea/`
- `.vs/`

### ถ้า GitHub Desktop ยังแสดงไฟล์เก่า/ใหญ่

เปิด PowerShell ที่โฟลเดอร์โปรเจกต์ (โฟลเดอร์เดียวกับ `Thesis.uproject`) แล้วรัน:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\FixGitForUnreal.ps1
```

สคริปต์ใช้ `git rm --cached` จึงเอาไฟล์ออกจาก Git เท่านั้น **ไม่ลบไฟล์จากเครื่อง**

หลังจากนั้นกลับไป GitHub Desktop แล้ว commit ใหม่ได้

> อย่ากด `Commit anyway` กับไฟล์ `.pch`, `UnrealEditor.json`, `Binaries`, `Intermediate` ที่เกิน 100MB

---

## 2) C++ ที่มีในโปรเจกต์

### `AInvestoryCharacterBase`
C++ base class สำหรับ `BP_TopDownCharacter` โดยสร้าง Component ให้อัตโนมัติ:

- `InvestoryStatus`
- `InvestoryInvestment`

และมีระบบแรงกดดันตอนทอย:

- ทอยปกติลด Happiness
- Burnout สามารถบล็อกการทอย
- ผู้เล่นเลือกฝืนได้และเสีย Happiness เพิ่ม
- ค่าครองชีพเกิดทุก N ครั้งที่ทอย
- ถ้าเงินที่ใช้ได้ไม่พอค่าครองชีพ จะเกิด penalty ต่อ Happiness

ค่าทั้งหมดแก้จาก Blueprint Class Defaults ได้ ไม่ได้ hard-code ตายตัว

### `UInvestoryStatusComponent`
เก็บและจัดการ:

- Money
- Happiness
- Knowledge
- Financial Stress
- Burnout
- Knowledge Tier

### `UInvestoryInvestmentComponent`
รองรับ:

- Buy/Sell Order
- Pending Order
- Reserved Cash
- Reserved Shares
- Transaction Fee
- Average Cost
- Market Value
- Unrealized P/L
- Realized P/L
- Break-even Price
- Order execution ในรอบถัดไป
- Reject order ถ้าราคาตอน execute สูงขึ้นจนเงินสดจริงไม่พอ
- `GetStockSnapshot()` สำหรับดึงข้อมูล UI หุ้นใน node เดียว

---

## 3) เชื่อมกับ `BP_TopDownCharacter` (แนะนำ)

1. Compile C++ ให้ผ่าน
2. เปิด `BP_TopDownCharacter`
3. `File > Reparent Blueprint`
4. เลือก `InvestoryCharacterBase`
5. Compile + Save

ระบบ Movement / Dice / CheckTile / Widget เดิมไม่ถูกลบ เพราะ `InvestoryCharacterBase` สืบทอดจาก `Character` เหมือน Parent เดิม

หลัง reparent ใน Blueprint จะเห็น inherited components:

- `InvestoryStatus`
- `InvestoryInvestment`

> ก่อน reparent แนะนำ Duplicate `BP_TopDownCharacter` ไว้ 1 ชุด เช่น `BP_TopDownCharacter_Backup`

---

## 4) เชื่อมระบบ Roll

ตรง Event ที่กำลังกดลูกเต๋า **ก่อน** เรียก logic ทอยเดิม:

```text
Try Start Roll
    Force Through Burnout = false
→ Branch Return Value
```

### True
ต่อเข้า Flow ทอยเดิมทันที

### False
เปิด popup เช่น:

```text
Burnout
[พัก]     [ฝืนต่อ]
```

- พัก → `Rest Instead Of Roll`
- ฝืน → `Try Start Roll(true)` → แล้วต่อเข้า Flow ทอยเดิม

`Break InvestoryTurnResult` จะอ่านได้ว่า:

- Living Expense Due
- Living Expense Paid
- Living Expense Unpaid
- Happiness Spent
- Money Spent
- Message

ใช้แสดง feedback บน UI ได้

---

## 5) เชื่อม Event / Scam / Shop กับ Status C++

จากเดิมที่ต้อง Set Money / Happiness / Knowledge ทีละตัว สามารถใช้:

```text
Apply Status Change
Money Delta
Happiness Delta
Knowledge Delta
```

ตัวอย่าง Scam ผิด:

```text
Money Delta = -2000
Happiness Delta = -10
Knowledge Delta = +5
```

แล้ว Blueprint แสดง feedback ว่า "ผิดเพราะอะไร" ตาม DataTable ได้ต่อเหมือนเดิม

---

## 6) เชื่อมหน้า Investment

เมื่อเลือกหุ้น เช่น `TECH`:

```text
InvestoryInvestment
→ Get Stock Snapshot
    Stock Id = TECH
    Current Price = PriceTECH
→ Break InvestoryStockSnapshot
```

จะได้พร้อมใช้:

- Shares
- Available Shares
- Average Cost
- Current Price
- Market Value
- Unrealized Profit Loss
- Unrealized Profit Loss Percent
- Break Even Price
- Has Pending Order
- Pending Order

เหมาะกับ Panel รายละเอียดหุ้นใน `WBP_Invest`

### ส่งคำสั่งซื้อ

```text
Get Spendable Money
→ Place Buy Order
    StockId
    Quantity
    SubmittedPrice
    AvailableCash
```

ถ้า Return = true:
- แสดง `Pending`
- Disable Buy/Sell หุ้นตัวนั้น
- ยังไม่เพิ่มหุ้นเข้าพอร์ตทันที

### ส่งคำสั่งขาย

```text
Place Sell Order
StockId
Quantity
SubmittedPrice
```

หุ้นที่ Pending Sell จะถูกนับเป็น Reserved Shares และขายซ้ำไม่ได้

---

## 7) Execute Pending Order ในรอบถัดไป

หลังผู้เล่นทำ gameplay action ถัดไป หรือก่อนเริ่ม Roll ถัดไป ให้เรียกต่อหุ้นที่มี Pending:

```text
Advance Order With Cash
StockId
CurrentPrice
CurrentCash = InvestoryStatus.Money
```

จาก Result:

```text
bSuccess
CashDelta
Fee
ExecutedPrice
RealizedProfitLoss
Message
```

ถ้า Success:

```text
InvestoryStatus → Change Money(CashDelta)
```

- Buy จะได้ CashDelta ติดลบ
- Sell จะได้ CashDelta เป็นบวก

ถ้า Buy order execute ที่ราคาสูงขึ้นจนเงินไม่พอ ระบบจะ Reject และคืน Reserved Cash ให้อัตโนมัติ

---

## 8) สำคัญ: ตัวแปร Blueprint เก่า

ตอนนี้ใน `BP_TopDownCharacter` ยังมีตัวแปรเก่า เช่น:

- Money
- Happiness
- Knowledge
- OwnTECH / OwnENG / ...
- AvgTECH / TotalCostTECH / ...

อย่าลบทันที

ช่วง migration ให้เลือก **C++ เป็นค่าหลัก** แล้วค่อยเปลี่ยน UI/Event ทีละจุด

ลำดับที่แนะนำ:

1. Investment ใช้ C++ ก่อน
2. UI Portfolio ใช้ `GetStockSnapshot`
3. Money/Happiness/Knowledge ย้ายมา StatusComponent
4. เมื่อทุก Widget ใช้ Component แล้วค่อยลบตัวแปรซ้ำใน Blueprint

วิธีนี้ลดโอกาส Blueprint พังทั้งโปรเจกต์พร้อมกัน

---

## 9) ค่า Default ที่ใส่ไว้ตอนนี้

ค่าพวกนี้เป็นค่าเพื่อให้ระบบทดสอบได้ ไม่ใช่ค่าที่ล็อกสำหรับงานวิจัย:

```text
Happiness per Roll = 1
Force through Burnout extra = 5
Living Expense every = 4 rolls
Living Expense = 500
Unpaid Expense Happiness Penalty = 5
Rest Happiness Gain = 10
Transaction Fee Rate = 0.001 (0.1%)
Pending Turn = 1
```

แก้ได้ทั้งหมดใน Blueprint Defaults หลัง reparent
