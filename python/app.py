import argparse
from distutils import util as distutils

import os
import sys
import platform
import multiprocessing

# for window production server
from waitress import serve


# If we're running in stand alone mode, run the application
if __name__ == '__main__':
    multiprocessing.freeze_support()

    app_data_path = os.path.relpath('.')

    if os.getenv('CWA_APP_DATA_PATH'):
        app_data_path = os.path.abspath(os.environ['CWA_APP_DATA_PATH'])

    use_zmq = True
    use_h5_swmr = True

    debug = True
    if getattr(sys, 'frozen', False):
        # we are running in a bundle
        debug = False

        # on windows for interaction with Matlab
        # h5 SWMR feature shall be disabled by default
        if platform.system() == 'Windows':
            use_h5_swmr = False

    # parse command line arguments, if any, they be erased config value
    parser = argparse.ArgumentParser()

    parser.add_argument("--app-data-path", default=app_data_path,
                        help='Set application data path (default: %(default)s)')

    parser.add_argument("--use-zmq", default=str(use_zmq),
                        help='Use ZMQ to store data (default: %(default)s)')

    parser.add_argument("--use-h5-swmr", default=str(use_h5_swmr),
                        help='Use HDF5 SWMR feature to store data (default: %(default)s)')

    args = parser.parse_args()

    use_zmq = distutils.strtobool(args.use_zmq)
    use_h5_swmr = distutils.strtobool(args.use_h5_swmr)

    if hasattr(args, 'app_data_path'):
        app_data_path = args.app_data_path

    app_data_path = os.path.abspath(app_data_path)

    from cwa_app import CwaApp
    app = CwaApp(
        app_data_path=app_data_path,
        use_zmq=use_zmq,
        use_h5_swmr=use_h5_swmr
    )

    if debug:
        app.run(debug=debug)
    else:
        serve(app, host='0.0.0.0', port=5000, threads=10)
