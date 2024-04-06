import pytest

import platform
import json
import time
import os

import numpy as np
import h5py

import flask

from .common import TestProcessBase


class TestProcess(TestProcessBase):

    def test_get_process_definitions(self):
        """Test /api/process_definition and /api/process_definition/{process_definition_id} get
        with copying one from source process tests
        """

        client = self.client

        pytest.process_definition_id = "process_test_1"

        response = client.get('/api/process_definition')

        assert response.status_code == 200

        process_definitions = json.loads(response.data)

        assert isinstance(process_definitions, list)
        assert len(process_definitions) > 0

        process_definition_ids = []
        for process_definition in process_definitions:
            process_definition_ids.append(process_definition['id'])

        assert pytest.process_definition_id in process_definition_ids

    def test_post_and_run_process(self):
        """Test /api/process post and /api/process_action
        """

        client = self.client

        # post data
        self.api_post_data()
        self.api_post_data_group_value()
        self.wait_until_uploaded()

        pytest.process_definition_id = 'process_test_1'

        response = client.get('/api/process')
        assert response.status_code == 200
        result = json.loads(response.data)

        assert isinstance(result, list)
        process_count = len(result)

        body = {
            "data_id": pytest.data_id,
            "process_definition_id": pytest.process_definition_id,
            "version": "1.0"
        }

        # this process needs mean_factor parameter to be set, it will fail with a 500 error code
        response = client.post('/api/process', data=json.dumps(body), content_type='application/json')

        assert response.status_code == 500

        # add the missing parameter and try again
        body['parameters'] = {
            "mean_factor": 2.0
        }

        response = client.post('/api/process', data=json.dumps(body), content_type='application/json')

        assert response.status_code == 200

        result = json.loads(response.data)

        assert 'status' in result
        assert result['status'] == 'ok'

        pytest.process_id = result['process_id']

        result_path = os.path.join(pytest.application_data_path, 'result');

        assert os.path.isfile(os.path.join(result_path, pytest.process_id + '.json'))

        response = client.get('/api/process')
        assert response.status_code == 200
        result = json.loads(response.data)

        assert isinstance(result, list)
        assert len(result) == process_count + 1

        process = next((x for x in result if x['id'] == pytest.process_id), None)

        assert 'post_time' in process
        del process['post_time']

        if 'start_time' in process:
            del process['start_time']

        if 'end_time' in process:
            del process['end_time']

        expected_process = {
            'data_id': pytest.data_id,
            'id': pytest.process_id,
            'parameters': {
                'mean_factor': 2.0,
            },
            'process_definition_id': pytest.process_definition_id,
            'status': {
                'done': False,
                'errors': [],
                'log': '',
                'progress': 0,
                'running': False
            },
            'version': '1.0'
        }

        assert process == expected_process

        response = client.get('/api/result')
        assert response.status_code == 200
        result = json.loads(response.data)

        assert isinstance(result, list)
        result_count = len(result)

        # check process is just created
        response = client.get('/api/process/' + pytest.process_id)
        assert response.status_code == 200
        result = json.loads(response.data)
        assert 'status' in result

        expected_status = {
            "running": False,
            "done": False,
            "progress": 0,
            "errors": [],
            "log": ""
        }

        status = result["status"]
        assert expected_status == status

        # start current process
        body = {
          "name": "start",
          "process_id": pytest.process_id,
          "version": "1.0"
        }

        response = client.post('/api/process_action', data=json.dumps(body), content_type='application/json')
        assert response.status_code == 200

        result = {}

        # wait until process started
        process_running = False

        start_time = time.time()

        while not process_running:
            response = client.get('/api/process/' + pytest.process_id)
            assert response.status_code == 200
            result = json.loads(response.data)
            assert 'status' in result
            assert 'running' in result['status']
            assert 'errors' in result['status']

            assert isinstance(result['status']['errors'], list)

            if len(result['status']['errors']) > 0:
                for error in result['status']['errors']:
                    print(error)

            errors = result['status']['errors']
            if len(errors) > 0:
                pass

            assert len(errors) == 0

            process_running = result['status']['running']

            # check if timeout
            if time.time() - start_time > 10:
                assert 0

        # wait until process done or not running anymore

        process_done = False

        start_time = time.time()

        while process_running and not process_done:
            response = client.get('/api/process/' + pytest.process_id)
            assert response.status_code == 200
            result = json.loads(response.data)
            assert 'status' in result
            assert 'running' in result['status']

            process_done = result['status']['done']
            process_running = result['status']['running']

            if process_done:
                break

            if not process_running:
                break

            # check if timeout
            if time.time() - start_time > 20:
                assert 0

            if len(result['status']['errors']) > 0:
                for error in result['status']['errors']:
                    print(error)

            assert len(result['status']['errors']) == 0

            time.sleep(0.5)

        # don't check log as it will perhaps change
        # and it is not a matter
        result["status"]["log"] = ""

        expected_status = {
            "running": False,
            "done": True,
            "progress": 100,
            "errors": [],
            "log": ""
        }

        status = result["status"]
        assert expected_status == status

        response = client.get('/api/result')
        assert response.status_code == 200
        result = json.loads(response.data)

        assert isinstance(result, list)
        assert len(result) == result_count + 1
        assert pytest.process_id in result

    def test_process_0(self):

        client = self.client

        self.api_post_data()

        self.api_post_data_group_value()
        self.wait_until_uploaded()

        pytest.process_definition_id = 'process_test_0'

        # add a new process from process_test_0 definition
        body = {
            "data_id": pytest.data_id,
            "process_definition_id": pytest.process_definition_id,
            "version": "1.0",
            "parameters": {
                "step_count": 10,
                "step_process_time": 0.05,
                "end_message": "Yes !!",
            }
        }

        response = client.post('/api/process', data=json.dumps(body), content_type='application/json')
        assert response.status_code == 200

        result = json.loads(response.data)
        assert result['status'] == 'ok'

        pytest.process_id = result['process_id']

        # start current process
        body = {
            "name": "start",
            "process_id": pytest.process_id,
            "version": "1.0"
        }

        # start process, it will wait until file group is stored to start
        response = client.post('/api/process_action', data=json.dumps(body), content_type='application/json')
        assert response.status_code == 200

        # wait until it starts
        process_running = False
        while not process_running:
            response = client.get('/api/process/' + pytest.process_id)
            result = json.loads(response.data)
            process_running = result['status']['running']

        # wait until it ends
        process_running = True
        while process_running:
            response = client.get('/api/process/' + pytest.process_id)
            result = json.loads(response.data)
            process_running = result['status']['running']

        response = client.get('/api/process/' + pytest.process_id)
        result = json.loads(response.data)
        assert result['status']['running'] is False

        if len(result['status']['errors']) > 0:
            for error in result['status']['errors']:
                print(error)

        assert len(result['status']['errors']) == 0
        assert result['status']['progress'] == 100
        assert result['status']['done']

    def test_process_1(self):

        client = self.client

        # post data
        self.api_post_data()
        self.api_post_data_group_value()
        self.wait_until_uploaded()

        pytest.process_definition_id = 'process_test_1'

        self.mean_factor = 2.0

        # add a new process from process_test_1 definition
        body = {
            "data_id": pytest.data_id,
            "process_definition_id": pytest.process_definition_id,
            "version": "1.0",
            "parameters": {
                'mean_factor': self.mean_factor,
            }
        }

        response = client.post('/api/process', data=json.dumps(body), content_type='application/json')
        assert response.status_code == 200

        result = json.loads(response.data)
        assert result['status'] == 'ok'

        pytest.process_id = result['process_id']

        # start current process
        body = {
            "name": "start",
            "process_id": pytest.process_id,
            "version": "1.0"
        }

        # start process, it will wait until file group is stored to start
        response = client.post('/api/process_action', data=json.dumps(body), content_type='application/json')
        assert response.status_code == 200

        # wait until it starts
        process_running = False
        while not process_running:
            response = client.get('/api/process/' + pytest.process_id)
            result = json.loads(response.data)
            process_running = result['status']['running']

        # wait until it ends
        process_running = True
        while process_running:
            response = client.get('/api/process/' + pytest.process_id)
            result = json.loads(response.data)
            process_running = result['status']['running']

        response = client.get('/api/process/' + pytest.process_id)
        result = json.loads(response.data)
        assert result['status']['running'] is False

        if len(result['status']['errors']) > 0:
            for error in result['status']['errors']:
                print(error)

        assert len(result['status']['errors']) == 0
        assert result['status']['progress'] == 100
        assert result['status']['done']

        self.check_process_1_result_file()
        self.check_process_1_result_api()

    def check_process_1_result_file(self):
        """Test /api/process_action post start when all is ok
        """

        # check data file created content and compare with what has been sent
        result_path = os.path.join(pytest.application_data_path, 'result')

        # open result file
        result_filepath = os.path.join(result_path, pytest.process_id + '.h5')

        file = h5py.File(result_filepath, 'r')

        slices = pytest.data_dims['slices']

        # check result file as one group for each slice from 000 to 0XX
        # where XX is the total number of slices
        for group in range(1, slices):
            group_name = 'slice_' + '%03d' % group
            assert group_name in file

        # get test data sent previously
        data = pytest.data

        mean_factor = self.mean_factor

        for group in range(0, slices):
            group_name = 'slice_' + '%03d' % group

            # check dataset are present
            assert 'mean' in file[group_name]
            assert 'std' in file[group_name]
            assert 'min' in file[group_name]
            assert 'max' in file[group_name]

            result_mean = file[group_name]['mean'][:]
            result_std = file[group_name]['std'][:]
            result_min = file[group_name]['min'][:]
            result_max = file[group_name]['max'][:]

            echos = int(data.shape[2] / 2)

            expected_mean = (np.mean(data[:, :, 0:echos, group], axis=2) * mean_factor) \
                .astype(data.dtype)
            expected_std = np.std(data[:, :, 0:echos, group], axis=2).astype(data.dtype)
            expected_min = np.min(data[:, :, 0:echos, group], axis=2)
            expected_max = np.max(data[:, :, 0:echos, group], axis=2)

            assert (result_mean == expected_mean).all()
            assert (result_std == expected_std).all()
            assert (result_min == expected_min).all()
            assert (result_max == expected_max).all()

        file.close()

    def check_process_1_result_api(self):
        """Test /api/process_action post start when all is ok
        """

        client = self.client

        slices = pytest.data_dims['slices']
        image_height = pytest.data_dims['height']
        image_width = pytest.data_dims['width']

        # get result general info
        response = client.get('/api/result/' + pytest.process_id)
        result = json.loads(response.data)

        assert 'groups' in result

        api_groups = result['groups']

        assert api_groups == slices

        # get test data sent previously
        data = pytest.data

        mean_factor = self.mean_factor

        for group in range(0, slices):
            group_name = 'slice_' + '%03d' % group

            group_url = '/api/result/' + pytest.process_id + '/group/' + str(group)
            response = client.get(group_url)
            result = json.loads(response.data)

            assert 'datasets' in result

            datasets = result['datasets']

            # check dataset are present
            assert 'mean' in datasets
            assert 'std' in datasets
            assert 'min' in datasets
            assert 'max' in datasets

            echos = int(data.shape[2] / 2)

            expected = dict()
            expected['mean'] = (np.mean(data[:, :, 0:echos, group], axis=2) * mean_factor) \
                .astype(data.dtype)
            expected['std'] = np.std(data[:, :, 0:echos, group], axis=2).astype(data.dtype)
            expected['min'] = np.min(data[:, :, 0:echos, group], axis=2)
            expected['max'] = np.max(data[:, :, 0:echos, group], axis=2)

            for dataset in datasets:
                dataset_url = group_url + '/dataset/' + dataset
                response = client.get(dataset_url)
                result = json.loads(response.data)

                assert 'shape' in result
                assert result['shape'] == [image_height, image_width]

                assert 'dtype' in result
                assert result['dtype'] == expected['mean'].dtype

                dtype = result['dtype']

                assert 'status' in result
                assert result['status'] == 'ok'

                value_url = dataset_url + '/value'
                response = client.get(value_url)

                body = response.data

                vector = np.frombuffer(body, dtype=dtype, count=image_height * image_width)
                result = np.reshape(vector, (image_height, image_width))

                assert (result == expected[dataset]).all()

    def test_process_2(self):

        if not self.app.app.use_h5_swmr:
            pytest.skip("This test needs ZMQ")

        client = self.client

        self.api_post_data()

        pytest.process_definition_id = 'process_test_2'

        self.mean_factor = 2.0

        # add a new process from process_test_2 definition
        body = {
            "data_id": pytest.data_id,
            "process_definition_id": pytest.process_definition_id,
            "version": "1.0",
            "parameters": {
                "mean_factor": self.mean_factor
            }
        }

        response = client.post('/api/process', data=json.dumps(body), content_type='application/json')
        assert response.status_code == 200

        result = json.loads(response.data)
        assert result['status'] == 'ok'

        pytest.process_id = result['process_id']

        # start current process
        body = {
            "name": "start",
            "process_id": pytest.process_id,
            "version": "1.0"
        }

        # start process, it will wait until file group is stored to start
        response = client.post('/api/process_action', data=json.dumps(body), content_type='application/json')
        assert response.status_code == 200

        # wait until it starts
        process_running = False
        while not process_running:
            response = client.get('/api/process/' + pytest.process_id)
            result = json.loads(response.data)
            process_running = result['status']['running']

        # start sending data with an artificial sleep after each send
        self.api_post_data_group_value(use_threads=True, join_threads=False, sleep_after_post_s=0.5)

        # wait until it ends
        process_running = True
        while process_running:
            response = client.get('/api/process/' + pytest.process_id)
            result = json.loads(response.data)
            process_running = result['status']['running']

        response = client.get('/api/process/' + pytest.process_id)
        result = json.loads(response.data)
        assert result['status']['running'] is False

        if len(result['status']['errors']) > 0:
            for error in result['status']['errors']:
                print(error)

        assert len(result['status']['errors']) == 0

        assert result['status']['progress'] == 100
        assert result['status']['done']

        self.check_process_1_result_file()
        self.check_process_1_result_api()
