#include<iostream>
#include<vector>
#include<algorithm>
#include<deque>   // deque (Double-Ended Queue) -> data structure that allows you to insert and delete elements from both the front and the back efficiently.
using namespace std;

// ================================================================================
// MULTILEVEL QUEUE (MLQ) SCHEDULING — FULL WALKTHROUGH
// ================================================================================
//
// THE BIG IDEA:
// -------------
// Instead of having ONE ready queue for every process, we split processes into
// several ready queues (e.g. "system processes", "interactive processes",
// "batch processes"). Each queue has its OWN scheduling algorithm (FCFS or
// Round Robin) and, if Round Robin, its own time quantum.
//
// Crucially, in classic MLQ, a process is PERMANENTLY assigned to one queue
// when it's created — it never moves to a different queue. (This is the key
// difference from Multilevel FEEDBACK Queue, where processes can be promoted
// or demoted between queues based on behaviour. MLQ has no such movement.)
//
// PRIORITY BETWEEN QUEUES:
// ------------------------
// Queue 0 is the highest priority, queue 1 is next, and so on. The scheduler
// follows a strict rule: "Never run a process from a lower-priority queue while
// ANY process is waiting in a higher-priority queue." This means:
//   - If queue 0 has anyone waiting, queue 0 always goes first.
//   - If a process from queue 1 is currently RUNNING and a NEW process suddenly
//     arrives in queue 0, the queue-1 process gets PREEMPTED (paused) right
//     away, even mid-execution, so queue 0 can run immediately.
//
// This preemption-across-queues is what makes MLQ genuinely "preemptive" even
// if every individual queue internally uses non-preemptive FCFS.
//
// WHY WE SIMULATE 1 TIME UNIT AT A TIME:
// ---------------------------------------
// Because a higher-priority process can arrive and preempt the CPU at ANY
// moment, we can't just compute completion times with a formula like we did
// for FCFS/SJF. We have to advance the clock one unit at a time and, at every
// tick, re-ask "who SHOULD be running right now?" This is the same technique
// used in your SRTF/LRTF files, just extended to handle multiple queues.
//
// DATA STRUCTURES USED AND WHY:
// ------------------------------
// - vector<deque<int>> readyQueues: one FIFO line of "process indices" per
//   priority level. We use deque (double-ended queue) instead of a plain
//   queue because we sometimes need to push a process back to the FRONT of
//   its own queue (when it gets preempted mid-turn, it shouldn't lose its
//   place in line to newer arrivals) — std::queue cannot do that, but
//   std::deque can via push_front().
// - QueueConfig struct: holds each queue's algorithm (FCFS or Round Robin)
//   and time quantum (only meaningful for Round Robin queues).
//
// STEP-BY-STEP LOGIC EXECUTED AT EVERY SINGLE TIME UNIT:
// --------------------------------------------------------
//  1. Add any process that has just arrived into its assigned queue.
//  2. If a process is currently running, check: has a process just shown up
//     in a HIGHER-priority queue? If yes, preempt the running process —
//     send it back to the FRONT of its own queue (it didn't finish its turn
//     voluntarily, so it shouldn't lose its spot to processes behind it).
//  3. If nobody is running right now, pick the front process of the
//     highest-priority NON-EMPTY queue.
//  4. Run the chosen process for exactly 1 time unit.
//  5. If it just finished, record its completion time.
//  6. If it hasn't finished AND its queue uses Round Robin AND its time
//     quantum for this turn has just run out, send it to the BACK of its
//     OWN queue (same level — MLQ never changes a process's queue) so its
//     queue-mates get a turn.
//     (If its queue uses FCFS, it simply keeps running — FCFS has no
//     quantum — until it finishes or gets preempted by a higher queue.)
//
// TIME COMPLEXITY:
// -----------------
// Let m = total simulation time (sum of all burst times + any idle gaps),
// and Q = number of queues.
// At every one of the m time units, we do O(Q) work to check for preemption
// and to find the next non-empty queue (a simple linear scan across queues,
// which is fast since Q is small — typically 2 to 5 queues in practice).
// Overall Time Complexity: O(m * Q)
//
// SPACE COMPLEXITY:
// -------------------
// O(n) for the process list + O(Q) for the queues + O(g) for the Gantt
// chart, where g = number of Gantt segments after merging (g <= m).
// ================================================================================


// ============================================================
// BACKEND INTEGRATION NOTE:
//
// ProcessInput is the "contract" between FastAPI and this C++
// scheduler. Notice the extra `queueIndex` field — unlike every
// other algorithm so far, MLQ needs the USER (or frontend form)
// to tell us which queue each process belongs to. This mirrors
// how a real MLQ setup requires an admin decision ("this process
// is a batch job, put it in queue 2") rather than the scheduler
// figuring it out automatically.
// ============================================================
struct ProcessInput{
    int id;
    int arrivalTime;
    int burstTime;
    int queueIndex;   // which queue (0 = highest priority) this process is permanently placed in
};

// ============================================================
// SchedulingAlgorithm: the two algorithms a single queue can run
// internally. We use `enum class` (instead of a plain enum) so
// that comparisons like `algorithm == SchedulingAlgorithm::FCFS`
// are explicit and can't accidentally be confused with a plain
// int — this is a small modern-C++ safety habit.
// ============================================================

// An enum (short for enumeration) is a user-defined type in C++ that lets you create a variable which can take only a fixed set of named values.
enum class SchedulingAlgorithm{    
    FCFS,
    ROUND_ROBIN
};

// ============================================================
// QueueConfig: describes ONE queue's behaviour.
// timeQuantum is meaningless (and ignored) if algorithm == FCFS,
// since FCFS runs a process to completion without slicing it.
// ============================================================
struct QueueConfig{
    SchedulingAlgorithm algorithm;
    int timeQuantum;
};

// ============================================================
// GanttSegment: one contiguous block of CPU execution.
// Just like SRTF/LRTF, MLQ can preempt mid-execution, so a
// single process can appear in MULTIPLE Gantt segments across
// the timeline. Consecutive 1-unit executions of the SAME
// process are merged into one segment (see appendGanttSegment).
// ============================================================
struct GanttSegment{
    int processId;   // -1 is reserved for CPU idle time
    int startTime;
    int endTime;
};

// ============================================================
// ProcessResult is the per-process OUTPUT contract — serialized
// back to JSON for the frontend table. We include queueIndex so
// the frontend can show "which queue was this process in?".
// ============================================================
struct ProcessResult{
    int id;
    int arrivalTime;
    int burstTime;
    int queueIndex;
    int completionTime;
    int turnaroundTime;
    int waitingTime;
    int responseTime;
};

// ============================================================
// SimulationResult bundles EVERYTHING one algorithm run
// produces — the same shared shape used by every other
// algorithm file in this project, so FastAPI's "compare all
// algorithms" endpoint can treat MLQ exactly like FCFS, SJF, etc.
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

// ============================================================
// Internal working struct — this is where the simulation keeps
// its own bookkeeping while it runs. Separate from ProcessInput
// (what comes IN) and ProcessResult (what goes OUT).
// ============================================================
struct Process{
    int id;
    int arrivalTime;
    int burstTime;
    int remainingTime;
    int queueIndex;   // fixed for the entire simulation in MLQ

    int completionTime;
    int turnaroundTime;
    int waitingTime;
    int responseTime;
};

// ============================================================
// appendGanttSegment: identical technique to your SRTF/LRTF
// files. Since we simulate 1 unit at a time, naively logging
// every tick would create hundreds of tiny 1-unit-wide Gantt
// boxes. We merge a new 1-unit execution into the PREVIOUS
// segment if it's the exact same process continuing with no
// gap — otherwise we start a fresh segment.
//
// Data structure: only ever touches the back() of a vector,
// so this is O(1) per call.
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

// ================================================================================
// findCompletionTime: THE CORE MLQ SIMULATION LOOP
//
// This is the heart of the whole algorithm. Read the header comment at the top
// of this file first if you haven't — it explains the intuition. Below, every
// step is numbered to match that walkthrough exactly.
// ================================================================================
void findCompletionTime(vector<Process>& processes, const vector<QueueConfig>& queueConfigs, vector<GanttSegment>& ganttChart){

    int n = processes.size();
    int numQueues = queueConfigs.size();

    // Sort by Arrival Time — same reasoning as every other algorithm in this
    // project: we need to process arrivals in chronological order so our
    // "who has arrived by time X" pointer (i, below) works correctly.
    sort(processes.begin(), processes.end(),
    [](Process &a, Process &b){

        if(a.arrivalTime == b.arrivalTime)
            return a.id < b.id;

        return a.arrivalTime < b.arrivalTime;
    });

    // Initialize Remaining Time -> initially, Remaining Time = Burst Time
    for(int idx = 0; idx < n; idx++)
        processes[idx].remainingTime = processes[idx].burstTime;

    // Tracks whether a process has ever been given the CPU before, so we
    // only record its Response Time on its VERY FIRST execution.
    vector<bool> started(n, false);

    // ONE DEQUE PER QUEUE LEVEL. readyQueues[0] is the highest-priority
    // queue's line of waiting processes (stored as indices into `processes`).
    // We use deque (not queue) specifically because preemption requires
    // pushing a process back onto the FRONT of its own queue (see step 2
    // in the loop below) — a plain std::queue can't do that.
    vector<deque<int>> readyQueues(numQueues);

    int currentTime = 0;
    int completed = 0;
    int i = 0;                 // pointer into `processes`, tracks "next process that hasn't arrived yet"

    int runningIdx = -1;       // index of the process CURRENTLY on the CPU, or -1 if CPU is free
    int runningQuantumUsed = 0; // how many units the running process has used of its CURRENT Round Robin turn

    while(completed < n){

        // ------------------------------------------------------------
        // STEP 1: Add every process that has arrived by `currentTime`
        // into the ready queue that it was permanently assigned to.
        // ------------------------------------------------------------
        while(i < n && processes[i].arrivalTime <= currentTime){

            int assignedQueue = processes[i].queueIndex;
            readyQueues[assignedQueue].push_back(i);

            i++;
        }

        // ------------------------------------------------------------
        // STEP 2: If someone is currently running, check whether a
        // HIGHER-priority queue (a smaller index number) now has any
        // process waiting. If so, we must preempt the running process
        // immediately — even though it hasn't finished its turn.
        //
        // We push it back onto the FRONT of its OWN queue (not the
        // back!) because it was cut off unfairly by an external, higher
        // priority process — it shouldn't also lose its place in line
        // to processes that arrived after it within its own queue.
        // ------------------------------------------------------------
        if(runningIdx != -1){   // i.e if a process is currently running (CPU is NOT free), check for preemption

            int runningLevel = processes[runningIdx].queueIndex;
            bool higherPriorityIsWaiting = false;

            for(int level = 0; level < runningLevel; level++){
                if(!readyQueues[level].empty()){
                    higherPriorityIsWaiting = true;
                    break;
                }
            }

            if(higherPriorityIsWaiting){

                readyQueues[runningLevel].push_front(runningIdx);

                runningIdx = -1;
                runningQuantumUsed = 0;
            }
        }

        // ------------------------------------------------------------
        // STEP 3: If the CPU is free (either nobody has run yet, or we
        // just preempted someone in Step 2), pick the next process to
        // run: the process at the FRONT of the highest-priority
        // NON-EMPTY queue.
        // ------------------------------------------------------------
        if(runningIdx == -1){

            int chosenLevel = -1;

            for(int level = 0; level < numQueues; level++){
                if(!readyQueues[level].empty()){
                    chosenLevel = level;
                    break;
                }
            }

            // CPU IDLE: every queue is empty right now, meaning no
            // process has arrived yet that's ready to run. Jump the
            // clock forward to the next arrival instead of ticking
            // one-by-one through empty time (this is just an
            // efficiency shortcut, the outcome is identical either way).
            if(chosenLevel == -1){

                appendGanttSegment(ganttChart, -1, currentTime, processes[i].arrivalTime);

                currentTime = processes[i].arrivalTime;
                continue;
            }

            runningIdx = readyQueues[chosenLevel].front();
            readyQueues[chosenLevel].pop_front();
            runningQuantumUsed = 0;

            // First time this process gets the CPU -> record Response Time
            if(!started[runningIdx]){

                started[runningIdx] = true;

                // RT = Start Time - Arrival Time
                processes[runningIdx].responseTime = currentTime - processes[runningIdx].arrivalTime;
            }
        }

        // ------------------------------------------------------------
        // STEP 4: Execute the chosen process for exactly 1 time unit.
        // ------------------------------------------------------------
        int unitStartTime = currentTime;

        processes[runningIdx].remainingTime--;
        currentTime++;

        appendGanttSegment(ganttChart, processes[runningIdx].id, unitStartTime, currentTime);

        // ------------------------------------------------------------
        // STEP 5: Did the process just finish completely?
        // ------------------------------------------------------------
        if(processes[runningIdx].remainingTime == 0){

            processes[runningIdx].completionTime = currentTime;
            completed++;
            runningIdx = -1;
        }
        else{

            // --------------------------------------------------------
            // STEP 6: The process hasn't finished. If its queue uses
            // Round Robin, check whether its time quantum for this
            // turn has just run out. If so, send it to the BACK of
            // its OWN queue (same level — MLQ never moves a process
            // to a different queue) so the next process in line gets
            // a turn.
            //
            // If its queue uses FCFS, we do nothing here — FCFS has
            // no time quantum, so the process simply keeps running
            // until it finishes or gets preempted by Step 2 above.
            // --------------------------------------------------------
            int level = processes[runningIdx].queueIndex;

            if(queueConfigs[level].algorithm == SchedulingAlgorithm::ROUND_ROBIN){

                runningQuantumUsed++;

                if(runningQuantumUsed == queueConfigs[level].timeQuantum){

                    readyQueues[level].push_back(runningIdx);

                    runningIdx = -1;
                    runningQuantumUsed = 0;
                }
            }
        }
    }
}

// TAT = CT - AT
void findTurnaroundTime(vector<Process>& processes){

    for(auto &process : processes){

        process.turnaroundTime = process.completionTime - process.arrivalTime;
    }
}

// WT = TAT - BT
void findWaitingTime(vector<Process>& processes){

    for(auto &process : processes){

        process.waitingTime = process.turnaroundTime - process.burstTime;
    }
}

// ============================================================
// findAverageTimes writes into the SimulationResult instead of
// printing with cout, since the FastAPI-facing version of this
// program can never print — only the returned struct is visible
// to the caller.
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
// computeContextSwitches: counts how many times the CPU moved
// from one process to a DIFFERENT process. Idle gaps don't count.
// Because segments are already merged by appendGanttSegment, this
// is a simple linear scan checking where processId changes.
// Time Complexity: O(g), g = number of Gantt segments.
// ============================================================
int computeContextSwitches(const vector<GanttSegment>& ganttChart){

    int contextSwitches = 0;
    int lastRunningProcessId = -1;

    for(auto &segment : ganttChart){

        if(segment.processId == -1)
            continue;

        if(lastRunningProcessId != -1 && lastRunningProcessId != segment.processId)
            contextSwitches++;

        lastRunningProcessId = segment.processId;
    }

    return contextSwitches;
}

// ============================================================
// computeUtilizationAndThroughput:
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
// runMultilevelQueueScheduling is the ONE function FastAPI will
// call for this algorithm. Notice it takes an EXTRA parameter
// (queueConfigs) compared to the simpler algorithms — the
// FastAPI endpoint for MLQ will need to accept a list of queue
// configurations (algorithm + quantum per queue) in its request
// body, on top of the process list.
//
// No cin, no cout — pure function.
// ============================================================
SimulationResult runMultilevelQueueScheduling(const vector<ProcessInput>& processInputs, const vector<QueueConfig>& queueConfigs){

    int n = processInputs.size();

    vector<Process> processes(n);

    for(int i = 0; i < n; i++){
        processes[i].id = processInputs[i].id;
        processes[i].arrivalTime = processInputs[i].arrivalTime;
        processes[i].burstTime = processInputs[i].burstTime;
        processes[i].queueIndex = processInputs[i].queueIndex;
    }

    SimulationResult result;

    findCompletionTime(processes, queueConfigs, result.ganttChart);
    findTurnaroundTime(processes);
    findWaitingTime(processes);

    findAverageTimes(processes, result);

    result.contextSwitches = computeContextSwitches(result.ganttChart);
    computeUtilizationAndThroughput(processes, result);

    // Sort back by Process ID for a stable, predictable output order.
    sort(processes.begin(), processes.end(),
    [](Process &a, Process &b){
        return a.id < b.id;
    });

    for(auto &process : processes){

        ProcessResult processResult;

        processResult.id = process.id;
        processResult.arrivalTime = process.arrivalTime;
        processResult.burstTime = process.burstTime;
        processResult.queueIndex = process.queueIndex;
        processResult.completionTime = process.completionTime;
        processResult.turnaroundTime = process.turnaroundTime;
        processResult.waitingTime = process.waitingTime;
        processResult.responseTime = process.responseTime;

        result.processResults.push_back(processResult);
    }

    return result;
}

// ============================================================
// printProcessDetails is ONLY used for local terminal testing
// in main() below. FastAPI never calls this.
// ============================================================
void printProcessDetails(const SimulationResult& result){

    cout<<"Process ID\tArrival\tBurst\tQueue\tCT\tTAT\tWT\tRT\n";

    for(auto &process : result.processResults){

        cout<<process.id<<"\t\t"
            <<process.arrivalTime<<"\t"
            <<process.burstTime<<"\t"
            <<process.queueIndex<<"\t"
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

// main() is a local test harness — the ONLY place cin/cout are
// allowed. It builds ProcessInput and QueueConfig manually,
// mimicking what FastAPI would eventually send in as JSON.
int main(){

    int numQueues;

    cout<<"Enter number of queues: ";
    cin>>numQueues;

    vector<QueueConfig> queueConfigs(numQueues);

    for(int level = 0; level < numQueues; level++){

        int algoChoice;

        cout<<"Queue "<<level<<" -> Enter algorithm (0 = FCFS, 1 = Round Robin): ";
        cin>>algoChoice;

        if(algoChoice == 1){

            queueConfigs[level].algorithm = SchedulingAlgorithm::ROUND_ROBIN;

            cout<<"Queue "<<level<<" -> Enter Time Quantum: ";
            cin>>queueConfigs[level].timeQuantum;
        }
        else{

            queueConfigs[level].algorithm = SchedulingAlgorithm::FCFS;
            queueConfigs[level].timeQuantum = 0; // unused for FCFS
        }
    }

    int n;

    cout<<"Enter number of processes: ";
    cin>>n;

    vector<ProcessInput> processInputs(n);

    for(int i = 0; i < n; i++){

        processInputs[i].id = i + 1;

        cout<<"Enter Arrival Time, Burst Time and Queue Index (0 to "<<numQueues-1<<") for Process "<<i+1<<": ";

        cin>>processInputs[i].arrivalTime
           >>processInputs[i].burstTime
           >>processInputs[i].queueIndex;
    }

    SimulationResult result = runMultilevelQueueScheduling(processInputs, queueConfigs);

    printProcessDetails(result);

    return 0;
}
