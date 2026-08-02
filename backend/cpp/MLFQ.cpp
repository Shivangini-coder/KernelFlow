#include<iostream>
#include<vector>
#include<algorithm>
#include<deque>   // deque (Double-Ended Queue) -> data structure that allows you to insert and delete elements from both the front and the back efficiently.
using namespace std;

// ================================================================================
// MULTILEVEL FEEDBACK QUEUE (MLFQ) SCHEDULING — FULL WALKTHROUGH
// ================================================================================
//
// HOW THIS DIFFERS FROM MLQ (READ THE MLQ FILE FIRST IF YOU HAVEN'T):
// -----------------------------------------------------------------------
// In plain Multilevel Queue (MLQ), a process is permanently glued to one
// queue forever. In Multilevel FEEDBACK Queue, processes can MOVE between
// queues based on how they behave. This is the "feedback" part — the
// scheduler watches how a process behaves and adjusts its priority
// accordingly. Two movements are possible:
//
//   1. DEMOTION (punishing CPU-hungry processes):
//      Every process STARTS in queue 0 (the highest-priority queue). If it
//      uses its ENTIRE time quantum in a Round Robin queue without finishing,
//      that's a signal "this process needs a lot of CPU time" — so it gets
//      pushed DOWN one queue level, where it will get a bigger time slice
//      but lower priority. This naturally separates short, interactive-style
//      jobs (which finish inside one quantum and never get demoted) from
//      long, CPU-bound jobs (which keep getting pushed further down).
//
//   2. PROMOTION via AGING (preventing starvation):
//      If a low-priority process is stuck waiting in its ready queue for
//      too long (because higher-priority queues keep getting new arrivals
//      and hogging the CPU), we don't want it to starve forever. If it has
//      been WAITING (not running) for longer than `agingThreshold` time
//      units, we promote it UP one queue level and reset its wait timer.
//      This is the classic fix for the "starvation problem" of strict
//      priority scheduling.
//
// Everything else — the cross-queue PREEMPTION rule (a higher-priority
// arrival interrupts whoever is currently running) — works exactly like MLQ.
//
// DATA STRUCTURES USED AND WHY:
// ------------------------------
// - vector<deque<int>> readyQueues: same idea as MLQ — one FIFO line per
//   queue level, using deque so we can push_front() a preempted process
//   back into its OWN queue without losing its place in line.
// - Process.currentQueueLevel: UNLIKE MLQ, this is NOT fixed — it changes
//   over the process's lifetime as it gets promoted/demoted.
// - Process.readyQueueEntryTime: records the exact time this process was
//   most recently PLACED into a ready queue (whether because it just
//   arrived, got preempted, got demoted, or got promoted). Aging is
//   computed as `currentTime - readyQueueEntryTime`. Every time a process
//   is placed back into a ready queue for any reason, this timestamp
//   resets — meaning aging measures "how long since this process last
//   entered a ready queue," a simplification we're making deliberately for
//   clarity (real operating systems sometimes use more elaborate metrics).
//
// STEP-BY-STEP LOGIC EXECUTED AT EVERY SINGLE TIME UNIT:
// --------------------------------------------------------
//  1. Add any process that has just arrived into QUEUE 0 (every process
//     always STARTS at the top, regardless of how long its burst is).
//  2. AGING CHECK: scan every waiting process in every queue below the top.
//     If any of them has been waiting >= agingThreshold time units, promote
//     it one level up and reset its wait timer. (Skipped entirely if
//     agingThreshold <= 0, meaning aging is disabled.)
//  3. PREEMPTION CHECK: if a process is currently running, and a HIGHER
//     priority queue now has someone waiting, preempt the running process —
//     push it back onto the FRONT of its OWN CURRENT queue level (not the
//     back), since it was cut off unfairly, not because its slice expired.
//  4. If the CPU is free, pick the process at the front of the
//     highest-priority NON-EMPTY queue.
//  5. Run the chosen process for exactly 1 time unit.
//  6. If it just finished, record its completion time.
//  7. If it hasn't finished AND its current queue uses Round Robin AND its
//     time quantum has just run out, DEMOTE it: move it one queue level
//     DOWN (or keep it at the lowest level if it's already there) and push
//     it to the BACK of that new queue, resetting its wait timer.
//     (If its current queue uses FCFS, nothing special happens here — FCFS
//     has no quantum, so it just keeps running until finished or preempted.)
//
// TIME COMPLEXITY:
// -----------------
// Let m = total simulation time, Q = number of queues, n = number of processes.
// Every tick does O(Q) work for the preemption/selection scan (same as MLQ),
// PLUS O(n) work in the worst case for the aging scan (since in the worst
// case every waiting process needs to be checked). This gives:
//   Overall Time Complexity: O(m * (Q + n))
// This is a straightforward, easy-to-follow implementation prioritizing
// clarity over raw performance — a more optimized version could track the
// single oldest-waiting process per queue instead of scanning everyone, but
// since n and Q are small in a classroom/demo setting, this is not a
// practical concern.
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
// scheduler. Notice there is NO queueIndex field here (unlike
// MLQ) — in MLFQ every process ALWAYS starts at queue 0, so the
// user never has to specify a starting queue. Its final queue
// level is decided entirely by the simulation itself.
// ============================================================
struct ProcessInput{
    int id;
    int arrivalTime;
    int burstTime;
};

// ============================================================
// SchedulingAlgorithm: identical concept to the MLQ file. Each
// queue level runs either FCFS or Round Robin internally.
// ============================================================
enum class SchedulingAlgorithm{
    FCFS,
    ROUND_ROBIN
};

// ============================================================
// QueueConfig: describes ONE queue level's behaviour.
// timeQuantum is ignored if algorithm == FCFS.
// In a typical MLFQ setup, lower levels get LARGER quanta (so
// CPU-heavy processes that sink down still get reasonably long
// turns), and the very last level is often plain FCFS — but this
// code lets the user configure it however they like.
// ============================================================
struct QueueConfig{
    SchedulingAlgorithm algorithm;
    int timeQuantum;
};

// ============================================================
// GanttSegment: one contiguous block of CPU execution. Same
// merging technique as every other preemptive algorithm in this
// project (see appendGanttSegment below).
// ============================================================
struct GanttSegment{
    int processId;   // -1 is reserved for CPU idle time
    int startTime;
    int endTime;
};

// ============================================================
// ProcessResult is the per-process OUTPUT contract. Beyond the
// usual timing stats, we ALSO report:
//   - finalQueueLevel: which queue the process ended up in when
//     it finished (useful to show "this looked CPU-bound, it
//     sank to queue 2" in the frontend).
//   - queueMovements: how many times it was promoted or demoted
//     in total — a nice at-a-glance signal of how "well-behaved"
//     (interactive) or "CPU-hungry" (batch-like) a process was.
// ============================================================
struct ProcessResult{
    int id;
    int arrivalTime;
    int burstTime;
    int completionTime;
    int turnaroundTime;
    int waitingTime;
    int responseTime;
    int finalQueueLevel;
    int queueMovements;
};

// ============================================================
// SimulationResult bundles EVERYTHING one algorithm run
// produces — same shared shape used by every other algorithm
// file in this project.
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
// Internal working struct.
//
// currentQueueLevel and readyQueueEntryTime are NOT fixed like
// they were in MLQ — they change throughout the simulation as
// the process gets promoted, demoted, or preempted.
// ============================================================
struct Process{
    int id;
    int arrivalTime;
    int burstTime;
    int remainingTime;

    int currentQueueLevel;      // which queue this process is in RIGHT NOW (changes over time)
    int readyQueueEntryTime;    // when it most recently entered a ready queue (used to measure aging)
    int queueMovements;         // total number of promotions + demotions this process has undergone

    int completionTime;
    int turnaroundTime;
    int waitingTime;
    int responseTime;
};

// ============================================================
// appendGanttSegment: identical technique used in every other
// preemptive algorithm file in this project. Merges consecutive
// 1-unit executions of the SAME process into a single segment.
// O(1) per call — only ever touches the back() of a vector.
// ============================================================
void appendGanttSegment(vector<GanttSegment>& ganttChart, int processId, int startTime, int endTime){

    if(!ganttChart.empty() &&
       ganttChart.back().processId == processId &&
       ganttChart.back().endTime == startTime){

        ganttChart.back().endTime = endTime;
    }
    else{

        ganttChart.push_back({processId, startTime, endTime});
    }
}

// ================================================================================
// findCompletionTime: THE CORE MLFQ SIMULATION LOOP
//
// This is the heart of the whole algorithm. Read the header comment at the top
// of this file first if you haven't — every step below is numbered to match
// that walkthrough exactly.
// ================================================================================
void findCompletionTime(vector<Process>& processes, const vector<QueueConfig>& queueConfigs, int agingThreshold, vector<GanttSegment>& ganttChart){

    int n = processes.size();
    int numQueues = queueConfigs.size();

    // Sort by Arrival Time, same reasoning as every other file.
    sort(processes.begin(), processes.end(),
    [](Process &a, Process &b){

        if(a.arrivalTime == b.arrivalTime)
            return a.id < b.id;

        return a.arrivalTime < b.arrivalTime;
    });

    // Initialize remainingTime and queueMovements for each process (Initially, remainingTime = burstTime, queueMovements = 0)
    for(int idx = 0; idx < n; idx++){
        processes[idx].remainingTime = processes[idx].burstTime;
        processes[idx].queueMovements = 0;
    }

    vector<bool> started(n, false);

    // One ready queue per level, EXACTLY like MLQ. The difference is that
    // in MLFQ, a process's presence in readyQueues[level] is TEMPORARY —
    // it can be moved to a different level's deque over time.
    vector<deque<int>> readyQueues(numQueues);

    int currentTime = 0;
    int completed = 0;
    int i = 0;

    int runningIdx = -1;
    int runningQuantumUsed = 0;

    while(completed < n){

        // ------------------------------------------------------------
        // STEP 1: Every NEW arrival always starts at queue level 0 —
        // the highest priority. MLFQ gives every process the benefit
        // of the doubt at first; it only gets demoted later if it
        // proves to be CPU-heavy.
        // ------------------------------------------------------------
        while(i < n && processes[i].arrivalTime <= currentTime){

            processes[i].currentQueueLevel = 0;
            processes[i].readyQueueEntryTime = currentTime;

            readyQueues[0].push_back(i);

            i++;
        }

        // ------------------------------------------------------------
        // STEP 2: AGING CHECK. Scan every WAITING process (i.e. sitting
        // in a ready queue, not currently running) at every level BELOW
        // the top. If it has been waiting >= agingThreshold time units,
        // promote it one level up and reset its wait timer.
        //
        // We scan from the LOWEST level upward on purpose: this way, a
        // process that gets promoted from level 2 -> level 1 during this
        // same pass has its wait timer freshly reset to 0, so it will
        // correctly NOT also qualify for an immediate second promotion
        // when we check level 1 right after.
        //
        // Skipped entirely if agingThreshold <= 0 (aging disabled).
        // ------------------------------------------------------------
        if(agingThreshold > 0){

            for(int level = numQueues - 1; level >= 1; level--){

                for(auto it = readyQueues[level].begin(); it != readyQueues[level].end(); ){

                    int idx = *it;

                    if(currentTime - processes[idx].readyQueueEntryTime >= agingThreshold){

                        // Remove this process from its current queue...
                        it = readyQueues[level].erase(it);

                        // ...and place it one level up, resetting its clock.
                        processes[idx].currentQueueLevel = level - 1;
                        processes[idx].readyQueueEntryTime = currentTime;
                        processes[idx].queueMovements++;

                        readyQueues[level - 1].push_back(idx);
                    }
                    else{
                        ++it;
                    }
                }
            }
        }

        // ------------------------------------------------------------
        // STEP 3: PREEMPTION CHECK. Exactly the same rule as MLQ: if
        // someone is running and a HIGHER-priority queue now has a
        // process waiting, preempt the running process. It goes back
        // to the FRONT of its OWN CURRENT queue level (not the back),
        // and its wait clock resets since it's freshly back in the
        // ready queue.
        // ------------------------------------------------------------
        if(runningIdx != -1){

            int runningLevel = processes[runningIdx].currentQueueLevel;
            bool higherPriorityIsWaiting = false;

            for(int level = 0; level < runningLevel; level++){
                if(!readyQueues[level].empty()){
                    higherPriorityIsWaiting = true;
                    break;
                }
            }

            if(higherPriorityIsWaiting){

                readyQueues[runningLevel].push_front(runningIdx);
                processes[runningIdx].readyQueueEntryTime = currentTime;

                runningIdx = -1;
                runningQuantumUsed = 0;
            }
        }

        // ------------------------------------------------------------
        // STEP 4: If the CPU is free, pick the process at the front of
        // the highest-priority NON-EMPTY queue.
        // ------------------------------------------------------------
        if(runningIdx == -1){

            int chosenLevel = -1;

            for(int level = 0; level < numQueues; level++){
                if(!readyQueues[level].empty()){
                    chosenLevel = level;
                    break;
                }
            }

            // CPU IDLE: nobody has arrived yet. Jump the clock forward.
            if(chosenLevel == -1){

                appendGanttSegment(ganttChart, -1, currentTime, processes[i].arrivalTime);

                currentTime = processes[i].arrivalTime;
                continue;
            }

            runningIdx = readyQueues[chosenLevel].front();
            readyQueues[chosenLevel].pop_front();
            runningQuantumUsed = 0;

            if(!started[runningIdx]){

                started[runningIdx] = true;
                
                // RT = Start Time - Arrival Time
                processes[runningIdx].responseTime = currentTime - processes[runningIdx].arrivalTime;
            }
        }

        // ------------------------------------------------------------
        // STEP 5: Execute the chosen process for exactly 1 time unit.
        // ------------------------------------------------------------
        int unitStartTime = currentTime;

        processes[runningIdx].remainingTime--;
        currentTime++;

        appendGanttSegment(ganttChart, processes[runningIdx].id, unitStartTime, currentTime);

        // ------------------------------------------------------------
        // STEP 6: Did the process just finish completely?
        // ------------------------------------------------------------
        if(processes[runningIdx].remainingTime == 0){

            processes[runningIdx].completionTime = currentTime;
            completed++;
            runningIdx = -1;
        }
        else{

            // --------------------------------------------------------
            // STEP 7: DEMOTION. The process hasn't finished. If its
            // CURRENT queue level uses Round Robin and its time
            // quantum has just run out, this is the classic MLFQ
            // signal "this process needed more CPU than its quantum
            // allowed" -> push it DOWN one level (or keep it at the
            // lowest level if it's already there), reset its wait
            // clock, and record that a demotion happened.
            //
            // If its current queue uses FCFS, nothing happens here —
            // FCFS has no quantum, so it just keeps running until it
            // finishes or gets preempted by Step 3.
            // --------------------------------------------------------
            int level = processes[runningIdx].currentQueueLevel;

            if(queueConfigs[level].algorithm == SchedulingAlgorithm::ROUND_ROBIN){

                runningQuantumUsed++;

                if(runningQuantumUsed == queueConfigs[level].timeQuantum){

                    int newLevel = min(level + 1, numQueues - 1);

                    processes[runningIdx].currentQueueLevel = newLevel;
                    processes[runningIdx].readyQueueEntryTime = currentTime;

                    // Only counts as a genuine "demotion" if it actually
                    // moved to a new level (a process already at the
                    // bottom level just cycles within it, which is
                    // standard Round Robin behaviour, not a demotion).
                    if(newLevel != level)
                        processes[runningIdx].queueMovements++;

                    readyQueues[newLevel].push_back(runningIdx);

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

// ============================================================
// findAverageTimes writes into the SimulationResult instead of
// printing with cout, matching every other algorithm file.
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
// computeContextSwitches: identical logic to every other file.
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
// computeUtilizationAndThroughput: identical logic to every
// other file in this project.
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
// runMLFQScheduling is the ONE function FastAPI will call for
// this algorithm. It takes TWO extra parameters compared to the
// simplest algorithms: the per-queue configuration list, and the
// aging threshold. The FastAPI endpoint for MLFQ will need to
// accept both of these in its request body alongside the process
// list.
//
// No cin, no cout — pure function.
// ============================================================
SimulationResult runMLFQScheduling(const vector<ProcessInput>& processInputs, const vector<QueueConfig>& queueConfigs, int agingThreshold){

    int n = processInputs.size();

    vector<Process> processes(n);

    for(int i = 0; i < n; i++){
        processes[i].id = processInputs[i].id;
        processes[i].arrivalTime = processInputs[i].arrivalTime;
        processes[i].burstTime = processInputs[i].burstTime;
    }

    SimulationResult result;

    findCompletionTime(processes, queueConfigs, agingThreshold, result.ganttChart);
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
        processResult.completionTime = process.completionTime;
        processResult.turnaroundTime = process.turnaroundTime;
        processResult.waitingTime = process.waitingTime;
        processResult.responseTime = process.responseTime;
        processResult.finalQueueLevel = process.currentQueueLevel;
        processResult.queueMovements = process.queueMovements;

        result.processResults.push_back(processResult);
    }

    return result;
}

// ============================================================
// printProcessDetails is ONLY used for local terminal testing
// in main() below. FastAPI never calls this.
// ============================================================
void printProcessDetails(const SimulationResult& result){

    cout<<"Process ID\tArrival\tBurst\tCT\tTAT\tWT\tRT\tFinalQ\tMoves\n";

    for(auto &process : result.processResults){

        cout<<process.id<<"\t\t"
            <<process.arrivalTime<<"\t"
            <<process.burstTime<<"\t"
            <<process.completionTime<<"\t"
            <<process.turnaroundTime<<"\t"
            <<process.waitingTime<<"\t"
            <<process.responseTime<<"\t"
            <<process.finalQueueLevel<<"\t"
            <<process.queueMovements<<endl;
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
            queueConfigs[level].timeQuantum = 0;
        }
    }

    int agingThreshold;

    cout<<"Enter Aging Threshold (max time a process can wait before being promoted; enter 0 to disable aging): ";
    cin>>agingThreshold;

    int n;

    cout<<"Enter number of processes: ";
    cin>>n;

    vector<ProcessInput> processInputs(n);

    for(int i = 0; i < n; i++){

        processInputs[i].id = i + 1;

        cout<<"Enter Arrival Time and Burst Time for Process "<<i+1<<": ";

        cin>>processInputs[i].arrivalTime
           >>processInputs[i].burstTime;
    }

    SimulationResult result = runMLFQScheduling(processInputs, queueConfigs, agingThreshold);

    printProcessDetails(result);

    return 0;
}
