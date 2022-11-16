#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;


bool compareDataFiles (pair<string, int> &a, pair<string, int> &b) {
  return (a.second > b.second);
}

int main(int argc, char *argv[]) {
    int mappers;    // Number of Mapper threads
    int reducers;   // Number of Reducer threads
    int data_files_nr; // Number of files to process
    int values;     // Number of values that a file contains
    ifstream input_file;   // File containing information about this test
    ifstream data_file;    // File containing data to process
    string input_file_name;
    string data_file_name;
    int data_file_size;
    vector<pair<string, int>> data_files;   // Array of pairs (file_name, file_size)

    // Check correct number of args
    if (argc < 3) {
        printf("Usage: ./main <mappers_number> <reducers_number> <input_file>\n");
        return 0;
    }

    // Get args values
    mappers = atoi(argv[1]);
    reducers = atoi(argv[2]);
    input_file_name = argv[3];

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

    sort(data_files.begin(), data_files.end(), compareDataFiles);

    for (int i = 0; i < data_files.size(); i++)
	{
		cout << data_files[i].first << ": " << data_files[i].second << endl;
	}
    
}