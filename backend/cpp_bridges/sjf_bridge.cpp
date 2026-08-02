// ================================================================================
// sjf_bridge.cpp — JSON bridge for Shortest Job First (Non-Preemptive).
// Same pattern as fcfs_bridge.cpp: #include the original file with `main`
// renamed away, then provide our own JSON-in/JSON-out main(). See
// fcfs_bridge.cpp for the full explanation of every step below.
// ================================================================================

#define main sjf_original_main
#include "../cpp/SJF_optimal.cpp"
#undef main

#include "mini_json.hpp"
#include "json_writer.hpp"
#include <iostream>
#include <sstream>

int main() {
    std::string requestBody = readAllStdin();
    JsonValue root = JsonValue::parse(requestBody);

    // Build the process list FastAPI sent us, in the shape SJF expects.
    std::vector<ProcessInput> processInputs;
    for (const auto& processJson : root["processes"].asArray()) {
        ProcessInput input;
        input.id = processJson["id"].asInt();
        input.arrivalTime = processJson["arrivalTime"].asInt();
        input.burstTime = processJson["burstTime"].asInt();
        processInputs.push_back(input);
    }

    // Run YOUR unmodified SJF algorithm.
    SimulationResult result = runSJFScheduling(processInputs);

    // Serialize the result back to JSON for FastAPI to forward to React.
    std::ostringstream out;
    out << "{\"processResults\":[";
    for (size_t i = 0; i < result.processResults.size(); i++) {
        const auto& pr = result.processResults[i];
        if (i > 0) out << ",";
        out << "{\"id\":" << pr.id
            << ",\"arrivalTime\":" << pr.arrivalTime
            << ",\"burstTime\":" << pr.burstTime
            << ",\"completionTime\":" << pr.completionTime
            << ",\"turnaroundTime\":" << pr.turnaroundTime
            << ",\"waitingTime\":" << pr.waitingTime
            << ",\"responseTime\":" << pr.responseTime
            << "}";
    }
    out << "],";
    writeCommonSummaryFields(out, result);
    out << "}";

    std::cout << out.str();
    return 0;
}
