import os
import hashlib

import flask
import connexion

import datetime

from . import file_utils
from . import image_utils

from .write_process import PrioritizedItem


# get and cache datas
def get_datas():
    return load_datas()


def load_data(data_id):

    data = dict()
    data['id'] = data_id

    data_path = get_data_path()
    filename = data_id + '.h5'
    filepath = os.path.join(data_path, filename)

    file = None

    swmr = flask.current_app.use_h5_swmr

    try:
        data = file_utils.load_data(filepath, file=file, swmr=swmr)
    except OSError as e:
        flask.current_app.logger.error('OSError: ' + str(e))
        pass
    except KeyError as e:
        flask.current_app.logger.error('KeyError: ' + str(e))
        pass

    data['id'] = data_id
    data['filename'] = filename

    return data


def load_datas():
    datas = {}
    for filename in os.listdir(get_data_path()):
        extension = '.h5'

        if len(filename) < len(extension) + 1:
            continue

        if not filename.endswith(extension):
            continue

        data_id = filename[0:-len(extension)]

        data = load_data(data_id)
        datas[data_id] = data

    return datas


def get_data_ids():
    data_ids = list()
    for filename in os.listdir(get_data_path()):
        extension = '.h5'

        if len(filename) < len(extension) + 1:
            continue

        if not filename.endswith(extension):
            continue

        data_id = filename[0:-len(extension)]

        data_ids.append(data_id)

    return data_ids


def get_data_path():
    application_data_path = flask.current_app.app_data_path
    return os.path.abspath(os.path.join(application_data_path, 'data'))


# list data ids, names of .h5 files found in data_path directory
def search():

    if 'details' in connexion.request.args:
        return list(get_datas().values())

    return get_data_ids()


# Create a handler for get data
def get(data_id):
    """@
    This function responds to a request for /api/data/{data_id}
    by recording data uploaded

    """

    # check if file exists
    filename = data_id + '.h5'
    filepath = os.path.join(get_data_path(), filename)

    if not os.path.isfile(filepath):
        return "data_id not found", 404

    # load data file information
    data = load_data(data_id)

    return data


# Create a handler for get data file
def get_file(data_id):
    """@
    This function responds to a request for /api/data/{data_id}
    by recording data uploaded

    """

    # check if file exists
    filename = data_id + '.h5'
    filepath = os.path.join(get_data_path(), filename)

    if not os.path.isfile(filepath):
        return "data_id not found", 404

    # if file exists, read it and return its bytes
    with open(filepath, 'rb') as f:

        headers = {
            'Content-Disposition': 'attachment; filename="' + filename + '"',
            'Content-Length': os.fstat(f.fileno()).st_size
        }

        return f.read(), 200, headers


# handler for post new data, value are sent after
def post(body):
    """@
    This function responds to a request for /api/data
    by recording data uploaded

    """

    # body is already a dict containing at least required parameters

    name = body['name']
    width = body['image_width']
    height = body['image_height']
    type_str = body['type']
    slice_locations = body['slice_locations']
    echo_times = body['echo_times']
    version = body['version']

    attributes = []
    if 'attributes' in body:
        attributes = body['attributes']

    h = hashlib.sha1(name.encode())
    data_id = h.hexdigest()
    data_id = data_id[0:16]

    # set data_id to body before adding it to datas
    body['id'] = data_id

    # add post time
    post_time = datetime.datetime.now().isoformat()

    if type_str == 'int8':
        dtype = 'i1'
    elif type_str == 'uint8':
        dtype = 'u1'
    elif type_str == 'int16':
        dtype = 'i2'
    elif type_str == 'uint16':
        dtype = 'u2'
    else:
        return "Type not supported", 400

    data_path = get_data_path()

    filepath = os.path.join(data_path, data_id + '.h5')

    # delete if already exists unless sync issues
    if os.path.isfile(filepath):
        os.remove(filepath)

    # the data file contains groups for each slide.
    # slice group names are slice_XXX where XXX is the number of the slice.
    # each slice contains tow datasets : magnitude and phase.

    # try to open the file as it can be already open by
    # another process

    # set early allocation flag, maybe very slow at
    # at creation on some system (rpi for instance)
    early_allocation = False

    item = {
        'data_path': data_path,
        'data_id': data_id,
        'name': name,
        'version': version,
        'post_time': post_time,
        'slice_locations': slice_locations,
        'echo_times': echo_times,
        'dtype': dtype,
        'width': width,
        'height': height,
        'early_allocation': early_allocation,
        'attributes': attributes
    }

    with flask.current_app.app_context():
        flask.current_app.write_queue.put(PrioritizedItem(1, item))
        flask.current_app.write_queue.join()

    response = {
        "status": "ok",
        "data_id": data_id
    }

    return response


class ExtractionError(Exception):
    """Base class for exceptions in this module."""

    def __init__(self, message):
        self.message = message


# extract integer list from string list of type 1,2,5-10,5
def extract_integer_list_from_string(list_string):

    integer_list = []

    groups = list_string.split(',')
    for group in groups:
        integer_range = group.split('-')

        # if it is a range of type 4-6
        if len(integer_range) == 2:
            if not integer_range[0].isdigit():
                raise ExtractionError("Bad integer list")

            if not integer_range[1].isdigit():
                raise ExtractionError("Bad integer list")

            integer_list.extend(
                range(int(integer_range[0]), int(integer_range[1])+1)
            )
        elif len(integer_range) == 1:
            if not integer_range[0].isdigit():
                raise ExtractionError("Bad integer list")

            integer_list.append(int(integer_range[0]))
        else:
            raise ExtractionError("Bad integer list")

    return integer_list


# handler for post value in new data
def post_value(data_id, group_ids, body):

    if data_id not in get_data_ids():
        flask.current_app.logger.error('data_id not in get_data_ids()')
        return "Data has not been posted", 400

    try:
        group_ids = extract_integer_list_from_string(group_ids)
    except ExtractionError as e:
        message = 'ExtractionError : {}'.format(e.message)
        return message, 500

    if len(body) % len(group_ids) != 0:
        message = 'Wrong body size'
        return message, 500

    body_part_size = int(len(body) / len(group_ids))

    data_path = get_data_path()

    # create item to put to queue
    body_item = {
        'data_path': data_path,
        'data_id': data_id,
        'group_ids': group_ids,
        'body': body,
    }

    # update group attributes

    attributes = list()

    for group_id in group_ids:
        group_name = 'slice_' + '%03d' % group_id

        uploaded_attr = {
            'group_id': group_name,
            'name': 'cwa_uploaded',
            'value': True
        }

        uploaded_bytes_attr = {
            'group_id': group_name,
            'name': 'cwa_uploaded_bytes',
            'value': body_part_size
        }

        attributes.extend([uploaded_attr, uploaded_bytes_attr])

    attributes_item = {
        'data_path': data_path,
        'data_id': data_id,
        'attributes': attributes
    }

    with flask.current_app.app_context():
        flask.current_app.write_queue.put(PrioritizedItem(1, attributes_item))

    # put body item to queue, need to get info on current queue for memory load
    with flask.current_app.app_context():
        # put first group in priority so that process that can will start earlier
        flask.current_app.write_queue.put(PrioritizedItem(10 + group_ids[0], body_item))

    response = {
        "status": "ok"
    }

    return response


# handler for post value in new data
def post_group_value(data_id, group_id, body):
    """@
    This function responds to a request for /api/data/{data_id}/group/{group_id}/value
    by recording data uploaded

    note about concurrency : h5py api is thread safe, no need to worry about
    lock in that method, good news, we can post in parallel

    """

    # group_id shall be an integer here, create a string list with
    # on element to post it via post_value
    group_str = str(group_id)

    return post_value(data_id, group_str, body)


def uploading_search():

    uploading_data_ids = list()

    for filename in os.listdir(get_data_path()):
        extension = '.json'

        if len(filename) < len(extension) + 1:
            continue

        if not filename.endswith(extension):
            continue

        data_id = filename[:-5]

        uploading_data_ids.append(data_id)

    return uploading_data_ids


def get_group_dataset_image(data_id, group_id, dataset_id, start=0, length=1):

    data_path = get_data_path()
    filepath = os.path.join(data_path, data_id + '.h5')

    swmr = flask.current_app.use_h5_swmr

    try:
        array = file_utils.load_dataset(filepath, group_id, dataset_id, start, length, swmr=swmr)
    except Exception as e:
        return str(e), 404

    http_accept = str()
    if connexion.request.headers.has_key('ACCEPT'):
        http_accept = connexion.request.headers['ACCEPT']

    http_accept = http_accept.split(',')

    image_type = "PNG"

    if 'image/png' in http_accept:
        pass
    elif 'image/jpeg' in http_accept:
        image_type = "JPEG"
    else:
        image_type = "PNG"
        # return 'Format not supported', 404

    try:
        app_data_path = flask.current_app.app_data_path
        body = image_utils.array_to_image(array, image_type, app_data_path)
    except Exception as e:
        return str(e), 500

    return body