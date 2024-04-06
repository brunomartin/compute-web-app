import connexion
from flask import Flask, render_template
from flask_cors import CORS
import sys
import os
import logging
import glob
from pathlib import Path
from threading import Thread
from queue import PriorityQueue

from api.write_process import WriteProcess

from api import process as process_api


def create_app():
    app_data_path = os.path.abspath('.')

    if os.getenv('CWA_APP_DATA_PATH'):
        app_data_path = os.path.abspath(os.environ['CWA_APP_DATA_PATH'])

    use_zmq = True
    use_h5_swmr = True

    app = CwaApp(
        app_data_path=app_data_path,
        use_zmq=use_zmq,
        use_h5_swmr=use_h5_swmr
    )

    return app


class CwaApp(connexion.App):
    def __init__(self, app_data_path, use_zmq=True, use_h5_swmr=True):

        if getattr(sys, 'frozen', False):
            # we are running in a bundle
            bundle_dir = sys._MEIPASS
        else:
            # we are running in a normal Python environment
            bundle_dir = os.path.dirname(os.path.abspath(__file__))

        # set static folder to vue js dist directory
        if getattr(sys, 'frozen', False):
            static_folder = os.path.join(bundle_dir, 'frontend', 'dist')
        else:
            static_folder = os.path.join(bundle_dir, '../frontend', 'dist')


        # Create the application instance
        # flask_app = Flask(__name__, template_folder="templates", static_url_path='')
        flask_app = Flask(
            __name__,
            template_folder=static_folder,
            static_folder=static_folder,
            static_url_path=''
        )

        # enable CORS
        CORS(flask_app)

        # Create the application instance
        # add swagger_ui to url path /api/ui
        # app = connexion.App(__name__, options={"swagger_ui": True})
        super().__init__(__name__, options={"swagger_ui": True})

        # pass configured flask app for static_url_path for instance
        # hoping it won't bug connexion app
        self.app = flask_app

        logging.basicConfig()
        logger = logging.getLogger('connexion.apis.flask_api')
        logger.setLevel(logging.INFO)
        logger.debug('Watch out!')

        if getattr(sys, 'frozen', False):
            spec = os.path.join(bundle_dir, 'openapi', 'openapi3.yml')
        else:
            spec = os.path.join(bundle_dir, '../openapi', 'openapi3.yml')
            
        self.add_api(spec)

        self.app.app_data_path = app_data_path

        # create directory of they not exist
        Path(os.path.join(app_data_path)).mkdir(parents=True, exist_ok=True)

        data_path = os.path.join(app_data_path, 'data')
        Path(data_path).mkdir(exist_ok=True)

        Path(os.path.join(app_data_path, 'process_definition')).mkdir(exist_ok=True)

        result_path = os.path.join(app_data_path, 'result')
        Path(result_path).mkdir(exist_ok=True)

        # remove lock files if exists
        file_list = glob.glob(os.path.join(result_path, '*.lock'))
        file_list.extend(glob.glob(os.path.join(data_path, '*.lock')))

        file_list.append('write_process')

        # Iterate over the list of filepaths & remove each file.
        for file_path in file_list:
            if not os.path.isfile(file_path):
                continue

            try:
                os.remove(file_path)
            except OSError:
                print("Error while deleting file : ", file_path)

        # create empty dynamic files
        process_api.write_json_file_with_lock(
            os.path.join(result_path, process_api.running_process_filename),
            list()
        )

        self.app.use_zmq = use_zmq
        self.app.use_h5_swmr = use_h5_swmr
        self.app.bundle_dir = bundle_dir

        logger.info("Store data to directory: {}".format(os.path.abspath(app_data_path)))
        logger.info("use_zmq: {}".format('yes' if use_zmq else 'no'))
        logger.info("use_h5_swmr: {}".format('yes' if use_h5_swmr else 'no'))

        write_queue = PriorityQueue()

        write_process = WriteProcess(port=5555)
        write_worker_thread = Thread(
            target=write_process.write_worker,
            args=(write_queue, None, use_zmq, use_h5_swmr),
            daemon=True
        )

        self.app.write_queue = write_queue

        write_worker_thread.start()

        if use_zmq:
            write_process.start(use_h5_swmr=use_h5_swmr)


        # Create a URL route in our application for "/"
        @self.app.route('/')
        def home():
            """
            This function just responds to the browser ULR
            localhost:5000/

            :return:        the rendered template 'home.html'
            """
            return render_template('index.html')