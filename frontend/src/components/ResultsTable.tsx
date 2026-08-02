import type { ProcessInput, ProcessResult } from "../types";

interface ResultsTableProps {
  results: ProcessResult[];
  processes: ProcessInput[];
  showPriority?: boolean;
  showQueueIndex?: boolean;
  showQueueMeta?: boolean; // MLFQ: finalQueueLevel / queueMovements
}

export default function ResultsTable({
  results,
  processes,
  showPriority,
  showQueueIndex,
  showQueueMeta,
}: ResultsTableProps) {
  const colorFor = (id: number) => processes.find((p) => p.id === id)?.color ?? "#334155";

  return (
    <div className="glass-card overflow-hidden p-5">
      <h3 className="mb-4 font-display text-sm font-semibold uppercase tracking-wide text-slate-400">
        Process Results
      </h3>
      <div className="thin-scroll overflow-x-auto">
        <table className="w-full min-w-[700px] font-mono text-sm">
          <thead>
            <tr className="border-b border-line text-left text-xs uppercase tracking-wide text-slate-500">
              <th className="py-2 pr-4">PID</th>
              <th className="py-2 pr-4">Arrival</th>
              <th className="py-2 pr-4">Burst</th>
              {showPriority && <th className="py-2 pr-4">Priority</th>}
              {showQueueIndex && <th className="py-2 pr-4">Queue</th>}
              <th className="py-2 pr-4">CT</th>
              <th className="py-2 pr-4">TAT</th>
              <th className="py-2 pr-4">WT</th>
              <th className="py-2 pr-4">RT</th>
              {showQueueMeta && <th className="py-2 pr-4">Final Queue</th>}
              {showQueueMeta && <th className="py-2 pr-4">Movements</th>}
            </tr>
          </thead>
          <tbody>
            {results.map((r) => (
              <tr key={r.id} className="border-b border-line/60 last:border-0">
                <td className="py-2 pr-4">
                  <span className="flex items-center gap-2">
                    <span className="h-2.5 w-2.5 rounded-sm" style={{ background: colorFor(r.id) }} />
                    P{r.id}
                  </span>
                </td>
                <td className="py-2 pr-4 text-slate-300">{r.arrivalTime}</td>
                <td className="py-2 pr-4 text-slate-300">{r.burstTime}</td>
                {showPriority && <td className="py-2 pr-4 text-slate-300">{r.priority}</td>}
                {showQueueIndex && <td className="py-2 pr-4 text-slate-300">{r.queueIndex}</td>}
                <td className="py-2 pr-4 text-slate-300">{r.completionTime}</td>
                <td className="py-2 pr-4 text-slate-300">{r.turnaroundTime}</td>
                <td className="py-2 pr-4 text-slate-300">{r.waitingTime}</td>
                <td className="py-2 pr-4 text-slate-300">{r.responseTime}</td>
                {showQueueMeta && <td className="py-2 pr-4 text-slate-300">{r.finalQueueLevel}</td>}
                {showQueueMeta && <td className="py-2 pr-4 text-slate-300">{r.queueMovements}</td>}
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  );
}
