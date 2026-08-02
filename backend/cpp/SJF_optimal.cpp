#include<iostream>
#include<vector>
#include<algorithm>
#include<queue>
using namespace std;

// ============================================================
// BACKEND INTEGRATION NOTE:
//
// ProcessInput is the "contract" between FastAPI and this C++
// scheduler. When the user submits processes from the frontend,
// FastAPI will serialize them as JSON, and this program will
// deserialize that JSON into a vector<ProcessInput>.
// ============================================================
struct ProcessInput{
    int id;
    int arrivalTime;
    int burstTime;
};

// ============================================================
// GanttSegment represents one contiguous block of execution
// on the CPU by a single process. SJF is non-preemptive, so
// each process produces exactly ONE segment.
// ============================================================
struct GanttSegment{
    int processId;   // -1 is reserved for CPU idle time
    int startTime;
    int endTime;
};

// ============================================================
// ProcessResult is the per-process OUTPUT contract — serialized
// back to JSON for the frontend table.
// ============================================================
struct ProcessResult{
    int id;
    int arrivalTime;
    int burstTime;
    int completionTime;
    int turnaroundTime;
    int waitingTime;
    int responseTime;
};

// ============================================================
// SimulationResult bundles EVERYTHING one algorithm run
// produces. Shared return type across every scheduling
// algorithm, so FastAPI can treat them all uniformly when
// building the "compare all algorithms" response.
// ============================================================
struct SimulationResult{
    vector<ProcessResult> processResults;
    vector<GanttSegment> ganttChart;

    int contextSwitches;

    float averageWaitingTime;
    float averageTurnaroundTime;
    float averageResponseTime;

    float cpuUtilization;
    float throughput;

    int totalTime;
};

// Internal working struct — UNCHANGED from your version.
struct Process{
    int id;
    int arrivalTime;
    int burstTime;
    int completionTime;
    int turnaroundTime;
    int waitingTime;
    int responseTime;
};

// Min Heap stores {Burst Time, Arrival Time, Index}
// Steps:
// 1. Sort processes by Arrival Time.
// 2. Push all arrived processes into the Min Heap, On basis of shortest Burst Time.
// 3. Pop the process with the shortest Burst Time.
// 4. Execute it completely (Non-Preemptive).
// 5. Repeat until all processes are completed.
//
//Time Complexity:
//
// Sorting processes by Arrival Time       -> O(n log n)
// Each process is pushed into Heap once   -> O(n log n)
// Each process is popped from Heap once   -> O(n log n)
//
// Overall Time Complexity = O(n log n)
//
// ============================================================
// BACKEND NOTE: This function now ALSO records Gantt segments
// and idle time in `ganttChart` (needed for the frontend chart
// and for computing CPU utilization / context switches later).
// The original selection logic is completely untouched.
// ============================================================
void findCompletionTime(vector<Process>& processes, vector<GanttSegment>& ganttChart){

    int n = processes.size();

    // Sort by Arrival Time
    sort(processes.begin(), processes.end(),
    [](Process &a, Process &b){

        if(a.arrivalTime == b.arrivalTime)
            return a.id < b.id;

        return a.arrivalTime < b.arrivalTime;
    });

    // {Burst Time, Arrival Time, Index}    // compare by Burst Time, then Arrival Time, then Index
    priority_queue<
        pair<pair<int,int>,int>,
        vector<pair<pair<int,int>,int>>,
        greater<pair<pair<int,int>,int>>
    > pq;

    int currentTime = 0;
    int completed = 0;
    int i = 0;

    while(completed < n){

        // Push all processes that have arrived
        while(i < n && processes[i].arrivalTime <= currentTime){

            pq.push({{processes[i].burstTime,
                      processes[i].arrivalTime}, i});

            i++;
        }

        // CPU Idle
        if(pq.empty()){

            // Record the idle gap as its own Gantt segment
            // (processId = -1 signals "no process running").
            ganttChart.push_back({-1, currentTime, processes[i].arrivalTime});

            currentTime = processes[i].arrivalTime;
            continue;
        }

        // Select process with shortest Burst Time
        int idx = pq.top().second;
        pq.pop();

        // RT = Start Time - Arrival Time
        processes[idx].responseTime =
            currentTime - processes[idx].arrivalTime;

        int startTime = currentTime;

        currentTime += processes[idx].burstTime;

        processes[idx].completionTime = currentTime;

        // Non-preemptive -> exactly one Gantt segment per process.
        ganttChart.push_back({processes[idx].id, startTime, currentTime});

        completed++;
    }
}

// TAT = CT - AT
void findTurnaroundTime(vector<Process>& processes){

    for(auto &process : processes){

        process.turnaroundTime =
        process.completionTime - process.arrivalTime;
    }
}

// WT = TAT - BT
void findWaitingTime(vector<Process>& processes){

    for(auto &process : processes){

        process.waitingTime =
        process.turnaroundTime - process.burstTime;
    }
}

// RT = WT (Non-Preemptive SJF)
void findResponseTime(vector<Process>& processes){

    for(auto &process : processes)
        process.responseTime = process.waitingTime;
}

// ============================================================
// findAverageTimes now WRITES into the SimulationResult instead
// of printing with cout, since the FastAPI-facing version of
// this program can never print — only the returned struct is
// visible to the caller.
// ============================================================
void findAverageTimes(vector<Process>& processes, SimulationResult& result){

    int totalTAT = 0;
    int totalWT = 0;
    int totalRT = 0;

    for(auto &process : processes){

        totalTAT += process.turnaroundTime;
        totalWT += process.waitingTime;
        totalRT += process.responseTime;
    }

    result.averageTurnaroundTime = (float)totalTAT / processes.size();
    result.averageWaitingTime = (float)totalWT / processes.size();
    result.averageResponseTime = (float)totalRT / processes.size();
}

// ============================================================
// NEW: computeContextSwitches
//
// A context switch happens every time the CPU moves from
// running one process to running a DIFFERENT process. Idle
// gaps are not counted as switches.
//
// Data structure: simple linear scan over ganttChart (a vector),
// since Gantt segments are already in chronological order.
// Time Complexity: O(g), where g = number of Gantt segments.
// ============================================================
int computeContextSwitches(const vector<GanttSegment>& ganttChart){

    int contextSwitches = 0;
    int lastRunningProcessId = -1;

    for(auto &segment : ganttChart){

        if(segment.processId == -1)
            continue; // idle segment, ignore

        if(lastRunningProcessId != -1 && lastRunningProcessId != segment.processId)
            contextSwitches++;

        lastRunningProcessId = segment.processId;
    }

    return contextSwitches;
}

// ============================================================
// NEW: computeUtilizationAndThroughput
//
// cpuUtilization = (total busy time / total elapsed time) * 100
// throughput     = number of processes completed / total elapsed time
// ============================================================
void computeUtilizationAndThroughput(vector<Process>& processes, SimulationResult& result){

    int totalBurstTime = 0;
    int totalTime = 0;

    for(auto &process : processes){

        totalBurstTime += process.burstTime;
        totalTime = max(totalTime, process.completionTime);
    }

    result.totalTime = totalTime;
    result.cpuUtilization = ((float)totalBurstTime / totalTime) * 100.0f;
    result.throughput = (float)processes.size() / totalTime;
}

// ============================================================
// runSJFScheduling is the ONE function FastAPI will call (via
// the compiled executable) for this algorithm.
//
// Input : vector<ProcessInput>  -> comes from deserialized JSON
// Output: SimulationResult      -> gets serialized back to JSON
//
// No cin, no cout — pure function.
// ============================================================
SimulationResult runSJFScheduling(const vector<ProcessInput>& processInputs){

    int n = processInputs.size();

    // Convert ProcessInput -> internal working Process struct
    vector<Process> processes(n);

    for(int i = 0; i < n; i++){
        processes[i].id = processInputs[i].id;
        processes[i].arrivalTime = processInputs[i].arrivalTime;
        processes[i].burstTime = processInputs[i].burstTime;
    }

    SimulationResult result;

    findCompletionTime(processes, result.ganttChart);
    findTurnaroundTime(processes);
    findWaitingTime(processes);
    findResponseTime(processes);

    findAverageTimes(processes, result);

    result.contextSwitches = computeContextSwitches(result.ganttChart);
    computeUtilizationAndThroughput(processes, result);

    // Sort back by Process ID for a stable, predictable output
    // order (frontend table should list Process 1, 2, 3... not
    // heap-selection order).
    sort(processes.begin(), processes.end(),
    [](Process &a, Process &b){
        return a.id < b.id;
    });

    for(auto &process : processes){
        ProcessResult processResult;

        processResult.id = process.id;
        processResult.arrivalTime = process.arrivalTime;
        processResult.burstTime = process.burstTime;
        processResult.completionTime = process.completionTime;
        processResult.turnaroundTime = process.turnaroundTime;
        processResult.waitingTime = process.waitingTime;
        processResult.responseTime = process.responseTime;

        result.processResults.push_back(processResult);
    }

    return result;
}

// ============================================================
// printProcessDetails is now ONLY used for local terminal
// testing in main() below. FastAPI never calls this.
// ============================================================
void printProcessDetails(const SimulationResult& result){

    cout<<"Process ID\tArrival\tBurst\tCT\tTAT\tWT\tRT\n";

    for(auto &process : result.processResults){

        cout<<process.id<<"\t\t"
            <<process.arrivalTime<<"\t"
            <<process.burstTime<<"\t"
            <<process.completionTime<<"\t"
            <<process.turnaroundTime<<"\t"
            <<process.waitingTime<<"\t"
            <<process.responseTime<<endl;
    }

    cout<<"\nContext Switches: "<<result.contextSwitches<<endl;
    cout<<"CPU Utilization: "<<result.cpuUtilization<<"%"<<endl;
    cout<<"Throughput: "<<result.throughput<<" processes/unit time"<<endl;
    cout<<"Average Turnaround Time: "<<result.averageTurnaroundTime<<endl;
    cout<<"Average Waiting Time: "<<result.averageWaitingTime<<endl;
    cout<<"Average Response Time: "<<result.averageResponseTime<<endl;

    cout<<"\nGantt Chart:\n";
    for(auto &segment : result.ganttChart){
        if(segment.processId == -1)
            cout<<"[IDLE: "<<segment.startTime<<" - "<<segment.endTime<<"] ";
        else
            cout<<"[P"<<segment.processId<<": "<<segment.startTime<<" - "<<segment.endTime<<"] ";
    }
    cout<<endl;
}

// main() is now JUST a local test harness — the ONLY place
// cin/cout are still allowed. It builds ProcessInput manually,
// mimicking what FastAPI would eventually send in.
int main(){

    int n;

    cout<<"Enter number of processes: ";
    cin>>n;

    vector<ProcessInput> processInputs(n);

    for(int i=0;i<n;i++){

        processInputs[i].id = i+1;

        cout<<"Enter Arrival Time and Burst Time for Process "
            <<i+1<<": ";

        cin>>processInputs[i].arrivalTime
           >>processInputs[i].burstTime;
    }

    SimulationResult result = runSJFScheduling(processInputs);

    printProcessDetails(result);

    return 0;
}
