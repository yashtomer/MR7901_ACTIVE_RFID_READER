[2025-06-14 13:20:39] <=====================================New Raw Packet Received =================================================================>:

[2025-06-14 13:20:39] Hex: 55aa003100040000007a00010000323330363036313031303030323630008b010011814106d8e89168000000c519060e0532061d5c

[2025-06-14 13:20:39] Raw Bytes: b'U\xaa\x001\x00\x04\x00\x00\x00z\x00\x01\x00\x00230606101000260\x00\x8b\x01\x00\x11\x81A\x06\xd8\xe8\x91h\x00\x00\x00\xc5\x19\x06\x0e\x052\x06\x1d\\'

[2025-06-14 13:20:39] Parsed Packet Result: {'command': '0x4', 'seq': 122, 'version': '0x1', 'device_id': '230606101000260\x00', 'data_hex': '8b010011814106d8e89168000000c519060e053206', 'data_bytes': b'\x8b\x01\x00\x11\x81A\x06\xd8\xe8\x91h\x00\x00\x00\xc5\x19\x06\x0e\x052\x06'}

[2025-06-14 13:20:39] Command: 0x4

Raw tag value: 814106d8e89168000000c519060e053206 => Y:2025 M:6 D:14 5:50:6, SOS: 0
[2025-06-14 13:20:39] Tags from 230606101000260:

[2025-06-14 13:20:39] {'tag_id': '4106D8E8', 'timestamp': '2025-06-14T05:50:06', 'sos': 0}


55aa003100040000007a00010000323330363036313031303030323630008b010011814106d8e89168000000c519060e0532061d5c

55aa 
0031
0004
00000086
0001
0000
32333036303631303130303032363000

tlv data
8b01
0011
81 Antena Channel Number
41 Tag Type 
06d8e891 Tag id
68  Checksum ID 
0000  Carrying Information
00 sos alarm
c5 RSSI
19060e053206  Receiving Time
1d5c


### 🧾 Parsed Tag Packet Breakdown (0x0004 - Tag Upload)

#### 🧪 Raw Data:
```
55aa003100040000007e00010000323330363036313031303030323630008b010011444106d8e89168000000c919060e0c0e2a84f9
```

---

### 🧱 Breakdown:

#### ✅ Header:
| Field             | Hex            | Meaning                          |
|------------------|----------------|----------------------------------|
| Start Flag       | `55AA`         | Packet start                     |
| Total Length     | `0031` = 49    | Total packet size                |
| Command          | `0004`         | Tag data upload                  |
| Sequence No.     | `007E` = 126   | For matching ACKs                |
| Version          | `0001`         | Protocol version 1               |
| Security Flag    | `0000`         | No encryption                    |
| Device ID        | `32333036...`  | ASCII: `230606101000260`         |

#### 🧩 TLV Data:
| Field               | Value      | Meaning                       |
|--------------------|------------|-------------------------------|
| TLV Tag Type       | `8B01`     | Tag report                    |
| TLV Length         | `0011`     | 17 bytes                      |
| Antenna Channel    | `44`       | Channel 68                    |
| Tag Type           | `41`       | (Custom type)                 |
| Tag ID             | `06D8E891` | Unique RFID Tag ID            |
| Checksum ID        | `68`       | Device checksum byte          |
| Carrying Info      | `0000`     | Reserved field                |
| Status Byte        | `00`       | No SOS triggered              |
| RSSI               | `C9`       | -55 dBm (signal strength)     |
| Timestamp Raw      | `19060e0c0e2a` | 2025-06-14 12:14:42        |

---

### 📘 How You Can Use This

| Field         | Use                                                            |
|---------------|-----------------------------------------------------------------|
| Tag ID        | Identify individual item or person with the tag                |
| RSSI          | Distance estimate (lower = closer)                             |
| Timestamp     | When the tag was detected                                       |
| SOS Alarm     | Use this to detect if emergency/help button was pressed        |
| Device ID     | Reader identity (useful if you have multiple readers deployed) |

---

### ✅ Final Verdict:
This packet shows that RFID tag `06D8E891` was read at `2025-06-14 12:14:42` by device `230606101000260` with good signal strength and no alarm triggered.

Let me know if you’d like to visualize this data, store it in a database, or trigger alerts on SOS or RSSI thresholds.
