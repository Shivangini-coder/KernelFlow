#!/usr/bin/env bash
# ================================================================================
# build.sh
# ================================================================================
# WHY THIS FILE EXISTS:
# FastAPI (main.py) calls COMPILED programs, not raw .cpp source files. This
# script compiles every bridge in backend/cpp_bridges/ into an executable
# inside backend/bin/, which is where main.py expects to find them.
#
# You only need to re-run this script when you CHANGE a .cpp file (either
# one of your original algorithm files, or a bridge file). You do NOT need
# to re-run it every time you restart the FastAPI server.
#
# HOW TO RUN IT:
#   cd backend
#   bash build.sh
# ================================================================================

set -e   # stop immediately if any single compile step fails

# Move into this script's own directory, so it works no matter where you
# ran `bash build.sh` FROM.
cd "$(dirname "$0")"

mkdir -p bin

echo "Compiling C++ scheduling bridges into backend/bin/ ..."

# -O2 turns on compiler optimizations (these programs still run in
# milliseconds either way for classroom-sized input, but there's no reason
# not to). -std=c++17 matches the modern C++ features used across the
# original files (enum class, lambdas, etc).
g++ -O2 -std=c++17 -Wno-unused-function -o bin/fcfs_bridge          cpp_bridges/fcfs_bridge.cpp
g++ -O2 -std=c++17 -Wno-unused-function -o bin/sjf_bridge           cpp_bridges/sjf_bridge.cpp
g++ -O2 -std=c++17 -Wno-unused-function -o bin/ljf_bridge           cpp_bridges/ljf_bridge.cpp
g++ -O2 -std=c++17 -Wno-unused-function -o bin/srtf_bridge          cpp_bridges/srtf_bridge.cpp
g++ -O2 -std=c++17 -Wno-unused-function -o bin/lrtf_bridge          cpp_bridges/lrtf_bridge.cpp
g++ -O2 -std=c++17 -Wno-unused-function -o bin/rr_bridge            cpp_bridges/rr_bridge.cpp
g++ -O2 -std=c++17 -Wno-unused-function -o bin/priority_np_bridge   cpp_bridges/priority_np_bridge.cpp
g++ -O2 -std=c++17 -Wno-unused-function -o bin/priority_p_bridge    cpp_bridges/priority_p_bridge.cpp
g++ -O2 -std=c++17 -Wno-unused-function -o bin/mlq_bridge           cpp_bridges/mlq_bridge.cpp
g++ -O2 -std=c++17 -Wno-unused-function -o bin/mlfq_bridge          cpp_bridges/mlfq_bridge.cpp

echo "Done. 10 executables are now in backend/bin/"
