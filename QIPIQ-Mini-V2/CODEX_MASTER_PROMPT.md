# CODEX: Implement QIPIQ Mini V2
You are the lead ESP32-S3 firmware, LVGL, and secure voice AI engineer. Inspect this repo and implement actual source code, not just a plan. Target Waveshare ESP32-S3-Touch-AMOLED-1.8 **V2** (CO5300 AMOLED, CST820 touch), 368x448, 16MB flash, 8MB PSRAM. Use verified official Waveshare V2 pin mappings; never assume V1 SH8601/FT3168 pins. First write a factory flash backup procedure; never erase without approval.

1. Build an ESP-IDF firmware with verified V2 display/touch/audio drivers; start with smoke tests and build commands.
2. Implement original nonblocking animated LVGL face with blink, smile, talking, thinking, sleep, surprise, charging, motion reactions. Do not copy Minions art/voice.
3. Add Kazakh UI, offline games, timer, battery/IMU, Wi-Fi provisioning and settings.
4. Add secure cloud STT→LLM→TTS with Kazakh support and authenticated backend; no secrets in browser/firmware. Assess XiaoZhi integration and licenses before reuse.
5. Improve `web-demo/index.html`, clearly marking mock AI. Test all UI and backend paths.
6. Provide Windows installation, backup, build, flash and rollback guide, GitHub Actions CI and attribution.

Track all features in `docs/IMPLEMENTATION_STATUS.md` as verified, implemented-unverified, prototype, or missing. Report actual test results and limitations. Commit logical increments and create a PR where supported. START IMPLEMENTING.
