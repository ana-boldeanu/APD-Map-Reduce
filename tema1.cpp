#include "utils.h"
#include "threads.h"

using namespace std;

int main(int argc, char *argv[]) {
    int mappers;            // Number of Mapper threads
    int reducers;           // Number of Reducer threads
    int max_exponent;       // Maximum exponent for checking perfect powers
    int data_files_nr;      // Number of files to process
    int data_file_size;     // The size (in bytes) of a data file
    ifstream input_file;    // File containing information about this test
    ifstream data_file;     // File containing data to process
    string input_file_name;
    string data_file_name;
    list<pair<string, int>> data_files; // Pairs of <file_name, file_size>
    list<int> ***mapper_results;        // results[mapper_ID][exponent][list_of_values*]

    // Check correct number of args
    if (argc < 3) {
        printf("Usage: ./tema1 <mappers_number> <reducers_number> <input_file>\n");
        return 0;
    }

    // Get args values
    mappers = atoi(argv[1]);
    reducers = atoi(argv[2]);
    input_file_name = argv[3];
    max_exponent = reducers + 2;

    // Allocate space for the lists of mapper results
    // Each mapper will be working on a vector mapper_results[ID], in which each partial list will 
    // be pointed to by mapper_results[ID][exponent]
    mapper_results = (list<int> ***) malloc(mappers * sizeof(list<int> **));
    for (int id = 0; id < mappers; id++) {
        mapper_results[id] = (list<int> **) malloc((max_exponent + 1) * sizeof(list<int> *));

        for (int expo = 0; expo <= max_exponent; expo++) {
            mapper_results[id][expo] = new list<int>;
        }
    }

    // Open input file and get number of data files to process
    input_file.open(input_file_name);
    input_file >> data_files_nr;

    for (int i = 0; i < data_files_nr; i++) {
        // Get the name of each data file and open it
        input_file >> data_file_name;
        data_file.open(data_file_name);

        // Compute its size and add this pair to the list
        streampos begin, end;
        begin = data_file.tellg();
        data_file.seekg (0, ios::end);
        end = data_file.tellg();
        data_file.close();
        data_file_size = end - begin;

        data_files.push_back(make_pair(data_file_name, data_file_size));
    }

    // Sort the list of files by their size in descending order
    data_files.sort(compareDataFiles);

    // Mapper and Reducer threads
    pthread_t threads[mappers + reducers];
    mapper_data *mapper_args[mappers];
    reducer_data *reducer_args[reducers];
    int ret;
  	void *status;

    // Create a mutex, to be used for synchronizing mappers when accessing data_files
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    // Create a barrier, to be used for synchronizing mappers and reducers
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, mappers + reducers);

    // Prepare arguments to be used by Mappers
    for (int i = 0; i < mappers; i++) {
        mapper_args[i] = new mapper_data();
        mapper_args[i]->data_files = &data_files;
        mapper_args[i]->results = mapper_results;
        mapper_args[i]->max_exponent = max_exponent;
        mapper_args[i]->id = i;
        mapper_args[i]->mutex = &mutex;
        mapper_args[i]->barrier = &barrier;
    }

    // Prepare arguments to be used by Reducers
    for (int i = 0; i < reducers; i++) {
        reducer_args[i] = new reducer_data();
        reducer_args[i]->mapper_results = mapper_results;
        reducer_args[i]->mappers = mappers;
        reducer_args[i]->id = i;
        reducer_args[i]->barrier = &barrier;
    }

    // Create and start Mappers 
  	for (int id = 0; id < mappers; id++) {
        ret = pthread_create(&threads[id], NULL, mapper_thread, (void*)mapper_args[id]);

        if (ret) {
			printf("Error creating thread %d\n", id);
			exit(-1);
		}
    }

    // Create and start Reducers
    for (int id = 0; id < reducers; id++) {
        ret = pthread_create(&threads[id + mappers], NULL, reducer_thread, (void*)reducer_args[id]);

        if (ret) {
			printf("Error creating thread %d\n", id);
			exit(-1);
		}
    }
    
    // Join all Mappers and Reducers
    for (int id = 0; id < mappers + reducers; id++) {
		ret = pthread_join(threads[id], &status);

        if (ret) {
			printf("Error joining thread %d\n", id);
			exit(-1);
		}
  	}

    // Free used memory
    pthread_mutex_destroy(&mutex);
    pthread_barrier_destroy(&barrier);

    pthread_exit(NULL);
}
