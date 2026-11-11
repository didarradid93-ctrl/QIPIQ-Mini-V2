"""One push-to-talk turn through OpenAI Realtime, with bounded PCM buffers."""

import asyncio
import base64
import json
import struct
from urllib.parse import urlencode

from websockets.asyncio.client import connect
from websockets.exceptions import WebSocketException

INPUT_RATE = 16_000
REALTIME_RATE = 24_000
MAX_REPLY_BYTES = REALTIME_RATE * 2 * 20
UPLOAD_CHUNK_BYTES = REALTIME_RATE * 2 // 5


class VoiceError(Exception):
    """The upstream session could not return a complete audio reply."""


def upsample_pcm(pcm: bytes) -> bytes:
    """Interpolate mono signed 16-bit little-endian samples from 16 to 24 kHz."""
    count = len(pcm) // 2
    samples = struct.unpack(f"<{count}h", pcm)
    output_count = (count * 3 + 1) // 2
    result = bytearray(output_count * 2)
    for index in range(output_count):
        left, fraction = divmod(index * 2, 3)
        right = min(left + 1, count - 1)
        value = (samples[left] * (3 - fraction) + samples[right] * fraction) // 3
        struct.pack_into("<h", result, index * 2, value)
    return bytes(result)


async def _receive(connection) -> dict:
    event = json.loads(await connection.recv())
    if event["type"] == "error":
        raise VoiceError("Realtime rejected the request")
    return event


async def _wait_for(connection, event_type: str) -> None:
    while (await _receive(connection))["type"] != event_type:
        pass


async def _exchange(connection, pcm: bytes, voice: str, instructions: str) -> bytes:
    await _wait_for(connection, "session.created")
    format_spec = {"type": "audio/pcm", "rate": REALTIME_RATE}
    await connection.send(json.dumps({
        "type": "session.update",
        "session": {
            "type": "realtime",
            "instructions": instructions,
            "output_modalities": ["audio"],
            "audio": {
                "input": {"format": format_spec, "turn_detection": None},
                "output": {"format": format_spec, "voice": voice},
            },
        },
    }))
    await _wait_for(connection, "session.updated")
    for offset in range(0, len(pcm), UPLOAD_CHUNK_BYTES):
        chunk = pcm[offset:offset + UPLOAD_CHUNK_BYTES]
        await connection.send(json.dumps({
            "type": "input_audio_buffer.append",
            "audio": base64.b64encode(chunk).decode("ascii"),
        }))
    await connection.send('{"type":"input_audio_buffer.commit"}')
    await _wait_for(connection, "input_audio_buffer.committed")
    await connection.send('{"type":"response.create"}')

    reply = bytearray()
    while True:
        event = await _receive(connection)
        if event["type"] == "response.output_audio.delta":
            reply.extend(base64.b64decode(event["delta"], validate=True))
            if len(reply) > MAX_REPLY_BYTES:
                raise VoiceError("Realtime reply exceeded the device limit")
        elif event["type"] == "response.done":
            if event["response"]["status"] != "completed" or not reply or len(reply) % 2:
                raise VoiceError("Realtime returned incomplete audio")
            return bytes(reply)


async def request_voice_reply(
    pcm: bytes, *, api_key: str, model: str, voice: str, instructions: str
) -> bytes:
    url = "wss://api.openai.com/v1/realtime?" + urlencode({"model": model})
    try:
        async with asyncio.timeout(40):
            async with connect(
                url,
                additional_headers={"Authorization": f"Bearer {api_key}"},
                open_timeout=8,
                close_timeout=1,
                max_size=512 * 1024,
            ) as connection:
                return await _exchange(connection, upsample_pcm(pcm), voice, instructions)
    except TimeoutError:
        raise
    except (WebSocketException, OSError, ValueError, KeyError, TypeError) as error:
        raise VoiceError("Realtime connection failed") from error
