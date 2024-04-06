import sys
import zmq
import numpy as np
import struct


def process(data_in):

    array_in = np.frombuffer(data_in, dtype=np.uint16)
    array_out = np.copy(array_in)

    for j in range(10):
        array_out = np.uint16(array_out * 1.5)
        array_out = np.uint16(array_out / 2.0)

    return array_out.tobytes()


server_address = "tcp://127.0.0.1"
port = 5555

if len(sys.argv) > 1:
    server_address = sys.argv[1];

# create context and connect to server
context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect("%s:%s" % (server_address, port))

# send server address seen from worker to server
socket.send_string(server_address)

# receive worker id and end points from server
worker_id = struct.unpack('i', socket.recv())
end_point_pull = socket.recv_string()
end_point_push = socket.recv_string()

# create pull and push socket for data
socket_pull = context.socket(zmq.PULL)
socket_push = context.socket(zmq.PUSH)

# connect them to end points
socket_pull.connect(end_point_pull)
socket_push.connect(end_point_push)

data_in = b""
data_out = b""

print("[WORKER %d] Started" % worker_id)

while True:

    # interpret as int32 to get -1 in case
    dataset = struct.unpack('i', socket_pull.recv())[0]

    # Check if there is still dataset to process, i.e., dataset not -1,
    # break if not
    if dataset == -1:
        print("[WORKER %d] Received dataset -1 from server" % worker_id)
        socket_push.send(struct.pack('i', dataset))
        break

    length = struct.unpack('I', socket_pull.recv())

    data_in = socket_pull.recv()

    data_out = process(data_in)

    socket_push.send(struct.pack('I', dataset))
    socket_push.send(struct.pack('I', len(data_out)))
    socket_push.send(data_out)
