# QIPIQ Mini V2 — prototype package

Target device: **Waveshare ESP32-S3-Touch-AMOLED-1.8 V2** (CO5300 display, CST820 touch). This repository contains the foundation for a Kazakh-speaking AI pocket companion.

**Status: PROTOTYPE. Firmware is not verified on real hardware and must not be flashed as a finished product.**

## Folders
- `firmware/qipiq_v2/` — ESP-IDF display and touch prototype using the Waveshare BSP.
- `web-demo/index.html` — working standalone browser interface simulation (open in Chrome/Edge).
- `backend/` — experimental FastAPI cloud AI integration; configure locally; not production audited.
- `firmware/QIPIQ_Cloud_Test/` — Arduino cloud prototype only. **NOT** V2 AMOLED firmware.
- `hardware/` — preliminary 3D-print STLs; physical fit not tested.
- `docs/` — Kazakh hardware, AI setup, implementation status and Codex instructions.
- `AGENTS.md` and `CODEX_MASTER_PROMPT.md` — instructions for Codex coding agent.

## Codex
1. Confirm GitHub app/connector repository access.
2. Select this repo in Codex.
3. Prompt: `Read AGENTS.md and CODEX_MASTER_PROMPT.md. Implement Phase 1 and report actual build/test results.`

## Safety
Do not erase factory firmware until you have backed it up. Never commit your Wi-Fi password, private API keys, or personal voice recordings. Review licenses of all integrated open-source references.

Official docs: https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.8

Official examples: https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.8
