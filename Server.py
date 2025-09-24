from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import csv
import os
from datetime import datetime

print("Current directory:", os.getcwd())

# === DATA FILE ===
CSV_FILE = "data_log.csv"
data_store = []

# Create CSV file with header if it doesn't exist
if not os.path.exists(CSV_FILE):
    with open(CSV_FILE, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["timestamp", "temperature", "humidity", "soil"])  # ✅ Add soil to header

# Load existing data
with open(CSV_FILE, "r") as f:
    reader = csv.DictReader(f)
    for row in reader:
        try:
            data_store.append({
                "timestamp": row["timestamp"],
                "temperature": float(row["temperature"]),
                "humidity": float(row["humidity"]),
                "soil": float(row.get("soil", 0))  # ✅ Default to 0 if missing
            })
        except:
            pass  # skip any malformed rows

# Latest sensor values
latest_data = {"temperature": 0, "humidity": 0, "soil": 0}  # ✅ Include soil

# === HTTP HANDLER ===
class RequestHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        content_len = int(self.headers.get('Content-Length', 0))
        post_body = self.rfile.read(content_len)

        try:
            payload = json.loads(post_body.decode())

            # ✅ Ensure soil is in payload, default to 0 if not
            temperature = payload["temperature"]
            humidity = payload["humidity"]
            soil = payload.get("soil", 0)

            timestamp = datetime.now().isoformat()

            # Save the latest values
            global latest_data
            latest_data = {"temperature": temperature, "humidity": humidity, "soil": soil}

            # Save to memory
            entry = {"timestamp": timestamp, "temperature": temperature, "humidity": humidity, "soil": soil}
            data_store.append(entry)

            # Append to CSV
            with open(CSV_FILE, "a", newline="") as f:
                writer = csv.writer(f)
                writer.writerow([timestamp, temperature, humidity, soil])  # ✅ Write soil to CSV

            self.send_response(200)
            self.end_headers()
            self.wfile.write(b"Data received")

        except Exception as e:
            print("Error:", e)
            self.send_response(400)
            self.end_headers()

    def do_GET(self):
        if self.path == "/":
            self.send_response(200)
            self.send_header("Content-Type", "text/html")
            self.end_headers()
            with open(r"C:\Users\calvin\Documents\Projects\Planty\Website server\dashboard.html", "rb") as file:
                self.wfile.write(file.read())

        elif self.path == "/data":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(latest_data).encode())

        elif self.path == "/download":
            self.send_response(200)
            self.send_header("Content-Type", "text/csv")
            self.send_header("Content-Disposition", "attachment; filename=data_log.csv")
            self.end_headers()
            with open(CSV_FILE, "rb") as f:
                self.wfile.write(f.read())

        else:
            self.send_response(404)
            self.end_headers()

# === START SERVER ===
server = HTTPServer(('192.168.0.215', 8080), RequestHandler)
print("Server running on http://192.168.0.215:8080")
server.serve_forever()
