import socket
import struct
import binascii
import datetime
import threading
import time
import json
import pytz
import mysql.connector
from dotenv import load_dotenv
import os


load_dotenv()  # Load environment variables from .env file

HOST = '0.0.0.0'
PORT = 12345


db = mysql.connector.connect(
    host=os.getenv('DB_HOST', 'localhost'),
    user=os.getenv('DB_USER', 'your_user'),
    password=os.getenv('DB_PASSWORD', 'your_pass'),
    database=os.getenv('DB_NAME', 'your_db')
)

def save_tag_to_mysql(tag_info, cursor, conn):
    try:
        cursor.execute('''
            INSERT INTO rfid_tags (
                tag_id,
                device_address,
                tlv_tag_type,
                tlv_length,
                antenna_channel,
                antenna_channel_description,
                tag_type,
                tag_type_hex,
                tag_type_description,
                checksum_id,
                checksum_description,
                carrying_info,
                carrying_info_description,
                tag_status_byte,
                sos_alarm,
                rssi,
                rssi_interpretation,
                recv_time_raw,
                recv_time,
                timestamp
            ) VALUES (
                %(tag_id)s,
                %(device_address)s,
                %(tlv_tag_type)s,
                %(tlv_length)s,
                %(antenna_channel)s,
                %(antenna_channel_description)s,
                %(tag_type)s,
                %(tag_type_hex)s,
                %(tag_type_description)s,
                %(checksum_id)s,
                %(checksum_description)s,
                %(carrying_info)s,
                %(carrying_info_description)s,
                %(tag_status_byte)s,
                %(sos_alarm)s,
                %(rssi)s,
                %(rssi_interpretation)s,
                %(recv_time_raw)s,
                %(recv_time)s,
                %(timestamp)s
            )
            ON DUPLICATE KEY UPDATE
                timestamp = VALUES(timestamp),
                sos_alarm = VALUES(sos_alarm),
                rssi = VALUES(rssi),
                recv_time = VALUES(recv_time)
        ''', tag_info)
        conn.commit()
    except Exception as e:
        log(f"MySQL DB error: {e}")

def log(msg):
    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"[{timestamp}] {msg}\n")


def crc16_ccitt(data: bytes, poly=0x1021, init_crc=0xFFFF):
    crc = init_crc
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ poly
            else:
                crc <<= 1
            crc &= 0xFFFF
    return crc

# def handle_tag_upload_ack(device_id, seq, ver, sec):
#     log(f"Sending tag upload ACK to device {device_id}")
#     resp_cmd = 0x8004  # Platform ACK for tag upload
#     response_service = b''  # No payload

#     header = struct.pack('>HHIHH16s', 28, resp_cmd, seq, ver, sec,
#                          device_id.encode('ascii').ljust(16, b'\x00'))
#     crc = crc16_ccitt(header + response_service)
#     return b'\x55\xAA' + header + response_service + struct.pack('>H', crc)

def handle_tag_upload_ack(device_id, seq, ver, sec):
    import datetime
    import struct

    log(f"Sending tag upload ACK to device {device_id}")
    resp_cmd = 0x8004  # Platform ACK for tag upload

    # Per Section 6.4.2 of the protocol, response must include:
    # 1 byte operation + 6 byte UTC timestamp (same as heartbeat)
    now = datetime.datetime.utcnow()
    operation_byte = b'\x00'  # 0x00 = No operation
    resp_time = struct.pack('>6B', now.year - 2000, now.month, now.day,
                             now.hour, now.minute, now.second)
    response_service = operation_byte + resp_time

    header = struct.pack('>HHIHH16s', len(response_service)+28, resp_cmd, seq, ver, sec,
                         device_id.encode('ascii').ljust(16, b'\x00'))
    crc = crc16_ccitt(header + response_service)

    return b'\x55\xAA' + header + response_service + struct.pack('>H', crc)


def parse_packet(packet_bytes):
    if not packet_bytes.startswith(b'\x55\xAA'):
        return {"error": "Invalid start flag"}

    try:
        header = packet_bytes[2:30]  # 28 bytes
        total_len, cmd, seq, ver, sec, dev_id = struct.unpack('>HHIHH16s', header)
        service_data = packet_bytes[30:-2]
        crc_received = struct.unpack('>H', packet_bytes[-2:])[0]
        crc_calculated = crc16_ccitt(packet_bytes[2:-2])

        if crc_received != crc_calculated:
            return {"error": "CRC mismatch", "expected": hex(crc_calculated), "received": hex(crc_received)}

        parsed = {
            "start_flag": "55AA",
            "total_length": total_len,
            "command": f"0x{cmd:04X}",
            "sequence_number": seq,
            "version": f"0x{ver:04X}",
            "security_flag": f"0x{sec:04X}",
            "device_id": dev_id.decode(errors='ignore').strip(),
            "data_hex": binascii.hexlify(service_data).decode(),
            "data_bytes": service_data,
            "crc_received": f"0x{crc_received:04X}",
            "crc_calculated": f"0x{crc_calculated:04X}"
        }

        return parsed

    except Exception as e:
        return {"error": str(e)}

def handle_registration(device_id, seq, ver, sec, data_bytes):
    log(f"Registering device: {device_id}")
    if len(data_bytes) < 6:
        return b"\x00"

    device_type = data_bytes[0]
    model_type = data_bytes[1]
    reg_code = data_bytes[2:6].hex().upper()
    log(f"Device Type: {device_type}, Model: {model_type}, Registration Code: {reg_code}")

    resp_cmd = 0x8008
    result = 0x00
    now = datetime.datetime.utcnow()
    timestamp = struct.pack('>6B', now.year - 2000, now.month, now.day, now.hour, now.minute, now.second)
    ip_string = b"104.255.220.22".ljust(32, b'\x00')
    port_bytes = struct.pack('<H', 12345)

    response_service = struct.pack('>B', result) + timestamp + ip_string + port_bytes
    header = struct.pack('>HHIHH16s', len(response_service)+28, resp_cmd, seq, ver, sec,
                         device_id.encode('ascii').ljust(16, b'\x00'))
    crc = crc16_ccitt(header + response_service)
    return b'\x55\xAA' + header + response_service + struct.pack('>H', crc)

def handle_login(device_id, seq, ver, sec):
    log(f"Login request from device: {device_id}")
    resp_cmd = 0x8001
    result = 0x00
    response_service = struct.pack('>B', result)
    header = struct.pack('>HHIHH16s', len(response_service)+28, resp_cmd, seq, ver, sec,
                         device_id.encode('ascii').ljust(16, b'\x00'))
    crc = crc16_ccitt(header + response_service)
    return b'\x55\xAA' + header + response_service + struct.pack('>H', crc)

# def handle_platform_heartbeat(device_id, seq, ver, sec, data_bytes):
#     log(f"Heartbeat from device {device_id}: {binascii.hexlify(data_bytes).decode()}")
#     resp_cmd = 0x8003
#     response_service = data_bytes
#     header = struct.pack('>HHIHH16s', len(response_service)+23, resp_cmd, seq, ver, sec,
#                          device_id.encode('ascii').ljust(16, b'\x00'))
#     crc = crc16_ccitt(header + response_service)
#     return b'\x55\xAA' + header + response_service + struct.pack('>H', crc)

def handle_platform_heartbeat(device_id, seq, ver, sec, data_bytes):
    import binascii
    import datetime
    import pytz
    import struct

    log(f"Heartbeat from device {device_id}: {binascii.hexlify(data_bytes).decode()}")

    # Per protocol doc (Section 6.3), the heartbeat service content has:
    # 2 bytes - device working status
    # 2 bytes - device status
    # 2 bytes - software version
    # 6 bytes - device time

    if len(data_bytes) < 12:
        log("Warning: Incomplete heartbeat payload")
    else:
        work_status = data_bytes[0:2]
        device_status = data_bytes[2:4]
        sw_version = data_bytes[4:6]
        timestamp_bytes = data_bytes[6:12]

        work_status_val = int.from_bytes(work_status, byteorder='big')
        device_status_val = int.from_bytes(device_status, byteorder='big')
        sw_major, sw_minor = sw_version[0], sw_version[1]

        try:
            year = 2000 + timestamp_bytes[0]
            month = timestamp_bytes[1]
            day = timestamp_bytes[2]
            hour = timestamp_bytes[3]
            minute = timestamp_bytes[4]
            second = timestamp_bytes[5]
            dt_utc = datetime.datetime(year, month, day, hour, minute, second, tzinfo=datetime.timezone.utc)
            dt_ist = dt_utc.astimezone(pytz.timezone("Asia/Kolkata"))
            log(f"Device Time (IST): {dt_ist.strftime('%Y-%m-%d %H:%M:%S')}")
        except Exception as e:
            log(f"Invalid timestamp in heartbeat: {e}")

        log(f"Working Status: 0x{work_status_val:04X}, Device Status: 0x{device_status_val:04X}, Software Version: V{sw_major}.{sw_minor}")

    # Response as per section 6.3 of protocol: 1 byte operation + 6 byte time
    now = datetime.datetime.utcnow()
    resp_cmd = 0x8003
    operation_byte = b'\x00'  # 0x00 = No operation
    resp_time = struct.pack('>6B', now.year - 2000, now.month, now.day, now.hour, now.minute, now.second)
    response_service = operation_byte + resp_time

    header = struct.pack('>HHIHH16s', len(response_service)+28, resp_cmd, seq, ver, sec,
                         device_id.encode('ascii').ljust(16, b'\x00'))
    crc = crc16_ccitt(header + response_service)

    return b'\x55\xAA' + header + response_service + struct.pack('>H', crc)




def parse_tag_report(data_bytes, device_id, cursor, conn):
    tags = []
    offset = 0

     # Convert device_id string to decimal representation
    device_clean = device_id.replace('\x00', '').strip()
    while offset + 4 <= len(data_bytes):
        try:
            tag_type, length = struct.unpack('>HH', data_bytes[offset:offset+4])
            #print('tag_type=======================',tag_type)
            offset += 4
            value = data_bytes[offset:offset+length]
            offset += length

            if tag_type == 0x8B01 and len(value) >= 17:
                
                rssi_val = int.from_bytes(value[10:11], byteorder='big', signed=True)
                tag_type_val = value[1]
                checksum_val = value[6]
                carry_hex = value[7:9].hex()
                antenna_val = value[0]
                #print('sos=======================',value)
                #print('sos=======================',value[9])

                #print('sos=======================',value[2:6])
                #print('sos=======================',value[2:6].hex())

                
                tag_info = {
                    "tlv_tag_type": "0x8B01",
                    "tlv_length": length,
                    "antenna_channel": antenna_val,
                    "antenna_channel_description": interpret_antenna_channel(antenna_val),
                    "tag_type": value[1],
                    "tag_type_hex": f"0x{tag_type_val:02X}",
                    "tag_type_description": interpret_tag_type(tag_type_val),
                    "tag_id": value[2:6].hex().upper(),
                    "checksum_id": value[6],
                    "checksum_description": interpret_checksum_id(checksum_val),
                    "carrying_info": carry_hex,
                    "carrying_info_description": interpret_carrying_info(carry_hex),
                    "tag_status_byte": value[9],
                    "sos_alarm": "Active" if value[9] == 128 else "Inactive",
                    "rssi": int.from_bytes(value[10:11], byteorder='big', signed=True),
                    "rssi_interpretation": interpret_rssi(int.from_bytes(value[10:11], byteorder='big', signed=True)),
                    "recv_time_raw": value[11:17].hex(),
                    "device_address": device_clean

                }

                try:
                    year = 2000 + value[11]
                    month = value[12]
                    day = value[13]
                    hour = value[14]
                    minute = value[15]
                    second = value[16]
                    utc_dt = datetime.datetime(year, month, day, hour, minute, second, tzinfo=datetime.timezone.utc)
                    ist = pytz.timezone("Asia/Kolkata")
                    local_dt = utc_dt.astimezone(ist)


                    #tag_info["recv_time"] = f"{year}-{month:02}-{day:02} {hour:02}:{minute:02}:{second:02}"
                    #tag_info["timestamp"] = datetime.datetime(year, month, day, hour, minute, second).isoformat()'
                    tag_info["recv_time"] = local_dt.strftime("%Y-%m-%d %H:%M:%S")
                    tag_info["timestamp"] = local_dt.isoformat()
                    
                    save_tag_to_mysql(tag_info, cursor, conn)

                except Exception as e:
                    tag_info["timestamp"] = f"Invalid timestamp: {e}"

                tags.append(tag_info)

            else:
                tags.append({
                    "tlv_tag_type": hex(tag_type),
                    "length": length,
                    "raw_value": value.hex()
                })

        except Exception as e:
            tags.append({"error": str(e), "raw": binascii.hexlify(data_bytes[offset:]).decode()})
            break

    return tags

def interpret_rssi(rssi):
    if rssi >= -40:
        return "Excellent signal (very close)"
    elif -60 < rssi < -40:
        return "Good signal (nearby)"
    elif -75 < rssi <= -60:
        return "Moderate signal (in room)"
    elif -90 < rssi <= -75:
        return "Weak signal (far)"
    else:
        return "Very weak or out of range"

def interpret_tag_type(tag_type):
    tag_type_map = {
        0x41: "Card Tag",
        0x42: "Wristband Tag",
        0x43: "Helmet Tag",
        0x44: "Badge Tag",
        0x45: "SOS Button Tag",
        0x46: "Asset Tag",
        0x47: "Temperature Sensor Tag"
    }
    return tag_type_map.get(tag_type, "Unknown")

def interpret_antenna_channel(ch):
        return f"Channel {ch}"

def interpret_checksum_id(byte_val):
    return f"Checksum Byte: {byte_val}" if byte_val > 0 else "No checksum or default"

def interpret_carrying_info(hexval):
    return "Empty" if hexval == "0000" else f"0x{hexval} (custom)"

def client_handler(conn, addr, db):

    cursor = db.cursor()

    log(f"Connected by {addr}")
    with conn:
        try:
            while True:
                data = conn.recv(1024)
                if not data:
                    log("Client disconnected.")
                    break

                log("<========= New Raw Packet Received =========>")
                log(f"Hex: {data.hex()}")
                log(f"Raw Bytes: {data}")

                result = parse_packet(data)
                log(f"Parsed Packet Result: {result}")

                cmd = result.get("command")
                log(f"Command: {cmd}")

                cmd_int = int(cmd, 16)

                if cmd_int == 0x08:
                    response = handle_registration(
                        result['device_id'], result['sequence_number'], int(result['version'], 16), 0, result['data_bytes']
                    )
                    time.sleep(0.2)
                    conn.sendall(response)
                    log(f"Terminal registration request: {response.hex()}")

                elif cmd_int == 0x01:
                    response = handle_login(
                        result['device_id'], result['sequence_number'], int(result['version'], 16), 0
                    )
                    time.sleep(0.2)
                    conn.sendall(response)
                    log(f"Terminal login request: {response.hex()}")

                elif cmd_int == 0x04:
                    tags = parse_tag_report(result['data_bytes'],result['device_id'], cursor, db)
                    log(f"Tags from {result['device_id']}:")
                    for tag in tags:
                        log(json.dumps(tag, indent=2))

                    response = handle_tag_upload_ack(
                        result['device_id'], result['sequence_number'], int(result['version'], 16), 0
                    )
                    conn.sendall(response)
                    log(f"The terminal sends the tag data (Tag Upload): {response.hex()}")    

                elif cmd_int == 0x03:
                    response = handle_platform_heartbeat(
                        result['device_id'], result['sequence_number'], int(result['version'], 16), 0, result['data_bytes']
                    )
                    conn.sendall(response)
                    log(f"Terminal heartbeat: {response.hex()}")

                else:
                    log(f"Unknown command packet: {result}")

        except ConnectionResetError:
            log("Connection reset by peer.")
        except Exception as e:
            log(f"Unexpected error: {e}")
        finally:
            cursor.close()  

log(f"Listening for RFID data on {HOST}:{PORT}...")


with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind(("0.0.0.0", 12345))
    server_socket.listen()

    while True:
        conn, addr = server_socket.accept()
        threading.Thread(target=client_handler, args=(conn, addr, db), daemon=True).start()
