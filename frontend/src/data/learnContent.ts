export interface ConceptCard {
  term: string;
  definition: string;
}

export const CORE_CONCEPTS: ConceptCard[] = [
  {
    term: "CPU Scheduling",
    definition:
      "The operating system's policy for deciding which ready process gets the CPU next. A good policy balances fairness, responsiveness, and throughput.",
  },
  {
    term: "Ready Queue",
    definition:
      "The set of processes that have arrived and are waiting for CPU time. Every scheduling algorithm is really a rule for picking the next process out of this queue.",
  },
  {
    term: "Context Switching",
    definition:
      "The overhead of saving one process's state and loading another's when the CPU switches who it's running. More switching means more overhead, but often better responsiveness.",
  },
  {
    term: "Completion Time (CT)",
    definition:
      "The exact moment a process finishes execution entirely.",
  },
  {
    term: "Turnaround Time (TAT)",
    definition:
      "How long a process spent in the system overall: Completion Time − Arrival Time.",
  },
  {
    term: "Waiting Time (WT)",
    definition:
      "How long a process spent waiting for the CPU rather than running: Turnaround Time − Burst Time.",
  },
  {
    term: "Response Time (RT)",
    definition:
      "How long a process waited before it got the CPU for the very first time — important for how 'snappy' a system feels.",
  },
  {
    term: "CPU Utilization",
    definition:
      "The percentage of total time the CPU was actually doing useful work, as opposed to sitting idle.",
  },
  {
    term: "CPU Throughput",
    definition:
      "How many processes complete per unit of time — a measure of overall system productivity.",
  },
  {
    term: "Context Switches",
    definition:
      "The total count of times the CPU handed control from one process to a different one over the whole simulation.",
  },
  {
    term: "Starvation",
    definition:
      "When a process (usually low priority) never gets scheduled because higher-priority work keeps arriving ahead of it.",
  },
  {
    term: "Aging",
    definition:
      "A fix for starvation: gradually increasing a waiting process's priority the longer it waits, until it eventually gets its turn.",
  },
  {
    term: "Preemptive Scheduling",
    definition:
      "The scheduler can interrupt a running process before it finishes, if a more urgent process becomes ready.",
  },
  {
    term: "Non-Preemptive Scheduling",
    definition:
      "Once a process starts running, it keeps the CPU until it finishes or voluntarily gives it up — nothing can interrupt it.",
  },
];
