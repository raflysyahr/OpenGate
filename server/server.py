import http.server
import ssl
import os

# Ganti dengan IP lokal komputer kamu (cek dengan ipconfig/ifconfig), biar ESP32 bisa akses
HOST = '0.0.0.0'  # Dengar di semua interface
PORT = 8070       # Port bebas >1024, hindari 443 kalau tidak root

# Path ke cert dan key
CERT_FILE = 'server.crt'
KEY_FILE = 'server.key'

# Folder yang disajikan (current directory)
directory = os.getcwd()

httpd = http.server.HTTPServer((HOST, PORT), http.server.SimpleHTTPRequestHandler)

# Setup SSL context (modern way)
context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
context.load_cert_chain(certfile=CERT_FILE, keyfile=KEY_FILE)

httpd.socket = context.wrap_socket(httpd.socket, server_side=True)

print(f"Server HTTPS berjalan di https://{HOST}:{PORT}")
print(f"Akses dari browser/ESP: https://192.168.1.2:{PORT}/ota-portal.bin")
httpd.serve_forever()
