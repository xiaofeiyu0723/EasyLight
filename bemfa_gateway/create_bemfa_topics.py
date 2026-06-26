#!/usr/bin/env python3
"""Create Bemfa MQTT switch topics in batches.

Usage:
    python create_bemfa_topics.py devices.example.json --dry-run
    python create_bemfa_topics.py devices.json

The script reads UID/secretID/secretKey from .config/bemfa_gateway_config.h.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import urllib.error
import urllib.request
from pathlib import Path


API_URL = "https://pro.bemfa.com/vs/web/v2/addTopics"
CONFIG_PATH = Path(__file__).resolve().parent / ".config" / "bemfa_gateway_config.h"


def read_define(text: str, name: str, required: bool = True) -> str:
    match = re.search(rf'#define\s+{re.escape(name)}\s+"([^"]*)"', text)
    if not match:
        if required:
            raise ValueError(f"Missing {name} in {CONFIG_PATH}")
        return ""
    return match.group(1).strip()


def load_config() -> dict[str, str]:
    text = CONFIG_PATH.read_text(encoding="utf-8")
    return {
        "uid": read_define(text, "BEMFA_UID"),
        "secretID": read_define(text, "BEMFA_SECRET_ID"),
        "secretKey": read_define(text, "BEMFA_SECRET_KEY"),
    }


def validate_topic(topic: str) -> None:
    if not re.fullmatch(r"[A-Za-z0-9]{1,64}", topic):
        raise ValueError(f"Invalid topic {topic!r}: only letters/numbers, max length 64")
    if not topic.endswith("006"):
        raise ValueError(f"Invalid switch topic {topic!r}: switch topics should end with 006")


def load_devices(path: Path) -> list[dict[str, str]]:
    devices = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(devices, list) or not devices:
        raise ValueError("Device file must be a non-empty JSON array")
    if len(devices) > 99:
        raise ValueError("Bemfa addTopics supports at most 99 topics per request")

    topics_seen: set[str] = set()
    normalized: list[dict[str, str]] = []
    for index, device in enumerate(devices, start=1):
        if not isinstance(device, dict):
            raise ValueError(f"Device #{index} must be an object")
        topic = str(device.get("topic", "")).strip()
        validate_topic(topic)
        if topic in topics_seen:
            raise ValueError(f"Duplicate topic {topic!r}")
        topics_seen.add(topic)

        normalized.append(
            {
                "type": 1,
                "topic": topic,
                "name": str(device.get("name", topic)).strip(),
                "room": str(device.get("room", "")).strip(),
                "group": str(device.get("group", "")).strip(),
                "unit": str(device.get("unit", "")).strip(),
            }
        )
    return normalized


def post_json(payload: dict) -> dict:
    body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
    request = urllib.request.Request(
        API_URL,
        data=body,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(request, timeout=20) as response:
            raw = response.read().decode("utf-8", errors="replace")
    except urllib.error.HTTPError as exc:
        raw = exc.read().decode("utf-8", errors="replace")
        raise RuntimeError(f"HTTP {exc.code}: {raw}") from exc
    return json.loads(raw)


def main() -> int:
    parser = argparse.ArgumentParser(description="Create Bemfa MQTT switch topics")
    parser.add_argument("devices", type=Path, help="JSON device list")
    parser.add_argument("--dry-run", action="store_true", help="validate and print request only")
    args = parser.parse_args()

    config = load_config()
    topics = load_devices(args.devices)
    payload = {
        "uid": config["uid"],
        "secretID": config["secretID"],
        "secretKey": config["secretKey"],
        "region": "cn",
        "topics": topics,
    }

    if not config["secretID"] or not config["secretKey"]:
        print("BEMFA_SECRET_ID/BEMFA_SECRET_KEY are empty in .config/bemfa_gateway_config.h")
        print("Use --dry-run to validate only, or fill the keys before creating topics.")
        if not args.dry_run:
            return 2

    safe_payload = dict(payload)
    safe_payload["uid"] = config["uid"][:6] + "..." + config["uid"][-4:]
    safe_payload["secretID"] = "***" if config["secretID"] else ""
    safe_payload["secretKey"] = "***" if config["secretKey"] else ""
    print(json.dumps(safe_payload, ensure_ascii=False, indent=2))

    if args.dry_run:
        return 0

    result = post_json(payload)
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0 if result.get("code") == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
