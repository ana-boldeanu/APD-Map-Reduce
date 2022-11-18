#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include <algorithm>

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


int main(int argc, char *argv[]) {
    int mappers;    // Number of Mapper threads
    int reducers;   // Number of Reducer threads
    int max_exponent;
    int data_files_nr; // Number of files to process
    int values;     // Number of values that a file contains
    int value;
    int base;
    bool found;
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
    max_exponent = reducers + 1;

    mapper_results = (list<int> ***) calloc(mappers, sizeof(list<int> **));
    for (int i = 0; i < mappers; i++) {
        mapper_results[i] = (list<int> **) calloc(max_exponent + 1, sizeof(list<int> *));

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

    // Mapper
    for (auto &file : data_files) {
        data_file.open(file.first);

        data_file >> values;
        // cout << "** There are " << values << " values." << endl;

        for (int i = 0; i < values; i++) {
            data_file >> value;
            // cout << "Checking value " << value << endl;

            for (int exp = 2; exp <= max_exponent; exp++) {
                base = findNthPowerBase(value, exp, found);
                if (found) {
                    // cout << "   Found base " << base << " for power " << exp << endl;
                    mapper_results[0][exp]->push_back(value);
                }
            }
        }
        data_file.close();
    }

    for (int exp = 2; exp <= max_exponent; exp++) {
        out_file_name = "out";
        out_file_name.append(to_string(exp));
        out_file_name.append(".txt");
        out_file.open(out_file_name);

        for (auto &value : *mapper_results[0][exp]) {
            out_file << value << endl;
        }

        out_file.close();
    }


}