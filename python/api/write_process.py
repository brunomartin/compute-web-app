"""A Socket subclass that adds some serialization methods."""

import zlib
import sys
import os
import h5py
import numpy as np
import datetime
import json
import platform

import zmq
from multiprocessing import Process

from . import file_utils, data as data_api

import signal

from threading import Thread
from queue import PriorityQueue

from dataclasses import dataclass, field
from typing import Any


@dataclass(order=True)
class PrioritizedItem:
    priority: int
    item: Any = field(compare=False)


# using compression can be CPU wasting, 3x slower on MacBookPro
use_compression = False


class WriteProcess:
    def __init__(self, port=5555):
        self.__write_process = None
        self.__port = port
        self.__address = 'tcp://127.0.0.1:'+str(port)
        self.__stop = False
        self.__server_socket = None
        self.__client_socket = None
        self.__files = dict()
        self.__write_queue = None
        self.__use_h5_swmr = False

        self.__zmq_copy = True

    @property
    def port(self):
        return self.__port

    def start(self, use_h5_swmr=False):
        self.__use_h5_swmr = use_h5_swmr

        self.__write_process = Process(target=self.target, daemon=True)
        self.__write_process.start()

    def signal_handler(self, signal_code, frame):
        print("WriteProcess killed")
        self.__stop = True
        sys.exit(0)

    def target(self):

        context = zmq.Context()

        try:
            server_socket = context.socket(zmq.REP)
            server_socket.bind(self.__address)
            print("WriteProcess started.")
        except zmq.ZMQError as e:
            print("WriteProcess already started by another worker.")
            return

        if platform.system() == 'Windows':
            signal.signal(signal.SIGINT, self.signal_handler)
        else:
            signal.signal(signal.SIGINT | signal.SIGQUIT, self.signal_handler)

        self.__write_queue = PriorityQueue()

        # turn-on the worker threads
        write_worker_thread = Thread(target=self.write_worker, args=(self.__write_queue, self.__files, False, self.__use_h5_swmr), daemon=True)
        write_worker_thread.start()

        while not self.__stop:
            try:
                item = server_socket.recv_json()
                zbody = server_socket.recv(copy=self.__zmq_copy)

                if 'body' in item:
                    if use_compression:
                        item['body'] = zlib.decompress(zbody)
                    else:
                        item['body'] = zbody

                priority = item['priority']

                self.__write_queue.put(PrioritizedItem(priority, item))

                # wait in case of file creation
                if 'post_time' in item:
                    self.__write_queue.join()

                server_socket.send_json({
                    'status': 'ok',
                    'message': ''
                })

            except zmq.Again as e:
                pass

        print('WriteProcess has been gracefully stopped')

    def send(self, queue_item):
        if not self.__client_socket:
            context = zmq.Context()
            self.__client_socket = context.socket(zmq.REQ)
            self.__client_socket.connect(self.__address)

        try:
            zbody = b''
            if 'body' in queue_item.item:
                body = queue_item.item['body']
                queue_item.item['body'] = len(body)

                if use_compression:
                    zbody = zlib.compress(body, level=1)
                else:
                    zbody = body

            priority = queue_item.priority
            queue_item.item['priority'] = priority

            self.__client_socket.send_json(queue_item.item, zmq.SNDMORE)
            self.__client_socket.send(zbody, copy=self.__zmq_copy)

            data = self.__client_socket.recv_json()

            if 'status' not in data:
                raise Exception("status not in data")

            if data['status'] != 'ok':
                raise Exception("Error: " + data['message'])

        except Exception as e:
            print(str(e))
            raise

    def write_worker(self, write_queue, open_files=None, use_zmq=False, use_h5_swmr=False):

        keep_files_open = isinstance(open_files, dict) and self.__use_h5_swmr

        while True:
            try:
                queue_item = write_queue.get()

                # if using zmq, forward queue_item and continue
                if use_zmq:
                    self.send(queue_item)
                    write_queue.task_done()
                    continue

                item = queue_item.item

                # if it is file creation, do it and continue
                if 'post_time' in item:
                    data_path = item['data_path']
                    data_id = item['data_id']
                    name = item['name']
                    version = item['version']
                    post_time = item['post_time']
                    slice_locations = item['slice_locations']
                    echo_times = item['echo_times']
                    dtype = item['dtype']
                    width = item['width']
                    height = item['height']
                    early_allocation = item['early_allocation']
                    attributes = item['attributes']

                    filepath = os.path.join(data_path, data_id + '.h5')

                    file = file_utils.create_file(
                        filepath, name, version, post_time,
                        slice_locations, echo_times, dtype, width, height, early_allocation,
                        swmr=use_h5_swmr, attributes=attributes)

                    json_filepath = os.path.join(data_path, data_id + '.json')
                    lock_file = file_utils.lock_file(json_filepath)

                    # for now, we don't need to dynamically store any data
                    # we keep it in case we need it
                    json_file = open(json_filepath, 'w')
                    json.dump({}, json_file)
                    json_file.close()

                    file_utils.unlock_file(json_filepath, lock_file)

                    if keep_files_open:
                        open_files[filepath] = file
                    else:
                        file.close()

                    write_queue.task_done()

                    continue

                # else continue with body or attributes

                data_path = item['data_path']
                data_id = item['data_id']

                filepath = os.path.join(data_path, data_id + '.h5')

                if keep_files_open:
                    # open file for the first time and keep it open
                    if filepath not in open_files:
                        file = file_utils.open_file_for_writing(filepath, swmr=use_h5_swmr)
                        open_files[filepath] = file
                    else:
                        file = open_files[filepath]
                else:
                    file = file_utils.open_file_for_writing(filepath, swmr=use_h5_swmr)

                # if it is a image part
                if 'body' in item:
                    self.__write_body_item_to_file(file, item)

                    if file.attrs['cwa_stored']:

                        json_filepath = os.path.join(data_path, data_id + '.json')
                        lock_file_id = file_utils.lock_file(json_filepath)

                        # remove file for dynamic content
                        os.remove(json_filepath)

                        file_utils.unlock_file(json_filepath, lock_file_id)

                # if it is attributes to write
                if 'attributes' in item:
                    self.__write_attributes_item_to_file(file, item)

                # if group or attributes cannot be flushed, flush the whole file
                if not hasattr(file['/'], 'flush'):
                    file.flush()

                if keep_files_open:
                    # if file has been stored, close it and remove it from the file list
                    if file.attrs['cwa_stored'] and file.attrs["cwa_uploaded"]:
                        del open_files[filepath]
                        file.close()
                else:
                    file.close()

                write_queue.task_done()

            except Exception as e:
                print(str(e))
                continue

    def __write_body_item_to_file(self, file, item):

        data_path = item['data_path']
        data_id = item['data_id']
        group_ids = item['group_ids']
        body = item['body']

        if isinstance(body, zmq.sugar.frame.Frame):
            body = body.buffer

        # build body list to send to queue in item
        body_part_size = int(len(body) / len(group_ids))
        bodies = []
        for i in range(0, len(group_ids)):
            body_part = body[i * body_part_size:(i + 1) * body_part_size]
            bodies.append(body_part)

        file_utils.write_bodies_to_file(file, group_ids, bodies)

        # store info come from data
        # mark data as stored at this point
        stored = True
        stored_bytes = 0
        store_progress = 0
        group_count = 0
        for name in file:
            if isinstance(file[name], h5py.Group):
                group_count += 1

                group_stored = bool(file[name].attrs['cwa_stored'])
                store_progress += 1 if group_stored else 0

                stored &= group_stored
                stored_bytes += int(file[name].attrs['cwa_stored_bytes'])

        store_progress = int(store_progress / group_count * 100)

        file.attrs['cwa_stored_bytes'] = stored_bytes
        file.attrs['cwa_store_progress'] = store_progress

        if stored:
            file.attrs["cwa_store_end_time"] = np.string_(datetime.datetime.now().isoformat())
            file.attrs['cwa_stored'] = True

    def __write_attributes_item_to_file(self, file, item):

        attributes = item['attributes']

        for attribute in attributes:
            name = attribute['name']
            value = attribute['value']
            group_id = attribute['group_id']

            if isinstance(value, str):
                value = np.string_(value)

            file[group_id].attrs[name] = value

        # upload info come from attributes
        # mark data as uploaded at this point
        uploaded = True
        uploaded_bytes = 0
        upload_progress = 0
        group_count = 0
        for name in file:
            if isinstance(file[name], h5py.Group):
                group_count += 1

                group_upload = bool(file[name].attrs['cwa_uploaded'])
                upload_progress += 1 if group_upload else 0

                uploaded &= group_upload
                uploaded_bytes += int(file[name].attrs['cwa_uploaded_bytes'])

        upload_progress = int(upload_progress / group_count * 100)

        file.attrs['cwa_uploaded_bytes'] = uploaded_bytes
        file.attrs['cwa_upload_progress'] = upload_progress

        if uploaded and not file.attrs['cwa_uploaded']:
            file.attrs["cwa_upload_end_time"] = np.string_(datetime.datetime.now().isoformat())
            file.attrs['cwa_uploaded'] = True

        if hasattr(file['/'], 'flush'):
            file['/'].flush()