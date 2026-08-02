import { useState } from "react";
import { motion, AnimatePresence } from "framer-motion";
import { BarChart, Bar, XAxis, YAxis, Tooltip, ResponsiveContainer, CartesianGrid, Cell } from "recharts";
import ProcessTable from "../components/ProcessTable";
import GanttChart from "../components/GanttChart";
import { ALGORITHMS, getAlgorithmMeta } from "../data/algorithms";
import { runSimulation } from "../lib/api";
import type { AlgorithmKey, ProcessInput, QueueConfig, SimulationResult } from "../types";

const SAMPLE_TEST_CASE: ProcessInput[] = [
  { id: 1, arrivalTime: 0, burstTime: 5, priority: 2, queueIndex: 0, color: "#3b82f6" },
  { id: 2, arrivalTime: 1, burstTime: 3, priority: 1, queueIndex: 0, color: "#22d3ee" },
  { id: 3, arrivalTime: 2, burstTime: 8, priority: 3, queueIndex: 1, color: "#818cf8" },
  { id: 4, arrivalTime: 3, burstTime: 6, priority: 2, queueIndex: 1, color: "#f472b6" },
];

const DEFAULT_QUEUES: QueueConfig[] = [
  { algorithm: "FCFS", timeQuantum: 0 },
  { algorithm: "ROUND_ROBIN", timeQuantum: 4 },
];

const METRICS: { key: keyof SimulationResult; label: string; lowerIsBetter: boolean }[] = [
  { key: "averageWaitingTime", label: "Avg Waiting Time", lowerIsBetter: true },
  { key: "averageTurnaroundTime", label: "Avg Turnaround Time", lowerIsBetter: true },
  { key: "averageResponseTime", label: "Avg Response Time", lowerIsBetter: true },
  { key: "cpuUtilization", label: "CPU Utilization (%)", lowerIsBetter: false },
  { key: "throughput", label: "Throughput", lowerIsBetter: false },
  { key: "contextSwitches", label: "Context Switches", lowerIsBetter: true },
];

const CHART_COLORS = ["#3b82f6", "#22d3ee", "#818cf8"];

export default function Compare() {
  const [processes, setProcesses] = useState<ProcessInput[]>(SAMPLE_TEST_CASE);
  const [selected, setSelected] = useState<AlgorithmKey[]>(["fcfs", "sjf"]);
  const [timeQuantum, setTimeQuantum] = useState(2);
  const [queues] = useState<QueueConfig[]>(DEFAULT_QUEUES);
  const [agingThreshold] = useState(10);

  const [results, setResults] = useState<Record<string, SimulationResult> | null>(null);
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  function toggleAlgorithm(key: AlgorithmKey) {
    setSelected((prev) => {
      if (prev.includes(key)) return prev.filter((k) => k !== key);
      if (prev.length >= 3) return prev; // cap at 3
      return [...prev, key];
    });
  }

  async function handleCompare() {
    if (selected.length < 2) {
      setError("Pick at least two algorithms to compare.");
      return;
    }
    if (processes.length === 0) {
      setError("Add at least one process before comparing.");
      return;
    }
    setIsLoading(true);
    setError(null);
    try {
      const entries = await Promise.all(
        selected.map(async (key) => {
          const res = await runSimulation(key, processes, { timeQuantum, queues, agingThreshold });
          return [key, res] as const;
        })
      );
      setResults(Object.fromEntries(entries));
    } catch (err) {
      setError(err instanceof Error ? err.message : "Comparison failed.");
      setResults(null);
    } finally {
      setIsLoading(false);
    }
  }

  function bestKeyFor(metric: (typeof METRICS)[number]) {
    if (!results) return null;
    const entries = selected.map((k) => [k, results[k]?.[metric.key] as number] as const).filter(([, v]) => v !== undefined);
    if (entries.length === 0) return null;
    return entries.reduce((best, cur) => {
      if (metric.lowerIsBetter) return cur[1] < best[1] ? cur : best;
      return cur[1] > best[1] ? cur : best;
    })[0];
  }

  const needsAnyQuantum = selected.some((k) => getAlgorithmMeta(k).needsQuantum);

  return (
    <div className="mx-auto max-w-7xl px-6 py-12">
      <div className="mb-8">
        <h1 className="font-display text-3xl font-bold text-white">Compare Algorithms</h1>
        <p className="mt-1 font-body text-sm text-slate-400">
          Choose up to three scheduling algorithms and run them on the exact same process set.
        </p>
      </div>

      <div className="grid grid-cols-1 gap-6 lg:grid-cols-[1fr_1fr]">
        <ProcessTable
          processes={processes}
          onChange={setProcesses}
          showPriority={selected.some((k) => getAlgorithmMeta(k).needsPriority)}
          showQueueIndex={selected.includes("mlq")}
          queueCount={queues.length}
        />

        <div className="space-y-6">
          <div className="glass-card p-5">
            <h3 className="mb-4 font-display text-sm font-semibold uppercase tracking-wide text-slate-400">
              Choose up to 3 algorithms
            </h3>
            <div className="flex flex-wrap gap-2">
              {ALGORITHMS.map((algo) => {
                const isSelected = selected.includes(algo.key);
                return (
                  <button
                    key={algo.key}
                    onClick={() => toggleAlgorithm(algo.key)}
                    disabled={!isSelected && selected.length >= 3}
                    className={`rounded-lg border px-3 py-1.5 font-body text-xs transition-colors ${
                      isSelected
                        ? "border-signal-cyan/60 bg-signal-gradient-soft text-white"
                        : "border-line text-slate-400 hover:border-signal-cyan/30 disabled:opacity-30"
                    }`}
                  >
                    {algo.name}
                  </button>
                );
              })}
            </div>

            {needsAnyQuantum && (
              <label className="mt-4 block max-w-xs">
                <span className="font-body text-xs text-slate-400">
                  Time Quantum (used by Round Robin, if selected)
                </span>
                <input
                  type="number"
                  min={1}
                  value={timeQuantum}
                  onChange={(e) => setTimeQuantum(Number(e.target.value))}
                  className="mt-1 w-full rounded-md border border-line bg-void-700 px-3 py-2 font-mono text-sm text-slate-200"
                />
              </label>
            )}
          </div>

          {error && (
            <div className="rounded-xl border border-red-500/30 bg-red-500/5 p-4 font-body text-sm text-red-300">
              {error}
            </div>
          )}

          <button
            onClick={handleCompare}
            disabled={isLoading}
            className="w-full rounded-xl bg-signal-gradient px-6 py-3 font-body text-sm font-semibold text-void-950 shadow-glow transition-opacity hover:opacity-90 disabled:opacity-50"
          >
            {isLoading ? "Comparing…" : "⚖ Compare"}
          </button>
        </div>
      </div>

      <AnimatePresence>
        {results && (
          <motion.div
            initial={{ opacity: 0, y: 20 }}
            animate={{ opacity: 1, y: 0 }}
            exit={{ opacity: 0 }}
            className="mt-12 space-y-10"
          >
            {/* Gantt charts, one per algorithm */}
            <div className="grid grid-cols-1 gap-6">
              {selected.map((key) => {
                const res = results[key];
                if (!res) return null;
                return (
                  <div key={key}>
                    <h3 className="mb-2 font-display text-lg font-semibold text-white">
                      {getAlgorithmMeta(key).name}
                    </h3>
                    <GanttChart segments={res.ganttChart} processes={processes} totalTime={res.totalTime} />
                  </div>
                );
              })}
            </div>

            {/* Comparison table */}
            <div className="glass-card overflow-hidden p-5">
              <h3 className="mb-4 font-display text-sm font-semibold uppercase tracking-wide text-slate-400">
                Comparison Table
              </h3>
              <div className="thin-scroll overflow-x-auto">
                <table className="w-full min-w-[600px] font-mono text-sm">
                  <thead>
                    <tr className="border-b border-line text-left text-xs uppercase tracking-wide text-slate-500">
                      <th className="py-2 pr-4">Metric</th>
                      {selected.map((key) => (
                        <th key={key} className="py-2 pr-4">{getAlgorithmMeta(key).name}</th>
                      ))}
                    </tr>
                  </thead>
                  <tbody>
                    {METRICS.map((metric) => {
                      const best = bestKeyFor(metric);
                      return (
                        <tr key={metric.key} className="border-b border-line/60 last:border-0">
                          <td className="py-2 pr-4 text-slate-400">{metric.label}</td>
                          {selected.map((key) => {
                            const value = results[key]?.[metric.key];
                            const isBest = best === key;
                            return (
                              <td
                                key={key}
                                className={`py-2 pr-4 ${isBest ? "text-signal-cyan font-semibold" : "text-slate-300"}`}
                              >
                                {typeof value === "number" ? value.toFixed(2) : "—"}
                                {isBest && " ★"}
                              </td>
                            );
                          })}
                        </tr>
                      );
                    })}
                  </tbody>
                </table>
              </div>
            </div>

            {/* Bar charts per metric */}
            <div className="grid grid-cols-1 gap-6 md:grid-cols-2">
              {METRICS.map((metric) => {
                const data = selected.map((key) => ({
                  name: getAlgorithmMeta(key).name.split(" ")[0],
                  value: (results[key]?.[metric.key] as number) ?? 0,
                }));
                return (
                  <div key={metric.key} className="glass-card p-5">
                    <h4 className="mb-3 font-body text-xs uppercase tracking-wide text-slate-500">
                      {metric.label}
                    </h4>
                    <ResponsiveContainer width="100%" height={180}>
                      <BarChart data={data}>
                        <CartesianGrid strokeDasharray="3 3" stroke="#212a3d" vertical={false} />
                        <XAxis dataKey="name" stroke="#64748b" fontSize={11} />
                        <YAxis stroke="#64748b" fontSize={11} />
                        <Tooltip
                          contentStyle={{ background: "#0f1420", border: "1px solid #212a3d", borderRadius: 8, fontSize: 12 }}
                        />
                        <Bar dataKey="value" radius={[4, 4, 0, 0]}>
                          {data.map((_, i) => (
                            <Cell key={i} fill={CHART_COLORS[i % CHART_COLORS.length]} />
                          ))}
                        </Bar>
                      </BarChart>
                    </ResponsiveContainer>
                  </div>
                );
              })}
            </div>
          </motion.div>
        )}
      </AnimatePresence>
    </div>
  );
}
