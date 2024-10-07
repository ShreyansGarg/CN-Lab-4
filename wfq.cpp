// Rinnegan
#include <bits/stdc++.h>
using namespace std;

int priority()
{
    int j = rand() % 2;
    return j + 1;
}
int bursty()
{
    int j = rand() % 2;
    return j * 10;
}
int see;
// Constants
const int NUM_PORTS = 8;         // Number of input/output ports
const int BUFFER_SIZE = 64;      // Buffer size per input/output port
const int SIMULATION_TIME = 400; // Simulation time units
const int TRAFFIC_INTERVAL = 4;  // Interval for generating traffic
// Packet structure
struct Packet
{
    int id;          // Packet ID
    int priority;    // Priority of the packet (0 for low, 1 for high)
    int arrivalTime; // Time when packet arrives
    int inputPort;   // The input port from which the packet arrived
    int outputPort;  // The output port to which the packet is directed
    double virtualFinish;
};

class RouterSwitchFabric
{
private:
    vector<vector<queue<Packet>>> InputQueues;
    // vector<double>LastVirtualFinish;
    vector<double> LastVirtualFinish_Low;
    vector<double> LastVirtualFinish_High;
    vector<queue<Packet>> outputQueues; // Output queues (one per output port)
    double current;
    int packetCount;
    int packetDropped;

    // Statistics
    int totalPacketsProcessed;
    int totalTurnaroundTime;
    int totalWaitingTime;
    int totalBufferOccupancy;

    // Mutex and condition variable for thread synchronization
    mutex mtx;

public:
    RouterSwitchFabric() : InputQueues(NUM_PORTS, vector<queue<Packet>>(NUM_PORTS)), outputQueues(NUM_PORTS), LastVirtualFinish_Low(NUM_PORTS), LastVirtualFinish_High(NUM_PORTS), current(0)
    {
        packetCount = 0;
        packetDropped = 0;
        totalPacketsProcessed = 0;
        totalTurnaroundTime = 0;
        totalWaitingTime = 0;
        totalBufferOccupancy = 0;
    }

    // Generate packets for input queues with variable traffic conditions
    void generateTraffic(int time, bool uniformTraffic, bool nonUniformTraffic, bool burstyTraffic)
    {
        while (time < SIMULATION_TIME)
        {
            {
                unique_lock<mutex> lock(mtx);
                for (int i = 0; i < NUM_PORTS; i++)
                {

                    int numPackets = 4;
                    if (burstyTraffic && i < 4)
                    {
                        numPackets = bursty();
                    }
                    // Create new packets and add them to input queues
                    for (int j = 0; j < numPackets; ++j)
                    {
                        Packet p;
                        see++;
                        p.id = packetCount++;
                        if (nonUniformTraffic && i < 4)
                            p.priority = priority();
                        else
                            p.priority = 1; // Higher priority for ports 0-3
                        p.arrivalTime = time;
                        p.inputPort = i;
                        p.outputPort = rand() % NUM_PORTS;
                        // debug(p.outputPort)
                        int virtualStart;
                        if (p.priority == 1)
                        {
                            virtualStart = max(current, LastVirtualFinish_Low[i]);
                            p.virtualFinish = virtualStart + 36 / double(i + 1);
                            LastVirtualFinish_Low[i] = p.virtualFinish;
                        }
                        else
                        {
                            virtualStart = max(current, LastVirtualFinish_High[i]);
                            p.virtualFinish = virtualStart + 36 / double(i + 1);
                            LastVirtualFinish_High[i] = p.virtualFinish;
                        }
                        if (InputQueues[i][p.outputPort].size() < BUFFER_SIZE)
                        {
                            InputQueues[i][p.outputPort].push(p);
                        }
                        else
                        {
                            packetDropped++; // Drop packet if buffer is full
                        }
                    }
                }
            }

            this_thread::sleep_for(chrono::milliseconds(TRAFFIC_INTERVAL));
            time += TRAFFIC_INTERVAL;
        }
    }

    // Simulate packet processing for priority scheduling
    void processPackets(int time)
    {

        while (time < SIMULATION_TIME)
        {
            set<int> s;
            unique_lock<mutex> lock(mtx); // Lock mutex for thread safety
            for (int i = 0; i < NUM_PORTS; i++)
            {
                double MinVirFinish = 1e9;
                int queuNum = -1;
                int priority_send = -1;

                for (int j = 0; j < NUM_PORTS; j++)
                {
                    if (s.find(j) != s.end())
                        continue;
                    if (!InputQueues[j][i].empty())
                    {
                        Packet p = InputQueues[j][i].front();
                        if ((p.virtualFinish < MinVirFinish) || (p.priority > priority_send))
                        {
                            MinVirFinish = p.virtualFinish;
                            queuNum = j;
                            priority_send = p.priority;
                        }
                    }
                }
                if (queuNum != -1)
                {
                    Packet send = InputQueues[queuNum][i].front();
                    s.insert(queuNum);
                    InputQueues[queuNum][i].pop();
                    if (outputQueues[i].size() < BUFFER_SIZE)
                    {
                        outputQueues[i].push(send);
                        // Update statistics
                        totalPacketsProcessed++;
                        int turnaroundTime = time - send.arrivalTime;
                        totalTurnaroundTime += turnaroundTime;
                        totalWaitingTime += turnaroundTime - 1; // Assume 1 unit time for processing
                        totalBufferOccupancy += outputQueues[i].size();
                        // Update the current virtual time based on the virtual finish of the processed packet
                        current = max(current, send.virtualFinish); // Update virtual time
                    }
                    else
                    {
                        packetDropped++; // Drop packet if output buffer is full
                    }
                }
            }

            if (time % 4 == 1)
            {
                for (int i = 0; i < NUM_PORTS; i++)
                {
                    if (!outputQueues[i].empty())
                    {
                        cout << i << "---->" << outputQueues[i].size() << " ";
                        outputQueues[i].pop();
                    }
                }
                cout << endl;
            }
            this_thread::sleep_for(chrono::milliseconds(1));
            time += 1;
        }
    }

    // Print statistics for the simulation
    void printStatistics(int time)
    {
        cout << "Simulation Time: " << time << endl;
        cout << "Total Packets Processed: " << totalPacketsProcessed << endl;
        cout << "Total Packets Dropped: " << packetDropped << endl;
        cout << "Average Turnaround Time: " << (totalPacketsProcessed > 0 ? (totalTurnaroundTime / totalPacketsProcessed) : 0) << endl;
        cout << "Average Waiting Time: " << (totalPacketsProcessed > 0 ? (totalWaitingTime / totalPacketsProcessed) : 0) << endl;
        cout << "Average Buffer Occupancy: " << (totalPacketsProcessed > 0 ? (totalBufferOccupancy / totalPacketsProcessed) : 0) << endl;
        // cout << "Packet Drop Rate: " <<((packetDropped*100)/(packetDropped+totalPacketsProcessed));
    }
    void display()
    {
        for (int i = 0; i < NUM_PORTS; i++)
        {
            for (int j = 0; j < NUM_PORTS; j++)
            {
                cout << InputQueues[i][j].size() << " ";
            }
            cout << endl;
        }
        for (int i = 0; i < NUM_PORTS; i++)
        {
            cout << outputQueues[i].size() << " ";
        }
    }
};

signed main()
{
    freopen("output.txt", "w", stdout);
    freopen("error.txt", "w", stderr);
    srand(time(0)); // Seed random number generator

    // Simulation conditions: uniform, non-uniform, bursty traffic
    bool uniformTraffic = false;
    bool nonUniformTraffic = false;
    bool burstyTraffic = true;

    // Create router switch fabric
    RouterSwitchFabric router;

    // Threads for generating traffic and processing packets
    thread trafficThread([&router, &uniformTraffic, &nonUniformTraffic, &burstyTraffic]()
                         {
        int time = 0;
        router.generateTraffic(time, uniformTraffic, nonUniformTraffic, burstyTraffic); });

    thread processingThread([&router]()
                            {
        int time = 0;
        router.processPackets(time); });

    // Run both threads
    trafficThread.join();
    processingThread.join();

    // Print the final statistics after the simulation
    router.printStatistics(SIMULATION_TIME);
    // debug(see)
    router.display();

    return 0;
}
