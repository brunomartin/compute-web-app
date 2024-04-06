import os
import flask
import connexion

import numpy as np

from . import file_utils
from . import image_utils


# get and cache results
def get_results():
    return load_results()


def load_results():
    results = dict()

    for filename in os.listdir(get_result_path()):
        extension = '.h5'

        if len(filename) < len(extension) + 1:
            continue

        if not filename.endswith(extension):
            continue

        filepath = os.path.join(get_result_path(), filename)

        try:

            f = file_utils.open_file_for_reading(filepath, swmr=flask.current_app.use_h5_swmr)

            f.close()

            result_id = filename[:-3]

            result = {
                'id': result_id
            }

            results[result_id] = result

        except OSError:
            pass

    return results


def get_result_path():
    application_data_path = flask.current_app.app_data_path
    return os.path.join(application_data_path, 'result')


def search():
    return list(get_results().keys())


def load_result(result_id):

    result = dict()
    result['id'] = result_id

    result_path = get_result_path()
    filename = result_id + '.h5'
    filepath = os.path.join(result_path, filename)

    file = None

    swmr = flask.current_app.use_h5_swmr

    try:
        result = file_utils.load_result(filepath, file=file, swmr=swmr)
    except OSError as e:
        flask.current_app.logger.error('OSError: ' + str(e))
        pass
    except KeyError as e:
        flask.current_app.logger.error('KeyError: ' + str(e))
        pass

    result['id'] = result_id
    result['filename'] = filename

    return result


# Create a handler for get result
def get(result_id):
    """@
    This function responds to a request for /api/data
    by recording data uploaded

    """

    # check if file exists
    filename = result_id + '.h5'
    filepath = os.path.join(get_result_path(), filename)

    if not os.path.isfile(filepath):
        return "result_id not found", 404

    # load data file information
    result = load_result(result_id)

    return result


# Create a handler for get data file
def get_file(result_id):
    """@
    This function responds to a request for /api/data/{data_id}
    by recording data uploaded

    """

    # check if file exists
    filename = result_id + '.h5'
    filepath = os.path.join(get_result_path(), filename)

    if not os.path.isfile(filepath):
        return "data_id not found", 404

    # if file exists, read it and return its bytes
    with open(filepath, 'rb') as f:

        headers = {
            'Content-Disposition': 'attachment; filename="' + filename + '"',
            'Content-Length': os.fstat(f.fileno()).st_size
        }

        return f.read(), 200, headers

        # If we want to compress file, uncomment following lines
        # commpressed_bytes_io = gzip.compress(f.read())
        #
        # headers = {
        #     'Content-Disposition': 'attachment; filename="' + filename + '"',
        #     'Content-Encoding': 'gzip',
        #     'Content-Length': len(commpressed_bytes_io)
        # }
        #
        # return commpressed_bytes_io, 200, headers


# Create a handler for get result group
def group_get(result_id, group_id):
    """@
    This function responds to a request for /api/data
    by recording data uploaded

    """

    # construct result file path
    filepath = os.path.join(get_result_path(), result_id + '.h5')

    group_name = 'slice_' + '%03d' % group_id

    response = {}

    try:

        f = file_utils.open_file_for_reading(filepath, swmr=flask.current_app.use_h5_swmr)

        group = f[group_name]

        for attr in group.attrs:
            response[attr] = group.attrs[attr]

        datasets = []

        for dataset in group:
            datasets.append(dataset)

        response["datasets"] = datasets

        f.close()

    except OSError:
        response = {
            "error": "file " + filepath + " does not exists"
        }

        return response, 400

    return response


# Create a handler for our read (POST) upload
def dataset_get(result_id, group_id, dataset_id):
    """@
    This function responds to a request for /api/data
    by recording data uploaded

    """

    # construct result file path
    filepath = os.path.join(get_result_path(), result_id + '.h5')

    # check if file exists
    try:
        f = file_utils.open_file_for_reading(filepath, swmr=flask.current_app.use_h5_swmr)
    except OSError as e:
        return "File " + filepath + " does not exists", 400

    # check if group exists
    try:
        group_name = 'slice_' + '%03d' % group_id
        group = f[group_name]
    except KeyError:
        return "Group does not exists", 400

    # check if dataset exists
    try:
        dataset = group[dataset_id]
    except KeyError:
        return "Dataset does not exists", 400

    response = {
        "status": "ok",
        "shape": dataset.shape,
        "dtype": str(dataset.dtype)
    }

    f.close()

    return response

# curl http://localhost:5000/api/result/0/dataset/0/value


# Create a handler for our read (GET) download
def dataset_value_get(result_id, group_id, dataset_id):
    """@
    This function responds to a request for /result/{}/group/{}/dataset/{}/value
    by recording data uploaded

    """

    query_data_type = None
    # default data type is the one of the result file
    # if type is in query, then try to return this data type
    if 'type' in connexion.request.args:
        query_data_type_str = connexion.request.args['type']

        if query_data_type_str == 'int8':
            query_data_type = 'i1'
        elif query_data_type_str == 'uint8':
            query_data_type = 'u1'
        elif query_data_type_str == 'int16':
            query_data_type = 'i2'
        elif query_data_type_str == 'uint16':
            query_data_type = 'u2'
        else:
            return "Type not supported", 400

    # construct result file path
    filepath = os.path.join(get_result_path(), result_id + '.h5')

    # check if file exists
    try:
        f = file_utils.open_file_for_reading(filepath, swmr=flask.current_app.use_h5_swmr)
    except OSError:
        return "File " + filepath + " does not exists", 400

    # check if group exists
    try:
        group_name = 'slice_' + '%03d' % group_id
        group = f[group_name]
    except KeyError:
        return "Group does not exists", 400

    # check if dataset exists
    try:
        dataset = group[dataset_id]
    except KeyError:
        return "Dataset does not exists", 400

    # get dataset values as an array
    array = dataset[()]

    # if query data type is set, adapt
    if query_data_type is not None:

        data_type = array.dtype
        if data_type in [np.float32, np.float64]:
            if query_data_type in ['u1', 'u2']:
                # move value on uint8/uint16 full scale and
                # convert it to uint8/uint16 type
                if array.max() != array.min():
                    array -= array.min()

                    max_value = np.iinfo(query_data_type).max
                    array *= float(max_value)/array.max()

                array = array.astype(query_data_type)

        else:
            message = 'Converting from ' + data_type
            message += ' to ' + query_data_type
            message += ' not supported yet.'
            return message, 400

    # convert it to bytes
    response = array.tobytes()

    f.close()

    return response


def get_group_dataset_image(result_id, group_id, dataset_id, start=0, length=1):

    result_path = get_result_path()
    filepath = os.path.join(result_path, result_id + '.h5')

    swmr = flask.current_app.use_h5_swmr

    http_accept = None
    if connexion.request.headers.has_key('ACCEPT'):
        http_accept = connexion.request.headers['ACCEPT']

    image_type = "PNG"

    if http_accept == 'image/png':
        pass
    elif http_accept == 'image/jpeg':
        image_type = "JPEG"
    else:
        image_type = "PNG"
        # return 'Format not supported', 404

    try:
        array = file_utils.load_dataset(filepath, group_id, dataset_id, start, length, swmr=swmr)
    except Exception as e:
        return str(e), 404

    try:
        app_data_path = flask.current_app.app_data_path
        body = image_utils.array_to_image(array, image_type, app_data_path)
    except Exception as e:
        return str(e), 500

    return body