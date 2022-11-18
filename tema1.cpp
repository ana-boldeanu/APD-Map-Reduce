#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <pthread.h>

using namespace std;


bool compareDataFiles (pair<string, int> &a, pair<string, int> &b) {
  return (a.second > b.second);
}


int power(int value, int power, bool &overflow) {
    int factor = value;
    int prev_value = 1;
    overflow = false;

    if (value == 0) {
        return 0;
    }

    if (power == 0) {
        return 1;
    }

    for (int i = 0; i < power; i++) {
        value = prev_value * factor;
        
        if (prev_value != value / factor) {
            // Was overflow
            overflow = true;
            return -1;
        }

        prev_value *= factor;
    }

    return value;
}

// Given value and N, we are looking for a base so that value = base^N
int findNthPowerBase (int value, int exp, bool &found) {
    int left = 0, right = value;
    int mid, result;
    bool overflow;
    
    while (left <= right) {
        mid = left + (right - left) / 2;

        result = power(mid, exp, overflow);

        if (!overflow) {
            if (result == value) {
                found = true;
                return mid;

            } else if (result > value) {
                right = mid - 1;

            } else {
                left = mid + 1;
            }

        } else {
            right = mid - 1;
        }
    }

    found = false;
    return -1;
}


struct mapper_data {
    int id;
    int max_exponent;
    list<pair<string, int>> *data_files;
    list<int> ***results;
    pthread_mutex_t *mutex;
    pthread_barrier_t *barrier;
    mapper_data(){};
} ;

void *mapper_thread(void *arg) {
  	mapper_data data = *(mapper_data*)arg;
	int id = data.id;
    int max_exp = data.max_exponent;
    list<pair<string, int>> *data_files = data.data_files;
    ifstream data_file;
    pair<string, int> file;
    string file_name;
    int values, value;
    bool found;
    list<int> ***mapper_results = data.results;

    while (!data_files->empty()) {
        pthread_mutex_lock(data.mutex);

        file = data_files->front();
        file_name = file.first;
        data_files->pop_front();

        pthread_mutex_unlock(data.mutex);

        data_file.open(file_name);
        data_file >> values;

        for (int i = 0; i < values; i++) {
            data_file >> value;

            if (value > 0) {
                for (int exp = 2; exp <= max_exp; exp++) {
                    findNthPowerBase(value, exp, found);
                    if (found) {
                        mapper_results[id][exp]->push_back(value);
                    }
                }
            }
        }
        data_file.close();
    }

    pthread_barrier_wait(data.barrier);

    pthread_exit(NULL);
}


struct reducer_data {
    int id;
    int mappers;
    list<int> ***mapper_results;
    pthread_barrier_t *barrier;
    reducer_data(){};
} ;

void *reducer_thread(void *arg) {
    reducer_data data = *(reducer_data*)arg;

    pthread_barrier_wait(data.barrier);

	int id = data.id;
    int exp = id + 2;
    int mappers = data.mappers;
    list<int> ***mapper_results = data.mapper_results;
    ofstream out_file;
    string out_file_name;

    list<int> *result = mapper_results[0][exp];
    
    for (int i = 1; i < mappers; i++) {
        result->splice(result->end(), *mapper_results[i][exp]);
    }

    result->sort();
    result->unique();

    out_file_name = "out";
    out_file_name.append(to_string(exp));
    out_file_name.append(".txt");
    out_file.open(out_file_name);

    out_file << result->size();

    out_file.close();
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    int mappers;    // Number of Mapper threads
    int reducers;   // Number of Reducer threads
    int max_exponent;
    int data_files_nr; // Number of files to process
    ifstream input_file;   // File containing information about this test
    ifstream data_file;    // File containing data to process
    ofstream out_file;
    string input_file_name;
    string data_file_name;
    string out_file_name;
    int data_file_size;
    list<pair<string, int>> data_files;   // Array of pairs (file_name, file_size)
    list<int> ***mapper_results;   // vector[mapper_ID][exponent][list_of_values*]

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

    mapper_results = (list<int> ***) malloc(mappers * sizeof(list<int> **));
    for (int i = 0; i < mappers; i++) {
        mapper_results[i] = (list<int> **) malloc((max_exponent + 1) * sizeof(list<int> *));

        for (int j = 0; j <= max_exponent; j++) {
            mapper_results[i][j] = new list<int>;
        }
    }

    // Open input file and get number of data files
    input_file.open(input_file_name);
    input_file >> data_files_nr;

    for (int i = 0; i < data_files_nr; i++) {
        // Get the name of each data file and open it
        input_file >> data_file_name;
        data_file.open(data_file_name);

        // Compute its size and add this pair to the vector
        streampos begin, end;
        begin = data_file.tellg();
        data_file.seekg (0, ios::end);
        end = data_file.tellg();
        data_file.close();
        data_file_size = end - begin;

        data_files.push_back(make_pair(data_file_name, data_file_size));
    }

    data_files.sort(compareDataFiles);



    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, mappers + reducers);

    mapper_data *mapper;
    reducer_data *reducer;

    pthread_t threads[mappers + reducers];
  	int ret;
  	void *status;

  	for (int id = 0; id < mappers; id++) {
        mapper = new mapper_data();
        mapper->data_files = &data_files;
        mapper->results = mapper_results;
        mapper->max_exponent = max_exponent;
        mapper->id = id;
        mapper->mutex = &mutex;
        mapper->barrier = &barrier;

        ret = pthread_create(&threads[id], NULL, mapper_thread, (void*)mapper);

        if (ret) {
			printf("Error creating thread %d\n", id);
			exit(-1);
		}
    }

    for (int id = 0; id < reducers; id++) {
        reducer = new reducer_data();
        reducer->mapper_results = mapper_results;
        reducer->mappers = mappers;
        reducer->id = id;
        reducer->barrier = &barrier;
        ret = pthread_create(&threads[id + mappers], NULL, reducer_thread, (void*)reducer);

        if (ret) {
			printf("Error creating thread %d\n", id);
			exit(-1);
		}
    }
    
    for (int id = 0; id < mappers + reducers; id++) {
		ret = pthread_join(threads[id], &status);

        if (ret) {
			printf("Error joining thread %d\n", id);
			exit(-1);
		}
  	}

    pthread_mutex_destroy(&mutex);
    pthread_barrier_destroy(&barrier);

    pthread_exit(NULL);
}