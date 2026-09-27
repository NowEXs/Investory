INVESTORY / THESIS - HYBRID C++ READY
====================================

สิ่งที่เพิ่มให้แล้ว
------------------
1) Source/Thesis/Thesis.Build.cs
2) Source/Thesis.Target.cs
3) Source/ThesisEditor.Target.cs
4) Primary game module (Thesis.h / Thesis.cpp)
5) UInvestoryStatusComponent
   - Money / Happiness / Knowledge
   - Financial Stress / Burnout / Knowledge Tier
   - Blueprint-callable functions + status changed event
6) UInvestoryInvestmentComponent
   - Position / Shares / Total Cost / Average Cost
   - Unrealized P/L / Realized P/L
   - Buy/Sell pending order
   - 1 pending order per stock
   - Reserved cash / reserved shares
   - Cancel / Advance / Execute order
   - Transaction fee support
7) Thesis.uproject เพิ่ม Runtime Module "Thesis" แล้ว

สำคัญ
------
- Blueprint และ .uasset เดิมไม่ได้ถูกแก้/ลบ
- Component C++ เป็นระบบเสริมแบบ opt-in: เกมเดิมจะยังไม่เปลี่ยนจนกว่าจะ Add Component ใน Blueprint แล้วต่อ node เข้าหา Function C++
- ค่า TransactionFeeRate = 0.001 เป็นค่า simulation placeholder ที่แก้ได้ใน Details; ก่อนใช้เป็นเนื้อหาความรู้จริงควรกำหนดจากแหล่งอ้างอิง/ขอบเขตโครงงาน
- ระบบ Pending Order นี้จำลองการส่งคำสั่งเพื่อ gameplay/learning ไม่ได้จำลอง matching engine ตลาดหลักทรัพย์เต็มรูปแบบ

ขั้นตอนเปิดครั้งแรกบน Windows / UE 5.7
---------------------------------------
1) ติดตั้ง Visual Studio 2022 + workload "Game development with C++"
   และ Windows SDK / MSVC ที่ Unreal Engine ต้องการ
2) แตก ZIP ไป path สั้น ๆ เช่น D:\\Investory\\Thesis
3) คลิกขวา Thesis.uproject -> Generate Visual Studio project files
   (หรือเปิด .uproject แล้วให้ Unreal สร้าง project files เมื่อถาม)
4) เปิด Thesis.sln ด้วย Visual Studio
5) เลือก Development Editor / Win64 แล้ว Build โปรเจกต์ Thesis
6) เมื่อ Build ผ่าน เปิด Thesis.uproject

แนวต่อ Blueprint ที่แนะนำ (ยังไม่ต้องทำทั้งหมดพร้อมกัน)
------------------------------------------------------
A) BP_TopDownCharacter
   - Add Component -> Investory Status Component
   - Add Component -> Investory Investment Component

B) ใน BeginPlay
   - Initialize Status ด้วยค่าปัจจุบันของ Blueprint (Money / Happiness / Knowledge)
   - ช่วง migration แนะนำให้มี source of truth เพียงที่เดียวทีละระบบ เพื่อลดค่าซ้ำ

C) WBP_Invest
   - Buy -> Place Buy Order
   - Sell -> Place Sell Order
   - ถ้า Has Pending Order = true ให้ Disable Buy/Sell ของหุ้นนั้น และแสดง Pending

D) หลังจบ Event/News/Scam/การเดินหนึ่ง action
   - Advance Order ของหุ้นที่ Pending โดยส่ง CurrentPrice จาก BP_InvestoryGameState
   - ถ้า Result.bSuccess = true ค่อย apply CashDelta ให้ Money และ refresh UI

หมายเหตุเรื่อง Reserved Cash
----------------------------
UInvestoryInvestmentComponent เก็บ ReservedCash เพื่อบอกว่าเงินส่วนไหนถูกกันไว้กับ Pending Buy
แต่ยังไม่ได้หัก Money อัตโนมัติ เพราะ Money ปัจจุบันยังอยู่ใน Blueprint ของโปรเจกต์เดิม
ช่วงเชื่อมระบบให้ใช้:
Available Cash = Current Money - Get Reserved Cash
และเมื่อ Order Execute ให้ใช้ ExecutionResult.CashDelta เปลี่ยน Money

จุดนี้ตั้งใจทำแบบนี้เพื่อไม่ทำลาย Blueprint เดิมก่อน migration เสร็จ
