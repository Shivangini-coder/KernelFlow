import { NavLink } from "react-router-dom";
import { motion } from "framer-motion";

const NAV_LINKS = [
  { to: "/", label: "Home" },
  { to: "/simulation", label: "Simulation" },
  { to: "/compare", label: "Compare" },
  { to: "/learn", label: "Learn CPU Scheduling" },
];

export default function Navbar() {
  return (
    <header className="sticky top-0 z-50 border-b border-line bg-void-900/80 backdrop-blur-xl">
      <nav className="mx-auto flex max-w-7xl items-center justify-between px-6 py-4">
        <NavLink to="/" className="flex items-center gap-2.5 group">
          {/* Cropped from the KernelFlow chip artwork (public/kernelflow-icon.png) —
              just the CPU-chip mark, since the full artwork's baked-in
              wordmark would blur unreadably at this size. Real text below
              stays crisp at any zoom level instead. */}
          <motion.img
            src="/kernelflow-icon.png"
            alt=""
            aria-hidden="true"
            className="h-9 w-9 rounded-lg object-cover"
            whileHover={{ rotate: -6, scale: 1.05 }}
            transition={{ type: "spring", stiffness: 300 }}
          />
          <span className="font-display text-lg font-bold tracking-tight text-white">
            Kernel<span className="text-transparent bg-clip-text bg-signal-gradient">Flow</span>
          </span>
        </NavLink>

        <div className="hidden items-center gap-1 md:flex">
          {NAV_LINKS.map((link) => (
            <NavLink
              key={link.to}
              to={link.to}
              className={({ isActive }) =>
                `rounded-lg px-4 py-2 font-body text-sm transition-colors ${
                  isActive
                    ? "text-white bg-white/5"
                    : "text-slate-400 hover:text-white hover:bg-white/5"
                }`
              }
            >
              {link.label}
            </NavLink>
          ))}
          <a
            href="https://github.com/Shivangini-coder"
            target="_blank"
            rel="noreferrer"
            className="ml-2 rounded-lg border border-line px-4 py-2 font-body text-sm text-slate-300 transition-colors hover:border-signal-cyan/50 hover:text-white"
          >
            GitHub
          </a>
        </div>

        {/* Compact mobile nav — simple horizontal scroll instead of a full
            hamburger menu, to keep this file's scope small. */}
        <div className="flex gap-3 overflow-x-auto md:hidden">
          {NAV_LINKS.map((link) => (
            <NavLink
              key={link.to}
              to={link.to}
              className="whitespace-nowrap text-xs text-slate-400"
            >
              {link.label}
            </NavLink>
          ))}
        </div>
      </nav>
    </header>
  );
}
