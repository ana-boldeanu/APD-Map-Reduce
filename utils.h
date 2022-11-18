#include <iostream>

using namespace std;

// Comparator for sorting data files by size (bytes), in descending order
bool compareDataFiles (pair<string, int> &a, pair<string, int> &b) {
  return (a.second > b.second);
}

// Compute the power of an integer value, also checking for overflow
int power(int value, int power, bool &overflow) {
    int factor = value;
    int prev_value = 1;
    overflow = false;

    if (value == 0) { return 0; }
    if (power == 0) { return 1; }

    for (int i = 0; i < power; i++) {
        value = prev_value * factor;
        
        if (prev_value != value / factor) {
            // Overflow detected
            overflow = true;
            return -1;
        }

        prev_value *= factor;
    }

    return value;
}

// Given value and N, look for a base so that value = base^N
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
