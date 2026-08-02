"""
================================================================================
main.py — KernelFlow's ENTIRE backend, in one file.
================================================================================
WHAT THIS FILE IS RESPONSIBLE FOR:
    This is the only Python file in the project. It is a FastAPI app that:
      1. Receives an HTTP request from the React frontend (JSON: a list of
         processes, plus algorithm-specific settings like a time quantum).
      2. Validates that JSON automatically, using Pydantic models (the
         "shape" classes defined below) — FastAPI does this for us just by
         us declaring the shape of the data we expect.
      3. Converts that validated data BACK into plain JSON text and hands it
         to one of the compiled C++ "bridge" programs in backend/bin/ as
         that program's stdin.
      4. Reads the JSON the C++ program prints to stdout, and sends it
         straight back to React as the HTTP response.

WHAT THIS FILE IS **NOT** RESPONSIBLE FOR:
    It never computes a single completion time, waiting time, or Gantt
    chart itself. ALL scheduling logic lives in your original .cpp files
    (backend/cpp/) — this file is just plumbing that connects React to
    those programs over a subprocess boundary.

WHY SUBPROCESSES INSTEAD OF, SAY, A C++ PYTHON EXTENSION?
    Subprocesses are the simplest possible way to call a compiled program
    from Python — there's no build tooling to learn (no pybind11, no
    ctypes, no ABI compatibility issues). We just run the program like you
    would from a terminal, feed it text on stdin, and read text back from
    stdout. This is intentionally the "easy way" since this project's goal
    is to teach FastAPI, not C/Python interop.

HOW REACT ACTUALLY TALKS TO THIS FILE:
    React's fetch() or axios call does something like:
        POST http://localhost:8000/simulate/fcfs
        Body: { "processes": [ { "id": 1, "arrivalTime": 0, "burstTime": 5 } ] }
    FastAPI receives that on the matching @app.post(...) route below,
    matches it against the Pydantic model for that route, runs the
    corresponding C++ bridge, and returns its JSON output. That's the
    entire request lifecycle for every endpoint in this file.

HOW TO RUN THIS FILE:
    1. First compile the C++ bridges:      bash build.sh
    2. Then start the FastAPI server:      python main.py
       (this reads the PORT constant defined below — currently 8000 — so
       that's the ONE place in this project that decides the port; see
       STEP 3b and STEP 8 further down for exactly how)
    3. Visit http://localhost:8000/docs to see (and try!) every endpoint
       FastAPI auto-generates an interactive API explorer for you — this
       is one of the best beginner-friendly features of FastAPI.

    Alternative: `uvicorn main:app --reload` also works and happens to land
    on the same port (8000 is uvicorn's own default too), but it does NOT
    read the PORT constant below — so if you ever change PORT, use
    `python main.py` (or pass `--port` yourself) instead.
================================================================================
"""

import json
import platform
import subprocess
from pathlib import Path
from typing import List, Optional

from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel


# ------------------------------------------------------------------------------
# STEP 1: Create the FastAPI application object.
# ------------------------------------------------------------------------------
# `app` is the central object FastAPI uses to know about every route
# (endpoint) we define below with @app.get(...) / @app.post(...). Uvicorn
# (the server that actually runs this) is told to serve THIS object.
# ------------------------------------------------------------------------------
app = FastAPI(
    title="KernelFlow API",
    description="Invokes hand-written C++ CPU scheduling algorithms and returns their results as JSON.",
    version="1.0.0",
)


# ------------------------------------------------------------------------------
# STEP 2: Allow the React frontend (running on a different port) to call us.
# ------------------------------------------------------------------------------
# Browsers block JavaScript from calling a different "origin" (different
# host/port) than the page itself, unless the SERVER explicitly says it's
# okay — this is called CORS (Cross-Origin Resource Sharing). Since React
# runs on http://localhost:5173 (Vite's default) and FastAPI runs on
# http://localhost:8000, we need this middleware or every fetch() call from
# React would be silently blocked by the browser.
# ------------------------------------------------------------------------------
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],   # For a learning project, allow any origin. In a
                            # real production app you'd list exact domains.
    allow_methods=["*"],
    allow_headers=["*"],
)


# ------------------------------------------------------------------------------
# STEP 3: Figure out where the compiled C++ bridge programs live on disk.
# ------------------------------------------------------------------------------
# Path(__file__).parent gives us the folder this main.py file is sitting in
# (backend/), no matter what directory the person running the server is
# currently standing in when they type `uvicorn main:app`.
# ------------------------------------------------------------------------------
BACKEND_DIR = Path(__file__).parent
BIN_DIR = BACKEND_DIR / "bin"


# ------------------------------------------------------------------------------
# STEP 3b: The port this server runs on — defined ONCE, right here.
# ------------------------------------------------------------------------------
# Nothing else in this project hardcodes a port number. If you start the
# server the recommended way (`python main.py`, see the bottom of this
# file), THIS constant is what decides the port. If you instead start it
# with the `uvicorn main:app --reload` command, uvicorn ignores this
# constant and uses its own default (also 8000) — so either way you land on
# the same port, but only one of those two paths actually reads this line.
# If you ever want KernelFlow on a different port, change it here — the
# frontend then needs to match via its VITE_API_BASE_URL (see frontend/.env).
# ------------------------------------------------------------------------------
PORT = 8002


# ------------------------------------------------------------------------------
# STEP 4: Pydantic "shape" models.
# ------------------------------------------------------------------------------
# Every class below that inherits from BaseModel describes the exact JSON
# shape FastAPI should expect for a request body. FastAPI reads the type
# hints (int, str, Optional[int], List[...]) and automatically:
#   - Rejects requests missing a required field, with a clear error message.
#   - Rejects requests where a field is the wrong type (e.g. a string where
#     an int was expected).
#   - Converts the incoming JSON into a real Python object we can just use
#     directly in our endpoint functions below.
# This is the single biggest reason FastAPI is beginner-friendly: you never
# hand-write JSON validation code.
# ------------------------------------------------------------------------------

class ProcessInput(BaseModel):
    """One process the user typed into the Simulation page's process table."""
    id: int
    arrivalTime: int
    burstTime: int
    # priority is ONLY required by the two Priority Scheduling endpoints.
    # queueIndex is ONLY required by the MLQ endpoint. Making both Optional
    # here means the SAME model can be reused across every endpoint below —
    # each endpoint's own C++ bridge just ignores whichever field it
    # doesn't need.
    priority: Optional[int] = None
    queueIndex: Optional[int] = None


class QueueConfig(BaseModel):
    """One ready-queue's settings, used only by the MLQ and MLFQ endpoints."""
    algorithm: str    # either "FCFS" or "ROUND_ROBIN"
    timeQuantum: int  # ignored by the C++ side when algorithm == "FCFS"


class SimpleSimulationRequest(BaseModel):
    """Request body shape for every algorithm that ONLY needs a process list:
    FCFS, SJF, LJF, SRTF, LRTF, and both Priority algorithms."""
    processes: List[ProcessInput]


class RoundRobinRequest(BaseModel):
    """Request body shape for Round Robin — same as SimpleSimulationRequest,
    plus the one extra field RR needs: the time quantum."""
    processes: List[ProcessInput]
    timeQuantum: int


class MultilevelQueueRequest(BaseModel):
    """Request body shape for MLQ — a process list PLUS a list of queue
    configurations (one per queue level, in priority order)."""
    processes: List[ProcessInput]
    queues: List[QueueConfig]


class MultilevelFeedbackQueueRequest(BaseModel):
    """Request body shape for MLFQ — same as MLQ, plus the aging threshold
    (how long a process can wait before being promoted a queue level)."""
    processes: List[ProcessInput]
    queues: List[QueueConfig]
    agingThreshold: int


# ------------------------------------------------------------------------------
# STEP 5: The one helper function every endpoint below calls.
# ------------------------------------------------------------------------------
# This is where the actual "talk to C++" work happens. Every endpoint does
# the exact same three things (build a JSON string, run a subprocess, parse
# the JSON it prints back) so we write that logic ONCE here instead of
# repeating it in every endpoint function.
# ------------------------------------------------------------------------------
def run_cpp_bridge(binary_name: str, request_payload: dict) -> dict:
    """
    Runs one compiled C++ bridge program and returns its JSON output as a
    Python dict.

    binary_name:     e.g. "fcfs_bridge" — must match a file in backend/bin/
    request_payload: a plain dict, which we turn into a JSON string and
                      feed to the C++ program's stdin.
    """

    # WINDOWS VS. LINUX/MAC NOTE:
    # build.sh compiles with g++, but g++ behaves differently depending on
    # the OS it's running on: on Windows (including MinGW, which is what
    # Git Bash uses) it automatically names the output "fcfs_bridge.exe",
    # while on Linux/macOS it names it just "fcfs_bridge" with no
    # extension — Windows executables always need an extension, Unix ones
    # don't. platform.system() tells us which OS THIS Python process is
    # currently running on, so we know which filename to actually look for.
    binary_filename = f"{binary_name}.exe" if platform.system() == "Windows" else binary_name
    binary_path = BIN_DIR / binary_filename

    # Friendly error if someone forgot to run build.sh first — this is the
    # #1 mistake a beginner following this project will make, so we give a
    # message that tells them exactly what to do.
    if not binary_path.exists():
        raise HTTPException(
            status_code=500,
            detail=(
                f"Compiled program '{binary_filename}' was not found in backend/bin/. "
                f"Did you run 'bash build.sh' (or the Git Bash equivalent) to compile the C++ bridges first?"
            ),
        )

    # json.dumps converts our Python dict into a JSON-formatted string —
    # this is exactly what the C++ side's mini_json.hpp parser expects to
    # read from stdin.
    request_json_text = json.dumps(request_payload)

    # subprocess.run launches the compiled C++ program as if we'd typed its
    # path into a terminal. `input=` feeds our JSON string to its stdin,
    # and `capture_output=True` lets us read back whatever it prints to
    # stdout (its JSON response) instead of it going to our own terminal.
    process_result = subprocess.run(
        [str(binary_path)],
        input=request_json_text,
        capture_output=True,
        text=True,       # treat stdin/stdout as text (str), not raw bytes
        timeout=10,       # safety net: never let one bad request hang forever
    )

    # A non-zero return code means the C++ program crashed or hit an
    # exception (e.g. it received malformed input). Surface that clearly
    # instead of silently returning nothing.
    if process_result.returncode != 0:
        raise HTTPException(
            status_code=500,
            detail=f"{binary_name} failed: {process_result.stderr.strip()}",
        )

    # Turn the JSON text the C++ program printed back into a Python dict,
    # which FastAPI will automatically re-serialize into the HTTP response.
    return json.loads(process_result.stdout)


# ------------------------------------------------------------------------------
# STEP 6: One endpoint per scheduling algorithm.
# ------------------------------------------------------------------------------
# Every function below is a "route": a URL path plus an HTTP method (POST,
# since the frontend is SENDING data, not just fetching a page). The
# `request: SimpleSimulationRequest` parameter tells FastAPI "parse and
# validate the incoming JSON body using this shape" — by the time our
# function body runs, `request` is already a fully validated Python object.
# `request.dict()` turns it back into a plain dict so we can hand it to
# run_cpp_bridge(...) above.
# ------------------------------------------------------------------------------

@app.post("/simulate/fcfs")
def simulate_fcfs(request: SimpleSimulationRequest):
    """First Come First Served — runs processes strictly in arrival order."""
    return run_cpp_bridge("fcfs_bridge", request.dict())


@app.post("/simulate/sjf")
def simulate_sjf(request: SimpleSimulationRequest):
    """Shortest Job First (Non-Preemptive) — always runs the shortest
    available burst time to completion."""
    return run_cpp_bridge("sjf_bridge", request.dict())


@app.post("/simulate/ljf")
def simulate_ljf(request: SimpleSimulationRequest):
    """Longest Job First (Non-Preemptive) — the mirror image of SJF."""
    return run_cpp_bridge("ljf_bridge", request.dict())


@app.post("/simulate/srtf")
def simulate_srtf(request: SimpleSimulationRequest):
    """Shortest Remaining Time First — the preemptive version of SJF."""
    return run_cpp_bridge("srtf_bridge", request.dict())


@app.post("/simulate/lrtf")
def simulate_lrtf(request: SimpleSimulationRequest):
    """Longest Remaining Time First — the preemptive version of LJF."""
    return run_cpp_bridge("lrtf_bridge", request.dict())


@app.post("/simulate/round-robin")
def simulate_round_robin(request: RoundRobinRequest):
    """Round Robin — every process gets a fixed time slice, then goes to
    the back of the line if it isn't finished yet."""
    return run_cpp_bridge("rr_bridge", request.dict())


@app.post("/simulate/priority-non-preemptive")
def simulate_priority_non_preemptive(request: SimpleSimulationRequest):
    """Priority Scheduling (Non-Preemptive) — lower priority number = runs
    first, and once started a process always runs to completion."""
    return run_cpp_bridge("priority_np_bridge", request.dict())


@app.post("/simulate/priority-preemptive")
def simulate_priority_preemptive(request: SimpleSimulationRequest):
    """Priority Scheduling (Preemptive) — same priority rule as above, but
    a newly-arrived higher-priority process can interrupt the running one."""
    return run_cpp_bridge("priority_p_bridge", request.dict())


@app.post("/simulate/mlq")
def simulate_multilevel_queue(request: MultilevelQueueRequest):
    """Multilevel Queue — processes are permanently assigned to one of
    several priority queues, each with its own internal algorithm."""
    return run_cpp_bridge("mlq_bridge", request.dict())


@app.post("/simulate/mlfq")
def simulate_multilevel_feedback_queue(request: MultilevelFeedbackQueueRequest):
    """Multilevel Feedback Queue — like MLQ, but processes can be
    promoted/demoted between queues based on their CPU behaviour."""
    return run_cpp_bridge("mlfq_bridge", request.dict())


# ------------------------------------------------------------------------------
# STEP 7: A tiny health-check / welcome route.
# ------------------------------------------------------------------------------
# Not strictly required, but it's a friendly way to confirm the server is
# actually running if you visit http://localhost:8000 directly in a browser,
# and it costs almost nothing to include.
# ------------------------------------------------------------------------------
@app.get("/")
def read_root():
    return {
        "message": "KernelFlow API is running.",
        "docs": "Visit /docs for the interactive API explorer.",
    }


# ------------------------------------------------------------------------------
# STEP 8: Let this file be run directly with `python main.py`.
# ------------------------------------------------------------------------------
# This is what actually makes PORT (defined above) take effect. `uvicorn.run`
# here does the exact same thing the `uvicorn main:app --reload` command
# does — it just reads the port from OUR constant instead of uvicorn's own
# default, so there is exactly one place in this whole project where the
# port is decided.
#
# `if __name__ == "__main__":` means this block only runs when you execute
# `python main.py` directly — NOT when uvicorn imports this file to serve
# `main:app` some other way (e.g. via the `uvicorn main:app --reload`
# command), which is what avoids starting the server twice.
# ------------------------------------------------------------------------------
if __name__ == "__main__":
    import uvicorn

    print(f"Starting KernelFlow API on http://localhost:{PORT}")
    uvicorn.run("main:app", host="0.0.0.0", port=PORT, reload=True)
