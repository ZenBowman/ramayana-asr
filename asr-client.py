import sys
import zmq

#  Socket to talk to server
context = zmq.Context()
socket = context.socket(zmq.SUB)
socket.setsockopt(zmq.SUBSCRIBE, '')

socket.connect("tcp://localhost:5556")

#socket.connect("ipc://asr.ipc")

# Process 5 updates
total_temp = 0
for update_nbr in range(5):
    string = socket.recv()
    print string
