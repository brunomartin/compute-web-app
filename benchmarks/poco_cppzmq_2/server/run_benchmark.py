import os
import re

pattern = '.*\[SERVER\] total duration: ([0-9]+)ms.*'

p = re.compile('.*\[SERVER\] total duration: ([0-9]+)ms.*')

test_count = 5

dataset_sizes_mb = [1, 2, 5, 10]
dataset_counts = [5, 10]
worker_counts = [-1, 1, 2, 4]

for dataset_size_mb in dataset_sizes_mb:
    for dataset_count in dataset_counts:

        no_worker_duration_ms = 0

        for worker_count in worker_counts:

            command = './server %d %d %d' % (worker_count, dataset_count, dataset_size_mb)

            durations_ms = []
            for i in range(test_count):
                stream = os.popen(command)
                output = stream.read()

                for line in output.splitlines():
                    result = p.match(line)
                    if result:
                        duration_ms = int(result.group(1))
                        durations_ms.append(duration_ms)
                        # print("duration_ms: %dms" % duration_ms)
                        break

            mean_duration_ms = 0.
            for duration_ms in durations_ms:
                mean_duration_ms += duration_ms
            mean_duration_ms /= len(durations_ms)

            if worker_count == -1 or no_worker_duration_ms == 0:
                no_worker_duration_ms = mean_duration_ms
                print("dataset_size: %.1fMB, datasets: %d, workers:%d, duration_ms: %dms"
                      % (dataset_size_mb, dataset_count, worker_count, mean_duration_ms))
            else:
                print("dataset_size: %.1fMB, datasets: %d, workers:%d, duration_ms: %dms (x%.1f)"
                      % (dataset_size_mb, dataset_count, worker_count, mean_duration_ms,
                         no_worker_duration_ms/mean_duration_ms))
