
using cwa::process::Definition
using cwa::process::ParameterValue
using cwa::process::Worker
using cwa::process::Process

using cwa::data::Dataset
using cwa::data::Data

int main() {

    // Create dataset object bounded to files
    Dataset dataset_0("dataset_0");
    Dataset dataset_1("dataset_1");

    // Create an input data object containing these datasets
    InputData input_data({dataset_0, dataset_1})

    // Create data supplier objects to provide data from input data
    // Suppliers may read from other sources
    Supplier supplier_0(input_data);
    Supplier supplier_1(input_data);

    // Create output data and and aggregator objets to receive data
    // from workers
    // Dataset is processed when aggregator receives and writes its output
    // This create a directory where datasets will be stored
    OutputData output_data("output_data")
    Aggregator aggregator_0(output_data);
    Aggregator aggregator_1(output_data);

    // Create a process definition object from a file
    Definition definition("process_definition");

    // Get parameter values from it and update them
    map<string, ParameterValue> parameter_values = definition.GetParameterValues();
    parameter_values["parameter_0"] = 45;
    parameter_values["parameter_1"] = "fem";

    // Create worker object bound to data supplier
    // It may be interesting to have the same supplier for multiple workers
    // If using tcp communication, they will be initiated when worker network
    // is known. We have to wait until worker process are started to give them
    // adequate address.
    Worker worker_0(supplier_0, aggregator_0);
    Worker worker_1(supplier_0, aggregator_0);
    Worker worker_2(supplier_1, aggregator_1);
    Worker worker_3(supplier_1, aggregator_1);

    // Create the process object
    // Process have access to input and output data via data supplier
    // and aggregator from workers. Aggregators give aggregated data, thus,
    // process progress.
    Process process(
        definition, parameter_values,
        {worker_0, worker_1, worker_2, worker_3}
    );

    // Start it, this method starts data supplier and aggregators
    // and launches worker processes with as arguments:
    // - supplier and aggregators connection information
    // - process parameters
    process.Start();

    // Periodically get progress
    while(!process.IsFinished()) {
        print(process.GetProgress());
        sleep(1000);
    }

    // Wait until process is finiched and output data is written to disk
    process.Wait();

    return;
}