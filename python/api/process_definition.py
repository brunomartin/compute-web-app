import os
import json
import platform

import flask


def get_process_definition_path():
    application_data_path = flask.current_app.app_data_path
    return os.path.join(application_data_path, 'process_definition')


class Error(Exception):
    """Base class for exceptions in this module."""
    pass


class ProcessDefinitionError(Error):
    """Exception raised for errors in the process definition.

    Attributes:
        message -- explanation of the error
    """

    def __init__(self, message):
        self.message = message


# check process definition, raise ProcessDefinitionError if
# not conform
def check_process_definition(process_definition):

    # check if has name
    if "name" not in process_definition:
        raise ProcessDefinitionError("Process definition shall contain name element")

    # check if has description
    if "description" not in process_definition:
        raise ProcessDefinitionError("Process definition shall contain description element")

    # check if has results
    if "results" not in process_definition:
        raise ProcessDefinitionError("Process definition shall contain results element")

    results = process_definition["results"]

    # check if results is a list
    if not isinstance(results, list):
        raise ProcessDefinitionError("Process definition results shall be a list")

    # check if has args
    if "args" not in process_definition:
        raise ProcessDefinitionError("Process definition shall contain args element")

    # extract arguments from process definition
    if platform.system() == "Windows":
        args = process_definition["win_args"]
    else:
        args = process_definition["args"]

    # check if args is a list
    if not isinstance(args, list):
        raise ProcessDefinitionError("Process definition args shall be a list")

    # check if has data_file
    if "data_file" not in process_definition["args"]:
        raise ProcessDefinitionError("Process definition args shall contain data_file element")

    # check if has result_file
    if "result_file" not in process_definition["args"]:
        raise ProcessDefinitionError("Process definition args shall contain result_file element")

    # check if has parameters
    if "parameters" not in process_definition:
        raise ProcessDefinitionError("Process definition shall contain parameters element")

    parameters = process_definition["parameters"]

    # check if parameters as type
    for parameter in parameters:
        if "type" not in parameters[parameter]:
            raise ProcessDefinitionError("Process definition parameter" + parameter + " shall contain type element")

        parameter_type = parameters[parameter]["type"]

        # check if type is integer, number or string
        if parameter_type not in ["integer", "number", "string"]:
            raise ProcessDefinitionError("Process definition parameter" + parameter +
                                         " type element shall be integer, number or string")

        # check if parameter is in args
        if parameter not in args:
            raise ProcessDefinitionError("Process definition parameter" + parameter + " shall be in args list")


# local function to get a process definition from filename
def get(process_definition_id):
    filepath = os.path.join(get_process_definition_path(), process_definition_id + ".json")

    with open(filepath) as json_file:
        data = json.load(json_file)

    return data


# search handler to list all process datas
def search():
    process_definitions = []
    for filename in os.listdir(get_process_definition_path()):

        # check that it is a json file
        if len(filename) < 4 or not filename.endswith(".json"):
            continue

        process_definition = dict()

        try:
            process_definition_id = filename[:-5]

            process_definition = dict()

            process_definition["id"] = process_definition_id

            # the next line may raise json.JSONDecodeError
            process_definition.update(get(process_definition_id))

            # the next line may raise ProcessDefinitionError or general Exception
            check_process_definition(process_definition)

            process_definition.pop('args', None)

            # if process can start earlier and h5 swmr is not enable
            # don't propose starting earlier
            if not flask.current_app.use_h5_swmr:
                if 'options' in process_definition:
                    process_options = process_definition['options']
                    if 'early_process' in process_options:
                        process_options.remove('early_process')

            process_definitions.append(process_definition)

        except json.JSONDecodeError as e:
            process_definition['error'] = 'json.JSONDecodeError : ' + e.msg
            process_definitions.append(process_definition)

        except ProcessDefinitionError as e:
            process_definition['error'] = 'ProcessDefinitionError : ' + e.message
            process_definitions.append(process_definition)

        except Exception as e:
            process_definition['error'] = 'Exception : ' + str(e)
            process_definitions.append(process_definition)

    return process_definitions
