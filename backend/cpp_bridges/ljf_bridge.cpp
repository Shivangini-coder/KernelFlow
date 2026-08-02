// ================================================================================
// ljf_bridge.cpp — JSON bridge for Longest Job First (Non-Preemptive).
// Identical pattern to sjf_bridge.cpp — see fcfs_bridge.cpp for full notes.
// ================================================================================

#define main ljf_original_main
#include "../cpp/LJF_optimal.cpp"
#undef main

#include "mini_json.hpp"
#include "json_writer.hpp"
#include <iostream>
#include <sstream>

int main() {
    std::string requestBody = readAllStdin();
    JsonValue root = JsonValue::parse(requestBody);

    std::vector<ProcessInput> processInputs;
    for (const auto& processJson : root["processes"].asArray()) {
        ProcessInput input;
        input.id = processJson["id"].asInt();
        input.arrivalTime = processJson["arrivalTime"].asInt();
        input.burstTime = processJson["burstTime"].asInt();
        processInputs.push_back(input);
    }

    SimulationResult result = runLJFScheduling(processInputs);

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
