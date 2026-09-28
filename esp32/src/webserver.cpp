#include "webserver.h"

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Honeywell ActivLink Gateway</title>
    <style>
        :root { --bg: #0f172a; --panel: #1e293b; --accent: #38bdf8; --text: #f8fafc; --muted: #94a3b8; }
        body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; background: var(--bg); color: var(--text); margin: 0; padding: 20px; }
        .container { max-width: 900px; margin: 0 auto; display: grid; gap: 20px; }
        .card { background: var(--panel); border-radius: 12px; padding: 20px; border: 1px solid #334155; }
        h1, h2 { margin-top: 0; color: var(--accent); }
        .form-group { margin-bottom: 15px; display: flex; flex-direction: column; gap: 5px; }
        label { color: var(--muted); font-size: 0.9rem; }
        input, select, button { padding: 10px; border-radius: 6px; border: 1px solid #475569; background: #0f172a; color: #fff; font-size: 1rem; }
        button { background: #0284c7; border: none; font-weight: bold; cursor: pointer; transition: 0.2s; }
        button:hover { background: #0369a1; }
        .grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; }
        table { width: 100%; border-collapse: collapse; margin-top: 10px; }
        th, td { text-align: left; padding: 10px; border-bottom: 1px solid #334155; }
        th { color: var(--muted); font-size: 0.85rem; text-transform: uppercase; }
        .badge { display: inline-block; padding: 3px 8px; border-radius: 12px; font-size: 0.8rem; font-weight: bold; background: #0369a1; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Honeywell ActivLink Web Control</h1>
        <div class="card">
            <h2>Transmit Signal</h2>
            <form id="txForm">
                <div class="grid-2">
                    <div class="form-group">
                        <label>Key ID (Hex)</label>
                        <input type="text" id="keyId" value="8BFA3" required>
                    </div>
                    <div class="form-group">
                        <label>Alert Mode</label>
                        <select id="alert">
                            <option value="0">0 - Normal Ring</option>
                            <option value="1">1 - Halo Light Pattern A</option>
                            <option value="2">2 - Halo Light Pattern B</option>
                            <option value="3">3 - Loud Alarm</option>
                        </select>
                    </div>
                </div>
                <div class="grid-2">
                    <div class="form-group">
                        <label>Secret Knock</label>
                        <select id="secretKnock">
                            <option value="0">No</option>
                            <option value="1">Yes (3x Press)</option>
                        </select>
                    </div>
                    <div class="form-group">
                        <label>Low Battery Flag</label>
                        <select id="lowbat">
                            <option value="0">Normal Battery</option>
                            <option value="1">Low Battery Alert</option>
                        </select>
                    </div>
                </div>
                <button type="submit">Send ActivLink Transmission</button>
            </form>
        </div>

        <div class="card">
            <h2>Live Received Packets (SSE)</h2>
            <table>
                <thead>
                    <tr>
                        <th>Time</th>
                        <th>Key ID</th>
                        <th>Type</th>
                        <th>Alert</th>
                        <th>Knock</th>
                        <th>Hex</th>
                    </tr>
                </thead>
                <tbody id="eventsTable">
                    <tr><td colspan="6" style="color:var(--muted); text-align:center;">Waiting for transmissions...</td></tr>
                </tbody>
            </table>
        </div>
    </div>

    <script>
        document.getElementById('txForm').addEventListener('submit', async (e) => {
            e.preventDefault();
            const data = {
                key_id: document.getElementById('keyId').value,
                alert: parseInt(document.getElementById('alert').value),
                secret_knock: parseInt(document.getElementById('secretKnock').value),
                lowbat: parseInt(document.getElementById('lowbat').value),
                relay: 0,
                burst: 50
            };
            await fetch('/api/transmit', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(data)
            });
            alert('Transmission triggered successfully!');
        });

        const evtSource = new EventSource('/api/events');
        evtSource.onmessage = (e) => {
            const pkt = JSON.parse(e.data);
            const table = document.getElementById('eventsTable');
            if (table.rows[0] && table.rows[0].cells.length === 1) table.innerHTML = '';

            const row = table.insertRow(0);
            row.innerHTML = `
                <td>${new Date().toLocaleTimeString()}</td>
                <td><span class="badge">${pkt.key_id}</span></td>
                <td>${pkt.device_type_str}</td>
                <td>${pkt.alert}</td>
                <td>${pkt.secret_knock ? 'Yes' : 'No'}</td>
                <td><code>${pkt.frame_hex}</code></td>
            `;
        };
    </script>
</body>
</html>
)rawliteral";

ActivLinkWebServer::ActivLinkWebServer(ActivLinkEngine& engine, CC1101Transceiver& radio, uint16_t port)
    : _server(port), _events("/api/events"), _engine(engine), _radio(radio) {}

void ActivLinkWebServer::begin() {
    setupRoutes();
    _server.addHandler(&_events);
    _server.begin();
}

void ActivLinkWebServer::setupRoutes() {
    _server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", INDEX_HTML);
    });

    _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(256);
        doc["status"] = "online";
        doc["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
        doc["ip"] = WiFi.localIP().toString();

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    AsyncCallbackJsonWebHandler *txHandler = new AsyncCallbackJsonWebHandler("/api/transmit", [this](AsyncWebServerRequest *request, JsonVariant &json) {
        JsonObject jsonObj = json.as<JsonObject>();
        String keyIdHex = jsonObj["key_id"] | "8BFA3";
        uint8_t alert = jsonObj["alert"] | 0;
        uint8_t secretKnock = jsonObj["secret_knock"] | 0;
        uint8_t lowbat = jsonObj["lowbat"] | 0;
        uint8_t relay = jsonObj["relay"] | 0;
        uint16_t burst = jsonObj["burst"] | 50;

        uint32_t keyId = strtoul(keyIdHex.c_str(), NULL, 16);
        String frameHex = _engine.buildFrameHex(keyId, secretKnock, alert, lowbat, relay);
        _engine.transmitFrame(frameHex, burst);

        DynamicJsonDocument doc(256);
        doc["status"] = "transmitted";
        doc["frame_hex"] = frameHex;

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    _server.addHandler(txHandler);
}

void ActivLinkWebServer::broadcastPacket(const ActivLinkPacket& packet) {
    DynamicJsonDocument doc(512);
    doc["frame_hex"] = packet.frameHex;
    doc["key_id"] = packet.keyIdHex;
    doc["device_type"] = packet.deviceType;
    doc["device_type_str"] = packet.deviceTypeStr;
    doc["alert"] = packet.alert;
    doc["secret_knock"] = packet.secretKnock;
    doc["relay"] = packet.relay;
    doc["lowbat"] = packet.lowbat;
    doc["parity_valid"] = packet.parityValid;

    String jsonStr;
    serializeJson(doc, jsonStr);
    _events.send(jsonStr.c_str(), "message", millis());
}
