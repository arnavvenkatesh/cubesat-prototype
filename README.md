# CubeSat Prototype
**A low-cost, student-built 1U CubeSat prototype for orbital deforestation monitoring**
---

## What This Is

This repository documents the full hardware and firmware build of a 1U CubeSat prototype — a solo engineering project built from scratch in my bedroom. The satellite is designed to capture imagery and transmit it to a ground station running a deforestation classifier.

The companion CV repository (the payload software) lives here: [satellite-deforestation-detection](https://github.com/arnavvenkatesh/satellite-deforestation-detection)

This started as curiosity after watching a video about CubeSats, turned into a study of magnetorquer attitude control algorithms (resulting in a published technical review proposing a hybrid approach), and eventually became this working prototype. It pushed me to learn hardware debugging, CAD, embedded firmware, and the reality of engineering trade-offs.

---
## Personal note
Before this project, I was just a dreamer with an idea to build a CubeSat. And after building it, I still am. This project lasted from May 2025 to May 2026. I cannot fully put into words how much it has taught me and how much it has built my confidence working with electronics and CAD. I had absolutely no clue how a satellite is built just over a year ago, and now one sits proudly in my cupboard. If anybody is thinking of building a CubeSat, I really hope this repo helps. It has all the details to get started — feel free to build on it and learn on your own. There are many AI tools that can help you learn, code, and debug. Be open to new resources and new methods. With all of this, you can have your own satellite on your desk in less than a year!

---

## Objectives

- Demonstrate a functional 1U CubeSat payload on a bench: live camera, attitude telemetry, and magnetorquer control under ₹10,000 ($105)
- Validate the image pipeline with the deforestation CV model
- Build a mechanically sound 1U frame via iterative CAD
- Document every trade-off and limitation 

---

## What Works (Achievements)

| Subsystem | Status |
|---|---|
| Camera (OV2640 via ESP32-CAM) | ✅ Live MJPEG stream over WiFi |
| Attitude sensing (MPU6050) | ✅ Pitch & roll via accelerometer |
| Web dashboard (camera + telemetry) | ✅ Browser accessible, no laptop needed |
| Magnetorquer control (L298N, bang-bang) | ✅ Logic working, bench tested |
| Battery-powered operation | ✅ 7.5V → 5V buck converter, fully wireless |
| 1U mechanical frame | ✅ CAD designed, 3D printed |
| CV payload integration (bench demo) | ✅ ESP32 feed piped into deforestation classifier |

---

## Limitations

- **WiFi ≠ real telemetry.** A real CubeSat uses UHF/VHF radio or optical links over hundreds of kilometres. WiFi works only within a local network. It is a pragmatic workaround while realistic telemetry hardware is sourced and tested.
- **Not launched.** This is a bench prototype. It has not been tested in vacuum, microgravity, or radiation environments. The main reason for this is the quality of components. CubeSats require space-grade aluminium, which I cannot afford under the budget of this project. So this mainly serves as an really close proof-of-concept.
- **Bang-bang attitude control only.** The magnetorquer control currently uses simple on/off logic.
- **No real orbital imagery.** The CV demo uses Google Earth shown to the ESP32 as a proxy for satellite imagery. Real orbital imagery requires altitude, optics, and actual orbital mechanics.
- **Magnetorquers** I spun copper wire around to make my own magnetorquers. These are really weak and can barely move a paperclip. Needless to say, it cannot actually move the satellite to re-orient itself, even though the motor drivers give the commands to do so.
- **Power budget.** Since this is a proof of concept, I have used rechardable Li-ion batteries. But I have not attached any solar panels.

---

## Hardware Stack

| Component | Role |
|---|---|
| ESP32-CAM (AI Thinker) | Main compute + camera + WiFi |
| MPU6050 | 3-axis accelerometer + gyroscope |
| L298N Motor Driver | Magnetorquer H-bridge control |
| 2× Magnetorquer coils | Attitude actuation (X and Y axis) |
| Battery pack + buck converter (→5V) | Power supply |
| FTDI USB-Serial | Programming only (not in final build) |

For full component list with costs, see [BOM.md](./BOM.md).

### Pin Map

| Signal | ESP32-CAM Pin |
|---|---|
| MPU6050 SDA | IO15 |
| MPU6050 SCL | IO14 |
| L298N IN1 (X-axis) | IO12 |
| L298N IN2 (X-axis) | IO13 |
| L298N IN3 (Y-axis) | IO2 |
| L298N IN4 (Y-axis) | IO4 |
| IO0 | GND during upload only |

> FTDI wiring: RX→UOT, TX→UOR, VCC→rail+, GND→rail−. Disconnect SDA/SCL wires from ESP32 during every firmware upload — they interfere with flash access.
> For schematics, see [wiring_schamtic.png](./hardware/schematic/wiring_schematic.png).


---

## Firmware

The full Arduino sketch is in `/firmware/cubesat_main/cubesat_main.ino`.

**Three HTTP endpoints served simultaneously:**
The firmware serves a browser dashboard at `http://[IP]/dashboard` showing the live camera feed and real-time attitude data. No laptop needed — any device on the same WiFi network works.

The data endpoint runs on a separate HTTP server (port 81) to prevent Safari from locking up the streaming server — a known issue with ESP32's `httpd` under concurrent requests.

**Key firmware features:**
- MPU6050 read at 10 Hz, values stored as globals, served on demand
- Bang-bang magnetorquer control with 5° deadband
- Magnetorquers disabled by default (`ENABLE_MAGNETORQUERS false`) — enable when coils are properly connected
- WiFi credentials in top of sketch — change to your network

---

## CAD / Mechanical

Files are in `/hardware/cad/`.

```
cad/
├── v1_tinkercad.stl/          # First iteration — basic 1U skeleton
└── v2_fusion360.f3d/          # Final design, print-ready
└── v2_fusion360.stl/          # Final design, print-ready


```

All versions are included to show design iteration. The final v2 frame is optimised for the electronics stack described above and is the version currently assembled.

---

## Repo Structure

```
cubesat-prototype/
├── firmware/
│   └── cubesat_main/
│       └── cubesat_main.ino       # Full ESP32 firmware
├── hardware/
│   ├── cad/
│   │   ├── v1_tinkercad.stl
│   │   ├── v3_fusion360.f3d/
│   │   └── v3_fusion360.stl/
│   └── schematic/
│       └── wiring_schematic.png   # Full breadboard schematic
├── media/ #All my pictures taken while assembling
├── BOM.md                             # Full bill of materials with costs
└── README.md
```

---

## Upload Guide (Quick Reference)

1. Disconnect SDA and SCL wires from ESP32
2. Connect IO0 → GND
3. Unplug USB, wait 2s, plug back in
4. Click Upload in Arduino IDE immediately
5. Wait for `Hard resetting via RTS pin...` with no error after
6. Remove IO0 wire, reconnect SDA/SCL, unplug/replug USB
7. Find ESP32 IP using Fing app → open `http://[IP]/dashboard`

---

## Payload Integration

To run the deforestation detection demo:

1. Boot the CubeSat and open the camera stream at `http://[IP]/`
2. Point the camera at satellite imagery (printed or on screen)
3. Run Cell 1 and Cell 5 of `Satellite image detection.ipynb` in the CV repo
4. The live scanner will classify the ESP32 feed in real time

---

## Publication

A technical review proposing a hybrid magnetorquer attitude control algorithm (combining bang-bang and PD control) was published as part of the research underpinning this project. [Link](https://www.oneyoungindia.com/white-papers/optimizing-cubesatellite-orientation-using-magnetorquers/arnav-venkatesh---nhvps-bangalore)

---

## What's Next

- [ ] Implement hybrid PD magnetorquer algorithm from the published review
- [ ] Replace WiFi telemetry with LoRa or UHF radio module
- [ ] Thermal and vacuum stress testing
- [ ] Fully enclosed frame with panel integration

---

## License

MIT — build on it, break it, learn from it.
