import { ALGORITHMS } from "../data/algorithms";
import type { AlgorithmKey } from "../types";

interface AlgorithmSelectorProps {
  selected: AlgorithmKey | null;
  onSelect: (key: AlgorithmKey) => void;
  /** When set, disables any algorithm already picked elsewhere (used on the Compare page). */
  disabledKeys?: AlgorithmKey[];
}

export default function AlgorithmSelector({ selected, onSelect, disabledKeys = [] }: AlgorithmSelectorProps) {
  return (
    <div className="glass-card p-5">
      <h3 className="mb-4 font-display text-sm font-semibold uppercase tracking-wide text-slate-400">
        Algorithm
      </h3>
      <div className="grid grid-cols-1 gap-2 sm:grid-cols-2">
        {ALGORITHMS.map((algo) => {
          const isSelected = selected === algo.key;
          const isDisabled = disabledKeys.includes(algo.key);
          return (
            <button
              key={algo.key}
              disabled={isDisabled}
              onClick={() => onSelect(algo.key)}
              className={`rounded-xl border p-3 text-left transition-all ${
                isSelected
                  ? "border-signal-cyan/60 bg-signal-gradient-soft shadow-glow"
                  : isDisabled
                  ? "cursor-not-allowed border-line/50 opacity-40"
                  : "border-line hover:border-signal-cyan/30 hover:bg-white/[0.02]"
              }`}
            >
              <div className="flex items-center justify-between">
                <span className="font-body text-sm font-semibold text-slate-100">{algo.name}</span>
                <span
                  className={`rounded-full px-2 py-0.5 text-[10px] font-mono uppercase tracking-wide ${
                    algo.preemptive
                      ? "bg-signal-cyan/10 text-signal-cyan"
                      : "bg-signal-violet/10 text-signal-violet"
                  }`}
                >
                  {algo.preemptive ? "preemptive" : "non-preemptive"}
                </span>
              </div>
              {isSelected && (
                <p className="mt-2 font-body text-xs leading-relaxed text-slate-400">{algo.short}</p>
              )}
            </button>
          );
        })}
      </div>
    </div>
  );
}
