#include <iostream>
#include <vector>
#include <algorithm>
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
};

// ============================================================
// GanttSegment represents one contiguous block of execution
// on the CPU by a single process.
//
// FCFS is non-preemptive, so each process produces exactly ONE
// segment (it runs start-to-finish once selected). We still
// need to record idle gaps (processId = -1) between processes,
// since CPU utilization / throughput depend on total idle time.
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
// Kept separate from ProcessInput/ProcessResult; it exists
// only for the algorithm's own bookkeeping while it runs.
struct Process {
    int id;
    int arrivalTime;
    int burstTime;
    int completionTime;
    int turnaroundTime;
    int waitingTime;
    int responseTime;
};

// CT = Start Time + Burst Time
//
// ============================================================
// BACKEND NOTE: This function now ALSO records Gantt segments
// and idle time in `ganttChart`, since that data is needed for
// the frontend chart AND for computing CPU utilization /
// context switches later. The original CT math is untouched —
// we just also note down the start time before writing CT.
// ============================================================
void findCompletionTime(vector<Process>& processes, vector<GanttSegment>& ganttChart) {

    // First process: if it doesn't arrive at time 0, the CPU
    // sits idle from 0 until it arrives. Record that idle gap.
    if (processes[0].arrivalTime > 0) {
        ganttChart.push_back({-1, 0, processes[0].arrivalTime});
    }

    int startTime = processes[0].arrivalTime;
    processes[0].completionTime = startTime + processes[0].burstTime;
    ganttChart.push_back({processes[0].id, startTime, processes[0].completionTime});

    for(int i = 1; i < processes.size(); i++) {

        int currentStartTime = max(processes[i - 1].completionTime, processes[i].arrivalTime);

        // If this process's start time is later than the previous
        // process's completion time, the CPU was idle in between.
        if (currentStartTime > processes[i - 1].completionTime) {
            ganttChart.push_back({-1, processes[i - 1].completionTime, currentStartTime});
        }

        processes[i].completionTime = currentStartTime + processes[i].burstTime;

        ganttChart.push_back({processes[i].id, currentStartTime, processes[i].completionTime});
    }
}

// TAT = CT - AT
void findturnaroundTime(vector<Process>& processes) {
    for (auto& process : processes) {
        process.turnaroundTime = process.completionTime - process.arrivalTime;
    }
}

// WT = TAT - BT
void findWaitingTime(vector<Process>& processes) {
    for (auto& process : processes) {
        process.waitingTime = process.turnaroundTime - process.burstTime;
    }
}

// RT = WT (In FCFS, response time is equal to waiting time)
void findResponseTime(vector<Process>& processes) {
    for (auto& process : processes) {
        process.responseTime = process.waitingTime; // In FCFS, response time is equal to waiting time
    }
}

// ============================================================
// findAverageTimes now WRITES into the SimulationResult instead
// of printing with cout. Printing is fine for local testing in
// main(), but the version FastAPI calls must never print — it
// only has access to the returned struct.
// ============================================================
void findAverageTimes(vector<Process>& processes, SimulationResult& result) {
    int totalTurnaroundTime = 0;
    int totalWaitingTime = 0;
    int totalResponseTime = 0;

    for (const auto& process : processes) {
        totalTurnaroundTime += process.turnaroundTime;
        totalWaitingTime += process.waitingTime;
        totalResponseTime += process.responseTime;
    }

    result.averageTurnaroundTime = (float)totalTurnaroundTime / processes.size();
    result.averageWaitingTime = (float)totalWaitingTime / processes.size();
    result.averageResponseTime = (float)totalResponseTime / processes.size();
}

// ============================================================
// NEW: computeContextSwitches
//
// A context switch happens every time the CPU moves from
// running one process to running a DIFFERENT process.
// Idle gaps are not counted as switches.
//
// Data structure: simple linear scan over ganttChart (a vector),
// since Gantt segments are already in chronological order.
// Time Complexity: O(g), where g = number of Gantt segments.
// ============================================================
int computeContextSwitches(const vector<GanttSegment>& ganttChart) {
    int contextSwitches = 0;
    int lastRunningProcessId = -1;

    for (auto& segment : ganttChart) {
        if (segment.processId == -1)
            continue; // idle segment, ignore

        if (lastRunningProcessId != -1 && lastRunningProcessId != segment.processId)
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
void computeUtilizationAndThroughput(vector<Process>& processes, SimulationResult& result) {
    int totalBurstTime = 0;
    int totalTime = 0;

    for (auto& process : processes) {
        totalBurstTime += process.burstTime;
        totalTime = max(totalTime, process.completionTime);
    }

    result.totalTime = totalTime;
    result.cpuUtilization = ((float)totalBurstTime / totalTime) * 100.0f;
    result.throughput = (float)processes.size() / totalTime;
}

// ============================================================
// runFCFSScheduling is the ONE function FastAPI will call (via
// the compiled executable) for this algorithm.
//
// Input : vector<ProcessInput>  -> comes from deserialized JSON
// Output: SimulationResult      -> gets serialized back to JSON
//
// No cin, no cout — pure function.
// ============================================================
SimulationResult runFCFSScheduling(vector<ProcessInput> processInputs) {

    // Sort processes by arrival time -> because FCFS scheduling is based on the order of arrival
    sort(processInputs.begin(), processInputs.end(),
    [](ProcessInput &a, ProcessInput &b){

        if(a.arrivalTime == b.arrivalTime)
            return a.id < b.id;

        return a.arrivalTime < b.arrivalTime;
    });

    int n = processInputs.size();

    // Convert ProcessInput -> internal working Process struct
    vector<Process> processes(n);

    for (int i = 0; i < n; i++) {
        processes[i].id = processInputs[i].id;
        processes[i].arrivalTime = processInputs[i].arrivalTime;
        processes[i].burstTime = processInputs[i].burstTime;
    }

    SimulationResult result;

    findCompletionTime(processes, result.ganttChart);
    findturnaroundTime(processes);
    findWaitingTime(processes);
    findResponseTime(processes);

    findAverageTimes(processes, result);

    result.contextSwitches = computeContextSwitches(result.ganttChart);
    computeUtilizationAndThroughput(processes, result);

    // Convert internal Process -> ProcessResult (output contract).
    // Processes are already in arrival order here, which is fine
    // for FCFS since arrival order == id order in typical test data,
    // but we still sort by id to guarantee a predictable table order.
    sort(processes.begin(), processes.end(),
    [](Process &a, Process &b){
        return a.id < b.id;
    });

    for (auto& process : processes) {
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
// testing in main() below. FastAPI never calls this — it only
// ever sees the SimulationResult struct.
// ============================================================
void printProcessDetails(const SimulationResult& result) {
    cout << "Process ID\tArrival Time\tBurst Time\tCompletion Time\tTurnaround Time\tWaiting Time\tResponse Time\n";
    for (const auto& process : result.processResults) {
        cout << process.id << "\t\t"
             << process.arrivalTime << "\t\t"
             << process.burstTime << "\t\t"
             << process.completionTime << "\t\t"
             << process.turnaroundTime << "\t\t"
             << process.waitingTime << "\t\t"
             << process.responseTime << endl;
    }

    cout << "\nContext Switches: " << result.contextSwitches << endl;
    cout << "CPU Utilization: " << result.cpuUtilization << "%" << endl;
    cout << "Throughput: " << result.throughput << " processes/unit time" << endl;
    cout << "Average Turnaround Time: " << result.averageTurnaroundTime << endl;
    cout << "Average Waiting Time: " << result.averageWaitingTime << endl;
    cout << "Average Response Time: " << result.averageResponseTime << endl;

    cout << "\nGantt Chart:\n";
    for (auto& segment : result.ganttChart) {
        if (segment.processId == -1)
            cout << "[IDLE: " << segment.startTime << " - " << segment.endTime << "] ";
        else
            cout << "[P" << segment.processId << ": " << segment.startTime << " - " << segment.endTime << "] ";
    }
    cout << endl;
}

// main() is now JUST a local test harness — this is the ONLY
// place cin/cout are still allowed. It builds ProcessInput
// manually, mimicking what FastAPI would eventually send in.
int main(){

    int n;
    cout << "Enter the number of processes: ";
    cin >> n;

    vector<ProcessInput> processInputs(n);
    for (int i = 0; i < n; i++) {
        processInputs[i].id = i + 1;
        cout << "Enter arrival time and burst time for process " << processInputs[i].id << ": ";
        cin >> processInputs[i].arrivalTime >> processInputs[i].burstTime;
    }

    SimulationResult result = runFCFSScheduling(processInputs);

    printProcessDetails(result);

    return 0;
}
