import os
import json
import random
import copy
import time
import datetime

from pathlib import Path

import flask

from . import file_utils
from .process_definition import get as get_process_definition
from .result import get_result_path

process_actions = {}
cache_processes = False
running_process_filename = 'running_process_ids.json'


# get and cache processes
def get_processes():

    if cache_processes:
        with flask.current_app.app_context():

            if 'processes' not in flask.g:
                flask.g.processes = load_processes()

            return g.datas
    else:
        return load_processes()


def write_json_file_with_lock(json_filename, data):

    lock_file = file_utils.lock_file(json_filename)

    # update locked file
    json_file = open(json_filename, 'w')
    json.dump(data, json_file)
    json_file.close()

    file_utils.unlock_file(json_filename, lock_file)


def read_json_file_with_lock(json_filename, default):

    if not os.path.isfile(json_filename):
        return default

    lock_file = file_utils.lock_file(json_filename)

    with open(json_filename, 'r') as json_file:
        # try to read in case of file is being updated
        json_ok = False
        while not json_ok:
            try:
                json_content = json.load(json_file)
                json_ok = True
            except json.decoder.JSONDecodeError as e:
                time.sleep(0.2)

        json_file.close()

    file_utils.unlock_file(json_filename, lock_file)

    return json_content


def load_processes():
    processes = dict()

    for filename in os.listdir(get_result_path()):
        extension = '.json'

        if len(filename) < len(extension) + 1:
            continue

        if not filename.endswith(extension):
            continue

        if filename == running_process_filename:
            continue

        process_json_file = os.path.join(get_result_path(), filename)

        process = read_json_file_with_lock(process_json_file, default=dict())

        process_id = filename[:-5]
        processes[process_id] = process

    return processes


# filter process element from inner fields
def filter_process(process, with_log_content=False):

    # expose all except private
    result = {}
    for key in process:
        if key == "private":
            pass
        elif key == "log" and with_log_content:
            # read log file
            log_filename = process["private"]["log_filename"]
            log_file = open(log_filename, "r")
            result["status"]["log"] = log_file.read()
            log_file.close()
        else:
            result[key] = copy.deepcopy(process[key])

    # add log if asked
    if with_log_content:
        # read log file
        log_filename = process["private"]["log_filename"]
        try:
            log_file = open(log_filename, "r")
            result["status"]["log"] = log_file.read()
            log_file.close()
        except OSError:
            print("Error while reading ", log_filename)
            result["status"]["log"] = ""
    else:
        result["status"]["log"] = ""

    # remove file names from parameters
    if 'data_file' in result['parameters']:
        result['parameters'].pop('data_file')

    if 'result_file' in result['parameters']:
        result['parameters'].pop('result_file')

    return result


def search():

    results = []

    # remove subprocess element from result
    for process_id in get_processes().keys():
        process = get_processes()[process_id]
        process['id'] = process_id

        process = filter_process(process)

        results.append(process)

    return results


def running_search():
    result_path = get_result_path()
    running_process_filepath = os.path.join(result_path, running_process_filename)
    running_process_ids = read_json_file_with_lock(running_process_filepath, default=list())
    return running_process_ids


# to test a process post with curl, run the following command:
# curl -H "Content-Type: application/json" --request POST --data '{"version":"1.0","process_definition_id":"process_test_0","data_id":"5f8fd383aeb9c925", "parameters":{"step_count": 200, "step_process_time": 0.02, "end_message":"Yahooo"}}' http://localhost:5000/api/process


def post(body):

    version = body["version"]
    process_definition_id = body["process_definition_id"]
    data_id = body["data_id"]

    if "parameters" not in body or not isinstance(body["parameters"], dict):
        body["parameters"] = {}

    parameters = body["parameters"]

    # generate a 16 hex random string id for this process
    process_id = str(hex(random.getrandbits(64)))[2:]

    # get parameters from process definition
    process_definition = get_process_definition(process_definition_id)
    process_parameters = process_definition["parameters"]

    # iterate over process parameters and check if assigned,
    # assign default one of any, return error if required
    missing_parameters = []
    for key in process_parameters:
        if key not in parameters:
            missing_parameters.append(key)

    if len(missing_parameters) > 0:
        return "Missing parameters : " + ', '.join(missing_parameters), 500

    process = body

    # add id to process
    process['id'] = process_id

    # add post time
    process['post_time'] = datetime.datetime.now().isoformat()

    # add status to process
    process["status"] = {
        "running": False,
        "done": False,
        "progress": 0,
        "errors": []
    }

    # add a private section for internal elements
    process["private"] = {}

    # create a log file into which the process will write
    log_filename = os.path.join(get_result_path(), process_id + ".log")

    process["private"]["log_filename"] = log_filename

    Path(log_filename).touch()

    get_processes()[process_id] = process

    process_json_file = os.path.join(get_result_path(), process_id + ".json")
    with open(process_json_file, 'w') as fp:
        json.dump(body, fp)

    response = {
        "status": "ok",
        "process_id": process_id
    }

    return response


def get(process_id):

    if process_id not in get_processes():
        return "process_id not found", 404

    process = get_processes()[process_id]
    response = filter_process(process, with_log_content=True)

    return response
