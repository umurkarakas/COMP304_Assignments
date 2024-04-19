#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <pthread.h>
#include <queue>
#include <unistd.h>
#include <ctime>
#include <cstring>
#include <algorithm> 
#include <unordered_map>

using namespace std;

typedef struct Event { // Event struct for control log
    string event;
    time_t event_time;
    string train_id;
    string trains_waiting_passage;
} Event;

struct Train { // Train struct
    int id; 
    char source;
    char destination; 
    int length;
    time_t departure_time;
    time_t arrival_time;

    bool operator>(const Train& other) const { // overload the > operator for priority queue
        return id > other.id;
    }
    bool operator<(const Train& other) const { // overload the < operator for priority queue
        return id < other.id;
    }
};

// mutexes
pthread_mutex_t tunnelMutex;
pthread_mutex_t trainMutex;
pthread_mutex_t coutMutex;

// queues
queue<Train> section1Queue;
queue<Train> section2Queue;
queue<Train> section3Queue;
queue<Train> section4Queue;
queue<Event> controlLogQueue;

bool debug = false;

// hash map for train log with id, train pairs
unordered_map<int, Train> trainsHashMap;

// global variables
time_t start_time;
bool clearance = false;
time_t time_to_clear;
int train_id = -1;

// Function prototypes
void* controlCenter(void*);
void* sectionFunction(void* source);

typedef struct argument_structures { // struct for passing arguments to threads
    char source;
    float p;
    int simulation_time;
} arg_struct;

// Function to add values in section queues to priority queue
void addSectionQueuesToPriorityQueue(priority_queue<Train, vector<Train>, greater<Train>>& pq, queue<Train>& q) {
    queue<Train> temp = q;
    while (!temp.empty()) {
        pq.push(temp.front());
        temp.pop();
    }
}

string trainsWaiting() { // function to get the trains waiting in the queues
    priority_queue<Train, vector<Train>, greater<Train>> pq;
  
    addSectionQueuesToPriorityQueue(pq, section1Queue);
    addSectionQueuesToPriorityQueue(pq, section2Queue);
    addSectionQueuesToPriorityQueue(pq, section3Queue);
    addSectionQueuesToPriorityQueue(pq, section4Queue);
   
    string trains_wait = "";
    while (!pq.empty()) { // pq orders the trains by id
        Train train = pq.top();
        trains_wait += to_string(train.id);
        pq.pop();
        if (!pq.empty()) {
            trains_wait += ",";
        }
    }
    return trains_wait;
}

// Control center function
void* controlCenter(void*) {

    while(1) {
        pthread_mutex_lock(&tunnelMutex);

        // Check if the all sectionQueues are empty
        pthread_mutex_lock(&trainMutex);
        bool allEmpty = section1Queue.empty() && section2Queue.empty() && section3Queue.empty() && section4Queue.empty();
        pthread_mutex_unlock(&trainMutex);
        if (allEmpty) {            
            if (clearance) { // if the system is in clearance mode, calculate the clearance time and push the message to the queue
                int clear_time = difftime(time(0), time_to_clear); // calculate clearance time
                controlLogQueue.push({"Tunnel Cleared", time(0), "#", "# Time to clear: " + to_string(clear_time) + " sec"}); // push the message to the queue
            }
            clearance = false; // set the clearance flag to false
        } else {
            // Pop from most crowded sectionQueue and calculate the arrival time
            pthread_mutex_lock(&trainMutex);
            int section1QueueSize = section1Queue.size();
            int section2QueueSize = section2Queue.size();
            int section3QueueSize = section3Queue.size();
            int section4QueueSize = section4Queue.size();
            
            int maxQueueSize = max(section1QueueSize, max(section2QueueSize, max(section3QueueSize, section4QueueSize)));

            struct Train train;
            if (maxQueueSize == section1QueueSize) {  
                train = section1Queue.front();
                section1Queue.pop();                
            } else if (maxQueueSize == section2QueueSize) {
                train = section2Queue.front();
                section2Queue.pop();
            } else if (maxQueueSize == section3QueueSize) {
                train = section3Queue.front();
                section3Queue.pop();
            } else {
                train = section4Queue.front();
                section4Queue.pop();
            }
            pthread_mutex_unlock(&trainMutex);
            
            if (debug) {
                time_t rawtime = time(0);
                pthread_mutex_lock(&coutMutex);
                cout << "--Enter Tunnel-- " << " Train " << train.id << " from " << train.source << " to " << train.destination << " at " << asctime(localtime(&rawtime)) << endl;
                pthread_mutex_unlock(&coutMutex);
            }
            controlLogQueue.push({"Tunnel Passing", time(0), to_string(train.id), trainsWaiting()});

            float p_breakdown = (float) rand()/RAND_MAX;
            int breakdown_time;
            sleep(1); // train's front reaches to the end of the tunnel
            // calculate the breakdown time and sleep accordingly 
            if (p_breakdown <= 0.1) {
                if(debug) {
                    pthread_mutex_lock(&coutMutex);
                    cout << "--Breakdown-- " << " Train " << train.id << " from " << train.source << " to " << train.destination << " at " << asctime(localtime(&train.arrival_time)) << endl;
                    pthread_mutex_unlock(&coutMutex);
                }
                breakdown_time = 4;
                controlLogQueue.push({"Breakdown", time(0), to_string(train.id), trainsWaiting()});

            } else {
                breakdown_time = 0;
            }
            sleep((train.length/100 + breakdown_time)); // sleep for the time that train's back passes the tunnel, i.e., tunnel entry is cleared

            if(debug) {
                time_t rawtime = time(0);
                pthread_mutex_lock(&coutMutex);
                cout << "--Exit Tunnel-- " << " Train " << train.id << " from " << train.source << " to " << train.destination << " at " << asctime(localtime(&rawtime)) << endl;
                pthread_mutex_unlock(&coutMutex);
            }
            pthread_mutex_lock(&trainMutex);
            trainsHashMap[train.id].arrival_time =  time(0) + 1; // plus 1 for the time spent in the last section
            pthread_mutex_unlock(&trainMutex);
        }
        pthread_mutex_unlock(&tunnelMutex);

    }
    return NULL;
}

// Train function
void* sectionFunction(void* arguments) {
    arg_struct* args = (arg_struct*)arguments;
    char source = args->source;
    float p = args->p;
    while(1) {
        // check if the system is clearance mode
        if (clearance) {
            continue;
        }
        // check if the system is overloaded and change the clearance flag accordingly
        pthread_mutex_lock(&trainMutex);
        // if the sum of the queues is greater than 10, the system is overloaded
        // trains that are either in the tunnel or has left the tunnel but not arrived to their destination are not counted
        // since they do not contribute to the system overload
        bool overload = section1Queue.size() + section2Queue.size() + section3Queue.size() + section4Queue.size() > 10; 

        if (overload) {
            if (!clearance) {
                time_to_clear = time(0); // set time_to_clear to current time for future calculation of clearance time
                controlLogQueue.push({"System Overload", time(0), "#", trainsWaiting()});
            }
            clearance = true;
        }

        pthread_mutex_unlock(&trainMutex);

        if (!clearance) {            
            float prob_create;
            float threshold = (float) rand()/RAND_MAX;

            if (source == 'B') { // probability of creating a train from B is 1-p
                prob_create = 1-p;
            } else { // probability of creating a train from A, E, F is p
                prob_create = p;
            }
            if (prob_create <= threshold) { // create a train if the probability is less than the threshold
                
                float rand_length = (float) rand()/RAND_MAX; 
                int length = rand_length<=0.7 ? 100:200; // 70% of the trains are 100m long and 30% of the trains are 200m long
                float rand_line = (float) rand()/RAND_MAX;
                char destination;
                if((source=='A') || source=='B') { // if the source is A or B, the destination is E or F
                    destination = rand_line<=0.5 ? 'E':'F';
                } else { // if the source is E or F, the destination is A or B
                    destination = rand_line<=0.5 ? 'A':'B';
                }

                struct Train train;
                pthread_mutex_lock(&trainMutex); // lock the train mutex for train id increment so that some number is not skipped
                train.id = ++train_id;

                train.source = source;
                train.destination = destination;
                train.length = length;
                train.departure_time = time(0);
                
                train.arrival_time = -1;

                if (source=='A') {
                    section1Queue.push(train);
                } else if (source=='B') {
                    section2Queue.push(train);
                } else if (source=='E') {
                    section3Queue.push(train);
                } else {
                    section4Queue.push(train);
                }

                trainsHashMap[train.id] = train; // add the train to the hash map for train log
                pthread_mutex_unlock(&trainMutex); // unlock the train mutex

               if (debug) {
                    pthread_mutex_lock(&coutMutex);
                    cout << "--Departure-- " << " Train " << train.id << " from " << train.source << " to " << train.destination << " at " << asctime(localtime(&train.departure_time)) <<  endl;
                    pthread_mutex_unlock(&coutMutex);
               }
            } 
            sleep(1);
        }
    }
    return NULL;
}

void* controlLog(void* /*arguments*/) {
    ofstream outfile;
    outfile.open("control-center.log");

    if (!outfile.is_open()) {
        std::cerr << "Error opening control-center.log" << std::endl;
        return nullptr;
    }

    outfile << "Event           Event Time Train ID  Trains Waiting Passage" << endl;

    while (!controlLogQueue.empty()) { // write the events to the file
        Event event = controlLogQueue.front(); // get the first element of the queue
        controlLogQueue.pop(); // pop the first element of the queue

        tm* ltm = localtime(&event.event_time); 
        ostringstream event_time;
        event_time << setw(2) << setfill('0') << ltm->tm_hour << ":"
                   << setw(2) << setfill('0') << ltm->tm_min << ":"
                   << setw(2) << setfill('0') << ltm->tm_sec;
        // write the event to the file
        outfile << left << setw(18) << event.event << setw(12) << event_time.str()
                << setw(9) << event.train_id << setw(50) << event.trains_waiting_passage << endl;
    }

    outfile.close();
    return nullptr;
}


void* trainLog(void* arguments) {
    arg_struct* args = (arg_struct*)arguments;
    int simulation_time = args->simulation_time;
    float p = args->p;

    ofstream outfile;
    outfile.open("train.log");

    if (!outfile.is_open()) {
        std::cerr << "Error opening train.log" << std::endl;
        return nullptr;
    }

    outfile << "Simulation Arguments: p=" << p << ", simulation time=" << simulation_time << ", debug mode=" << debug << endl << endl;
    outfile << "Train ID Starting Point Destination Point Length(m) Departure Time Arrival Time" << endl;

    for(int i=0; i<train_id+1; i++) {
        Train train = trainsHashMap[i];
        // Use std::ostringstream for creating formatted strings
        ostringstream row, departure_time, arrival_time;

        row << setw(8) << train.id << " "
            << setw(14) << train.source << " "
            << setw(11) << train.destination << " "
            << setw(15) << train.length << " ";

        // Format departure time
        tm* dtm = localtime(&train.departure_time);
        departure_time << setfill('0') << setw(2) << dtm->tm_hour << ":"
            << setfill('0') << setw(2) << dtm->tm_min << ":"
            << setfill('0') << setw(2) << dtm->tm_sec << " ";
        row << setw(15) << departure_time.str();   

        // Format arrival time if it arrived to its destination
        if (train.arrival_time != -1) {
            tm* atm = localtime(&(train.arrival_time));
            arrival_time << setfill('0') << setw(2) << atm->tm_hour << ":"
                << setfill('0') << setw(2) << atm->tm_min << ":"
                << setfill('0') << setw(2) << atm->tm_sec << " ";
            row << setw(13) << arrival_time.str();   

        } else {
            // The id does not exist in the hash map
            row << setw(12) << "#" << " ";
        }
        
        outfile << row.str() << endl;
    }


    ostringstream row;

    row << endl << endl << "#" << " Stands for trains that did not enter the tunnel";
    row << endl << endl << "Number of trains: " << trainsHashMap.size() << " and number of different ids: " << train_id + 1 ;

    outfile << row.str() << endl;

    outfile.close();
    return nullptr;
}

int main(int argc, char* argv[]) {


    if (argc < 7) {
        cout << "Usage: ./metro_simulation -s <simulation_time> -p <probability> -d <debug:T or F>" << endl;
        return 0;
    } 

    int simulation_time; 
    float p;

    // Get the arguments from the command line
    for(int i=1; i<argc; i++) {
        if (strcmp(argv[i], "-s") == 0) {
            simulation_time = atoi(argv[i+1]);
        } else if (strcmp(argv[i], "-p") == 0) {
            p = atof(argv[i+1]);
        } else if (strcmp(argv[i], "-d") == 0) {
            if (strcmp(argv[i+1], "T") == 0) {
                debug = true;
            } else {
                debug = false;
            }
        }
    }

    // Check if the probability is between 0 and 1
    if (p < 0 || p > 1) {
        cout << "Probability must be between 0 and 1" << endl;
        return 0;
    }

    start_time = time(0); // get the start time of the simulation
    // Initialize the mutexes;
    pthread_mutex_init(&tunnelMutex, NULL);
    pthread_mutex_init(&trainMutex, NULL);
    if (debug) {
        pthread_mutex_init(&coutMutex, NULL);
    }

    int numberOfSections = 4;
    // threads
    pthread_t sectionsThread[numberOfSections];
    pthread_t controlLogThread;
    pthread_t controlCenterThread;
    pthread_t trainLogThread;

    // Create threads for trains
    char sources[numberOfSections] = {'A', 'B', 'E', 'F'};
    arg_struct arguments[4];
    for (int i = 0; i < numberOfSections; i++) {  
        arguments[i].p = p;
        arguments[i].source = sources[i];
        pthread_create(&sectionsThread[i], NULL, sectionFunction, (void*)&arguments[i]);
    }

    // Create control center thread
    pthread_create(&controlCenterThread, NULL, controlCenter,NULL);
    
    
    srand(0); // set the seed for random number generator
    while(difftime(time(0), start_time) < simulation_time); // wait until the simulation time is over
    for (int i = 0; i < numberOfSections; i++) {
        pthread_cancel(sectionsThread[i]); // cancel the section threads
    }
    pthread_cancel(controlCenterThread); // cancel the control center thread

    pthread_create(&controlLogThread, NULL, controlLog, NULL); // create control log thread after simulation is over
    
    arg_struct logArguments;
    logArguments.p = p;
    logArguments.simulation_time = simulation_time;

    // Create train log thread
    pthread_create(&trainLogThread, NULL, trainLog, (void*)&logArguments);

    // Join the threads when they are finished
    pthread_join(controlLogThread, NULL);
    pthread_join(trainLogThread, NULL);

    // Destroy the mutexes
    pthread_mutex_destroy(&tunnelMutex);
    pthread_mutex_destroy(&trainMutex);
    if (debug) {
        pthread_mutex_destroy(&coutMutex);
    }

    return 0;
}
