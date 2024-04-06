
int main(int argc, char* argv[]) {

    // Instantiate a connection to running cwa server. Process is launched by
    // cwa with specific arguments. cwa::Server will read these arguments
    // Arguments contains reference to input data, process parameters and 
    cwa::Server server(argc, argv);

    // Get process object
    cwa::Process process = server.GetProcess();

    // Get input data object
    cwa::Data input_data = process.GetInputData();

    // This process has be launched for processing specific datasets.
    // Other processes launched in parallel is processing other datasets
    vector<cwa::Dataset> datasets = input_data.GetDatasets();

    // Retrieve parameters for this process
    map<string, cwa::Parameter> parameters = process.GetParameters();

    // Create now an output data so that cwa is aware of it
    cwa::Data output_data = process.CreateOutput(dataset.GetId());

    // Now run processes on dedicated datasets
    for(cwa::Dataset dataset : datasets) {

        // Create a vector to store data
        vector<uint16> data;

        // Read data from dataset. In case data has not completly arrived
        // it blocks until data is available
        dataset.ReadData(data);

        // data is filled now with data to process. Instantiate a result
        // vector for the current dataset
        vector<double> output_data;
        int output_int;

        // Begin process data...
        // ...

        // If warning, notice it
        process.AddWarning(dataset.GetId(), "A value is too high");

        // If needed, one can report its progress on this dataset to cwa server
        process.UpdateProgress(dataset.GetId(), 60);

        // If error, notice it
        process.AddError(dataset.GetId(), "This error is not handled");

        // ...
        // End process data...
        // Generate output

        // Create a dataset for the output data
        cwa::Dataset output_dataset = process.CreateOutput(dataset.GetId());
        output_dataset.WriteData(output_data);
        output_dataset.AddAttribute("output_int", int32(output_int));

        // Flush dataset so that content is stored
        output_dataset.Flush();

        // Now output is computed and flushed, tell process is finished
        // for this dataset
        process.UpdateProgress(dataset.GetId(), 100);
    }

    // Return success exit code here so that cwa will consider process as done
    // without error
    return 0;
}