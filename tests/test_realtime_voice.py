import asyncio
import base64
import json
import os
import struct
import unittest
from unittest.mock import patch

from fastapi.testclient import TestClient

from backend import app as gateway
from backend import realtime_voice


class FakeSocket:
    def __init__(self, events):
        self.events = iter(events)
        self.sent = []

    async def recv(self):
        return json.dumps(next(self.events))

    async def send(self, message):
        self.sent.append(json.loads(message))


class VoiceTests(unittest.TestCase):
    def setUp(self):
        self.pcm = struct.pack("<3200h", *([120] * 3200))
        self.headers = {
            "Content-Type": "application/octet-stream",
            "X-Device-Token": "voice-test-token",
        }
        env = patch.dict(os.environ, {
            "QIPIQ_DEVICE_TOKEN": "voice-test-token",
            "OPENAI_API_KEY": "test-placeholder",
            "QIPIQ_DEMO_MODE": "false",
        })
        env.start()
        self.addCleanup(env.stop)
        self.client = TestClient(gateway.app)
        self.addCleanup(self.client.close)

    def test_realtime_turn_commits_pcm_before_requesting_reply(self):
        encoded = base64.b64encode(b"\x01\x00").decode()
        socket = FakeSocket([
            {"type": "session.created"},
            {"type": "session.updated"},
            {"type": "input_audio_buffer.committed"},
            {"type": "response.output_audio.delta", "delta": encoded},
            {"type": "response.output_audio.delta", "delta": encoded},
            {"type": "response.done", "response": {"status": "completed"}},
        ])
        pcm = realtime_voice.upsample_pcm(self.pcm)
        self.assertEqual(len(pcm), 9_600)
        self.assertEqual(pcm, struct.pack("<4800h", *([120] * 4800)))
        reply = asyncio.run(realtime_voice._exchange(socket, pcm, "marin", "Speak Kazakh"))
        self.assertEqual(reply, b"\x01\x00" * 2)
        self.assertEqual([event["type"] for event in socket.sent], [
            "session.update", "input_audio_buffer.append",
            "input_audio_buffer.commit", "response.create",
        ])
        self.assertIsNone(socket.sent[0]["session"]["audio"]["input"]["turn_detection"])
        self.assertEqual(base64.b64decode(socket.sent[1]["audio"]), pcm)

    def test_authenticated_route_returns_device_audio(self):
        with patch.object(gateway, "request_voice_reply", return_value=b"\x10\x00") as provider:
            response = self.client.post("/v1/voice", content=self.pcm, headers=self.headers)
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.content, b"\x10\x00")
        self.assertEqual(response.headers["X-QIPIQ-Sample-Rate"], "24000")
        self.assertEqual(response.headers["X-QIPIQ-Audio-Format"], "pcm-s16le")
        self.assertEqual(response.headers["Cache-Control"], "no-store")
        self.assertEqual(provider.await_args.args, (self.pcm,))

    def test_invalid_requests_do_not_contact_provider(self):
        with patch.object(gateway, "request_voice_reply") as provider:
            cases = [
                (self.pcm, {"Content-Type": "application/octet-stream"}, 401),
                (self.pcm, {**self.headers, "Content-Type": "audio/wav"}, 415),
                (self.pcm + b"\x00", self.headers, 422),
                (bytes(gateway.MAX_VOICE_BYTES + 2), self.headers, 413),
            ]
            for audio, headers, status in cases:
                with self.subTest(status=status):
                    response = self.client.post("/v1/voice", content=audio, headers=headers)
                    self.assertEqual(response.status_code, status)
            provider.assert_not_awaited()

    def test_provider_failure_returns_a_bounded_error(self):
        for error, status in ((TimeoutError(), 504), (realtime_voice.VoiceError(), 502)):
            with self.subTest(status=status):
                with patch.object(gateway, "request_voice_reply", side_effect=error):
                    response = self.client.post("/v1/voice", content=self.pcm, headers=self.headers)
                self.assertEqual(response.status_code, status)

    def test_incomplete_provider_reply_is_not_played(self):
        socket = FakeSocket([
            {"type": "session.created"},
            {"type": "session.updated"},
            {"type": "input_audio_buffer.committed"},
            {"type": "response.output_audio.delta", "delta": "AQA="},
            {"type": "response.done", "response": {"status": "incomplete"}},
        ])
        with self.assertRaises(realtime_voice.VoiceError):
            asyncio.run(realtime_voice._exchange(socket, self.pcm, "marin", "Speak Kazakh"))
