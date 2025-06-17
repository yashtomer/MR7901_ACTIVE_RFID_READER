# MR7901 Active RFID Reader Communication Protocol

This repository contains a robust Python-based TCP server implementation for parsing and acknowledging data from MR7901 Active RFID Reader devices. It includes registration, login, heartbeat, and tag upload support as per the official protocol specification.

## 🚀 Features

- Full support for MR7901 device communication protocol
- CRC16-CCITT checksum verification
- Tag parsing with SOS alarm status
- UTC and IST timestamp support
- MySQL database integration for tag storage
- Modular architecture with threading support

## 📚 Use Cases

- Real-time asset tracking
- Industrial worker safety using SOS cards
- RFID-based attendance or presence detection

## 🧠 Protocol Support

Implements:
- Terminal Registration (`0x0008`)
- Terminal Login (`0x0001`)
- Heartbeat Response (`0x0003`)
- Tag Upload Response (`0x0004`)

## 📁 Project Structure

```
├── tlv_rfid_tcp_server_with_registration.py   # Main server script
├── protocol_docs/                             # Original protocol specs
├── .env                                       # MySQL credentials
└── README.md                                  # Project overview
```

## 💾 Database Schema

SQL schema to support tag storage is included in the repo under `rfid_tags_schema.sql`.

## ⚙️ Setup

```bash
# Clone the repo
git clone https://github.com/yashtomer/MR7901_ACTIVE_RFID_READER.git
cd MR7901_ACTIVE_RFID_READER

# Install Python dependencies
pip install -r requirements.txt

# Set up environment variables
cp .env.example .env  # and edit it

# Start the server
python3 tlv_rfid_tcp_server_with_registration.py
```

## 🔌 Integration

- MySQL 8.0+
- Python 3.8+
- Flask or NestJS API (included separately)

## 🧪 Testing

Tested with MR7901 active tags, SOS button tags, and simulated TCP payloads.

## 📸 Screenshots

> Add screenshots of terminal logs, MySQL table rows, and parsed tag examples for better SEO and visuals.

## 📈 SEO Keywords

> RFID Reader, MR7901, Active Tag, Python TCP Server, SOS Card, Tag Parsing, RFID Asset Tracking, Real-Time Tag Reader

## 📄 License

MIT License

---

For questions or feature requests, please [open an issue](https://github.com/yashtomer/MR7901_ACTIVE_RFID_READER/issues).

Made with 💡 by [Yash Tomer](https://github.com/yashtomer)
