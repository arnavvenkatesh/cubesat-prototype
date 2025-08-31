# CubeSat Prototype

## Status
Work in progress — prototype hardware and software for a 1U CubeSat. Current scope: 3D-printed frame, two ESP32-CAMs (payload), STM32-based flight computer (on-bench), three magnetorquers for attitude control, battery power testing, and a vision payload demo (deforestation detection). See `docs/` and `hardware/` for details.

## Mission summary
- Objective: Demonstrate a low-cost, proof-of-concept 1U CubeSat payload capable of capturing images and running an onboard classifier to flag deforestation/land-change.
- Key deliverables: mechanical frame, electronics stack, firmware for attitude/power management, payload image pipeline, bench test reports, demo videos.

## Repo layout
- `docs/` → project specification, integration plan, diagrams
- `hardware/` → CAD files, wiring diagrams
- `software/` → firmware (stm32/), payload code (esp32-cam/), groundstation/
- `tests/` → bench test reports, checklists, demo video links
- `results/` → photos, screenshots, measurement logs
- `design_log.md` → dated build & test log (lab notebook)


## How I document progress
- All work is recorded in `design_log.md` (date + short note).  
- Each hardware test includes pictures, measured values, and a pass/fail line in `tests/`.  
- Software commits are organized by feature-branch and linked to issues.

## Contribution & authorship
Author: **Arnav Venkatesh**  
This repo documents my personal prototype work. If you reuse or adapt anything, please credit this repo and follow the license.

## License
MIT — see `LICENSE`.
