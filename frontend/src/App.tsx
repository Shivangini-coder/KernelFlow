import { Routes, Route } from "react-router-dom";
import Navbar from "./components/Navbar";
import Footer from "./components/Footer";
import Landing from "./pages/Landing";
import Simulation from "./pages/Simulation";
import Compare from "./pages/Compare";
import Learn from "./pages/Learn";

export default function App() {
  return (
    <div className="flex min-h-screen flex-col">
      <Navbar />
      <main className="flex-1">
        <Routes>
          <Route path="/" element={<Landing />} />
          <Route path="/simulation" element={<Simulation />} />
          <Route path="/compare" element={<Compare />} />
          <Route path="/learn" element={<Learn />} />
        </Routes>
      </main>
      <Footer />
    </div>
  );
}
