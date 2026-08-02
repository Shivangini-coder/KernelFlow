import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

// Vite config: React plugin only. The dev server runs on port 5173 by
// default, which is the origin backend/main.py's CORS middleware allows.
export default defineConfig({
  plugins: [react()],
  server: { port: 5173 },
});
