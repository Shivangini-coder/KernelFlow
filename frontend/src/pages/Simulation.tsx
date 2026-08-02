import { useState } from "react";
import { motion, AnimatePresence } from "framer-motion";
import ProcessTable, { nextColor } from "../components/ProcessTable";
import AlgorithmSelector from "../components/AlgorithmSelector";
import AlgorithmSettings from "../components/AlgorithmSettings";
import GanttChart from "../components/GanttChart";
import MetricCard from "../components/MetricCard";
import ResultsTable from "../components/ResultsTable";
import { getAlgorithmMeta } from "../data/algorithms";
import { runSimulation } from "../lib/api";
import type { AlgorithmKey, ProcessInput, QueueConfig, SimulationResult } from "../types";

const SAMPLE_TEST_CASE: ProcessInput[] = [
  { id: 1, arrivalTime: 0, burstTime: 5, priority: 2, queueIndex: 0, color: "#3b82f6" },
  { id: 2, arrivalTime: 1, burstTime: 3, priority: 1, queueIndex: 0, color: "#22d3ee" },
  { id: 3, arrivalTime: 2, burstTime: 8, priority: 3, queueIndex: 1, color: "#818cf8" },
  { id: 4, arrivalTime: 3, burstTime: 6, priority: 2, queueIndex: 1, color: "#f472b6" },
];

function generateRandomProcesses(count = 5): ProcessInput[] {
  const processes: ProcessInput[] = [];
  for (let i = 1; i <= count; i++) {
    processes.push({
      id: i,
      arrivalTime: Math.floor(Math.random() * 8),
      burstTime: 1 + Math.floor(Math.random() * 9),
      priority: 1 + Math.floor(Math.random() * 5),
      queueIndex: Math.floor(Math.random() * 2),
      color: nextColor(processes),
    });
  }
  return processes;
}

function downloadCsv(processes: ProcessInput[], result: SimulationResult) {
  const header = ["PID", "Arrival", "Burst", "CT", "TAT", "WT", "RT"];
  const rows = result.processResults.map((r) => [r.id, r.arrivalTime, r.burstTime, r.completionTime, r.turnaroundTime, r.waitingTime, r.responseTime]);
  const csv = [header, ...rows].map((row) => row.join(",")).join("\n");
  const blob = new Blob([csv], { type: "text/csv" });
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  link.href = url;
  link.download = "kernelflow_results.csv";
  link.click();
  URL.revokeObjectURL(url);
}

export default function Simulation() {
  const [processes, setProcesses] = useState<ProcessInput[]>(SAMPLE_TEST_CASE);
  const [algorithm, setAlgorithm] = useState<AlgorithmKey>("fcfs");
  const [timeQuantum, setTimeQuantum] = useState(2);
  const [queues, setQueues] = useState<QueueConfig[]>([
    { algorithm: "FCFS", timeQuantum: 0 },
    { algorithm: "ROUND_ROBIN", timeQuantum: 4 },
  ]);
  const [agingThreshold, setAgingThreshold] = useState(10);

  const [result, setResult] = useState<SimulationResult | null>(null);
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const meta = getAlgorithmMeta(algorithm);

  async function handleRun() {
    if (processes.length === 0) {
      setError("Add at least one process before running a simulation.");
      return;
    }
    setIsLoading(true);
    setError(null);
    try {
      const simulationResult = await runSimulation(algorithm, processes, {
        timeQuantum,
        queues,
        agingThreshold,
      });
      setResult(simulationResult);
    } catch (err) {
      setError(err instanceof Error ? err.message : "Something went wrong running the simulation.");
      setResult(null);
    } finally {
      setIsLoading(false);
    }
  }

  function handleReset() {
    setProcesses([]);
    setResult(null);
    setError(null);
  }

  return (
    <div className="mx-auto max-w-7xl px-6 py-12">
      <div className="mb-8">
        <h1 className="font-display text-3xl font-bold text-white">Simulation</h1>
        <p className="mt-1 font-body text-sm text-slate-400">
          Build a process set, pick an algorithm, and run the simulation using the actual C++ scheduler.
        </p>
      </div>

      <div className="grid grid-cols-1 gap-6 lg:grid-cols-[1fr_1fr]">
        <div className="space-y-6">
          <div className="flex flex-wrap gap-2">
            <button
              onClick={() => setProcesses(generateRandomProcesses())}
              className="rounded-lg border border-line px-3 py-2 font-body text-xs text-slate-300 hover:border-signal-cyan/50 hover:text-white"
            >
              🎲 Random Processes
            </button>
            <button
              onClick={() => setProcesses(SAMPLE_TEST_CASE.map((p) => ({ ...p })))}
              className="rounded-lg border border-line px-3 py-2 font-body text-xs text-slate-300 hover:border-signal-cyan/50 hover:text-white"
            >
              📋 Load Sample Test Case
            </button>
            <button
              onClick={handleReset}
              className="rounded-lg border border-line px-3 py-2 font-body text-xs text-slate-300 hover:border-red-400/50 hover:text-red-400"
            >
              ↺ Reset
            </button>
          </div>

          <ProcessTable
            processes={processes}
            onChange={setProcesses}
            showPriority={meta.needsPriority}
            showQueueIndex={meta.needsQueues && algorithm === "mlq"}
            queueCount={queues.length}
          />
        </div>

        <div className="space-y-6">
          <AlgorithmSelector selected={algorithm} onSelect={setAlgorithm} />
          <AlgorithmSettings
            meta={meta}
            timeQuantum={timeQuantum}
            onTimeQuantumChange={setTimeQuantum}
            queues={queues}
            onQueuesChange={setQueues}
            agingThreshold={agingThreshold}
            onAgingThresholdChange={setAgingThreshold}
          />

          {error && (
            <div className="rounded-xl border border-red-500/30 bg-red-500/5 p-4 font-body text-sm text-red-300">
              {error}
            </div>
          )}

          <button
            onClick={handleRun}
            disabled={isLoading}
            className="w-full rounded-xl bg-signal-gradient px-6 py-3 font-body text-sm font-semibold text-void-950 shadow-glow transition-opacity hover:opacity-90 disabled:opacity-50"
          >
            {isLoading ? "Running Simulation…" : "▶ Run Simulation"}
          </button>
        </div>
      </div>

      <AnimatePresence>
        {result && (
          <motion.div
            initial={{ opacity: 0, y: 20 }}
            animate={{ opacity: 1, y: 0 }}
            exit={{ opacity: 0 }}
            className="mt-12 space-y-6"
          >
            <div className="flex items-center justify-between">
              <h2 className="font-display text-2xl font-bold text-white">Results — {meta.name}</h2>
              <button
                onClick={() => downloadCsv(processes, result)}
                className="rounded-lg border border-line px-3 py-2 font-body text-xs text-slate-300 hover:border-signal-cyan/50 hover:text-white"
              >
                ⬇ Export CSV
              </button>
            </div>

            <GanttChart segments={result.ganttChart} processes={processes} totalTime={result.totalTime} />

            <div className="grid grid-cols-2 gap-4 sm:grid-cols-4">
              <MetricCard label="Avg Waiting Time" value={result.averageWaitingTime.toFixed(2)} accent />
              <MetricCard label="Avg Turnaround Time" value={result.averageTurnaroundTime.toFixed(2)} accent />
              <MetricCard label="Avg Response Time" value={result.averageResponseTime.toFixed(2)} accent />
              <MetricCard label="CPU Utilization" value={`${result.cpuUtilization.toFixed(1)}%`} />
              <MetricCard label="Throughput" value={result.throughput.toFixed(3)} />
              <MetricCard label="Total Time" value={String(result.totalTime)} />
              <MetricCard label="Context Switches" value={String(result.contextSwitches)} />
              <MetricCard
                label="Idle Time"
                value={String(result.ganttChart.filter((g) => g.processId === -1).reduce((sum, g) => sum + (g.endTime - g.startTime), 0))}
              />
            </div>

            <ResultsTable
              results={result.processResults}
              processes={processes}
              showPriority={meta.needsPriority}
              showQueueIndex={algorithm === "mlq"}
              showQueueMeta={algorithm === "mlfq"}
            />
          </motion.div>
        )}
      </AnimatePresence>
    </div>
  );
}
