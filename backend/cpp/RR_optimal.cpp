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
// Round Robin ALSO needs a Time Quantum from the user, which is
// why runRoundRobinScheduling (below) takes an extra parameter
// beyond just the process list — this mirrors how the FastAPI
// endpoint for RR will need an extra "timeQuantum" field in its
// request body, unlike the other algorithms.
// ============================================================
struct ProcessInput{
    int id;
    int arrivalTime;
    int burstTime;
};

// ============================================================
// GanttSegment represents one contiguous block of execution on
// the CPU by a single process.
//
// Round Robin is PREEMPTIVE via Time Quantum — a process runs
// for at most `timeQuantum` units before being sent to the back
// of the queue. A process can appear in MULTIPLE Gantt segments
// across the timeline (once per quantum slice it gets). We merge
// consecutive slices of the SAME process into a single segment
// (see appendGanttSegment below) in the rare case where a process
// happens to get the CPU back-to-back with no other process in
// between (e.g. only one process left in the ready queue).
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
    int remainingTime;

    int completionTime;
    int turnaroundTime;
    int waitingTime;
    int responseTime;
};

// ============================================================
// appendGanttSegment: helper used only by preemptive algorithms.
//
// Even though RR naturally executes in quantum-sized chunks
// (not 1 unit at a time like SRTF/LRTF), the SAME process can
// still run in two "back-to-back" chunks if it's the only
// process left in the ready queue near the end of the
// simulation. Merging keeps the Gantt chart clean instead of
// showing two adjacent boxes for the same process.
//
// Data structure: operates on the back() of a vector — O(1).
// ============================================================
void appendGanttSegment(vector<GanttSegment>& ganttChart, int processId, int startTime, int endTime){

    if(!ganttChart.empty() &&
       ganttChart.back().processId == processId &&
       ganttChart.back().endTime == startTime){

        // Same process continued running with no gap -> extend it
        ganttChart.back().endTime = endTime;
    }
    else{

        // Different process (or a fresh idle gap) -> new segment
        ganttChart.push_back({processId, startTime, endTime});
    }
}

// Queue stores the Index of Ready Processes
//
// Algorithm:
// 1. Sort processes by Arrival Time.
// 2. Push the first arrived process into the Queue.  (PQ is not used here, as we are executing processes in the order they arrive, thus we can use a simple Queue)
// 3. Execute it for Time Quantum.
// 4. Add newly arrived processes.
// 5. If process is unfinished, push it back.
// 6. Repeat until all processes finish.
//
// Time Complexity:
//
// Sorting : O(n log n)
// Queue Operations : O(m)
//
// Overall : O(n log n + m)
//
// m = Total CPU execution units
//
// ============================================================
// BACKEND NOTE: This function now ALSO records Gantt segments
// (via appendGanttSegment) for each quantum slice executed,
// needed for the frontend chart and for computing CPU
// utilization / context switches later. The original queue
// rotation logic is completely untouched.
// ============================================================
void findCompletionTime(vector<Process>& processes,int timeQuantum, vector<GanttSegment>& ganttChart){

    int n = processes.size();

    // Sort by Arrival Time
    sort(processes.begin(),processes.end(),
    [](Process &a,Process &b){

        if(a.arrivalTime==b.arrivalTime)
            return a.id<b.id;

        return a.arrivalTime<b.arrivalTime;
    });

    // Initialize Remaining Time ; initially, Remaining Time = Burst Time
    for(int i=0;i<n;i++)
        processes[i].remainingTime = processes[i].burstTime;

    vector<bool> started(n,false);

    queue<int> q;

    int currentTime = 0;
    int completed = 0;
    int i = 0;

    // Add first arrived process
    currentTime = processes[0].arrivalTime;
    q.push(0);
    i = 1;

    // If the first process doesn't arrive at time 0, the CPU was
    // idle from 0 until it arrived. Record that idle gap.
    if(processes[0].arrivalTime > 0){
        appendGanttSegment(ganttChart, -1, 0, processes[0].arrivalTime);
    }

    while(completed<n){

        int idx = q.front();
        q.pop();

        // First time process gets CPU
        if(!started[idx]){

            started[idx]=true;

            processes[idx].responseTime = currentTime-processes[idx].arrivalTime;
        }

        int quantumStartTime = currentTime;

        // Execute for one Time Quantum
        int executionTime =
        min(timeQuantum,processes[idx].remainingTime);

        processes[idx].remainingTime -= executionTime;

        currentTime += executionTime;

        // Record this quantum slice on the Gantt chart.
        appendGanttSegment(ganttChart, processes[idx].id, quantumStartTime, currentTime);

        // Add newly arrived processes
        while(i<n && processes[i].arrivalTime<=currentTime){

            q.push(i);
            i++;
        }

        // Process completed
        if(processes[idx].remainingTime==0){

            processes[idx].completionTime=currentTime;
            completed++;
        }

        // Push back if unfinished
        else{

            q.push(idx);
        }

        // CPU Idle
        if(q.empty() && i<n){

            // Record the idle gap between currentTime and the
            // next process's arrival.
            appendGanttSegment(ganttChart, -1, currentTime, processes[i].arrivalTime);

            currentTime = processes[i].arrivalTime;

            q.push(i);
            i++;
        }
    }
}

// TAT = CT - AT
void findTurnaroundTime(vector<Process>& processes){

    for(auto &process:processes){

        process.turnaroundTime =
        process.completionTime-process.arrivalTime;
    }
}

// WT = TAT - BT
void findWaitingTime(vector<Process>& processes){

    for(auto &process:processes){

        process.waitingTime =
        process.turnaroundTime-process.burstTime;
    }
}

// ============================================================
// findAverageTimes now WRITES into the SimulationResult instead
// of printing with cout, since the FastAPI-facing version of
// this program can never print — only the returned struct is
// visible to the caller.
//
// Response Time average is included here too (it wasn't in your
// original findAverageTimes, but responseTime IS already being
// computed inline during the simulation above via `started[]`,
// so we just aggregate it here for the summary stats).
// ============================================================
void findAverageTimes(vector<Process>& processes, SimulationResult& result){

    int totalTAT=0;
    int totalWT=0;
    int totalRT=0;

    for(auto &process:processes){

        totalTAT+=process.turnaroundTime;
        totalWT+=process.waitingTime;
        totalRT+=process.responseTime;
    }

    result.averageTurnaroundTime = (float)totalTAT/processes.size();
    result.averageWaitingTime = (float)totalWT/processes.size();
    result.averageResponseTime = (float)totalRT/processes.size();
}

// ============================================================
// NEW: computeContextSwitches
//
// A context switch happens every time the CPU moves from
// running one process to running a DIFFERENT process. Idle
// gaps are not counted as switches.
//
// Because segments are already merged (appendGanttSegment),
// this is just: count how many times processId changes between
// consecutive non-idle segments.
//
// Data structure: linear scan over ganttChart (a vector).
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
// runRoundRobinScheduling is the ONE function FastAPI will call
// (via the compiled executable) for this algorithm.
//
// Note the extra `timeQuantum` parameter — this is the one
// algorithm in the whole project whose function signature
// differs from the others, since RR needs a user-supplied
// quantum. The FastAPI endpoint for RR will need to accept and
// forward this extra field, unlike the other algorithm endpoints.
//
// Input : vector<ProcessInput>, int timeQuantum
// Output: SimulationResult      -> gets serialized back to JSON
//
// No cin, no cout — pure function.
// ============================================================
SimulationResult runRoundRobinScheduling(const vector<ProcessInput>& processInputs, int timeQuantum){

    int n = processInputs.size();

    // Convert ProcessInput -> internal working Process struct
    vector<Process> processes(n);

    for(int i = 0; i < n; i++){
        processes[i].id = processInputs[i].id;
        processes[i].arrivalTime = processInputs[i].arrivalTime;
        processes[i].burstTime = processInputs[i].burstTime;
    }

    SimulationResult result;

    findCompletionTime(processes, timeQuantum, result.ganttChart);
    findTurnaroundTime(processes);
    findWaitingTime(processes);

    findAverageTimes(processes, result);

    result.contextSwitches = computeContextSwitches(result.ganttChart);
    computeUtilizationAndThroughput(processes, result);

    // Sort back by Process ID for a stable, predictable output
    // order (frontend table should list Process 1, 2, 3... not
    // queue-rotation order).
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

    int n,timeQuantum;

    cout<<"Enter number of processes: ";
    cin>>n;

    vector<ProcessInput> processInputs(n);

    for(int i=0;i<n;i++){

        processInputs[i].id=i+1;

        cout<<"Enter Arrival Time and Burst Time for Process "
            <<i+1<<": ";

        cin>>processInputs[i].arrivalTime
           >>processInputs[i].burstTime;
    }

    cout<<"Enter Time Quantum: ";
    cin>>timeQuantum;

    SimulationResult result = runRoundRobinScheduling(processInputs, timeQuantum);

    printProcessDetails(result);

    return 0;
}
