# Codex repository instructions — QIPIQ Mini V2

You are implementing software for the **Waveshare ESP32-S3-Touch-AMOLED-1.8 V2** (NOT V1). Start by reading `CODEX_MASTER_PROMPT.md`, `README.md`, and `docs/IMPLEMENTATION_STATUS.md`.

## Project priorities
1. Preserve existing demo and source files; make targeted improvements.
2. Establish a reproducible ESP-IDF project using official **V2 CO5300 + CST820** vendor examples and verified GPIO maps. Never guess pin numbers.
3. Develop non-blocking LVGL emotional QIPIQ face and touchscreen UI, then device audio, motion, battery, offline games and secure Kazakh STT/LLM/TTS backend.
4. Keep `web-demo/index.html` working as a standalone offline browser demo and distinguish simulations from real device features.
5. Update documentation in Kazakh. Add build checks and test scripts. Show actual test output rather than claiming tests passed.

## Safety and honesty
- This repository starts as a **prototype**, not commercially ready firmware.
- Never erase or flash hardware without a clearly documented factory-backup and recovery procedure and user authorization.
- Never hardcode or commit passwords, API keys or real Wi-Fi credentials. Use `.env.example` only.
- Do not invent board V2 pin assignments or pretend to have physically tested hardware.
- Use original character/sound assets; do not copy Minions' images or original voices.
- Attribute reused source code and verify licenses before integrating.
- Treat phone/PC demo and device hardware as different execution environments.

## Begin now
Run available repository checks, identify existing gaps, and implement the highest-confidence next working increment. Track status and blockers. Do not stop after a planning-only answer.
