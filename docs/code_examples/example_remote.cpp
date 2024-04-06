
int main() {

    // Instantiate a server object for communication
    cwa::Server server("http://cwa-server:3000");

    // Create an input data on the server with 10 datasets
    // Return string will be like "http://cwa-server:3000/input/XXX-XXX"
    cwa::Data input_data = server.CreateInputData(10);

    // Dataset is composed of data and attributes
    for(int i=0;i<10;i++) {
        vector<uint16> data;

        cwa::Dataset dataset(i);
        dataset.WriteData(data);
        dataset.AddAttribute("test", uint8(42+i));
        input_data.AddDataset(dataset);

        // Explicitly send to server
        input_data.Flush();
    }

    input_data.Close();

    cwa::ProcessDefinition process_definition;

    // Process definition may have some parameters
    // generate a default parameter value map
    map<string, Value> parameter_values =
        process_definition.GenerateParameterValues();

    // Process needs to know the input data and parameters values if required
    cwa::Process process(process_definition, input_data, parameter_values);

    // Start the process
    process.Start();

    // Once process is started, output data exists and is available
    cwa::Data output_data = process.GetOutputData();

    // Retrieve output datasets
    vector<cwa::Dataset> output_datasets = output_data.GetDatasets();

    // Follow process status to retrieve output data
    cwa::ProcessStatus status = process.GetStatus();

    int progress = status.GetProgress();
    while(process.IsRunning()) {

        // get data every 10%
        if(status.GetProgress() < progress + 10) continue;
        status = process.GetStatus();
        
        // Read data from output dataset and display it
        for(cwa::Dataset output_dataset : output_datasets) {
            vector<uint16> data;
            output_dataset.ReadData(data);
            print(output_dataset.GetId() + " : " + data);
        }
    }

    return;

}