import pytest

import json
import os
import numpy as np
import h5py
import time
import datetime

from .common import TestData


class TestPostData(TestData):
    """Test /api/data/{data_id}/group/{group_id}/value post response."""

    def test_post(self):
        self.api_post_data()
        self.api_post_data_group_value(use_threads=False, group_step=1)
        self.check()

    def test_threaded_post(self):
        self.api_post_data()
        self.api_post_data_group_value(use_threads=True, group_step=1)
        self.check()

    def test_groups_post(self):
        self.api_post_data()
        self.api_post_data_group_value(use_threads=False, group_step=4)
        self.check()

    def test_groups_threaded_post(self):
        self.api_post_data()
        self.api_post_data_group_value(use_threads=True, group_step=4)
        self.check()

    def check(self, refresh=False):
        """Test /api/data/{data_id}/group/{group_id}/value post response."""

        # check data file created content and compare with what has been sent
        data_path = os.path.join(pytest.application_data_path, 'data')

        # construct result file path
        filepath = os.path.join(data_path, pytest.data_id + '.h5')

        # open file
        # file = h5py.File(filepath, 'r')
        file = self.open_file_for_reading(filepath, swmr=False)

        assert 'cwa_stored' in file.attrs
        assert 'cwa_upload_end_time' in file.attrs
        assert 'cwa_store_end_time' in file.attrs

        if not refresh:
            # wait until uploaded and stored by closing/opening file
            while not (file.attrs["cwa_uploaded"] and file.attrs["cwa_stored"]):
                # time.sleep(0.2)
                file.close()
                file = self.open_file_for_reading(filepath, swmr=False)

        for group_id in range(0, len(pytest.slice_locations)):

            group_array = pytest.data[:, :, :, group_id]

            group_name = 'slice_' + '%03d' % group_id

            assert group_name in file

            group = file[group_name]

            assert 'cwa_stored' in group.attrs

            if refresh:
                while not group.attrs['cwa_stored']:
                    time.sleep(0.2)
                    group.refresh()

            assert group.attrs['cwa_stored']

            # check magnitude dataset type and value
            assert 'magnitude' in group
            dataset = group['magnitude']
            assert dataset.dtype == 'uint16'

            if refresh:
                dataset.refresh()

            for i in range(0, len(pytest.echo_times)):
                image = dataset[:, :, i]
                expected_image = group_array[:, :, i]

                if (image != expected_image).any():
                    print("image mismatch !!!")

                assert (image == expected_image).all()

            # check magnitude dataset type and value
            assert 'phase' in group
            dataset = group['phase']
            assert dataset.dtype == 'uint16'

            if refresh:
                dataset.refresh()

            for i in range(0, len(pytest.echo_times)):
                image_index = len(pytest.echo_times) + i
                assert (dataset[:, :, i] == group_array[:, :, image_index]).all()

        if refresh:
            # wait until uploaded and stored by refreshing root group
            while not (file.attrs["cwa_uploaded"] and file.attrs["cwa_stored"]):
                time.sleep(0.2)
                file["/"].refresh()

        # get upload and store end time
        upload_end_time_str = file.attrs['cwa_upload_end_time'].decode()
        store_end_time_str = file.attrs['cwa_store_end_time'].decode()

        message = "{}".format(file.attrs)

        file.close()

        upload_end_time = None
        store_end_time = None

        try:
            upload_end_time = datetime.datetime.fromisoformat(upload_end_time_str)
            store_end_time = datetime.datetime.fromisoformat(store_end_time_str)
        except ValueError as e:
            pass

        # assert that upload_end_time is earlier than store_end_time
        assert upload_end_time <= store_end_time

