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
//
// Keeping this struct separate from the internal `Process`
// struct (below) means the algorithm's internal working data
// (completionTime, turnaroundTime, etc.) never leaks into the
// input contract. Input and output are cleanly separated.
// ============================================================
struct ProcessInput{
    int id;
    int arrivalTime;
    int burstTime;
    int priority;
};

// ============================================================
// GanttSegment represents one contiguous block of execution
// on the CPU by a single process.
//
// For non-preemptive Priority scheduling, each process produces
// exactly ONE segment, since it runs to completion once selected.
// ============================================================
struct GanttSegment{
    int processId;   // -1 is reserved for CPU idle time
    int startTime;
    int endTime;
};

// ============================================================
// ProcessResult is the per-process OUTPUT contract — this is
// what gets serialized back to JSON and sent to the frontend
// table (Process ID / CT / TAT / WT / RT columns).
// ============================================================
struct ProcessResult{
    int id;
    int arrivalTime;
    int burstTime;
    int priority;
    int completionTime;
    int turnaroundTime;
    int waitingTime;
    int responseTime;
};

// ============================================================
// SimulationResult bundles EVERYTHING one algorithm run
// produces. This is the single return type shared by every
// scheduling algorithm (FCFS, SJF, RR, etc.) in the whole
// project.
//
// Why this matters architecturally:
// -> FastAPI's "compare all algorithms" endpoint will just
//    call runFCFS(), runSJF(), runPriorityScheduling(), etc.
//    in a loop, collect a vector<SimulationResult>, and hand
//    it straight to the frontend. No algorithm-specific
//    handling needed at the API layer.
// ============================================================
struct SimulationResult{
    vector<ProcessResult> processResults;
    vector<GanttSegment> ganttChart;

    int contextSwitches;

    float averageWaitingTime;
    float averageTurnaroundTime;
    float averageResponseTime;

    float cpuUtilization;   // percentage of total time CPU was busy
    float throughput;       // processes completed per unit time

    int totalTime;          // total simulation time (last completion time)
};

// Internal working struct — UNCHANGED from your version.
// This is intentionally kept separate from ProcessInput/ProcessResult;
// it exists only for the algorithm's own bookkeeping while it runs.
struct Process{
    int id;
    int arrivalTime;
    int burstTime;
    int priority;

    int completionTime;
    int turnaroundTime;
    int waitingTime;
    int responseTime;
};

// Min Heap stores {Priority, Arrival Time, Index}
//
// Algorithm:
// 1. Sort processes by Arrival Time.
// 2. Push all arrived processes into the Min Heap.
// 3. Pick the process with the highest Priority.
// 4. Execute it completely.
// 5. Repeat until all processes finish.
//
// Time Complexity:
//
// Sorting  : O(n log n)
// Heap Push: O(n log n)
// Heap Pop : O(n log n)
//
// Overall  : O(n log n)
//
// ============================================================
// BACKEND NOTE: This function now ALSO records Gantt segments
// and idle time, since ganttChart += those events is needed
// for the frontend chart AND for computing CPU utilization /
// context switches later. Everything else about your original
// logic is untouched.
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

    // {Priority, Arrival Time, Index}   // Compare by Priority, then Arrival Time, then Index
    priority_queue<
        pair<pair<int,int>,int>,
        vector<pair<pair<int,int>,int>>,
        greater<pair<pair<int,int>,int>>
    > pq;

    int currentTime = 0;
    int completed = 0;
    int i = 0;

    while(completed < n){

        // Add all arrived processes
        while(i < n && processes[i].arrivalTime <= currentTime){

            pq.push({{processes[i].priority, processes[i].arrivalTime}, i});

            i++;
        }

        // CPU is idle
        if(pq.empty()){

            // Record the idle gap as its own Gantt segment
            // (processId = -1 signals "no process running").
            // Needed so the frontend chart shows gaps accurately
            // and CPU utilization accounts for idle time correctly.
            ganttChart.push_back({-1, currentTime, processes[i].arrivalTime});

            currentTime = processes[i].arrivalTime;
            continue;
        }

        // Select highest priority process
        int idx = pq.top().second;
        pq.pop();

        processes[idx].responseTime =
            currentTime - processes[idx].arrivalTime;

        int startTime = currentTime;

        // Execute process
        currentTime += processes[idx].burstTime;

        processes[idx].completionTime = currentTime;

        // Non-preemptive -> exactly one Gantt segment per process,
        // spanning its full burst time in one shot.
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

// RT = WT (Non-Preemptive Priority)
void findResponseTime(vector<Process>& processes){

    for(auto &process : processes)
        process.responseTime = process.waitingTime;
}

// ============================================================
// findAverageTimes now RETURNS values into the SimulationResult
// instead of printing with cout. Printing is fine for local
// testing in main(), but the version FastAPI calls must never
// print — it only has access to the returned struct.
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
// running one process to running a DIFFERENT process.
// Idle gaps are not counted as switches — the CPU isn't
// "switching to another process", it's just sitting empty.
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
//
// totalTime is taken as the LAST completion time across all
// processes (i.e., when the simulation actually ends).
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
// runPriorityScheduling is the ONE function FastAPI will call
// (via the compiled executable) for this algorithm.
//
// Input : vector<ProcessInput>  -> comes from deserialized JSON
// Output: SimulationResult      -> gets serialized back to JSON
//
// No cin, no cout — pure function. This is what makes it
// callable from an automated backend instead of a human typing
// into a terminal.
// ============================================================
SimulationResult runPriorityScheduling(const vector<ProcessInput>& processInputs){

    int n = processInputs.size();

    // Convert ProcessInput -> internal working Process struct
    vector<Process> processes(n);

    for(int i = 0; i < n; i++){

        processes[i].id = processInputs[i].id;
        processes[i].arrivalTime = processInputs[i].arrivalTime;
        processes[i].burstTime = processInputs[i].burstTime;
        processes[i].priority = processInputs[i].priority;
    }

    SimulationResult result;

    findCompletionTime(processes, result.ganttChart);
    findTurnaroundTime(processes);
    findWaitingTime(processes);
    findResponseTime(processes);

    findAverageTimes(processes, result);

    result.contextSwitches = computeContextSwitches(result.ganttChart);
    computeUtilizationAndThroughput(processes, result);

    // Sort back by Process ID for a stable, predictable output order
    // (frontend table should list Process 1, 2, 3... not heap order)
    sort(processes.begin(), processes.end(),
    [](Process &a, Process &b){

        return a.id < b.id;
    });

    // Convert internal Process -> ProcessResult (output contract)
    for(auto &process : processes){

        ProcessResult processResult;

        processResult.id = process.id;
        processResult.arrivalTime = process.arrivalTime;
        processResult.burstTime = process.burstTime;
        processResult.priority = process.priority;
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
// testing in main() below. FastAPI never calls this — it only
// ever sees the SimulationResult struct.
// ============================================================
void printProcessDetails(const SimulationResult& result){

    cout<<"Process ID\tArrival\tBurst\tPriority\tCT\tTAT\tWT\tRT\n";

    for(auto &process : result.processResults){

        cout<<process.id<<"\t\t"
            <<process.arrivalTime<<"\t"
            <<process.burstTime<<"\t"
            <<process.priority<<"\t\t"
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

// main() is now JUST a local test harness — this is the ONLY
// place cin/cout are still allowed. It builds ProcessInput
// manually instead of calling runPriorityScheduling() directly
// with hardcoded/user-typed data, mimicking what FastAPI would
// eventually send in.
int main(){

    int n;

    cout<<"Enter number of processes: ";
    cin>>n;

    vector<ProcessInput> processInputs(n);

    for(int i=0;i<n;i++){

        processInputs[i].id = i+1;

        cout<<"Enter Arrival Time, Burst Time and Priority for Process "
            <<i+1<<": ";

        cin>>processInputs[i].arrivalTime
           >>processInputs[i].burstTime
           >>processInputs[i].priority;
    }

    SimulationResult result = runPriorityScheduling(processInputs);

    printProcessDetails(result);

    return 0;
}
