import type { ProcessInput } from "../types";

const PALETTE = ["#3b82f6", "#22d3ee", "#818cf8", "#f472b6", "#fb923c", "#34d399", "#facc15", "#f87171"];

export function nextColor(existing: ProcessInput[]): string {
  return PALETTE[existing.length % PALETTE.length];
}

interface ProcessTableProps {
  processes: ProcessInput[];
  onChange: (processes: ProcessInput[]) => void;
  showPriority: boolean;
  showQueueIndex: boolean;
  queueCount: number;
}

export default function ProcessTable({
  processes,
  onChange,
  showPriority,
  showQueueIndex,
  queueCount,
}: ProcessTableProps) {
  function updateProcess(id: number, patch: Partial<ProcessInput>) {
    onChange(processes.map((p) => (p.id === id ? { ...p, ...patch } : p)));
  }

  function addProcess() {
    const nextId = processes.length > 0 ? Math.max(...processes.map((p) => p.id)) + 1 : 1;
    onChange([
      ...processes,
      {
        id: nextId,
        arrivalTime: 0,
        burstTime: 1,
        priority: 1,
        queueIndex: 0,
        color: nextColor(processes),
      },
    ]);
  }

  function deleteProcess(id: number) {
    onChange(processes.filter((p) => p.id !== id));
  }

  return (
    <div className="glass-card p-5">
      <div className="mb-4 flex items-center justify-between">
        <h3 className="font-display text-sm font-semibold uppercase tracking-wide text-slate-400">
          Processes
        </h3>
        <button
          onClick={addProcess}
          className="rounded-lg bg-signal-gradient px-3 py-1.5 font-body text-xs font-semibold text-void-950 transition-opacity hover:opacity-90"
        >
          + Add Process
        </button>
      </div>

      {processes.length === 0 ? (
        <div className="rounded-xl border border-dashed border-line py-10 text-center">
          <p className="font-body text-sm text-slate-500">
            No processes yet. Add one, generate random processes, or load a sample test case below.
          </p>
        </div>
      ) : (
        <div className="thin-scroll overflow-x-auto">
          <table className="w-full min-w-[560px] font-mono text-sm">
            <thead>
              <tr className="border-b border-line text-left text-xs uppercase tracking-wide text-slate-500">
                <th className="py-2 pr-3">Color</th>
                <th className="py-2 pr-3">PID</th>
                <th className="py-2 pr-3">Arrival</th>
                <th className="py-2 pr-3">Burst</th>
                {showPriority && <th className="py-2 pr-3">Priority</th>}
                {showQueueIndex && <th className="py-2 pr-3">Queue</th>}
                <th className="py-2 pr-3" />
              </tr>
            </thead>
            <tbody>
              {processes.map((p) => (
                <tr key={p.id} className="border-b border-line/60 last:border-0">
                  <td className="py-2 pr-3">
                    <input
                      type="color"
                      value={p.color}
                      onChange={(e) => updateProcess(p.id, { color: e.target.value })}
                      className="h-6 w-6 cursor-pointer rounded border-0 bg-transparent"
                      aria-label={`Color for process ${p.id}`}
                    />
                  </td>
                  <td className="py-2 pr-3 text-slate-300">P{p.id}</td>
                  <td className="py-2 pr-3">
                    <input
                      type="number"
                      min={0}
                      value={p.arrivalTime}
                      onChange={(e) => updateProcess(p.id, { arrivalTime: Number(e.target.value) })}
                      className="w-16 rounded-md border border-line bg-void-700 px-2 py-1 text-slate-200"
                    />
                  </td>
                  <td className="py-2 pr-3">
                    <input
                      type="number"
                      min={1}
                      value={p.burstTime}
                      onChange={(e) => updateProcess(p.id, { burstTime: Number(e.target.value) })}
                      className="w-16 rounded-md border border-line bg-void-700 px-2 py-1 text-slate-200"
                    />
                  </td>
                  {showPriority && (
                    <td className="py-2 pr-3">
                      <input
                        type="number"
                        min={1}
                        value={p.priority ?? 1}
                        onChange={(e) => updateProcess(p.id, { priority: Number(e.target.value) })}
                        className="w-16 rounded-md border border-line bg-void-700 px-2 py-1 text-slate-200"
                      />
                    </td>
                  )}
                  {showQueueIndex && (
                    <td className="py-2 pr-3">
                      <select
                        value={p.queueIndex ?? 0}
                        onChange={(e) => updateProcess(p.id, { queueIndex: Number(e.target.value) })}
                        className="rounded-md border border-line bg-void-700 px-2 py-1 text-slate-200"
                      >
                        {Array.from({ length: queueCount }).map((_, i) => (
                          <option key={i} value={i}>
                            Queue {i}
                          </option>
                        ))}
                      </select>
                    </td>
                  )}
                  <td className="py-2 pr-3 text-right">
                    <button
                      onClick={() => deleteProcess(p.id)}
                      className="rounded-md px-2 py-1 text-xs text-slate-500 transition-colors hover:bg-red-500/10 hover:text-red-400"
                      aria-label={`Delete process ${p.id}`}
                    >
                      Remove
                    </button>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}
    </div>
  );
}
