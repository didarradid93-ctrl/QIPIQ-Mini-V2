"""QIPIQ Mini local/development OpenAI API gateway.

Privacy: no chat or audio is written to disk; in-memory usage counters reset
on restart. Protect the app before making it accessible publicly.
"""
from __future__ import annotations

import hmac
import io
import os
import time
from collections import defaultdict, deque
from pathlib import Path
from threading import Lock
from typing import Literal

from fastapi import Depends, FastAPI, File, Header, HTTPException, UploadFile
from fastapi.responses import FileResponse, Response
import httpx
from dotenv import load_dotenv
from pydantic import BaseModel, Field

APP_DIR = Path(__file__).resolve().parent.parent
load_dotenv(APP_DIR / "backend" / ".env")
app = FastAPI(title="QIPIQ Mini – OpenAI Gateway", version="1.0.0")

# A single-user prototype limiter. For multi-instance deployments use Redis and
# per-device identity and provider spending controls.
usage: dict[str, deque[float]] = defaultdict(deque)
requests_today: dict[str, tuple[str, int]] = {}
lock = Lock()

SYSTEM_PROMPT = (
    "Сен QIPIQ Mini атты шағын цифрлық көмекшісің. "
    "Қазақ тілінде жылы, қысқа, түсінікті 1–3 сөйлеммен жауап бер. "
    "Күрделі сұрақ болса нақты мысал келтір, білмесең ашық айт. "
    "Сен шынайы адам емессің: пайдаланушыны өзіңе тәуелді ететін сөздерді айтпа. "
    "Пайдаланушы кәмелетке толмаған болуы мүмкін: жеке мекенжай, телефон, құпиясөз, "
    "мектеп мекенжайы сияқты деректерді сұрама; қауіпті әрекеттерге нұсқау берме. "
    "Нақты уақыттағы ауа райы, сағат немесе интернет жаңалықтарын білетініңді мәлімдеме, "
    "егер құрал арқылы расталмаса. Сенде камераға немесе нақты құрылғының сенсорларына "
    "тікелей қатынас жоқ. Жауапта Markdown қолданба."
)

class HistoryTurn(BaseModel):
    role: Literal["user", "assistant"]
    content: str = Field(min_length=1, max_length=420)

class ChatRequest(BaseModel):
    message: str = Field(min_length=1, max_length=420)
    history: list[HistoryTurn] = Field(default_factory=list, max_length=6)

class SpeechRequest(BaseModel):
    text: str = Field(min_length=1, max_length=280)


def settings():
    return {
        "api_key": os.getenv("OPENAI_API_KEY", ""),
        "token": os.getenv("QIPIQ_DEVICE_TOKEN", ""),
        "model": os.getenv("QIPIQ_TEXT_MODEL", "gpt-4o-mini"),
        "stt_model": os.getenv("QIPIQ_STT_MODEL", "gpt-transcribe"),
        "tts_model": os.getenv("QIPIQ_TTS_MODEL", "gpt-4o-mini-tts"),
        "tts_voice": os.getenv("QIPIQ_TTS_VOICE", "marin"),
        "demo": os.getenv("QIPIQ_DEMO_MODE", "false").lower() == "true",
        "minute_limit": max(1, int(os.getenv("QIPIQ_REQUESTS_PER_MINUTE", "12"))),
        "daily_limit": max(1, int(os.getenv("QIPIQ_REQUESTS_PER_DAY", "150"))),
    }


def authorize(x_device_token: str = Header(default="")) -> str:
    valid_token = settings()["token"]
    if not valid_token:
        raise HTTPException(status_code=503, detail="QIPIQ_DEVICE_TOKEN серверде орнатылмаған")
    if not hmac.compare_digest(x_device_token, valid_token):
        raise HTTPException(status_code=401, detail="Құрылғы коды қате")
    return "device"


def throttle(device: str) -> None:
    s = settings()
    now = time.monotonic()
    day = time.strftime("%Y-%m-%d", time.gmtime())
    with lock:
        q = usage[device]
        while q and q[0] <= now - 60:
            q.popleft()
        old_day, used = requests_today.get(device, (day, 0))
        if old_day != day:
            used = 0
        if len(q) >= s["minute_limit"] or used >= s["daily_limit"]:
            raise HTTPException(status_code=429, detail="QIPIQ сұрау шегі бітті, кейін қайталаңыз")
        q.append(now)
        requests_today[device] = (day, used + 1)


def get_key() -> str:
    value = settings()["api_key"]
    if not value:
        raise HTTPException(status_code=503, detail="OPENAI_API_KEY серверге енгізілмеген")
    return value


def openai_headers() -> dict[str, str]:
    return {"Authorization": "Bearer " + get_key()}


def get_http_client() -> httpx.Client:
    return httpx.Client(timeout=30.0, follow_redirects=False)


def provider_json(response: httpx.Response) -> dict:
    response.raise_for_status()
    return response.json()


@app.get("/")
def index():
    return FileResponse(APP_DIR / "web" / "index.html")


@app.get("/health")
def health():
    s = settings()
    return {"status": "ok", "ai_ready": bool(s["api_key"]),
            "auth_ready": bool(s["token"]), "mode": "demo" if s["demo"] else "live",
            "text_model": s["model"], "tts_model": s["tts_model"]}


@app.get("/v1/ping")
def ping(device: str = Depends(authorize)):
    return {"authenticated": True, "service": "QIPIQ Mini"}

def reply_for(message: str, history: list[HistoryTurn]) -> str:
    messages = [{"role": x.role, "content": x.content} for x in history]
    messages.append({"role": "user", "content": message})
    with get_http_client() as client:
        response = client.post(
            "https://api.openai.com/v1/responses",
            headers={**openai_headers(), "Content-Type": "application/json"},
            json={"model": settings()["model"], "instructions": SYSTEM_PROMPT,
                  "input": messages, "max_output_tokens": 300, "store": False,
                  **({"reasoning": {"effort": "none"}} if settings()["model"] == "gpt-4o-mini" else {})},
        )
    data = provider_json(response)
    # The SDK's output_text is a convenience helper. Raw HTTP Responses use
    # output[].content[].text (output items may include non-message content).
    chunks = []
    for item in data.get("output", []):
        if item.get("type") != "message":
            continue
        for part in item.get("content", []):
            if part.get("type") == "output_text":
                chunks.append(part.get("text", ""))
    answer = "\n".join(chunks).strip()
    if not answer:
        raise RuntimeError("Model returned no text")
    return answer[:850]


def mood_from_text(s: str):
    s = s.lower()
    if any(t in s for t in ("қуаныш", "керемет", "рахмет", "жарайсың", "😊")):
        return "happy"
    if any(t in s for t in ("кешір", "өкініш", "қинал")):
        return "sad"
    return "friendly"


@app.post("/v1/chat")
def chat(body: ChatRequest, device: str = Depends(authorize)):
    throttle(device)
    if settings()["demo"]:
        return {"reply": f"Сәлем! Мен QIPIQ Mini демомын. Сіз: «{body.message}» деп сұрадыңыз. ChatGPT үшін DEMO_MODE=false орнатыңыз.",
                "emotion": "happy", "mode": "demo"}
    try:
        answer = reply_for(body.message, body.history)
    except HTTPException:
        raise
    except Exception:
        raise HTTPException(502, "AI қызметі жауап бермеді. API кілтін, баланс пен модельді тексеріңіз")
    return {"reply": answer, "emotion": mood_from_text(answer), "mode": "openai"}


AUDIO_EXTENSIONS = {".webm", ".wav", ".mp3", ".m4a", ".mp4", ".mpeg", ".mpga"}
MAX_AUDIO_BYTES = 2 * 1024 * 1024

@app.post("/v1/transcribe")
def transcribe(audio: UploadFile = File(...), device: str = Depends(authorize)):
    throttle(device)
    suffix = Path(audio.filename or "").suffix.lower()
    if suffix not in AUDIO_EXTENSIONS:
        raise HTTPException(415, "WAV, MP3, M4A немесе WebM аудио жіберіңіз")
    raw = audio.file.read(MAX_AUDIO_BYTES + 1)
    if not raw or len(raw) > MAX_AUDIO_BYTES:
        raise HTTPException(413, "Аудио көлемі 2 МБ-тан аспауы керек")
    if settings()["demo"]:
        return {"text": "Сәлем, QIPIQ!", "mode": "demo"}
    try:
        with get_http_client() as client:
            response = client.post(
                "https://api.openai.com/v1/audio/transcriptions",
                headers=openai_headers(),
                data={"model": settings()["stt_model"],
                      "prompt": "Қазақша және орысша сөйлеу. QIPIQ Mini, Қипиқ, AI гаджет."},
                files={"file": (f"recording{suffix}", raw, audio.content_type or "application/octet-stream")},
            )
        data = provider_json(response)
        return {"text": str(data.get("text", ""))[:420], "mode": "openai"}

    except HTTPException:
        raise
    except Exception:
        raise HTTPException(502, "Дауысты тану сәтсіз аяқталды")


@app.post("/v1/speech")
def speech(body: SpeechRequest, device: str = Depends(authorize)):
    throttle(device)
    if settings()["demo"]:
        raise HTTPException(503, "Демо режимінде синтезделген дыбыс жоқ")
    try:
        with get_http_client() as client:
            response = client.post(
                "https://api.openai.com/v1/audio/speech",
                headers={**openai_headers(), "Content-Type": "application/json"},
                json={"model": settings()["tts_model"], "voice": settings()["tts_voice"],
                      "input": body.text, "response_format": "mp3",
                      "instructions": "Speak Kazakh clearly, warmly, and naturally, as a friendly digital assistant."},
            )
        response.raise_for_status()
        if len(response.content) > 4 * 1024 * 1024:
            raise RuntimeError("Unexpected oversized audio")
        return Response(content=response.content, media_type="audio/mpeg",
                        headers={"Cache-Control": "no-store"})

    except HTTPException:
        raise
    except Exception:
        raise HTTPException(502, "Дыбыстап жауап беру сәтсіз аяқталды")
