
int main() {

    // File is composed of datasets and attributes
    File input_file("input");

    vector<uint16> data;

    // Dataset is composed of data and attributes
    Dataset dataset("1");
    dataset.SetData(data);
    dataset.AddAttribute("test", uint8(42));

    input_file.AddDataset(dataset);
    input_file.Close();

    ProcessDefinition process_definition;

    map<string, Value> parameter_values =
        process_definition.GenerateParameterValues();

    string output_filename = "output";

    Process process(process_definition, input_file,
        parameter_values, output_filename);

    process.Start();

    File output_file(output_filename);

    ProcessStatus status =
        process.GetStatus();

    while(!status.is_finished) {

        // get data every 10%
        if(process.GetStatus().progress <
            status.progress + 10) continue;

        status = process.GetStatus();
        
        vector<uint16> data = output_file.GetData();
        display(data);
    }

    output_file.Close();

    return;

}