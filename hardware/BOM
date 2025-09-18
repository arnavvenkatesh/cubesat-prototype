# 🛰️ CubeSat Prototype — Bill of Materials (BOM)

This is the complete list of electronics required for the **1U CubeSat Prototype** project.  
Goal: A functional demo satellite in a 3D printed frame that can stream live camera feed, provide telemetry (IMU-based orientation), and allow control of magnetorquers.

---

## ✅ Core Microcontroller & Communication
- [ ] **ESP32-CAM module (OV2640 lens, 160° FOV)** × 1–2  
  ⟶ Wi-Fi connection, camera streaming, telemetry forwarding  
- [ ] **FTDI USB-to-Serial adapter** × 1  
  ⟶ Required to program the ESP32-CAM (since no onboard USB)  
- [ ] **ESP32 DevKit board (non-CAM)** × 1 *(optional)*  
  ⟶ Backup or for running non-camera tasks  

---

## 📡 Sensors (Telemetry & Orientation)
- [ ] **MPU6050 IMU (6-axis accelerometer + gyroscope)** × 1  
  ⟶ Provides raw orientation data (AX, AY, AZ, GX, GY, GZ)  
- [ ] *(Optional upgrade)* **BNO055 IMU** × 1  
  ⟶ Absolute orientation output (quaternions/heading)  

---

## 🧲 Magnetorquers & Control
- [ ] **Enameled copper wire (30–34 AWG)** — 1 spool  
  ⟶ Wind 3 coils (X, Y, Z axes)  
- [ ] **MOSFET driver modules** × 3  
  ⟶ Drives current through coils safely  
  *(Alternative: L293D motor driver ICs)*  
- [ ] **Arduino Uno / Nano clone** × 1  
  ⟶ Controls magnetorquers + IMU  
  *(ESP32 handles Wi-Fi + camera tasks)*  

---

## 🔋 Power System (Prototype Only — Not Flight-Rated)
- [ ] **3 × 18650 Li-ion rechargeable batteries**  
- [ ] **3-slot 18650 battery holder**  
- [ ] **TP4056 charging module** × 1  
  ⟶ Recharge batteries via USB safely  
- [ ] **Step-down buck converter (LM2596)** × 1  
  ⟶ Provides regulated 5V and 3.3V rails  

---

## 🔧 Miscellaneous
- [ ] **Breadboard + jumper wires**  
- [ ] **Perfboard (for final mounting)**  
- [ ] **M2/M3 screws, standoffs, nuts** (for frame + PCB mounting)  
- [ ] **Heat-shrink tubing & electrical tape**  
- [ ] **Multimeter** *(essential for debugging)*  

---

## 📂 Project Structure (for this Repo)
