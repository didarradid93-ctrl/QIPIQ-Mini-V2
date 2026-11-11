# QIPIQ Mini + OpenAI API (2026-10-27)

Бұл жинақ **QIPIQ Mini-ге ChatGPT тәрізді OpenAI AI жауаптарын қосуға арналған** жұмыс үлгісі. Өнім прототипі; электрлік тақшада және тірі OpenAI API-да сыналмаған.

## Қазір не бар?

- `server/app.py` – Python FastAPI, OpenAI **Responses API** (`gpt-6-luna`) мәтіндік чат; `gpt-transcribe` дауысты мәтінге; уақытша `gpt-4o-mini-tts` дыбыстауға.
- `web/index.html` – телефон/компьютер браузеріндегі QIPIQ пиксельді бет, мәтін, микрофон түймесі, дыбысты тыңдау.
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
cd QIPIQ_OpenAI_Integration_v1
python -m venv .venv
.venv\Scripts\Activate.ps1
pip install -r server/requirements.txt
Copy-Item server/.env.example server/.env
python -c "import secrets; print(secrets.token_urlsafe(32))"
```

Linux/macOS:

```bash
cd QIPIQ_OpenAI_Integration_v1
python3 -m venv .venv
source .venv/bin/activate
pip install -r server/requirements.txt
cp server/.env.example server/.env
python -c "import secrets; print(secrets.token_urlsafe(32))"
```

`server/.env` ішіне:

- `OPENAI_API_KEY=...` – тек сервердегі кілт.
- `QIPIQ_DEVICE_TOKEN=...` – терминалда генерациялаған **бөлек кездейсоқ 32+ таңбалы** құпия код.
- `QIPIQ_DEMO_MODE=false` – шынымен OpenAI жауап берсін. Ал кілтсіз UI-ды сынау үшін `true` қойыңыз; демо **ChatGPT емес**.
- `QIPIQ_TEXT_MODEL=gpt-6-luna` – арзанырақ мәтіндік нұсқа. Модель қолжетімділігі мен бағасын OpenAI Platform-да тексеріңіз.

> Ескерту: `gpt-4o-mini-tts` /audio/speech уақытша нұсқа, ресми тоқтатылуы 2027 жылғы 6 қаңтарға жоспарланған. Сериялық өнім үшін Realtime voice-қа көшу керек. Ол тек модель атауын өзгерту емес, бөлек API іске асыруын талап етеді.

## 3. Серверді іске қосу

```bash
uvicorn server.app:app --host 127.0.0.1 --port 8000
```

Браузерде **http://127.0.0.1:8000** ашыңыз. Құрылғы токенін енгізіңіз; мәтіндік сұрақ қойыңыз. Дауыстық енгізу/ойнатуды Chrome/Android Chrome браузері мен нақты микрофон арқылы сынаңыз. Локалхостта микрофон рұқсаты қолжетімді, ал жалпы сайтта HTTPS талап етіледі.

Осы браузер UI тек **жергілікті MVP**. Оны құпия токен өрісімен интернетке ашық түрде жарияламаңыз: production-та пайдаланушы аутентификациясы, HTTPS, rate limits, per-device credentials, server-side budgets, privacy policy міндетті.

## 4. Тесттер

```bash
python -m pytest tests -q
```

Ағымдағы жинақ: 11 тест өткізілді (mock transport). Бұл **тірі OpenAI шақыруы жасалды** деген сөз емес. Тірі іске қосу үшін OpenAI API кілті мен төлем қажет.

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
- **ESP32 модулі физикалық қолда жоқ**: firmware-ді құрылғыға флэштеу, акустика және экранға мәтін шығару сыналған жоқ.
- **Production-ready емес:** әр құрылғыға жеке credentials, DB/Redis rate limit, cost quotas, device provisioning, OTA, TLS certificate rotation, firmware hardening, logs redaction, ата-аналарға арналған ережелер қажет.
- **Балалар:** дауыс/мәтін үшін келісім, жеке деректерді қорғау; балаларға арналған өнімді сатуға шығар алдында тиісті талаптарды тексеріңіз.
- **Латенттілік:** бұл turn-by-turn (алдымен дауыс тану, сосын GPT, кейін TTS); лезде сөйлесу үшін болашақ Realtime API интеграциясы.
- **Сыртқы құралдар:** қазіргі демо ауа райын, сағатты немесе жаңалықтарды сұраса, сервердегі GPT автоматты түрде интернеттен іздемейді. Мұндай функционалды бөлек құралдармен қосу керек.

## API endpoints

| Endpoint | Әрекет |
|---|---|
| `GET /` | Телефонға арналған QIPIQ интерфейс |
| `GET /v1/ping` | Токенді тексеру, OpenAI сұрауы жоқ |
| `GET /health` | Сервис конфигурациясы (кілтті көрсетпейді) |
| `POST /v1/chat` | JSON `{ "message":"Сәлем", "history":[] }` |
| `POST /v1/transcribe` | multipart `audio` файл (`webm`, `wav`, `mp3`, `m4a` т.б.) |
| `POST /v1/speech` | JSON `{ "text":"Сәлем" }` → MP3 |

POST endpoints `X-Device-Token` header талап етеді; бұл прототипте ортақ токен ғана, коммерциялық өнімге жарамсыз.

## Қосымша ресми құжаттама

- https://developers.openai.com/api/docs/guides/text
- https://developers.openai.com/api/docs/guides/speech-to-text
- https://developers.openai.com/api/docs/guides/text-to-speech
- https://developers.openai.com/api/docs/pricing
