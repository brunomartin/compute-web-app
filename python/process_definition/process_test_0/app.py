try:
    import time
    import sys
    import argparse
    import h5py
    import shutil

    # parse arguments
    parser = argparse.ArgumentParser()

    parser.add_argument("step_count", help="process step count", default=100, type=int)
    parser.add_argument("step_process_time", help="step process time in seconds", default=0.05, type=float)
    parser.add_argument("end_message", help="message to display at the end", type=str)
    parser.add_argument("data_file", help="data file", type=str)
    parser.add_argument("result_file", help="result file", type=str)

    args = parser.parse_args()

    step_count = args.step_count
    step_process_time = args.step_process_time
    end_message = args.end_message
    data_file = args.data_file
    result_file = args.result_file

    data = h5py.File(data_file, 'r', libver='latest')

    result = h5py.File(result_file, 'w')

    for group_name in data:
        data_group = data[group_name]

        if isinstance(data_group, h5py.Group):
            result_group = result.create_group(group_name)
            dataset = result_group.create_dataset('test', shape=(2, 2), dtype='f')
            dataset[:] = [[0, 1], [2, 3]]

    for i in range(0, step_count):
        # sleep a while
        time.sleep(step_process_time)

        # print progress message that will be catch by the scheduler
        print("[PROGRESS {}%]".format(int((i+1)/step_count*100)))

        # to be sure the process will output something, flush
        sys.stdout.flush()

    result.close()
    data.close()

    print(end_message)

    # exit with OK status code
    sys.exit(0)

except Exception as e:
    # print error message that will be catch by the scheduler
    print("[ERROR {}]".format(str(e)))

    # to be sure the process will output something, flush
    sys.stdout.flush()

    # exit with an error status code
    sys.exit(-1)