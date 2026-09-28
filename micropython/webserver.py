"""MicroPython uasyncio HTTP server for Honeywell ActivLink controls."""

import json
import uasyncio as asyncio

INDEX_HTML = """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Honeywell ActivLink MicroPython Gateway</title>
    <style>
        body { font-family: sans-serif; background: #0f172a; color: #f8fafc; padding: 20px; }
        .card { background: #1e293b; border-radius: 8px; padding: 20px; max-width: 600px; margin: 0 auto; }
        h1, h2 { color: #38bdf8; }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; color: #94a3b8; }
        input, select, button { width: 100%; padding: 10px; border-radius: 4px; border: 1px solid #475569; background: #0f172a; color: #fff; box-sizing: border-box; }
        button { background: #0284c7; font-weight: bold; cursor: pointer; border: none; margin-top: 10px; }
        button:hover { background: #0369a1; }
    </style>
</head>
<body>
    <div class="card">
        <h1>MicroPython Honeywell ActivLink</h1>
        <h2>Transmit Signal</h2>
        <form id="txForm">
            <div class="form-group">
                <label>Key ID (Hex)</label>
                <input type="text" id="keyId" value="8BFA3" required>
            </div>
            <div class="form-group">
                <label>Alert Mode</label>
                <select id="alert">
                    <option value="0">0 - Normal Ring</option>
                    <option value="1">1 - Halo Light Flash</option>
                    <option value="3">3 - Loud Alarm</option>
                </select>
            </div>
            <div class="form-group">
                <label>Secret Knock</label>
                <select id="secretKnock">
                    <option value="0">No</option>
                    <option value="1">Yes (3x Press)</option>
                </select>
            </div>
            <button type="submit">Transmit Signal</button>
        </form>
    </div>
    <script>
        document.getElementById('txForm').addEventListener('submit', async (e) => {
            e.preventDefault();
            const payload = {
                key_id: document.getElementById('keyId').value,
                alert: parseInt(document.getElementById('alert').value),
                secret_knock: parseInt(document.getElementById('secretKnock').value)
            };
            await fetch('/api/transmit', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(payload)
            });
            alert('Signal transmitted!');
        });
    </script>
</body>
</html>
"""


class MicroWebServer:
    def __init__(self, engine, radio, port=80):
        self.engine = engine
        self.radio = radio
        self.port = port

    async def handle_client(self, reader, writer):
        request_line = await reader.readline()
        if not request_line:
            writer.close()
            await writer.wait_closed()
            return

        req_str = request_line.decode("utf-8")
        parts = req_str.split(" ")
        method = parts[0] if len(parts) > 0 else "GET"
        path = parts[1] if len(parts) > 1 else "/"

        headers = {}
        while True:
            line = await reader.readline()
            if line == b"\r\n" or not line:
                break
            header_str = line.decode("utf-8")
            if ":" in header_str:
                k, v = header_str.split(":", 1)
                headers[k.strip().lower()] = v.strip()

        if method == "GET" and path == "/":
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + INDEX_HTML
            writer.write(response.encode("utf-8"))
        elif method == "GET" and path == "/api/status":
            body = json.dumps({"status": "online", "freq_mhz": self.radio.freq_mhz})
            response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" + body
            writer.write(response.encode("utf-8"))
        elif method == "POST" and path == "/api/transmit":
            content_length = int(headers.get("content-length", 0))
            body_bytes = await reader.readexactly(content_length) if content_length > 0 else b"{}"
            data = json.loads(body_bytes.decode("utf-8"))

            key_id = data.get("key_id", "8BFA3")
            alert = data.get("alert", 0)
            secret_knock = data.get("secret_knock", 0)
            burst = data.get("burst", 50)

            frame_hex = self.engine.build_frame_hex(key_id, secret_knock=secret_knock, alert=alert)
            self.engine.transmit_frame(frame_hex, burst_count=burst)

            resp_data = json.dumps({"status": "transmitted", "frame_hex": frame_hex})
            response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" + resp_data
            writer.write(response.encode("utf-8"))
        else:
            writer.write(b"HTTP/1.1 404 Not Found\r\n\r\nNot Found")

        await writer.drain()
        writer.close()
        await writer.wait_closed()

    async def start(self):
        server = await asyncio.start_server(self.handle_client, "0.0.0.0", self.port)
        print("MicroPython Web Server running on port", self.port)
