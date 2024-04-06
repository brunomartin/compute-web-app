import os
import h5py
import numpy as np
import time
import datetime
from pathlib import Path
import json


def lock_file(filename):
    lock_filename = filename + '.lock'

    lock_file_id = None

    # wait until lock file is deleted
    file_locked = False
    while not file_locked:
        try:
            # try create a lock file
            lock_file_id = os.open(lock_filename, os.O_CREAT | os.O_EXCL | os.O_RDWR)
            file_locked = True
        except OSError as e:
            time.sleep(0.2)

    return lock_file_id


def unlock_file(filename, lock_file_id):
    lock_filename = filename + '.lock'

    # remove lock file
    os.close(lock_file_id)

    if Path(lock_filename).exists():
        Path(lock_filename).unlink()


def safely_update_json_file(filename, update_method):

    # lock process file
    file = lock_file(filename)

    try:
        json_file = open(filename, 'r')
        json_data = json.load(json_file)
        json_file.close()
    except FileNotFoundError:
        unlock_file(filename, file)
        raise Exception(filename + ' not found')

    update_method(json_data)

    json_file = open(filename, 'w')
    json.dump(json_data, json_file, indent=4)
    json_file.close()

    # unlock process file
    unlock_file(filename, file)


def load_groups_from_h5_file(file, data):
    data['groups'] = 0
    data['datasets'] = list()

    for group_name in file:
        group = file[group_name]
        if not isinstance(group, h5py.Group):
            continue
        data['groups'] += 1

        # get info about dataset in the first group
        # all groups need to have the same dataset structures

        if data['groups'] > 1:
            continue

        for dataset_name in group:
            dataset = group[dataset_name]
            if not isinstance(dataset, h5py.Dataset):
                continue

            data_dataset = dict()

            data_dataset['name'] = dataset_name
            data_dataset['shape'] = dataset.shape
            data_dataset['dtype'] = str(dataset.dtype)

            data['datasets'].append(data_dataset)


def load_data_from_h5_file(file):

    data = dict()

    try:
        # compute web app information
        data['name'] = file.attrs['cwa_name'].decode()
        data['version'] = file.attrs['cwa_version'].decode()
        data['post_time'] = file.attrs['cwa_post_time'].decode()
        data['upload_end_time'] = file.attrs['cwa_upload_end_time'].decode()
        data['store_end_time'] = file.attrs['cwa_store_end_time'].decode()

        data['uploaded'] = bool(file.attrs['cwa_uploaded'])
        data['uploaded_bytes'] = int(file.attrs['cwa_uploaded_bytes'])
        data['upload_progress'] = int(file.attrs['cwa_upload_progress'])

        data['stored'] = bool(file.attrs['cwa_stored'])
        data['stored_bytes'] = int(file.attrs['cwa_stored_bytes'])
        data['store_progress'] = int(file.attrs['cwa_store_progress'])

        load_groups_from_h5_file(file, data)

    except Exception as e:
        print("load_data_from_h5_file Exception : {}".format(str(e)))
        pass

    return data


def load_result_from_h5_file(file):

    result = dict()

    try:

        load_groups_from_h5_file(file, result)

    except Exception as e:
        print("load_result_from_h5_file Exception : {}".format(str(e)))
        pass

    return result


def load_data(filepath, file=None, swmr=False):

    file_given = file

    if not file_given:
        file = open_file_for_reading(filepath, swmr=swmr)

    data = load_data_from_h5_file(file)

    if not file_given:
        file.close()

    # groups = data['groups']

    status = {
        'uploaded': data['uploaded'],
        'stored': data['stored'],
        'upload_progress': data['upload_progress'],
        'store_progress': data['store_progress']
    }

    data['status'] = status

    return data


def load_result(filepath, file=None, swmr=False):

    file_given = file

    if not file_given:
        file = open_file_for_reading(filepath, swmr=swmr)

    result = load_result_from_h5_file(file)

    if not file_given:
        file.close()

    return result


def load_dataset(filepath, group_id, dataset_id, start, length, swmr=False):

    file = open_file_for_reading(filepath, swmr=swmr)

    group_name = 'slice_' + '%03d' % group_id
    group = file[group_name]

    shape = group[dataset_id].shape

    if start is None:
        start = 0

    if length is None:
        length = shape[2] - start

    end = start + length

    if len(shape) > 2:
        dataset = np.copy(group[dataset_id][:, :, start:end])
    else:
        dataset = np.copy(group[dataset_id][:, :])

    file.close()

    return dataset


def create_file(filepath, name, version, post_time, slice_locations, echo_times, dtype, width, height, early_allocation=False, swmr=True, attributes=[]):

    libver = 'latest' if swmr else 'earliest'

    file = h5py.File(filepath, 'w', libver=libver)

    slices = len(slice_locations)
    echos = len(echo_times)

    # store compute web app information
    file.attrs.create("cwa_name", np.string_(name))
    file.attrs.create("cwa_version", np.string_(version))
    file.attrs.create("cwa_post_time", np.string_(post_time))

    file.attrs.create("cwa_uploaded", False)
    file.attrs.create("cwa_uploaded_bytes", 0)
    file.attrs.create("cwa_upload_progress", 0)
    file.attrs.create("cwa_upload_end_time", np.string_(""))

    file.attrs.create("cwa_stored", False)
    file.attrs.create("cwa_stored_bytes", 0)
    file.attrs.create("cwa_store_progress", 0)
    file.attrs.create("cwa_store_end_time", np.string_(""))

    # create echo times and slide location datasets
    file.create_dataset('slice_locations', data=slice_locations)
    file.create_dataset('echo_times', data=echo_times)

    for key in attributes:
        value = attributes[key]
        if isinstance(value, str):
            file.attrs.create(key, np.string_(value))
        else:
            file.attrs.create(key, value)

    # create groups and their attributes
    for i in range(0, slices):
        group_name = 'slice_' + '%03d' % i
        group = file.create_group(group_name)

        group.attrs.create("cwa_stored", False)
        group.attrs.create("cwa_stored_bytes", 0)

        group.attrs.create("cwa_uploaded", False)
        group.attrs.create("cwa_uploaded_bytes", 0)

    # create datasets in groups
    for i in range(0, slices):
        group_name = 'slice_' + '%03d' % i
        group = file[group_name]

        shape = (height, width, echos)

        if early_allocation:
            # with early allocation
            group.create_dataset(
                'magnitude',
                shape=shape,
                maxshape=shape,
                dtype=dtype,
                data=np.zeros(shape=shape, dtype=dtype)
            )

            group.create_dataset(
                'phase',
                shape=shape,
                maxshape=shape,
                dtype=dtype,
                data=np.zeros(shape=shape, dtype=dtype)
            )
        else:
            # without early allocation
            group.create_dataset(
                'magnitude',
                shape=shape,
                maxshape=shape,
                dtype=dtype
            )

            group.create_dataset(
                'phase',
                shape=shape,
                maxshape=shape,
                dtype=dtype
            )

    if swmr:
        # set SWMR mode here, after all elements created
        file.swmr_mode = True

    return file


def open_file_for_writing(filepath, swmr=True):

    file = None

    libver = 'latest' if swmr else 'earliest'

    file_open = False
    while not file_open:
        try:
            file = h5py.File(filepath, 'r+', libver=libver)
            file_open = True
            file.swmr_mode = True if swmr else False
        except ValueError as e:
            pass
        except OSError as e:
            pass

    return file


def open_file_for_reading(filepath, swmr=True):

    file = None

    libver = 'latest' if swmr else 'earliest'

    file_open = False
    while not file_open:
        try:
            file = h5py.File(filepath, 'r', libver=libver, swmr=swmr)
            file_open = True
        except ValueError as e:
            pass
        except OSError as e:
            pass

    return file


def write_body_to_file_group(body, file, group_id):

    group_name = 'slice_' + '%03d' % group_id

    if group_name not in file:
        raise Exception("group not found")

    group = file[group_name]

    # get type and shape from magnitude dataset
    dtype = group['magnitude'].dtype
    shape = group['magnitude'].shape

    echos = shape[2]

    # in version 1.0, receives vector of row major matrices.
    # vector size is echos * 2.
    # the first half is magnitudes, the second half is phases
    # each image size is image_width*image_height

    expected_body_size = shape[0]*shape[1]*shape[2]*dtype.itemsize*2
    body_size = len(body)

    # check if body has right size
    if body_size != expected_body_size:
        message = 'Body has not the right size, should be '\
                  + str(expected_body_size) + 'and is '\
                  + str(body_size)
        file.close()

        return message, 500

    body_part_size = int(len(body)/(2*echos))

    part_vector = np.zeros(dtype=dtype, shape=shape[0]*shape[1])
    part_array = np.zeros(dtype=dtype, shape=(shape[0], shape[1]))
    slice_array = np.zeros(dtype=dtype, shape=shape)

    # update magnitude from the first half
    for i in range(0, echos):
        part = body[i*body_part_size:(i+1)*body_part_size]
        part_vector = np.frombuffer(part, dtype=dtype, count=shape[0]*shape[1])
        part_array = np.reshape(part_vector, (shape[0], shape[1]))
        slice_array[:, :, i] = part_array

    group['magnitude'][:] = slice_array

    # update magnitude from the second half
    for i in range(0, echos):
        part = body[(echos+i)*body_part_size:(echos+i+1)*body_part_size]
        part_vector = np.frombuffer(part, dtype=dtype, count=shape[0]*shape[1])
        part_array = np.reshape(part_vector, (shape[0], shape[1]))
        slice_array[:, :, i] = part_array

    group['phase'][:] = slice_array

    group.attrs['cwa_stored'] = True
    group.attrs["cwa_stored_bytes"] = body_size

    if hasattr(group, 'flush'):
        group['magnitude'].flush()
        group['phase'].flush()
        group.flush()


def write_bodies_to_file(file, group_ids, bodies):

    for (group_id, body) in zip(group_ids, bodies):
        write_body_to_file_group(body, file, group_id)
