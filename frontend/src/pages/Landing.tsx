import { motion } from "framer-motion";
import { Link } from "react-router-dom";

const TIMELINE_DEMO = [
  { id: 1, color: "#3b82f6", width: 22 },
  { id: 2, color: "#22d3ee", width: 14 },
  { id: 3, color: "#818cf8", width: 30 },
  { id: 4, color: "#f472b6", width: 18 },
  { id: 5, color: "#34d399", width: 16 },
];

export default function Landing() {
  return (
    <div className="mx-auto max-w-7xl px-6">
      {/* HERO */}
      <section className="flex flex-col items-center pb-24 pt-20 text-center">
        <motion.div
          initial={{ opacity: 0, y: 12 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ duration: 0.5 }}
          className="mb-6 flex items-center gap-2 rounded-full border border-line bg-void-800/60 px-4 py-1.5"
        >
          <span className="h-1.5 w-1.5 animate-pulse rounded-full bg-signal-cyan" />
          <span className="font-mono text-xs text-slate-400">10 scheduling algorithms · C++ core</span>
        </motion.div>

        <motion.img
          initial={{ opacity: 0, y: 16, scale: 0.96 }}
          animate={{ opacity: 1, y: 0, scale: 1 }}
          transition={{ duration: 0.7, delay: 0.05, ease: "easeOut" }}
          src="/kernelflow-logo-full.png"
          alt="KernelFlow — interactive CPU scheduling visualizer"
          className="w-full max-w-md drop-shadow-[0_0_60px_rgba(34,211,238,0.15)] sm:max-w-lg"
        />

        <motion.h1
          initial={{ opacity: 0, y: 16 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ duration: 0.6, delay: 0.12 }}
          className="sr-only"
        >
          KernelFlow
        </motion.h1>

        <motion.p
          initial={{ opacity: 0, y: 16 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ duration: 0.6, delay: 0.12 }}
          className="mt-2 font-display text-xl font-semibold text-slate-300"
        >
          Visualize. Compare. Understand CPU Scheduling.
        </motion.p>

        <motion.p
          initial={{ opacity: 0, y: 16 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ duration: 0.6, delay: 0.18 }}
          className="mt-4 max-w-xl font-body text-base text-slate-400"
        >
          An interactive platform for visualizing, comparing, and understanding CPU scheduling
          algorithms — from First Come First Served to Multilevel Feedback Queue.
        </motion.p>

        <motion.div
          initial={{ opacity: 0, y: 16 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ duration: 0.6, delay: 0.24 }}
          className="mt-8 flex flex-wrap items-center justify-center gap-3"
        >
          <Link
            to="/simulation"
            className="rounded-xl bg-signal-gradient px-6 py-3 font-body text-sm font-semibold text-void-950 shadow-glow transition-transform hover:scale-[1.02]"
          >
            Start Simulation
          </Link>
          <Link
            to="/learn"
            className="rounded-xl border border-line px-6 py-3 font-body text-sm font-semibold text-slate-200 transition-colors hover:border-signal-cyan/50"
          >
            Learn CPU Scheduling
          </Link>
        </motion.div>

        {/* Signature element: a live-looking Gantt timeline as the hero's
            visual centerpiece, echoing the product itself. */}
        <motion.div
          initial={{ opacity: 0, y: 24 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ duration: 0.7, delay: 0.32 }}
          className="glass-panel mt-16 w-full max-w-3xl rounded-2xl p-6"
        >
          <div className="mb-3 flex items-center justify-between">
            <span className="font-mono text-xs text-slate-500">gantt_chart.live</span>
            <span className="flex gap-1.5">
              <span className="h-2 w-2 rounded-full bg-red-400/60" />
              <span className="h-2 w-2 rounded-full bg-yellow-400/60" />
              <span className="h-2 w-2 rounded-full bg-green-400/60" />
            </span>
          </div>
          <div className="flex h-10 overflow-hidden rounded-lg border border-line">
            {TIMELINE_DEMO.map((block, i) => (
              <motion.div
                key={block.id}
                initial={{ scaleX: 0 }}
                animate={{ scaleX: 1 }}
                transition={{ duration: 0.5, delay: 0.5 + i * 0.1, ease: "easeOut" }}
                style={{ width: `${block.width}%`, background: block.color, transformOrigin: "left" }}
                className="flex items-center justify-center border-r border-void-950 font-mono text-[11px] text-white/90"
              >
                P{block.id}
              </motion.div>
            ))}
          </div>
        </motion.div>
      </section>

      <div className="signal-divider" />

      {/* WHY KERNELFLOW */}
      <section className="grid grid-cols-1 gap-6 py-20 sm:grid-cols-3">
        {[
          {
            title: "10 real algorithms",
            body: "FCFS, SJF, LJF, SRTF, LRTF, Round Robin, both Priority variants, MLQ, and MLFQ — each backed by hand-written C++.",
          },
          {
            title: "Compare side by side",
            body: "Run up to three algorithms on the same process set and see exactly where each one wins or loses.",
          },
          {
            title: "Built to teach",
            body: "Every algorithm ships with a plain-English explanation, advantages, disadvantages, and typical use cases.",
          },
        ].map((card) => (
          <div key={card.title} className="glass-card p-6">
            <h3 className="font-display text-lg font-semibold text-white">{card.title}</h3>
            <p className="mt-2 font-body text-sm leading-relaxed text-slate-400">{card.body}</p>
          </div>
        ))}
      </section>
    </div>
  );
}
