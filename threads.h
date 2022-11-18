#include <fstream>
#include <string>
#include <list>
#include <pthread.h>
#include "utils.h"

using namespace std;

// Structure containing data to be passed to Mapper threads
struct mapper_data {
    int id;                     // Thread ID
    int max_exponent;           // Maximum exponent for checking perfect powers
    pthread_mutex_t *mutex;     // Mutex used to synchronize data_files list access
    pthread_barrier_t *barrier; // Common barrier to synchronize Mappers with Reducers
    list<int> ***results;       // results[mapper_ID][exponent][list_of_values*]
    list<pair<string, int>> *data_files;    // List of data_files available for processing
    mapper_data(){};
} ;

// Structure containing data to be passed to Reducer threads
struct reducer_data {
    int id;                     // Thread ID
    int mappers;                // Number of Mappers (needed for combining partial lists)
    pthread_barrier_t *barrier; // Common barrier to synchronize Mappers with Reducers
    list<int> ***mapper_results;    // Partial lists of results obtained from Mappers
    reducer_data(){};
} ;

// Function ran by each Mapper thread
void *mapper_thread(void *arg) {
    // Extract data from thread arguments
  	mapper_data data = *(mapper_data*)arg;
	int id = data.id;
    int max_exp = data.max_exponent;
    list<int> ***mapper_results = data.results;
    list<pair<string, int>> *data_files = data.data_files;
    
    // Information about the file currently being processed
    pair<string, int> file;
    ifstream data_file;
    string file_name;
    int values;     // Number of values that a data file contains
    int value;      // The current value to be checked
    bool found;     // True if the current value is a perfect power
    
    // Process data_files so long as there are still files remaining
    while (!data_files->empty()) {
        // Lock access to data_files list (only one thread can extract a file at any time)
        pthread_mutex_lock(data.mutex);

        // Obtain the next file to process and remove it from the list
        file = data_files->front();
        file_name = file.first;
        data_files->pop_front();

        pthread_mutex_unlock(data.mutex);

        // Start reading values from the file
        data_file.open(file_name);
        data_file >> values;

        // For each value, check if it is a perfect power for each possible exponent
        for (int i = 0; i < values; i++) {
            data_file >> value;

            if (value > 0) {
                for (int exp = 2; exp <= max_exp; exp++) {
                    findNthPowerBase(value, exp, found);
                    if (found) {
                        // Add this value to the partial list of results
                        mapper_results[id][exp]->push_back(value);
                    }
                }
            }
        }
        data_file.close();
    }

    // Wait for all Mappers to finish before Reducers start combining partial lists
    pthread_barrier_wait(data.barrier);

    pthread_exit(NULL);
}

// Function ran by each Reducer thread
void *reducer_thread(void *arg) {
    // Extract data from thread arguments
    reducer_data data = *(reducer_data*)arg;
    int mappers = data.mappers;
    int id = data.id;
    int exp = id + 2;   // The exponent assigned to the Reducer

    // Wait for all Mappers to finish building partial lists
    pthread_barrier_wait(data.barrier);
    list<int> ***mapper_results = data.mapper_results;

    // Merge all partial lists assigned to the Reducer
    list<int> *result = mapper_results[0][exp];
    for (int i = 1; i < mappers; i++) {
        result->splice(result->end(), *mapper_results[i][exp]);
    }

    // Sort the final list and keep only unique values
    result->sort();
    result->unique();

    // Open the out file assigned to this Reducer
    ofstream out_file;
    string out_file_name;
    out_file_name = "out";
    out_file_name.append(to_string(exp));
    out_file_name.append(".txt");
    out_file.open(out_file_name);

    // Write the final result
    out_file << result->size();

    out_file.close();
    pthread_exit(NULL);
}
