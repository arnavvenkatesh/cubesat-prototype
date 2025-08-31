# Project Specifications — CubeSat Prototype

## Project title
1U CubeSat prototype — on-board imaging + deforestation detection demo

## Purpose / Mission objectives
1. Demonstrate end-to-end prototype of a 1U CubeSat with imaging payload.  
2. Integrate two ESP32-CAM units to capture nadir/oblique images.  
3. Implement on-board local processing (inference) to flag candidate deforested areas.  
4. Verify basic attitude control via three-axis magnetorquers (bench-simulated).  
5. Produce test logs, demo videos, BOM and CAD for competition/scholarship submissions.

## Constraints
- Size: 10×10×10 cm (1U form factor)  
- Power: battery-based bench tests; no flight batteries intended at prototype stage.  
- Cost target: prototype ≤ ₹10,000 (approx) — actual costs listed in BOM.  
- Safety: all bench tests use low-voltage DC supplies; ESC/actuator tests isolated.

## System block diagram (brief)
- Power system → Battery
- MCU (STM32) → magnetorquer drivers + telemetry  
- Payload → ESP32-CAM(s) → onboard inference (or streamed to groundstation)  
- Groundstation → USB/serial link for telemetry and logs (no RF stage in prototype)

## Success criteria (minimum)
- Frame and payload assembled and mechanically stable.  
- ESP32-CAMs capture sequence of images at target resolution and save to storage.  
- STM32 can run simple control loop and command magnetorquers (bench-safe tests achieved).  
- End-to-end inference demo: show camera pointed at monitor (Google Earth) and detection logged.  
- Documentation: BOM, CAD files, wiring diagrams, bench tests recorded.

## Interfaces & requirements
- Mechanical mount interfaces: CAD dimensions found in `hardware/CAD_Files/`  
- Electrical connectors: JST 2-pin/3-pin standard — see `hardware/Wiring_Diagram.png`  
- Software protocols: UART for telemetry, SPI/I2C as required for sensors.

## Timeline (high-level)
- Phase A (weeks 1–8): finalise BOM, 3D print frame, purchase critical components.  
- Phase B (weeks 9–12): integrate electronics, bench power tests, camera integration.  
- Phase C (weeks 12–15): software integration, live-demo, prepare docs & demo video.

## Risks & mitigations
- Risk: magnetorquer torque insufficient → mitigate by bench torque measurement & scaled current tests.  
- Risk: thermal issues in enclosed 3D frame → mitigate with thermal dye tests and ventilation holes.  
- Risk: limited dataset for payload inference → mitigate via augmentation + synthetic screenshots.

## Revision log
- 2025-08-31 — v0.1 initial draft (author: Arnav Venkatesh)
