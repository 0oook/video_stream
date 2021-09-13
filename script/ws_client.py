#!/usr/bin/python3

import json
from threading import Thread
from time import sleep, time
import websocket
import random


def test_connect():
    for i in range(5):
        ws = websocket.WebSocket()
        try:
            # ws.connect("ws://192.168.215.5:9002")
            ws.connect("ws://127.0.0.1:9002")
            adds = ["rtsp://admin:admin12345@192.168.111.6:554",
                    "rtsp://admin:admin@192.168.111.233:555/webcam",
                    "rtsp://admin:admin12345@192.168.111.233:554",
                    "rtsp://admin:admin12345@192.168.111.230:555/h264/ch1/main/av_stream",
                    "rtsp://admin:admin12345@192.168.111.230:555/h264/ch2/main/av_stream",
                    ]
            msg = {"c":
                {
                    "address": adds[random.randint(0, len(adds)-1)],
                    "cam_id": random.randint(0, 10)
                }
            }
            ws.send(json.dumps(msg))
            ss = time()
            while time() - ss < 30:
                data = ws.recv()
                print("Receive a msg! msg length: ", len(data))
        except Exception as e:
            print("Error: ", e)
        except KeyboardInterrupt:
            ws.close()
            return
        finally:
            ws.close()
            print("Thread terminaled!")


if __name__ == '__main__':
    qq = 1
    while True:
        for i in range(10):
            T = Thread(target=test_connect, args=())
            T.start()
        sleep(40)
        print("Compelete a test, test num: ", qq)
        qq = qq + 1
