# RFID TCP Server and Tag Interpretation by Aeologic

This project implements a TCP server in Python that listens for data sent by RFID devices, parses the incoming packets using the MR7901 protocol, and outputs structured tag information with interpreted metadata.

## Features

* ✅ CRC16-CCITT validation
* ✅ Parses TLV tag structure (`0x8B01`)
* ✅ Interprets:

  * Tag type (e.g., Card Tag, SOS Button Tag)
  * RSSI signal strength and proximity estimate
  * SOS alarm status
  * Timestamp and raw values
* ✅ Sends appropriate ACKs for registration, login, heartbeat, and tag uploads
* ✅ Outputs logs with structured JSON for every tag read

---

## Example Tag JSON Output

```json
{
  "tlv_tag_type": "0x8B01",
  "tlv_length": 17,
  "antenna_channel": 68,
  "antenna_channel_description": "Channel 68",
  "tag_type": 65,
  "tag_type_hex": "0x41",
  "tag_type_description": "Card Tag",
  "tag_id": "06D8E891",
  "checksum_id": 104,
  "checksum_description": "Checksum Byte: 104",
  "carrying_info": "0000",
  "carrying_info_description": "Empty",
  "tag_status_byte": 0,
  "sos_alarm": "Inactive",
  "rssi": -55,
  "rssi_interpretation": "Good signal (nearby)",
  "recv_time_raw": "19060e0c342a",
  "recv_time": "2025-06-14 12:52:42",
  "timestamp": "2025-06-14T12:52:42"
}
```

---

## How to Run

1. Clone this repo and navigate into the directory.
2. Make sure Python 3.7+ is installed.
3. Run the server:

```bash
python3 tlv_rfid_tcp_server.py
```

The server listens on port `12345` and outputs parsed logs as tags are received.

---

## Architecture

* `client_handler()` handles per-socket communication
* `parse_packet()` validates and parses packet header + CRC
* `parse_tag_report()` extracts and interprets tag TLV values
* Uses `struct` and `binascii` for low-level binary unpacking

---

## Frontend Dashboard (Optional)

Refer to the [Web Prompt Document](./rfid_tag_web_prompt.md) for AI-generated design instructions to build a real-time dashboard using React or Tailwind CSS.

---

## Use Cases

* Warehouse inventory tracking
* Asset management
* Staff attendance monitoring
* Real-time proximity alert systems

---

## License

This project is private and proprietary. Reach out for licensing or integration support.
