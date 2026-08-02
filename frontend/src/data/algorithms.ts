import type { AlgorithmKey } from "../types";

export interface AlgorithmMeta {
  key: AlgorithmKey;
  name: string;
  short: string;         // one-line description shown next to the selector
  preemptive: boolean;
  needsPriority: boolean;
  needsQuantum: boolean;
  needsQueues: boolean;  // MLQ / MLFQ
  needsAging: boolean;   // MLFQ only
  howItWorks: string;
  advantages: string[];
  disadvantages: string[];
  useCases: string;
}

export const ALGORITHMS: AlgorithmMeta[] = [
  {
    key: "fcfs",
    name: "First Come First Served",
    short: "Runs processes strictly in the order they arrive — simple, but a long process can hold up everyone behind it.",
    preemptive: false,
    needsPriority: false,
    needsQuantum: false,
    needsQueues: false,
    needsAging: false,
    howItWorks:
      "Processes are queued in arrival order and each one runs to completion before the next begins. No process is ever interrupted.",
    advantages: ["Simple to understand and implement", "No starvation — every process eventually runs", "Fair in the sense of first-in, first-served"],
    disadvantages: ["Convoy effect: a long process delays every short process behind it", "Poor average waiting time compared to SJF", "Not suitable for time-sharing systems"],
    useCases: "Batch systems with no interactivity requirement, or as a simple baseline to compare other algorithms against.",
  },
  {
    key: "sjf",
    name: "Shortest Job First",
    short: "Always picks the available process with the smallest burst time; non-preemptive once a process starts.",
    preemptive: false,
    needsPriority: false,
    needsQuantum: false,
    needsQueues: false,
    needsAging: false,
    howItWorks:
      "Whenever the CPU is free, the scheduler looks at every process that has arrived and picks the one with the shortest burst time, running it to completion.",
    advantages: ["Minimizes average waiting time among non-preemptive algorithms", "Good throughput for short jobs"],
    disadvantages: ["Requires knowing burst times in advance (rarely true in real systems)", "Long processes can starve if short jobs keep arriving"],
    useCases: "Batch processing where burst times are known or predictable, and long-term scheduling decisions.",
  },
  {
    key: "ljf",
    name: "Longest Job First",
    short: "The mirror image of SJF — always picks the longest available burst time. Mostly used to illustrate the cost of a poor scheduling policy.",
    preemptive: false,
    needsPriority: false,
    needsQuantum: false,
    needsQueues: false,
    needsAging: false,
    howItWorks:
      "Whenever the CPU is free, the scheduler picks the arrived process with the LONGEST burst time and runs it to completion.",
    advantages: ["Simple to implement", "Useful as a teaching contrast to SJF"],
    disadvantages: ["Very poor average waiting time", "Short processes can starve behind long ones", "Rarely used in real systems"],
    useCases: "Mostly educational — demonstrating why burst-time-aware scheduling matters and what happens when you optimize for the wrong thing.",
  },
  {
    key: "srtf",
    name: "Shortest Remaining Time First",
    short: "The preemptive version of SJF — a newly arrived shorter job can interrupt whatever is currently running.",
    preemptive: true,
    needsPriority: false,
    needsQuantum: false,
    needsQueues: false,
    needsAging: false,
    howItWorks:
      "At every moment, the process with the smallest REMAINING time gets the CPU. If a new process arrives with a shorter remaining time than the current one, it preempts it immediately.",
    advantages: ["Optimal average waiting time among all scheduling algorithms", "Responsive to short jobs arriving late"],
    disadvantages: ["High context-switch overhead", "Long processes can starve", "Needs accurate remaining-time estimates"],
    useCases: "Systems where minimizing average waiting time matters more than fairness or switching overhead.",
  },
  {
    key: "lrtf",
    name: "Longest Remaining Time First",
    short: "The preemptive version of LJF — always runs whichever ready process has the most remaining work left.",
    preemptive: true,
    needsPriority: false,
    needsQuantum: false,
    needsQueues: false,
    needsAging: false,
    howItWorks:
      "At every moment, the process with the LARGEST remaining time gets the CPU, preempting the current process if a longer-remaining one is ready.",
    advantages: ["Simple preemptive rule to implement", "Useful as a teaching contrast to SRTF"],
    disadvantages: ["Very poor average waiting and turnaround time", "Extremely high context-switch overhead", "Short jobs can starve almost indefinitely"],
    useCases: "Mostly educational — demonstrating the opposite extreme from SRTF.",
  },
  {
    key: "round-robin",
    name: "Round Robin",
    short: "Every process gets a fixed time slice (quantum) in a rotating queue — the backbone of time-sharing systems.",
    preemptive: true,
    needsPriority: false,
    needsQuantum: true,
    needsQueues: false,
    needsAging: false,
    howItWorks:
      "Ready processes sit in a FIFO queue. Each gets the CPU for at most one time quantum; if it isn't finished, it goes to the back of the queue and the next process runs.",
    advantages: ["Fair — every process gets regular CPU access", "Good response time for interactive systems", "No starvation"],
    disadvantages: ["Performance is very sensitive to quantum size", "Too small a quantum causes excessive context switching", "Too large a quantum degrades toward FCFS"],
    useCases: "Time-sharing and interactive operating systems, where fairness and responsiveness matter more than raw throughput.",
  },
  {
    key: "priority-non-preemptive",
    name: "Priority Scheduling (Non-Preemptive)",
    short: "Runs the highest-priority available process to completion; a lower number means a higher priority.",
    preemptive: false,
    needsPriority: true,
    needsQuantum: false,
    needsQueues: false,
    needsAging: false,
    howItWorks:
      "Whenever the CPU is free, the process with the best (numerically lowest) priority among arrived processes is chosen and runs to completion.",
    advantages: ["Important processes can be favored explicitly", "Simple to reason about once priorities are assigned"],
    disadvantages: ["Low-priority processes can starve indefinitely", "No responsiveness guarantee for lower-priority work"],
    useCases: "Systems where some tasks are genuinely more important than others, such as real-time-adjacent or safety-critical workloads.",
  },
  {
    key: "priority-preemptive",
    name: "Priority Scheduling (Preemptive)",
    short: "Same priority rule as above, but a newly arrived higher-priority process interrupts whatever is running.",
    preemptive: true,
    needsPriority: true,
    needsQuantum: false,
    needsQueues: false,
    needsAging: false,
    howItWorks:
      "At every moment, the highest-priority ready process runs. A new arrival with better priority than the current process preempts it immediately.",
    advantages: ["Very responsive to high-priority arrivals", "Good fit for systems with hard priority requirements"],
    disadvantages: ["Starvation risk for low-priority processes is even worse than non-preemptive", "Frequent preemption adds context-switch overhead"],
    useCases: "Real-time-adjacent systems where urgent work must interrupt lower-priority work immediately.",
  },
  {
    key: "mlq",
    name: "Multilevel Queue",
    short: "Processes are permanently split into several priority queues, each with its own algorithm — no movement between queues.",
    preemptive: true,
    needsPriority: false,
    needsQuantum: false,
    needsQueues: true,
    needsAging: false,
    howItWorks:
      "Each process is assigned to one fixed queue when created. Queue 0 always runs before queue 1, and so on — a process from a higher queue always preempts one from a lower queue.",
    advantages: ["Lets different classes of processes (system, interactive, batch) be scheduled differently", "Predictable priority structure"],
    disadvantages: ["A process can never move queues even if its behaviour changes", "Low-priority queues can starve under heavy high-priority load"],
    useCases: "Systems with clearly distinct, permanent categories of work — e.g. system processes vs. batch jobs.",
  },
  {
    key: "mlfq",
    name: "Multilevel Feedback Queue",
    short: "Like MLQ, but processes are promoted or demoted between queues based on their observed CPU behaviour.",
    preemptive: true,
    needsPriority: false,
    needsQuantum: false,
    needsQueues: true,
    needsAging: true,
    howItWorks:
      "Every process starts in the top queue. Using its full time quantum without finishing demotes it a level (it looks CPU-bound); waiting too long without running promotes it a level (aging, to prevent starvation).",
    advantages: ["Adapts automatically to whether a process behaves interactively or CPU-bound", "Aging prevents indefinite starvation of low-priority work"],
    disadvantages: ["More complex to configure and tune (queues, quanta, aging threshold)", "Behaviour can be harder to predict than simpler algorithms"],
    useCases: "General-purpose operating systems (this is close to what real OS schedulers like those in Windows and Solaris historically used).",
  },
];

export function getAlgorithmMeta(key: AlgorithmKey): AlgorithmMeta {
  const found = ALGORITHMS.find((a) => a.key === key);
  if (!found) throw new Error(`Unknown algorithm key: ${key}`);
  return found;
}
