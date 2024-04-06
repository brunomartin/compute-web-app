from . import data, process, process_definition, result


# Create a handler for our read (POST) upload
def search():
    return {
        "status": "ok",
        "message": "Welcome to the Compute Web App",
        "version": "1.0",
        "data_path": data.get_data_path(),
        "process_definition_path": process_definition.get_process_definition_path(),
        "result_path": process.get_result_path()
    }
