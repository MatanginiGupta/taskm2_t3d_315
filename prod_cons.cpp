#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <chrono>
#include <ctime>
#include <random>
#include <algorithm>
#include <map>
#include <fstream>
#include <sstream>

using namespace std;

const int traffic_lights_total = 8;
const int capacity_of_buffer = 10;
const int measurement_per_hour = 12;
const int interval_measurement= 300; // 5 minutes in seconds
const int max_congested = 3;

mutex bufferLock;
queue<tuple<string, int, int>> trafficBuffer;
map<int, int> cong_TL;

// Producer function
void produceTrafficData() {
    while (true) {
        // Simulate traffic signal data generation
        auto currentTime = chrono::system_clock::now();
        auto timeNow = chrono::system_clock::to_time_t(currentTime);
        string TIME_STAMP = ctime(&timeNow);
        int TL_ID = rand() % traffic_lights_total + 1;
        int CARS_PASSED = rand() % 50 + 1;

        // Add data to buffer
        {
            lock_guard<mutex> guard(bufferLock);
            if (trafficBuffer.size() >= capacity_of_buffer) {
                cout << "Buffer full, waiting to add data..." << endl;
            }
            trafficBuffer.push(make_tuple(TIME_STAMP, TL_ID, CARS_PASSED));
            cout << "Produced: " << TIME_STAMP << ", Traffic Light ID: " << TL_ID << ", Cars Passed: " << CARS_PASSED << endl;
        }
        this_thread::sleep_for(chrono::milliseconds(rand() % 1000 + 100)); // Simulate variable time to produce data
    }
}

// Consumer function
void consumeTrafficData() {
    int measurementCount = 0;
    while (measurementCount < measurement_per_hour) {
        this_thread::sleep_for(chrono::seconds(interval_measurement));

        // Process traffic data
        {
            lock_guard<mutex> guard(bufferLock);
            while (!trafficBuffer.empty()) {
                auto data = trafficBuffer.front();
                trafficBuffer.pop();
                string TIME_STAMP = get<0>(data);
                int TL_ID = get<1>(data);
                int CARS_PASSED = get<2>(data);
                if (cong_TL.find(TL_ID) != cong_TL.end()) {
                    cong_TL[TL_ID] += CARS_PASSED;
                } else {
                    cong_TL[TL_ID] = CARS_PASSED;
                }
            }
        }

        // Find top N congested traffic lights
        vector<pair<int, int>> CONG_SORT(cong_TL.begin(), cong_TL.end());
        partial_sort(CONG_SORT.begin(), CONG_SORT.begin() + min(max_congested, (int)CONG_SORT.size()), CONG_SORT.end(),
                     [](const pair<int, int> &a, const pair<int, int> &b) { return a.second > b.second; });
        cout << "Top " << max_congested << " congested traffic lights at " << chrono::system_clock::to_time_t(chrono::system_clock::now()) << " - ";
        for (int i = 0; i < min(max_congested, (int)CONG_SORT.size()); ++i) {
            cout << CONG_SORT[i].first << " (Cars Passed: " << CONG_SORT[i].second << "), ";
        }
        cout << endl;

        ++measurementCount;
    }
}

// Read traffic data from file and feed into the buffer
void readTrafficDataFromFile(const string& filename) {
    ifstream file(filename);
    if (file.is_open()) {
        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            string TIME_STAMP;
            int TL_ID, CARS_PASSED;
            if (getline(ss, TIME_STAMP, ',') && ss >> TL_ID && ss.ignore(1) && ss >> CARS_PASSED) {
                lock_guard<mutex> guard(bufferLock);
                trafficBuffer.push(make_tuple(TIME_STAMP, TL_ID, CARS_PASSED));
                cout << "Read from file: " << TIME_STAMP << ", Traffic Light ID: " << TL_ID << ", Cars Passed: " << CARS_PASSED << endl;
            } else {
                cout << "Timetsamp_datafile: " << line << endl;
            }
        }
        file.close();
    } else {
        cout << "Unable to open file for reading: " << filename << endl;
    }
}

int main() {
    srand(time(nullptr));

    // Generate traffic data file
    ofstream dataFile("traffic_data.txt");
    if (dataFile.is_open()) {
        for (int i = 0; i < measurement_per_hour; ++i) {
            // Generate random TIME_STAMP
            time_t currentTime = time(nullptr);
            string TIME_STAMP = ctime(&currentTime);

            // Generate random traffic light ID and cars passed
            int TL_ID = rand() % traffic_lights_total + 1; // Assuming there are 5 traffic lights
            int CARS_PASSED = rand() % 50 + 1; // Assuming maximum 50 cars passed

            // Write data to file
            dataFile << TIME_STAMP << "," << TL_ID << "," << CARS_PASSED << endl;
        }
        dataFile.close();
        cout << "Traffic data file generated successfully." << endl;
    } else {
        cout << "Unable to open file for writing." << endl;
    }

    // Read data from file and feed into the buffer
    readTrafficDataFromFile("traffic_data.txt");

    // Create producer and consumer threads
    thread producerThread(produceTrafficData);
    thread consumerThread(consumeTrafficData);

    // Join threads
    producerThread.join();
    consumerThread.join();

    return 0;
}
