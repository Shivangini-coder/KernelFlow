import { motion } from "framer-motion";

interface MetricCardProps {
  label: string;
  value: string;
  accent?: boolean;
}

export default function MetricCard({ label, value, accent }: MetricCardProps) {
  return (
    <motion.div
      initial={{ opacity: 0, y: 8 }}
      animate={{ opacity: 1, y: 0 }}
      className="glass-card p-4"
    >
      <p className="font-body text-xs uppercase tracking-wide text-slate-500">{label}</p>
      <p
        className={`mt-1 font-mono text-2xl font-semibold ${
          accent ? "text-transparent bg-clip-text bg-signal-gradient" : "text-white"
        }`}
      >
        {value}
      </p>
    </motion.div>
  );
}
