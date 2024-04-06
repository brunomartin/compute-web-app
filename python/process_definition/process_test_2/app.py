try:

    import time
    import sys
    import argparse
    import h5py
    import numpy as np

    # parse arguments
    parser = argparse.ArgumentParser()

    parser.add_argument("mean_factor", help="Mean factor to apply after mean operation", default=1.0, type=float)
    parser.add_argument("data_file", help="data file name", type=str)
    parser.add_argument("result_file", help="result file name", type=str)

    args = parser.parse_args()

    mean_factor = args.mean_factor
    data_filename = args.data_file
    result_filename = args.result_file

    # open data file for reading
    data_file = h5py.File(data_filename, 'r', libver='latest', swmr=True)

    # open result file for writing
    result_file = h5py.File(result_filename, 'w', libver='latest')

    # copy attributes
    for attr in data_file.attrs:
        result_file.attrs[attr] = data_file.attrs[attr]

    # get slice count
    slices = len(data_file) - 2
    process_slices = 0

    # first, create all groups and datasets to enable swmr mode

    for group_name in data_file:
        # only create groups for data file groups
        group = data_file[group_name]

        if not isinstance(group, h5py.Group):
            continue

        group = data_file[group_name]
        dtype = group['magnitude'].dtype
        shape = group['magnitude'].shape

        shape = (shape[0], shape[1])

        result_group = result_file.create_group(group_name)
        result_group.create_dataset("mean", shape=shape, dtype=dtype)
        result_group.create_dataset("std", shape=shape, dtype=dtype)
        result_group.create_dataset("min", shape=shape, dtype=dtype)
        result_group.create_dataset("max", shape=shape, dtype=dtype)

    result_file.swmr_mode = True

    # create average, std, min and max dataset for each group slice
    for group_name in data_file:
        group = data_file[group_name]

        if isinstance(group, h5py.Group):

            while not group.attrs['cwa_stored']:
                data_file.close()
                time.sleep(0.5)
                data_file = h5py.File(data_filename, 'r', libver='latest', swmr=True)
                group = data_file[group_name]

            magnitude = group["magnitude"]

            mean_value = (np.mean(magnitude, axis=2)*mean_factor).astype(magnitude.dtype)
            std_value = np.std(magnitude, axis=2).astype(magnitude.dtype)
            min_value = np.min(magnitude, axis=2)
            max_value = np.max(magnitude, axis=2)

            result_group = result_file[group_name]

            result_group["mean"][:] = mean_value
            result_group["std"][:] = std_value
            result_group["min"][:] = min_value
            result_group["max"][:] = max_value

            process_slices += 1

            # print progress message that will be catch by the scheduler
            progress = int(process_slices/slices*100)
            print("[PROGRESS {}%]".format(progress))

            # to be sure the process will output something, flush
            sys.stdout.flush()

    # close result file
    result_file.close()

    # close data file
    data_file.close()

    # exit with OK status code
    sys.exit(0)

except Exception as e:
    # print error message that will be catch by the scheduler
    print("[ERROR {}]".format(str(e)))

    # to be sure the process will output something, flush
    sys.stdout.flush()

    # exit with an error status code
    sys.exit(-1)

