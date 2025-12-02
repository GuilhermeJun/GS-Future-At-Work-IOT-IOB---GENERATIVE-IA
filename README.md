# GS2 IoT Oracle Integration

This project integrates an ESP32 IoT device with a Node-RED backend and an Oracle XE database. The ESP32 generates sensor data and sends it via HTTP POST to a Node-RED flow. The flow normalizes the payload and inserts the readings into the `leitura_iot` table using an Oracle client.

## Features
- ESP32 generates fake accelerometer/gyroscope readings when a hardware button is pressed.
- Data is sent as JSON to a Node-RED REST endpoint.
- Node-RED normalizes the payload and inserts it into Oracle XE.
- Debug nodes show normalized payload and Oracle responses.
- Modular architecture to allow extension (MQTT, dashboards, etc).

## Components
### 1. ESP32 Firmware (`.ino`)
- Connects to Wi-Fi
- Reads button input
- Generates fake sensor data
- Sends POST request to Node-RED endpoint: `/api/leituras-iot`

### 2. Node-RED Flow
Nodes:
- **HTTP In** (`/api/leituras-iot`)
- **JSON Parser**
- **Function** (normalize payload)
- **Oracle Insert Node**
- **Debug Nodes**
- **HTTP Response**

### 3. Oracle XE Database
Expected schema includes:
- `dispositivo_iot`
- `leitura_iot`
- Sequences and triggers for primary keys

Connection parameters:
- Host: `<your_host>`
- Port: 1521
- Service: `GSDB`
- User: `GSUSER`
- Password: `gspassword`

Requires Oracle Instant Client installed on the machine running Node-RED.

## How to Run
### Install Node-RED Nodes
```bash
npm install node-red-contrib-oracledb-mod
```

### Import Node-RED Flow
1. Open Node-RED (`http://localhost:1880`)
2. Menu → Import → Paste JSON → Deploy

### Install Oracle Instant Client
Download 64-bit client from Oracle website and configure PATH environment variable.

### Upload ESP32 Code
Use Arduino IDE or PlatformIO to flash the provided `.ino` file.

## API Usage
POST example:
```json
{
  "identificador_hw": "ESP32-ABC-001",
  "acc_x": 0.01,
  "acc_y": -0.02,
  "acc_z": 1.00,
  "gyro_x": 0.03,
  "gyro_y": 0.00,
  "gyro_z": 0.08,
  "movimentado": 1,
  "choque": 0,
  "estado_ativo": "EMP",
  "observacao": "Teste via HTTP"
}
```

Endpoint:
```
POST http://<ip>:1880/api/leituras-iot
```

## Debugging Tips
- If ESP32 can’t reach Node-RED: Ensure PC and ESP32 are on the same Wi-Fi network.
- If Oracle insert fails: Check Instant Client installation, PATH, and credentials.
- Use Node-RED Debug Pane to inspect payloads.

## Repository Structure
```
/esp32
  firmware.ino
/node-red
  flow.json
/database
  schema.sql
  samples.sql
README.md
```

## License
MIT License
