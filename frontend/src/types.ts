// These types mirror the JSON shapes backend/main.py sends and receives.
// Keeping the field names IDENTICAL to the Pydantic models and the C++
// ProcessResult/SimulationResult structs means we never have to translate
// between "frontend names" and "backend names" anywhere in this app.

export type AlgorithmKey =
  | "fcfs"
  | "sjf"
  | "ljf"
  | "srtf"
  | "lrtf"
  | "round-robin"
  | "priority-non-preemptive"
  | "priority-preemptive"
  | "mlq"
  | "mlfq";

export interface ProcessInput {
  id: number;
  arrivalTime: number;
  burstTime: number;
  priority?: number;
  queueIndex?: number;
  color: string; // frontend-only field, used to color the Gantt chart / table
}

export interface QueueConfig {
  algorithm: "FCFS" | "ROUND_ROBIN";
  timeQuantum: number;
}

export interface GanttSegment {
  processId: number; // -1 means CPU idle
  startTime: number;
  endTime: number;
}

export interface ProcessResult {
  id: number;
  arrivalTime: number;
  burstTime: number;
  priority?: number;
  queueIndex?: number;
  completionTime: number;
  turnaroundTime: number;
  waitingTime: number;
  responseTime: number;
  finalQueueLevel?: number;
  queueMovements?: number;
}

export interface SimulationResult {
  processResults: ProcessResult[];
  ganttChart: GanttSegment[];
  contextSwitches: number;
  averageWaitingTime: number;
  averageTurnaroundTime: number;
  averageResponseTime: number;
  cpuUtilization: number;
  throughput: number;
  totalTime: number;
}
