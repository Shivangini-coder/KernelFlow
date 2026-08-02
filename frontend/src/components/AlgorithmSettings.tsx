import type { AlgorithmMeta } from "../data/algorithms";
import type { QueueConfig } from "../types";

interface AlgorithmSettingsProps {
  meta: AlgorithmMeta;
  timeQuantum: number;
  onTimeQuantumChange: (value: number) => void;
  queues: QueueConfig[];
  onQueuesChange: (queues: QueueConfig[]) => void;
  agingThreshold: number;
  onAgingThresholdChange: (value: number) => void;
}

// Only shown when the selected algorithm actually needs one of these
// settings — Round Robin needs a quantum; MLQ/MLFQ need queue
// configuration; MLFQ additionally needs an aging threshold.
export default function AlgorithmSettings({
  meta,
  timeQuantum,
  onTimeQuantumChange,
  queues,
  onQueuesChange,
  agingThreshold,
  onAgingThresholdChange,
}: AlgorithmSettingsProps) {
  if (!meta.needsQuantum && !meta.needsQueues) return null;

  function updateQueue(index: number, patch: Partial<QueueConfig>) {
    onQueuesChange(queues.map((q, i) => (i === index ? { ...q, ...patch } : q)));
  }

  function addQueue() {
    onQueuesChange([...queues, { algorithm: "FCFS", timeQuantum: 2 }]);
  }

  function removeQueue(index: number) {
    if (queues.length <= 1) return; // always keep at least one queue
    onQueuesChange(queues.filter((_, i) => i !== index));
  }

  return (
    <div className="glass-card p-5">
      <h3 className="mb-4 font-display text-sm font-semibold uppercase tracking-wide text-slate-400">
        Algorithm Settings
      </h3>

      {meta.needsQuantum && (
        <label className="block max-w-xs">
          <span className="font-body text-xs text-slate-400">Time Quantum</span>
          <input
            type="number"
            min={1}
            value={timeQuantum}
            onChange={(e) => onTimeQuantumChange(Number(e.target.value))}
            className="mt-1 w-full rounded-md border border-line bg-void-700 px-3 py-2 font-mono text-sm text-slate-200"
          />
        </label>
      )}

      {meta.needsQueues && (
        <div className="space-y-3">
          <div className="flex items-center justify-between">
            <span className="font-body text-xs text-slate-400">
              Queues (Queue 0 = highest priority)
            </span>
            <button
              onClick={addQueue}
              className="rounded-md border border-line px-2 py-1 font-body text-xs text-slate-300 hover:border-signal-cyan/50 hover:text-white"
            >
              + Add Queue
            </button>
          </div>

          {queues.map((queue, index) => (
            <div key={index} className="flex flex-wrap items-center gap-3 rounded-lg border border-line/70 p-3">
              <span className="font-mono text-xs text-slate-500">Queue {index}</span>

              <select
                value={queue.algorithm}
                onChange={(e) => updateQueue(index, { algorithm: e.target.value as QueueConfig["algorithm"] })}
                className="rounded-md border border-line bg-void-700 px-2 py-1 font-mono text-xs text-slate-200"
              >
                <option value="FCFS">FCFS</option>
                <option value="ROUND_ROBIN">Round Robin</option>
              </select>

              {queue.algorithm === "ROUND_ROBIN" && (
                <label className="flex items-center gap-2">
                  <span className="font-body text-xs text-slate-500">Quantum</span>
                  <input
                    type="number"
                    min={1}
                    value={queue.timeQuantum}
                    onChange={(e) => updateQueue(index, { timeQuantum: Number(e.target.value) })}
                    className="w-16 rounded-md border border-line bg-void-700 px-2 py-1 font-mono text-xs text-slate-200"
                  />
                </label>
              )}

              <button
                onClick={() => removeQueue(index)}
                disabled={queues.length <= 1}
                className="ml-auto rounded-md px-2 py-1 text-xs text-slate-500 transition-colors hover:bg-red-500/10 hover:text-red-400 disabled:opacity-30"
              >
                Remove
              </button>
            </div>
          ))}

          {meta.needsAging && (
            <label className="block max-w-xs pt-2">
              <span className="font-body text-xs text-slate-400">
                Aging Threshold (time units waiting before promotion)
              </span>
              <input
                type="number"
                min={0}
                value={agingThreshold}
                onChange={(e) => onAgingThresholdChange(Number(e.target.value))}
                className="mt-1 w-full rounded-md border border-line bg-void-700 px-3 py-2 font-mono text-sm text-slate-200"
              />
            </label>
          )}
        </div>
      )}
    </div>
  );
}
