
using cwa::client::Client
using cwa::client::Dataset

int main(argc, argv) {

    // Instantiate a client object
    Client client(argc, argv);

    // Loop over datasets
    Dataset dataset;
    while(client.PullDataset(dataset)) {
        void* data = dataset.GetData();

        void* result_data;
        // Process data

        // Create a result dataset
        Dataset result(result_data);

        // push it to CWA server
        client.PushDataset(result);
    }

    return;
}