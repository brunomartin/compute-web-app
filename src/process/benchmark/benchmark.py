import argparse
import subprocess
import os
import json

test_count = 10
dataset_heights = [256, 512, 1024, 2048]
datasets = 20
algorithm = 0
sup_type = 'socket'
agg_type = 'socket'

parser = argparse.ArgumentParser()

parser.add_argument("--tests", default=test_count, type=int,
                    help='Number of test to run for each'
                    ' configuration (default: %(default)s)')

parser.add_argument("--dataset-heights", default=dataset_heights,
                    nargs="+", type=int,
                    help='Dataset heights (default: %(default)s)')

parser.add_argument("--datasets", default=datasets, type=int,
                    help='Number of datasets (default: %(default)s)')

parser.add_argument("--algorithm", default=algorithm, type=int,
                    help='Alogrithm index to run (default: %(default)s)')

parser.add_argument("--sup-type", default=sup_type, type=str,
                    choices=['file', 'socket'],
                    help='Supplier type (default: %(default)s)')

parser.add_argument("--agg-type", default=agg_type, type=str,
                    choices=['file', 'socket'],
                    help='Aggregator type (default: %(default)s)')

args = parser.parse_args()

test_count = args.tests
dataset_heights = args.dataset_heights
datasets = args.datasets
algorithm = args.algorithm
sup_type = args.sup_type
agg_type = args.agg_type

cpu_count = os.cpu_count()

# if datasets is lower than cpu count, set it to cpu count
if datasets < cpu_count:
  datasets = cpu_count

print('cpu_count: {}'.format(cpu_count))
print('test_count : {}'.format(test_count))
print('dataset_heights : {}'.format(dataset_heights))
print('datasets : {}'.format(datasets))
print('algorithm : {}'.format(algorithm))
print('sup_type : {}'.format(sup_type))
print('agg_type : {}'.format(agg_type))

def clear_fs_cache():
  # Clear FS cache
  # print('Clearing FS cache...')
  args = ['vmtouch', '-eq', 'input/*']
  result = subprocess.run(args, stdout=subprocess.PIPE)

def generate_datasets(dataset_height):
  # Create test files
  # print('Generating {} datasets heights...'.format(dataset_height))
  args = [
    './CwaProcessBenchmarkMain', '--generate',
    '--dataset-height', str(dataset_height),
    '--datasets', str(datasets)
  ]
  result = subprocess.run(args, stdout=subprocess.PIPE)

durations_ms = {}

for dataset_height in dataset_heights:
  # Create datasets
  generate_datasets(dataset_height)

  durations_ms[dataset_height] = {}

  for workers in range(1, cpu_count+1):

    duration_ms = 0
    for test in range(test_count):
      # Clear FS cache
      clear_fs_cache()

      # Run benchmark
      args = [
        './CwaProcessBenchmarkMain',
        '--cwa-sup-type', sup_type,
        '--cwa-agg-type', agg_type,
        '--datasets', str(datasets),
        '--workers', str(workers),
        '--algorithm', str(algorithm),
        '-q'
      ]

      result = subprocess.run(args, stdout=subprocess.PIPE)

      # result is duration in ms, get result and print it
      duration_ms += float(result.stdout)

    duration_ms /= test_count
    durations_ms[dataset_height][workers] = duration_ms

    dataset_size_kb = dataset_height*dataset_height*2/1024

    factor = 1
    if workers > 1:
      factor = durations_ms[dataset_height][1]/durations_ms[dataset_height][workers]

    print('{}-{}, {:.0f}KB, {} workers, duration_ms: {:.2f}, x{:.1f}'.format(sup_type, agg_type, dataset_size_kb, workers, duration_ms, factor))

# Store in json file
json_string = json.dumps(durations_ms, sort_keys=True, indent=4)
with open('results.json', 'w') as outfile:
    outfile.write(json_string)

print('Done.')