# QIPIQ Mini + OpenAI API (2026-10-27)

Бұл жинақ **QIPIQ Mini-ге ChatGPT тәрізді OpenAI AI жауаптарын қосуға арналған** жұмыс үлгісі. Өнім прототипі; электрлік тақшада және тірі OpenAI API-да сыналмаған.

## Қазір не бар?

- `backend/app.py` – FastAPI сервері. Мәтін үшін Responses API, бір дауыстық сұрақ пен жауап үшін Realtime API қолданылады.
- `web-demo/index.html` – браузерде ашылатын интерактивті демо.
- `firmware/QIPIQ_Cloud_Test` – ESP32-S3 құрылғысының **USB Serial Monitor-ынан** серверге HTTPS сұрақ жіберетін сынақ коды. Нағыз микрофон/динамик оған жалғанбаған.
- `tests/` – OpenAI-ға ақы төлетпейтін mock тесттер.

## 1. OpenAI кілтін алу

1. https://platform.openai.com/api-keys – OpenAI Platform ішінде жобалық API key жасаңыз.
2. **ChatGPT Plus жазылымы API төлемін қамтымайды**: Platform billing-ті жеке баптаңыз.
3. Бюджет лимиті мен usage alerts орнатыңыз: https://platform.openai.com/settings/organization/billing/overview
4. API кілтті мұнда чатқа, сайт кодының ішіне немесе ESP32 firmware-ге **енгізбеңіз**.

## 2. Компьютерде дайындау

Windows PowerShell (Python 3.11+):

```powershell
python -m venv .venv
.venv\Scripts\Activate.ps1
pip install -r backend/requirements.txt
Copy-Item backend/.env.example backend/.env
python -c "import secrets; print(secrets.token_urlsafe(32))"
```

Linux/macOS:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r backend/requirements.txt
cp backend/.env.example backend/.env
python -c "import secrets; print(secrets.token_urlsafe(32))"
```

`backend/.env` ішіне:

- `OPENAI_API_KEY=...` – тек сервердегі кілт.
- `QIPIQ_DEVICE_TOKEN=...` – терминалда генерациялаған **бөлек кездейсоқ 32+ таңбалы** құпия код.
- `QIPIQ_DEMO_MODE=false` – шынымен OpenAI жауап берсін. Ал кілтсіз UI-ды сынау үшін `true` қойыңыз; демо **ChatGPT емес**.
- `QIPIQ_TEXT_MODEL=gpt-4o-mini` – мәтіндік жауап моделі.
- `QIPIQ_REALTIME_MODEL=gpt-realtime-2.1-mini` – дауыстық жауап моделі.

> Ескерту: `gpt-4o-mini-tts` /audio/speech уақытша нұсқа, ресми тоқтатылуы 2027 жылғы 6 қаңтарға жоспарланған. Сериялық өнім үшін Realtime voice-қа көшу керек. Ол тек модель атауын өзгерту емес, бөлек API іске асыруын талап етеді.

## 3. Серверді іске қосу

```bash
uvicorn backend.app:app --host 127.0.0.1 --port 8000
```

Браузерде **http://127.0.0.1:8000** мекенжайын ашсаңыз, интерактивті демо көрінеді. Сервер параметрлерін `GET /health` арқылы тексеріңіз.

Осы браузер UI тек **жергілікті MVP**. Оны құпия токен өрісімен интернетке ашық түрде жарияламаңыз: production-та пайдаланушы аутентификациясы, HTTPS, rate limits, per-device credentials, server-side budgets, privacy policy міндетті.

## 4. Тесттер

```bash
python -m unittest discover -s tests -p test_realtime_voice.py
```

Тесттер OpenAI WebSocket оқиғаларын имитациялайды. Нақты қызметті тексеру үшін OpenAI API кілті мен төлем қажет.

### Дауыстық сұрауды тексеру

`POST /v1/voice` сұрауына `X-Device-Token` тақырыбын және `application/octet-stream` мазмұн түрін жіберіңіз. Сұрау денесіне тақырыпшасыз 16 кГц, 16 биттік little-endian моно PCM салынады. Жазба 0,2–6 секунд болуы керек. Жауапта 24 кГц моно PCM және форматты көрсететін `X-QIPIQ-*` тақырыптары қайтады. Сервер аудионы файлға сақтамайды.

Мысалдағы `QIPIQ_DEVICE_TOKEN` орта айнымалысында серверде бапталған токен тұруы керек.

```powershell
curl.exe --fail-with-body -H "X-Device-Token: $env:QIPIQ_DEVICE_TOKEN" -H "Content-Type: application/octet-stream" --data-binary "@question.pcm" --output answer.pcm http://127.0.0.1:8000/v1/voice
```

## 5. ESP32-S3 Wi-Fi сынағы

1. Arduino IDE және Espressif ESP32 board support 3.x орнатыңыз; ArduinoJson 7 кітапханасын қосыңыз.
2. `firmware/QIPIQ_Cloud_Test/wifi_config.example.h` файлын сол жерде `wifi_config.h` деп көшіріңіз.
3. Wi-Fi SSID/пароль, интернетке ашық HTTPS серверіңіз, DEVICE_TOKEN және **сол сервер сертификатының дұрыс root CA PEM** мәнін толтырыңыз. Жалған сертификат немесе `setInsecure()` қолданбаңыз.
4. `.ino` скетчті құрылғыға флэштеңіз. Serial Monitor 115200 baud, newline режимін таңдаңыз. Қазақша сұрақ теріп Enter басыңыз.
5. Жауап **Serial Monitor** ішінде көрінеді. Экранда қазақ әріптері мен аудио әзірше **жоқ**.

Құрылғы `127.0.0.1` арқылы компьютердің localhost-ына жете алмайды. Қашықтағы ESP үшін дұрыс HTTPS сервер және физикалық плата қажет.

## 6. Жүйе схемасы

```text
Телефон / браузер (мәтін / микрофон)          ESP32 (Serial)
               |                                 |
               +------------ HTTPS --------------+
                               |
                       QIPIQ FastAPI
                  (құрылғы токені, лимит)
                               |
                       OPENAI_API_KEY
                        OpenAI HTTPS API
              ┌────────────────┼────────────────┐
            Responses          STT              TTS
           gpt-6-luna      gpt-transcribe  gpt-4o-mini-tts*
            (мәтін)         (дауыс→мәтін)   (мәтін→дауыс)

* TTS уақытша; 2027-01-06 sunset жоспарланған.
```

## 7. Жобаның маңызды шектеулері

- **OpenAI API кілті берілмеген**: нақты модельге сұрау жасамадық.
- **V2 плата**: дисплей, сенсор, микрофон және динамик физикалық құрылғыда тексерілді. Firmware-ден AI серверіне дауыс жіберу және жауапты ойнату әзірленіп жатыр.
- **Production-ready емес:** әр құрылғыға жеке credentials, DB/Redis rate limit, cost quotas, device provisioning, OTA, TLS certificate rotation, firmware hardening, logs redaction, ата-аналарға арналған ережелер қажет.
- **Балалар:** дауыс/мәтін үшін келісім, жеке деректерді қорғау; балаларға арналған өнімді сатуға шығар алдында тиісті талаптарды тексеріңіз.
- **Кідіріс:** `/v1/voice` бір жазбаны толық қабылдағаннан кейін жауап қайтарады.
- **Сыртқы құралдар:** қазіргі демо ауа райын, сағатты немесе жаңалықтарды сұраса, сервердегі GPT автоматты түрде интернеттен іздемейді. Мұндай функционалды бөлек құралдармен қосу керек.

## API endpoints

| Endpoint | Әрекет |
|---|---|
| `GET /` | Телефонға арналған QIPIQ интерфейс |
| `GET /v1/ping` | Токенді тексеру, OpenAI сұрауы жоқ |
| `GET /health` | Сервис конфигурациясы (кілтті көрсетпейді) |
| `POST /v1/chat` | JSON `{ "message":"Сәлем", "history":[] }` |
| `POST /v1/voice` | 16 кГц моно PCM16 сұрау → 24 кГц моно PCM16 жауап |
| `POST /v1/transcribe` | multipart `audio` файл (`webm`, `wav`, `mp3`, `m4a` т.б.) |
| `POST /v1/speech` | JSON `{ "text":"Сәлем" }` → MP3 |

POST endpoints `X-Device-Token` header талап етеді; бұл прототипте ортақ токен ғана, коммерциялық өнімге жарамсыз.

## Қосымша ресми құжаттама

- https://developers.openai.com/api/docs/guides/text
- https://developers.openai.com/api/docs/guides/realtime-conversations
- https://developers.openai.com/api/docs/guides/speech-to-text
- https://developers.openai.com/api/docs/guides/text-to-speech
- https://developers.openai.com/api/docs/pricing
