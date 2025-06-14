import socket
import struct
import binascii
import datetime
import threading
import time

HOST = '0.0.0.0'
PORT = 12345

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

def parse_packet(packet_bytes):
    if not packet_bytes.startswith(b'\x55\xAA'):
        return {"error": "Invalid start flag"}
    try:
        header = packet_bytes[2:30]
        total_len, cmd, seq, ver, sec, dev_id = struct.unpack('>HHIHH16s', header)
        service_data = packet_bytes[30:-2]
        crc_received = struct.unpack('>H', packet_bytes[-2:])[0]
        crc_calculated = crc16_ccitt(packet_bytes[2:-2])

        if crc_received != crc_calculated:
            return {"error": "CRC mismatch", "expected": hex(crc_calculated), "received": hex(crc_received)}

        return {
            "command": hex(cmd),
            "seq": seq,
            "version": hex(ver),
            "device_id": dev_id.decode(errors='ignore').strip(),
            "data_hex": binascii.hexlify(service_data).decode(),
            "data_bytes": service_data
        }
    except Exception as e:
        return {"error": str(e)}

def handle_registration(device_id, seq, ver, sec, data_bytes):
    print(f"Registering device: {device_id}")
    if len(data_bytes) < 6:
        return b"\x00"

    device_type = data_bytes[0]
    model_type = data_bytes[1]
    reg_code = data_bytes[2:6].hex().upper()
    print(f"Device Type: {device_type}, Model: {model_type}, Registration Code: {reg_code}")

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
    print(f"Login request from device: {device_id}")
    resp_cmd = 0x8001
    result = 0x00
    response_service = struct.pack('>B', result)
    header = struct.pack('>HHIHH16s', len(response_service)+28, resp_cmd, seq, ver, sec,
                         device_id.encode('ascii').ljust(16, b'\x00'))
    crc = crc16_ccitt(header + response_service)
    return b'\x55\xAA' + header + response_service + struct.pack('>H', crc)

def handle_platform_heartbeat(device_id, seq, ver, sec, data_bytes):
    print(f"Heartbeat from device {device_id}: {binascii.hexlify(data_bytes).decode()}")
    resp_cmd = 0x8003
    response_service = data_bytes
    header = struct.pack('>HHIHH16s', len(response_service)+28, resp_cmd, seq, ver, sec,
                         device_id.encode('ascii').ljust(16, b'\x00'))
    crc = crc16_ccitt(header + response_service)
    return b'\x55\xAA' + header + response_service + struct.pack('>H', crc)

def parse_tag_report(data_bytes):
    import datetime
    import struct
    import binascii

    tags = []
    offset = 0

    while offset + 4 <= len(data_bytes):
        try:
            tag_type, length = struct.unpack('>HH', data_bytes[offset:offset+4])
            offset += 4
            value = data_bytes[offset:offset+length]
            offset += length

            if tag_type == 0x8B01 and len(value) >= 17:
                tag_id_bytes = value[1:5]  # Extract 4-byte tag ID
                tag_id = tag_id_bytes.hex().upper()
                
                print("value  ====>",list(value))
                status_byte = value[9]  # Corrected index for status byte
                print("status_byte at value[9]===>",status_byte)
                sos_active = status_byte & 0x01  # Check bit 7 for alarm
            
                print("sos_active====>",sos_active)

                try:
                    year = 2000 + value[11]
                    month = value[12]
                    day = value[13]
                    hour = value[14]
                    minute = value[15]
                    second = value[16]

                    print(f"Raw tag value: {value.hex()} => Y:{year} M:{month} D:{day} {hour}:{minute}:{second}, SOS: {sos_active}")

                    if 1 <= month <= 12 and 1 <= day <= 31:
                        timestamp = datetime.datetime(
                            year=year, month=month, day=day,
                            hour=hour, minute=minute, second=second
                        )
                        tags.append({
                            "tag_id": tag_id,
                            "timestamp": timestamp.isoformat(),
                            "sos": sos_active
                        })
                    else:
                        tags.append({
                            "tag_id": tag_id,
                            "timestamp": f"Invalid date M:{month} D:{day}",
                            "sos": sos_active,
                            "raw": value.hex()
                        })
                except Exception as ve:
                    tags.append({
                        "tag_id": tag_id,
                        "timestamp": f"Exception decoding timestamp: {ve}",
                        "sos": sos_active,
                        "raw": value.hex()
                    })
            else:
                tags.append({"type": hex(tag_type), "raw": value.hex()})

        except Exception as e:
            tags.append({"error": str(e), "raw": binascii.hexlify(data_bytes[offset:]).decode()})
            break

    return tags

def client_handler(conn, addr):
    print(f"Connected by {addr}")
    with conn:
        try:
            while True:
                data = conn.recv(1024)
                if not data:
                    print("Client disconnected.")
                    break
                print("Raw Packet:====> in hex", data.hex())
                print("Raw Packet:====> in raw data", data)

                result = parse_packet(data)

                print("parse packit result ======>",result)
                cmd = result.get("command")

                if cmd == "0x8":
                    response = handle_registration(
                        result['device_id'], result['seq'], int(result['version'], 16), 0, result['data_bytes']
                    )
                    time.sleep(0.2)
                    conn.sendall(response)
                    print("ACK Sent:", response.hex())
                    print("Sent registration ACK")

                elif cmd == "0x1":
                    response = handle_login(
                        result['device_id'], result['seq'], int(result['version'], 16), 0
                    )
                    time.sleep(0.2)
                    conn.sendall(response)
                    print("Sent login ACK")

                elif cmd == "0x4":
                    tags = parse_tag_report(result['data_bytes'])
                    print(f"Tags from {result['device_id']}:", tags)

                elif cmd == "0x3":
                    response = handle_platform_heartbeat(
                        result['device_id'], result['seq'], int(result['version'], 16), 0, result['data_bytes']
                    )
                    conn.sendall(response)
                    print("Sent heartbeat ACK")

                else:
                    print("Unknown command packet:", result)

        except ConnectionResetError:
            print("Connection reset by peer.")
        except Exception as e:
            print(f"Unexpected error: {e}")

print(f"Listening for RFID data on {HOST}:{PORT}...")

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((HOST, PORT))
    server_socket.listen()

    while True:
        conn, addr = server_socket.accept()
        threading.Thread(target=client_handler, args=(conn, addr), daemon=True).start()


