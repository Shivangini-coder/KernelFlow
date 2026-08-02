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
// GanttSegment represents one contiguous block of execution on
// the CPU by a single process.
//
// SRTF is PREEMPTIVE — the running process can be interrupted
// every single time unit if a shorter job shows up. That means
// a single process can legitimately appear in MULTIPLE Gantt
// segments across the timeline (run 2 units, get preempted,
// come back later and run 3 more, etc). We merge consecutive
// 1-unit executions of the SAME process into a single segment
// (see appendGanttSegment below) so the chart doesn't render
// one tiny box per time unit.
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
// Since SRTF executes 1 time unit at a time, naively recording
// a Gantt segment per unit would produce hundreds of 1-unit-wide
// segments for a process that actually ran uninterrupted for,
// say, 5 units. Instead, we check: does the last segment belong
// to the SAME process and does it end exactly where this new
// unit starts? If so, we just extend it. Otherwise, we start a
// brand new segment (a real context switch or idle gap happened).
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

// Min Heap stores {Remaining Time, Arrival Time, Index}
//
// Algorithm:
// 1. Sort processes by Arrival Time.
// 2. Push all arrived processes into the Min Heap.
// 3. Pick the process with the shortest Remaining Time.
// 4. Execute it for 1 unit.
// 5. If completed, calculate CT.
//    Otherwise push it back into the Heap.
// 6. Repeat until all processes finish.
//
// Time Complexity:
//
// Sorting  : O(n log n)
// Heap Push: O(m log n)
// Heap Pop : O(m log n)
//
// Overall  : O((n + m) log n)
//
// m = Total CPU execution units
//
// ============================================================
// BACKEND NOTE: This function now ALSO records Gantt segments
// (merged via appendGanttSegment) and idle time, needed for the
// frontend chart and for computing CPU utilization / context
// switches later. The original selection logic is untouched —
// we're still picking shortest-remaining-time exactly as before.
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

    // Initialize Remaining Time    ;  Initially, Remaining Time = Burst Time
    for(int i=0;i<n;i++)
        processes[i].remainingTime = processes[i].burstTime;

    // Stores first execution of process
    vector<bool> started(n,false);

    // {Remaining Time, Arrival Time, Index}   // Compare by Remaining Time, then Arrival Time, then Index
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
        while(i<n && processes[i].arrivalTime<=currentTime){

            pq.push({{processes[i].remainingTime, processes[i].arrivalTime}, i});

            i++;
        }

        // CPU is idle -> thus pq is empty
        if(pq.empty()){

            // Record the idle gap; appendGanttSegment will merge
            // it with a previous idle segment if they're adjacent.
            appendGanttSegment(ganttChart, -1, currentTime, processes[i].arrivalTime);

            currentTime = processes[i].arrivalTime;
            continue;
        }

        // Select shortest remaining job
        int idx = pq.top().second;
        pq.pop();

        // First time process gets CPU
        if(!started[idx]){

            started[idx] = true;

            processes[idx].responseTime = currentTime - processes[idx].arrivalTime;  // Response Time = Start Time - Arrival Time
        }

        int unitStartTime = currentTime;

        // Execute for 1 unit
        processes[idx].remainingTime--;
        currentTime++;

        // Record this 1-unit execution; merges automatically with
        // the previous segment if the same process ran just before.
        appendGanttSegment(ganttChart, processes[idx].id, unitStartTime, currentTime);

        // Add newly arrived processes (which came during 1 unit of CPU execution) in PQ
        while(i<n && processes[i].arrivalTime<=currentTime){

            pq.push({{processes[i].remainingTime, processes[i].arrivalTime}, i});

            i++;
        }

        // Process completed
        if(processes[idx].remainingTime==0){

            processes[idx].completionTime = currentTime;
            completed++;
        }

        // Push back in PQ if the running process is not completed
        else{

            pq.push({{processes[idx].remainingTime,
                      processes[idx].arrivalTime},idx});
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
// Time Complexity: O(g), where g = number of Gantt segments
// (which is much smaller than total time units, thanks to merging).
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
// runSRTFScheduling is the ONE function FastAPI will call (via
// the compiled executable) for this algorithm.
//
// Input : vector<ProcessInput>  -> comes from deserialized JSON
// Output: SimulationResult      -> gets serialized back to JSON
//
// No cin, no cout — pure function.
// ============================================================
SimulationResult runSRTFScheduling(const vector<ProcessInput>& processInputs){

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

        processInputs[i].id=i+1;

        cout<<"Enter Arrival Time and Burst Time for Process "
            <<i+1<<": ";

        cin>>processInputs[i].arrivalTime
           >>processInputs[i].burstTime;
    }

    SimulationResult result = runSRTFScheduling(processInputs);

    printProcessDetails(result);

    return 0;
}
