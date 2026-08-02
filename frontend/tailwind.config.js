/**
 * Tailwind config — this is where KernelFlow's visual identity (the "design
 * tokens" from the README/design plan) actually gets wired up:
 *   - a near-black, slightly blue-tinted background ("void")
 *   - a blue -> cyan signal gradient used for every accent
 *   - Sora for display type, Inter for body, JetBrains Mono for anything
 *     that reads like real scheduler data (timings, IDs, metrics)
 */
export default {
  content: ["./index.html", "./src/**/*.{js,ts,jsx,tsx}"],
  theme: {
    extend: {
      colors: {
        void: {
          950: "#05070d",
          900: "#0a0e17",
          800: "#0f1420",
          700: "#151b2b",
          600: "#1c2438",
        },
        signal: {
          blue: "#3b82f6",
          cyan: "#22d3ee",
          violet: "#818cf8",
        },
        line: "#212a3d",
      },
      fontFamily: {
        display: ["Sora", "sans-serif"],
        body: ["Inter", "sans-serif"],
        mono: ["JetBrains Mono", "monospace"],
      },
      backgroundImage: {
        "signal-gradient": "linear-gradient(90deg, #3b82f6 0%, #22d3ee 100%)",
        "signal-gradient-soft": "linear-gradient(135deg, rgba(59,130,246,0.15) 0%, rgba(34,211,238,0.08) 100%)",
      },
      boxShadow: {
        glow: "0 0 40px rgba(34,211,238,0.15)",
      },
    },
  },
  plugins: [],
};
