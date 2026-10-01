import base64
import subprocess
import time
from google import genai
import socket
import wave
import requests


serverTTS = subprocess.Popen(
    ["python", "openai-edge-tts/app/server.py"],
    stdout=subprocess.DEVNULL,
    stderr=subprocess.DEVNULL
)
time.sleep(1)

clientGoogle = genai.Client()               ########
server = socket.socket()
server.bind(("0.0.0.0", 1234))
server.listen(1)

conn, addr = server.accept()
print("Connected by", addr)
TIME_LISTENING = 5  #uguale al massimo tempo di registrazione del mic: oltre i 5s si blocca il buffer TCP
# TARGET_READ = TIME_LISTENING * 16000 * 2
audio = bytearray()

elapsed = 0.0
start = 0.0
startReceived = False

while True:
    data = conn.recv(4096)
    if not data:
        break
    if not startReceived:
        start = time.time()
        startReceived = True
    audio.extend(data)
    print("Ricevuti", len(data), "byte")
    elapsed = time.time()
    if elapsed - start >= TIME_LISTENING:
        break


with wave.open("sound.wav", "wb") as out_f:
    out_f.setnchannels(1)
    out_f.setsampwidth(2) # number of bytes
    out_f.setframerate(16000)
    out_f.writeframes(audio)



# with open("sound.wav", 'rb') as f:
#     audio_bytes = f.read()
# interaction = clientGoogle.interactions.create(
#     model="gemini-3.8-flash",
#     input=[{"type": "audio",
#             "data": base64.b64encode(audio_bytes).decode('utf-8'),
#             "mime_type": "audio/wav"
#             }
#     ]
# )
# print(interaction.output_text)
#
# if interaction.output_text:
#     url = 'http://localhost:5050/v1/audio/speech'
#     headers = {'Content-Type': 'application/json', 'Authorization': 'Bearer your_api_key_here'}
#     data = {"input": interaction.output_text,
#             "voice": "it-IT-DiegoNeural",
#             "response_format": "mp3",
#             "speed": 1.1
#             }
#
#     r = requests.post(url, json = data, headers = headers)
#     with open("output.mp3", "wb") as f:
#         f.write(r.content)

start = time.time()
elapsed = 0.0
w = 20.0
while elapsed - start< w:
    elapsed = time.time()
    print(elapsed - start)

ffmpeg = subprocess.Popen(
    [
        "ffmpeg",
        "-i", "output.mp3",
        "-f", "s32le",
        "-ac", "1",
        "-ar", "16000",
        "-"
    ],
    stdout=subprocess.PIPE
)

total = 0

while True:
    chunk = ffmpeg.stdout.read(2048)

    if not chunk:
        print("Fine dello stream")
        break

    total += len(chunk)

    print("Invio:", len(chunk), "byte - totale:", total)

    conn.sendall(chunk)

conn.close()
server.close()
ffmpeg.wait()

print("Totale inviato:", total)