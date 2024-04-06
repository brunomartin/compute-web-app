
import pytest

import unittest
import os
import platform
import tempfile
import shutil
import glob
from pathlib import Path
import time
import json
import numpy as np
import hashlib
import h5py
import sys

from threading import Thread

from cwa_app import CwaApp

local_test_dir = False
# local_test_dir = True


class TestBase(unittest.TestCase):

    """
        Base class for flask unit tests.
    """

    @classmethod
    def setUpClass(cls):
        pytest.application_data_path = tempfile.mkdtemp()

        use_zmq = True
        use_h5_swmr = True

        if local_test_dir:
            Path('test').mkdir(exist_ok=True)
            pytest.application_data_path = os.path.abspath('test')
            Path('test').mkdir(exist_ok=True)

        app = CwaApp(pytest.application_data_path, use_zmq=use_zmq, use_h5_swmr=use_h5_swmr)

        app.app.config['TESTING'] = True

        # create directory of they not exist
        Path(os.path.join(pytest.application_data_path, 'data')).mkdir(exist_ok=True)
        Path(os.path.join(pytest.application_data_path, 'process_definition')).mkdir(exist_ok=True)
        Path(os.path.join(pytest.application_data_path, 'result')).mkdir(exist_ok=True)

        with app.app.test_client() as client:
            with app.app.app_context():
                client.allow_subdomain_redirects = True
                cls.client = client

        cls.app = app

    @classmethod
    def tearDownClass(cls):
        del cls.client
        if not local_test_dir:
            shutil.rmtree(pytest.application_data_path)
        pass


class TestData(TestBase):

    """
        Base class for process unit tests.
    """

    @classmethod
    def setUpClass(cls):
        super().setUpClass()

    @classmethod
    def tearDownClass(cls):
        super().tearDownClass()
        pass

    def api_post_data(self):
        """Test /api/data/ post response."""

        client = self.client

        pytest.data_dims = {
            'height': 20,
            'width': 30,
            'echos': 10,
            'slices': 20
        }

        image_height = pytest.data_dims['height']
        image_width = pytest.data_dims['width']
        echo_times = np.arange(float(pytest.data_dims['echos']))
        slice_locations = np.arange(float(pytest.data_dims['slices']))

        body = {
            "echo_times": list(echo_times),
            "id": 0,
            "image_height": image_height,
            "image_width": image_width,
            "name": "test_data",
            "slice_locations": list(slice_locations),
            "type": "uint16",
            "version": "1.0"
        }

        response = client.post('/api/data', data=json.dumps(body), content_type='application/json')

        assert response.status_code == 200

        # compute expected data id
        h = hashlib.sha1(body["name"].encode())
        pytest.data_id = h.hexdigest()
        pytest.data_id = pytest.data_id[0:16]

        expected = {
          "data_id": pytest.data_id,
          "status": "ok"
        }

        result = json.loads(response.data)

        assert result == expected

        response = client.get('/api/data')

        assert response.status_code == 200

        result = json.loads(response.data)

        assert pytest.data_id in result

    def post_group_value(self, url, body):
        client = self.client

        response = None

        try:
            response = client.post(url, data=body, content_type='application/octet-stream', buffered=False)
        except Exception as e:
            pass

        expected = {
            "status": "ok"
        }

        result = json.loads(response.data)

        assert result == expected

    def api_post_data_group_value(self, use_threads=False, group_step=1, join_threads=True, sleep_after_post_s=0.):
        """Test /api/data/{data_id}/group/{group_id}/value post response."""

        data_id = pytest.data_id

        image_height = pytest.data_dims['height']
        image_width = pytest.data_dims['width']
        pytest.echo_times = np.arange(float(pytest.data_dims['echos']))
        pytest.slice_locations = np.arange(float(pytest.data_dims['slices']))

        assert len(pytest.slice_locations) % group_step == 0

        array_size = (
            image_height,
            image_width,
            len(pytest.echo_times)*2,
            len(pytest.slice_locations)
        )

        pytest.data = np.random.randint(0, high=1000, size=array_size, dtype='uint16')

        post_threads = []

        t = time.time()

        # post data from array
        for group_start in range(0, len(pytest.slice_locations), group_step):
            # build array image by image
            body = b''

            for group in range(group_start, group_start + group_step):
                group_array = pytest.data[:, :, :, group]

                for i in range(0, len(pytest.echo_times)*2):
                    body += group_array[:, :, i].tobytes()

            url = '/api/data/' + data_id + '/group/' + str(group_start) + '/value'

            # if group_step > 1, adapt url for posting multiple groups
            if group_step > 1:
                url = '/api/data/' + data_id + '/group/value?group_ids='
                url += str(group_start) + '-' + str(group_start+group_step-1)

            if use_threads:
                post_thread = Thread(target=self.post_group_value, args=(url, body))
                post_thread.start()

                post_threads.append(post_thread)
            else:
                self.post_group_value(url, body)

            time.sleep(sleep_after_post_s)

        if use_threads and join_threads:
            for post_thread in post_threads:
                post_thread.join()

        elapsed = time.time() - t

        # print('Elapsed time : {} seconds'.format(elapsed))

    def wait_until_uploaded(self):
        data_id = pytest.data_id

        data_stored = False

        while not data_stored:
            response = self.client.get('/api/data/{}'.format(pytest.data_id))

            result = json.loads(response.data)
            assert 'status' in result
            assert 'stored' in result['status']
            data_stored = result['status']['stored']

    def open_file_for_reading(self, filepath, swmr=True):

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


class TestProcessBase(TestData):

    """
        Base class for process unit tests.
    """

    @classmethod
    def setUpClass(cls):
        super().setUpClass()

        source_process_definition_path = os.path.abspath(
            os.path.join(os.path.dirname(__file__), '..', 'process_definition')
        )

        process_definition_path = os.path.join(pytest.application_data_path, 'process_definition')

        file_types = ('*.json', '*/*.py', 'requirements.txt')  # the tuple of file types
        file_paths = []
        for file_type in file_types:
            search_path = os.path.join(source_process_definition_path, file_type)
            search_path = os.path.relpath(search_path)
            file_paths.extend(glob.glob(search_path, recursive=True))

        for file_path in file_paths:
            dir_name = os.path.dirname(file_path)
            dst_path = os.path.join(pytest.application_data_path, dir_name)
            os.makedirs(dst_path, exist_ok=True)
            shutil.copy(file_path, dst_path)

        cwd = os.getcwd()

        # create virtual environment for test processes
        # and install requirement for processes
        os.chdir(process_definition_path)

        if platform.system() in ['Darwin', 'Linux']:
            assert os.system('python3 -m venv .venv') == 0
            assert os.system('.venv/bin/python -m pip install -r requirements.txt') == 0
        elif platform.system() == 'Windows':
            assert os.system('python.exe -m venv .venv') == 0
            assert os.system('.venv\\Scripts\\python.exe -m pip install -r requirements.txt') == 0

        os.chdir(cwd)

        print('process_definition_path : {}'.format(process_definition_path))

    @classmethod
    def tearDownClass(cls):

        process_definition_path = os.path.join(pytest.application_data_path, 'process_definition')

        if not local_test_dir:
            for filename in os.listdir(process_definition_path):
                file_path = os.path.join(process_definition_path, filename)
                try:
                    if os.path.isfile(file_path) or os.path.islink(file_path):
                        os.unlink(file_path)
                    elif os.path.isdir(file_path):
                        shutil.rmtree(file_path)
                except Exception as e:
                    print('Failed to delete %s. Reason: %s' % (file_path, e))

        super().tearDownClass()
        pass
