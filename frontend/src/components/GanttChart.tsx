import { motion } from "framer-motion";
import type { GanttSegment, ProcessInput } from "../types";

interface GanttChartProps {
  segments: GanttSegment[];
  processes: ProcessInput[];
  totalTime: number;
}

export default function GanttChart({ segments, processes, totalTime }: GanttChartProps) {
  const colorFor = (processId: number) =>
    processes.find((p) => p.id === processId)?.color ?? "#334155";

  // Time-axis tick marks — show at most ~12 labels so long simulations
  // don't turn into unreadable clutter.
  const tickCount = Math.min(12, totalTime || 1);
  const tickStep = Math.max(1, Math.round((totalTime || 1) / tickCount));
  const ticks = Array.from({ length: Math.floor((totalTime || 1) / tickStep) + 1 }, (_, i) => i * tickStep);

  return (
    <div className="glass-card p-5">
      <h3 className="mb-4 font-display text-sm font-semibold uppercase tracking-wide text-slate-400">
        Gantt Chart
      </h3>

      <div className="thin-scroll overflow-x-auto pb-2">
        <div className="relative h-14 min-w-[600px] overflow-hidden rounded-lg border border-line bg-void-950">
          {segments.map((segment, index) => {
            const widthPercent = totalTime > 0 ? ((segment.endTime - segment.startTime) / totalTime) * 100 : 0;
            const leftPercent = totalTime > 0 ? (segment.startTime / totalTime) * 100 : 0;
            const isIdle = segment.processId === -1;

            return (
              <motion.div
                key={index}
                initial={{ scaleX: 0, opacity: 0 }}
                animate={{ scaleX: 1, opacity: 1 }}
                transition={{ duration: 0.4, delay: index * 0.03, ease: "easeOut" }}
                style={{
                  position: "absolute",
                  left: `${leftPercent}%`,
                  width: `${widthPercent}%`,
                  top: 0,
                  bottom: 0,
                  transformOrigin: "left",
                  background: isIdle ? "repeating-linear-gradient(45deg, #151b2b, #151b2b 4px, #0a0e17 4px, #0a0e17 8px)" : colorFor(segment.processId),
                }}
                className="group flex items-center justify-center border-r border-void-950"
                title={`${isIdle ? "Idle" : `P${segment.processId}`}: ${segment.startTime} → ${segment.endTime}`}
              >
                {widthPercent > 4 && (
                  <span className="font-mono text-[11px] font-semibold text-white/90 drop-shadow-[0_1px_1px_rgba(0,0,0,0.5)]">
                    {isIdle ? "idle" : `P${segment.processId}`}
                  </span>
                )}
              </motion.div>
            );
          })}
        </div>

        {/* Time axis */}
        <div className="relative mt-1 h-4 min-w-[600px]">
          {ticks.map((tick) => (
            <span
              key={tick}
              style={{ position: "absolute", left: `${totalTime > 0 ? (tick / totalTime) * 100 : 0}%` }}
              className="-translate-x-1/2 font-mono text-[10px] text-slate-500"
            >
              {tick}
            </span>
          ))}
        </div>
      </div>

      {/* Legend */}
      <div className="mt-3 flex flex-wrap gap-3">
        {processes.map((p) => (
          <div key={p.id} className="flex items-center gap-1.5">
            <span className="h-2.5 w-2.5 rounded-sm" style={{ background: p.color }} />
            <span className="font-mono text-xs text-slate-400">P{p.id}</span>
          </div>
        ))}
        <div className="flex items-center gap-1.5">
          <span className="h-2.5 w-2.5 rounded-sm border border-line bg-void-950" />
          <span className="font-mono text-xs text-slate-500">idle</span>
        </div>
      </div>
    </div>
  );
}
