import os
import unittest
from pathlib import Path
from unittest.mock import patch

from fastapi import HTTPException
from fastapi.responses import FileResponse
from pydantic import ValidationError

from backend.app import ChatRequest, authorize, chat, health, index, requests_today, usage


class BackendSmokeTests(unittest.TestCase):
    def setUp(self):
        self.env = patch.dict(os.environ, {
            "QIPIQ_DEVICE_TOKEN": "test-only-token",
            "QIPIQ_DEMO_MODE": "true",
            "OPENAI_API_KEY": "",
        })
        self.env.start()
        usage.clear()
        requests_today.clear()

    def tearDown(self):
        self.env.stop()

    def test_health_reports_demo_without_secrets(self):
        status = health()
        self.assertEqual(status["status"], "ok")
        self.assertEqual(status["mode"], "demo")
        self.assertFalse(status["ai_ready"])
        self.assertTrue(status["auth_ready"])

    def test_root_serves_existing_standalone_demo(self):
        result = index()
        self.assertIsInstance(result, FileResponse)
        self.assertEqual(Path(result.path).name, "index.html")
        self.assertTrue(Path(result.path).is_file())
        self.assertEqual(Path(result.path).parent.name, "web-demo")

    def test_device_token_is_required_and_compared(self):
        self.assertEqual(authorize("test-only-token"), "device")
        with self.assertRaises(HTTPException) as error:
            authorize("incorrect-token")
        self.assertEqual(error.exception.status_code, 401)

    def test_demo_chat_returns_explicit_demo_mode(self):
        result = chat(ChatRequest(message="Сәлем"), "device")
        self.assertEqual(result["mode"], "demo")
        self.assertIn("Сәлем", result["reply"])

    def test_chat_request_rejects_oversized_input(self):
        with self.assertRaises(ValidationError):
            ChatRequest(message="x" * 421)


if __name__ == "__main__":
    unittest.main()
