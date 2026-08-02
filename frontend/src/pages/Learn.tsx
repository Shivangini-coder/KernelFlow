import { motion } from "framer-motion";
import { ALGORITHMS } from "../data/algorithms";
import { CORE_CONCEPTS } from "../data/learnContent";

export default function Learn() {
  return (
    <div className="mx-auto max-w-7xl px-6 py-12">
      <div className="mb-10">
        <h1 className="font-display text-3xl font-bold text-white">Learn CPU Scheduling</h1>
        <p className="mt-1 max-w-2xl font-body text-sm text-slate-400">
          The core vocabulary of CPU scheduling, followed by a plain-English breakdown of every
          algorithm KernelFlow can simulate.
        </p>
      </div>

      {/* Core concepts */}
      <section className="mb-16">
        <h2 className="mb-5 font-display text-lg font-semibold text-white">Core Concepts</h2>
        <div className="grid grid-cols-1 gap-4 sm:grid-cols-2 lg:grid-cols-3">
          {CORE_CONCEPTS.map((concept, i) => (
            <motion.div
              key={concept.term}
              initial={{ opacity: 0, y: 10 }}
              whileInView={{ opacity: 1, y: 0 }}
              viewport={{ once: true }}
              transition={{ delay: i * 0.03 }}
              className="glass-card p-5"
            >
              <h3 className="font-display text-sm font-semibold text-signal-cyan">{concept.term}</h3>
              <p className="mt-2 font-body text-sm leading-relaxed text-slate-400">{concept.definition}</p>
            </motion.div>
          ))}
        </div>
      </section>

      <div className="signal-divider mb-16" />

      {/* Algorithms */}
      <section>
        <h2 className="mb-5 font-display text-lg font-semibold text-white">Scheduling Algorithms</h2>
        <div className="space-y-6">
          {ALGORITHMS.map((algo, i) => (
            <motion.div
              key={algo.key}
              initial={{ opacity: 0, y: 10 }}
              whileInView={{ opacity: 1, y: 0 }}
              viewport={{ once: true }}
              transition={{ delay: Math.min(i * 0.04, 0.3) }}
              className="glass-card p-6"
            >
              <div className="flex flex-wrap items-center justify-between gap-2">
                <h3 className="font-display text-xl font-bold text-white">{algo.name}</h3>
                <span
                  className={`rounded-full px-2.5 py-0.5 text-[10px] font-mono uppercase tracking-wide ${
                    algo.preemptive ? "bg-signal-cyan/10 text-signal-cyan" : "bg-signal-violet/10 text-signal-violet"
                  }`}
                >
                  {algo.preemptive ? "preemptive" : "non-preemptive"}
                </span>
              </div>

              <p className="mt-3 font-body text-sm leading-relaxed text-slate-300">{algo.howItWorks}</p>

              <div className="mt-5 grid grid-cols-1 gap-6 sm:grid-cols-3">
                <div>
                  <h4 className="font-body text-xs font-semibold uppercase tracking-wide text-emerald-400">
                    Advantages
                  </h4>
                  <ul className="mt-2 space-y-1.5">
                    {algo.advantages.map((a) => (
                      <li key={a} className="font-body text-xs leading-relaxed text-slate-400">
                        · {a}
                      </li>
                    ))}
                  </ul>
                </div>
                <div>
                  <h4 className="font-body text-xs font-semibold uppercase tracking-wide text-rose-400">
                    Disadvantages
                  </h4>
                  <ul className="mt-2 space-y-1.5">
                    {algo.disadvantages.map((d) => (
                      <li key={d} className="font-body text-xs leading-relaxed text-slate-400">
                        · {d}
                      </li>
                    ))}
                  </ul>
                </div>
                <div>
                  <h4 className="font-body text-xs font-semibold uppercase tracking-wide text-signal-cyan">
                    Typical Use Cases
                  </h4>
                  <p className="mt-2 font-body text-xs leading-relaxed text-slate-400">{algo.useCases}</p>
                </div>
              </div>
            </motion.div>
          ))}
        </div>
      </section>
    </div>
  );
}
