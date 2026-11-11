# QIPIQ Mini V2 — prototype package

Target device: **Waveshare ESP32-S3-Touch-AMOLED-1.8 V2** (CO5300 display, CST820 touch). This repository is a development prototype for a Kazakh-speaking AI pocket companion.

> **Not a finished product.** The browser demo is mock-only, firmware has not been verified on real hardware, and the cloud backend has not received a production security audit. Do not flash it as finished firmware or expose the development backend publicly.

## Project contents
- `web-demo/index.html` — standalone browser simulation; no real AI, microphone, ESP32 or device sensors.
- `backend/` — FastAPI prototype for authenticated chat, STT and TTS endpoints. Live calls require server-side secrets and may incur provider charges.
- `firmware/QIPIQ_Cloud_Test/` — Arduino cloud test only; **not** V2 AMOLED firmware.
- `hardware/` — preliminary STL files; physical fit is untested.
- `character/` — original emotion/pose model with Node tests.
- `docs/` — Kazakh setup and hardware notes, including implementation status.

## Local checks
Requirements: Python 3.11+, Node.js 20+.

```sh
python -m pip install -r backend/requirements.txt
python -m compileall -q backend
python -m unittest discover -s tests -p 'test_*.py' -v
node --test tests/*.test.mjs
```

Run the local API (from the repository root):

```sh
python -m pip install -r backend/requirements.txt
uvicorn backend.app:app --reload
```

Open `http://127.0.0.1:8000/` for the mock UI and `/health` for service configuration status. To try the demo chat endpoint, configure a long random `QIPIQ_DEVICE_TOKEN` and `QIPIQ_DEMO_MODE=true` in the untracked `backend/.env`; send that token as `X-Device-Token`. Never commit `.env`, provider keys, Wi-Fi credentials or personal audio. Do not set live AI mode until you understand the associated costs and security requirements.

## Hardware safety
Do not erase factory firmware before backing it up. Never flash this repository as V2 hardware firmware: verified CO5300/CST820 drivers and board-specific pin mapping are still missing. Use the exact board revision and Waveshare's official examples to establish pins; do not guess.

Official docs: https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.8

Official examples: https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.8
