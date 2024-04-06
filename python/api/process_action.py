import os
import random
import subprocess
import psutil
import re
import platform
import sys
from pathlib import Path
import datetime
import dateutil.parser
import shutil

from threading import Thread, get_ident
import threading

from .process_definition import get as get_process_definition
from .process import write_json_file_with_lock,\
    running_process_filename, get_processes

from .process_definition import get_process_definition_path
from .data import get_data_path
from .result import get_result_path

from .file_utils import safely_update_json_file

process_actions = list()


# search handler to list all process action triggered
def search():

    return process_actions


# to test a process action with curl, run the following command:
# curl -H "Content-Type: application/json" --request POST --data '{"version":"1.0","process_id":"3c6aa0280028951b","name":"start"}' http://localhost:5000/api/process_action
# curl -H "Content-Type: application/json" --request POST --data '{"version":"1.0","process_id":"3c6aa0280028951b","name":"suspend"}' http://localhost:5000/api/process_action
# curl -H "Content-Type: application/json" --request POST --data '{"version":"1.0","process_id":"3c6aa0280028951b","name":"resume"}' http://localhost:5000/api/process_action
# curl -H "Content-Type: application/json" --request POST --data '{"version":"1.0","process_id":"3c6aa0280028951b","name":"stop"}' http://localhost:5000/api/process_action
# curl -H "Content-Type: application/json" --request POST --data '{"version":"1.0","process_id":"3c6aa0280028951b","name":"reset"}' http://localhost:5000/api/process_action

# post a new action to a specific process already posted
def post(body):

    version = body['version']
    process_id = body['process_id']
    name = body['name']

    if process_id not in get_processes():
        return 'process_id not found', 400

    if name not in ['start', 'suspend', 'resume', 'stop', 'reset']:
        return 'invalid action name', 400

    process = get_processes()[process_id]

    result_path = get_result_path()
    process_filename = os.path.join(result_path, process_id + '.json')
    running_process_filepath = os.path.join(result_path, running_process_filename)

    if name == 'start':

        process_definition_id = process['process_definition_id']
        process_definition = get_process_definition(process_definition_id)

        # extract arguments from process definition
        if platform.system() == 'Windows':
            args = process_definition['win_args']
        else:
            args = process_definition['args']

        parameters = process['parameters']

        # add mandatory parameters
        data_id = process['data_id']
        parameters['data_file'] = os.path.join(get_data_path(), data_id + '.h5')

        process_id = process['id']
        parameters['result_file'] = os.path.join(get_result_path(), process_id + '.h5')

        # look if any arg name of process definition is in parameters
        # list of process, replace it with parameter value
        for i in range(0, len(args)):
            if args[i] in parameters:
                args[i] = str(parameters[args[i]])

        # set current working directory to current process definition directory
        cwd = os.path.join(get_process_definition_path(), process_definition_id)

        # create a thread for running process
        process_args = process, cwd, result_path, args
        process_thread = Thread(target=run_process, args=process_args)
        process_thread.start()

    elif name == 'suspend':
        if 'pid' in process['private']:
            process_pid = process['private']['pid']
            ps_process = psutil.Process(pid=process_pid)
            ps_process.suspend()

            update_process_status(process_filename, {
                'suspended': True
            })

            # remove this process id from running processes
            remove_running_process(running_process_filepath, process_id)

    elif name == 'resume':
        if 'pid' in process['private']:
            process_pid = process['private']['pid']
            ps_process = psutil.Process(pid=process_pid)
            ps_process.resume()

            update_process_status(process_filename, {
                'suspended': False
            })

            # add this process id to running processes
            add_running_process(running_process_filepath, process_id)

    elif name == 'stop':
        if 'pid' in process['private']:
            process_pid = process['private']['pid']
            ps_process = psutil.Process(pid=process_pid)
            ps_process.kill()

            thread_id = process['private']['tid']

            update_process_status(process_filename, {
                'done': False,
                'running': False,
            })

    elif name == 'reset':

        try:
            # make trash directory
            trash_path = os.path.join(result_path, 'trash')
            Path(trash_path).mkdir(exist_ok=True)

            start_time_str = process['start_time']
            start_time = dateutil.parser.parse(start_time_str)
            if not start_time:
                start_time = datetime.datetime.now()

            start_time_suffix = start_time.strftime("-%Y%m%d_%H%M%S")

            # copy json file to it
            shutil.copyfile(
                os.path.join(result_path, process_id + '.json'),
                os.path.join(trash_path, process_id + start_time_suffix + '.json'),
            )

            # move log file to it
            os.rename(
                os.path.join(result_path, process_id + '.log'),
                os.path.join(trash_path, process_id + start_time_suffix + '.log'),
            )

            # move h5 file to it
            os.rename(
                os.path.join(result_path, process_id + '.h5'),
                os.path.join(trash_path, process_id + start_time_suffix + '.h5'),
            )
        except OSError:
            print("Error while resetting process : ", process_id)

        update_process_status(process_filename, {
            'running': False,
            'done': False,
            'progress': 0,
            'errors': []
        })

    # generate a 16 hex random string id for this process action
    process_action_id = str(hex(random.getrandbits(64)))[2:]

    body['id'] = process_action_id

    process_actions.append(body)

    response = {
        'status': 'ok',
        'process_action_id': process_action_id
    }

    return response


def parse_log(status, line):

    # try to find a "[PROGRESS XX%]" in log
    # check progress, store result
    matches = re.findall(r'.*\[PROGRESS\s([0-9]+)[%]\].*', line)

    # get the last one of the line,
    if len(matches) > 0:
        progress = int(matches[-1])
        status['progress'] = progress

    # check errors, store all errors found in the line
    matches = re.findall(r'.*\[ERROR\s*(.*)\].*', line)

    for match in matches:
        status['errors'].append(match)


def update_process_status(json_filename, status):

    def update_method(json_data: dict):
        json_data['status'].update(status)

    safely_update_json_file(json_filename, update_method)


def update_process(json_filename, process):

    def update_method(json_data: dict):
        json_data.update(process)

    safely_update_json_file(json_filename, update_method)


def add_running_process(json_filename, process_id):

    def update_method(json_data: list):
        if process_id not in json_data:
            json_data.append(process_id)

    safely_update_json_file(json_filename, update_method)


def remove_running_process(json_filename, process_id):

    def update_method(json_data: list):
        if process_id in json_data:
            json_data.remove(process_id)

    safely_update_json_file(json_filename, update_method)


def run_process(process, cwd, result_path, args):

    process_id = process['id']
    process_json_file = os.path.join(result_path, process_id + '.json')

    process['status'] = {
        'running': True,
        'done': False,
        'progress': 0,
        'errors': []
    }

    process['start_time'] = datetime.datetime.now().isoformat()
    process['end_time'] = None

    process['private']['tid'] = threading.currentThread().native_id

    update_process(process_json_file, process)

    # add this process id to running processes
    running_process_filepath = os.path.join(result_path, running_process_filename)
    add_running_process(running_process_filepath, process_id)

    log_filename = process['private']['log_filename']
    log_file = open(log_filename, 'w+')

    return_code = -1

    print('Starting process with cwd: {}\n and args: {}'.format(cwd, ' '.join(args)))

    try:
        inner_process = subprocess.Popen(
            args=args, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, cwd=cwd,
            text=True, executable=os.path.join(cwd, args[0])
        )

        process['private']['pid'] = inner_process.pid
        update_process(process_json_file, process)

        process_finished = False
        while not process_finished:
            line = inner_process.stdout.readline()
            return_code = inner_process.poll()
            if line == '' and return_code is not None:
                process_finished = True
                continue
            if line:
                parse_log(process['status'], line)

                # update log file
                log_file.write(line)
                log_file.flush()

                update_process_status(process_json_file, process['status'])

    except FileNotFoundError as e:
        message = 'FileNotFoundError : {}'.format(e)
        print(message)
        return_code = -1
        process['status']['errors'].append(message)

    errors = process['status']['errors']

    process['end_time'] = datetime.datetime.now().isoformat()

    # if bad return code, add it to errors and at this end of log file
    if return_code != 0:

        print('return_code : {}'.format(return_code))
        message = 'Bad return code : {}'.format(return_code)

        errors.append(message)

        line = '[ERROR ' + message + ']\n'
        log_file.write(line)
        log_file.flush()

    log_file.close()

    # try to convert to v18 h5 removing swmr tags from file so that
    # it can be read by Matlab, for instance
    try:
        result_file = process['parameters']['result_file']
        subprocess.call('h5format_convert ' + result_file, shell=True)
    except OSError as e:
        print("h5format_convert execution failed:", e, file=sys.stderr)

    done = len(errors) == 0

    # update process without changing progress
    process['status'].update({
        'running': False,
        'done': done,
        'errors': errors
    })

    update_process(process_json_file, process)

    # remove this process id from running processes
    remove_running_process(running_process_filepath, process_id)
