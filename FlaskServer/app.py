from flask import Flask, request
from datetime import datetime
import json

app = Flask(__name__)

LOG_FILE = f"sensor_log_{datetime.now().date()}.txt"

@app.route("/data", methods=["POST"])
def data():
    payload = request.json

    # add timestamp
    entry = {
        "time": datetime.now().isoformat(),
        "data": payload
    }

    line = f"{entry['time']} | TEMP={payload['temp']}C HUM={payload['humidity']}% RSSI={payload['rssi']} MAC={payload['mac']}\n"

    # print to console
    print(line)

    # append to file
    with open(LOG_FILE, "a") as f:
        f.write(line + "\n")

    return "OK"

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)