// ================================================================================
// fcfs_bridge.cpp
// ================================================================================
// WHY THIS FILE EXISTS:
// This is the ONLY file that talks to FastAPI for the FCFS algorithm. It is
// a brand-new file — your original FCFS.cpp is never edited, renamed, or
// touched in any way. Instead, this bridge #includes FCFS.cpp directly so
// it can call runFCFSScheduling(...) as a normal C++ function, exactly the
// way your file's own comments say FastAPI eventually would.
//
// THE #define main TRICK (read this before you get worried!):
// FCFS.cpp already has its OWN `int main(){...}` used for local terminal
// testing (typing arrival/burst times by hand). If we just #include the
// file as-is, we'd get a "duplicate main()" compile error, because this
// bridge file ALSO needs its own main() (one that reads JSON instead of
// keyboard input). The fix: right before including FCFS.cpp, we tell the
// preprocessor to silently rename every occurrence of the word "main" to
// "fcfs_original_main". This happens purely at compile time, in memory —
// FCFS.cpp on disk is completely unchanged. FCFS.cpp's own test-harness
// main() still exists in the compiled program, it's just never called; OUR
// main() below (the JSON one) is what actually runs.
//
// HOW REACT -> FASTAPI -> THIS BRIDGE -> BACK WORKS, END TO END:
//   1. User fills in the process table on the Simulation page and clicks
//      "Run Simulation" in the React app.
//   2. React sends a POST request (as JSON) to a FastAPI endpoint like
//      POST /simulate/fcfs.
//   3. FastAPI (see backend/main.py) receives that JSON, and instead of
//      doing any scheduling math itself, it launches this COMPILED bridge
//      program as a subprocess and pipes the JSON into its stdin.
//   4. This program (below) parses that JSON, builds a vector<ProcessInput>,
//      calls your unmodified runFCFSScheduling(...), and prints the result
//      back out as JSON on stdout.
//   5. FastAPI reads that JSON off the subprocess's stdout and forwards it
//      straight back to React as the HTTP response.
// ================================================================================

#define main fcfs_original_main
#include "../cpp/FCFS.cpp"
#undef main

#include "mini_json.hpp"
#include "json_writer.hpp"
#include <iostream>
#include <sstream>

// This is the REAL main() that runs when FastAPI executes this program.
// It never uses cin/cout for interactive prompts — it only reads one JSON
// blob from stdin and writes one JSON blob to stdout, which is exactly
// what makes it callable from an automated backend instead of a human.
int main() {

    // Step 1: read the entire JSON request body FastAPI piped into stdin.
    std::string requestBody = readAllStdin();
    JsonValue root = JsonValue::parse(requestBody);

    // Step 2: turn the JSON "processes" array into the vector<ProcessInput>
    // that runFCFSScheduling(...) expects — this mirrors how FastAPI's own
    // Pydantic models will deserialize the same JSON on the Python side.
    std::vector<ProcessInput> processInputs;
    for (const auto& processJson : root["processes"].asArray()) {
        ProcessInput input;
        input.id = processJson["id"].asInt();
        input.arrivalTime = processJson["arrivalTime"].asInt();
        input.burstTime = processJson["burstTime"].asInt();
        processInputs.push_back(input);
    }

    // Step 3: run YOUR unmodified scheduling algorithm. Nothing about how
    // FCFS is computed lives in this bridge file — it's 100% inside
    // FCFS.cpp, exactly as required.
    SimulationResult result = runFCFSScheduling(processInputs);

    // Step 4: serialize the result back to JSON on stdout for FastAPI to
    // read. We write processResults by hand here because its exact fields
    // differ slightly per algorithm; the shared summary fields (Gantt
    // chart, averages, utilization, etc.) come from json_writer.hpp so we
    // don't repeat that code in every bridge file.
    std::ostringstream out;
    out << "{";

    out << "\"processResults\":[";
    for (size_t i = 0; i < result.processResults.size(); i++) {
        const auto& processResult = result.processResults[i];
        if (i > 0) out << ",";
        out << "{"
            << "\"id\":" << processResult.id << ","
            << "\"arrivalTime\":" << processResult.arrivalTime << ","
            << "\"burstTime\":" << processResult.burstTime << ","
            << "\"completionTime\":" << processResult.completionTime << ","
            << "\"turnaroundTime\":" << processResult.turnaroundTime << ","
            << "\"waitingTime\":" << processResult.waitingTime << ","
            << "\"responseTime\":" << processResult.responseTime
            << "}";
    }
    out << "],";

    writeCommonSummaryFields(out, result);

    out << "}";

    std::cout << out.str();
    return 0;
}
