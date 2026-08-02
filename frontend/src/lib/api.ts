import type {
  AlgorithmKey,
  ProcessInput,
  QueueConfig,
  SimulationResult,
} from "../types";

// The FastAPI server's base URL. During local development this is
// backend's uvicorn address (see backend/README section "Run"). In
// production you'd point this at wherever the FastAPI app is deployed.
const API_BASE_URL = import.meta.env.VITE_API_BASE_URL || "http://localhost:8000";

// Maps each algorithm's frontend key to the FastAPI route that handles it
// (see backend/main.py's @app.post(...) paths).
const ENDPOINT_BY_ALGORITHM: Record<AlgorithmKey, string> = {
  fcfs: "/simulate/fcfs",
  sjf: "/simulate/sjf",
  ljf: "/simulate/ljf",
  srtf: "/simulate/srtf",
  lrtf: "/simulate/lrtf",
  "round-robin": "/simulate/round-robin",
  "priority-non-preemptive": "/simulate/priority-non-preemptive",
  "priority-preemptive": "/simulate/priority-preemptive",
  mlq: "/simulate/mlq",
  mlfq: "/simulate/mlfq",
};

export interface RunSimulationOptions {
  timeQuantum?: number;        // required for "round-robin"
  queues?: QueueConfig[];      // required for "mlq" and "mlfq"
  agingThreshold?: number;     // required for "mlfq"
}

// Strips the frontend-only `color` field off each process before sending
// to the backend — the backend's Pydantic model doesn't know about it (and
// doesn't need to, since coloring is purely a display concern).
function toBackendProcesses(processes: ProcessInput[]) {
  return processes.map(({ color, ...rest }) => rest);
}

export async function runSimulation(
  algorithm: AlgorithmKey,
  processes: ProcessInput[],
  options: RunSimulationOptions = {}
): Promise<SimulationResult> {
  const endpoint = ENDPOINT_BY_ALGORITHM[algorithm];

  const body: Record<string, unknown> = {
    processes: toBackendProcesses(processes),
  };

  if (algorithm === "round-robin") {
    body.timeQuantum = options.timeQuantum ?? 2;
  }
  if (algorithm === "mlq") {
    body.queues = options.queues ?? [];
  }
  if (algorithm === "mlfq") {
    body.queues = options.queues ?? [];
    body.agingThreshold = options.agingThreshold ?? 10;
  }

  const response = await fetch(`${API_BASE_URL}${endpoint}`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body),
  });

  if (!response.ok) {
    const errorBody = await response.json().catch(() => ({}));
    throw new Error(
      errorBody.detail || `Simulation request failed (${response.status})`
    );
  }

  return response.json();
}
