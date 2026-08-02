// ================================================================================
// priority_p_bridge.cpp — JSON bridge for Priority Scheduling (Preemptive).
// Same shape as priority_np_bridge.cpp, just calling the preemptive
// algorithm. See fcfs_bridge.cpp for the full explanation of every step.
// ================================================================================

#define main priority_p_original_main
#include "../cpp/Priority_preemptive_optimal.cpp"
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
        input.priority = processJson["priority"].asInt();
        processInputs.push_back(input);
    }

    SimulationResult result = runPriorityPreemptiveScheduling(processInputs);

    std::ostringstream out;
    out << "{\"processResults\":[";
    for (size_t i = 0; i < result.processResults.size(); i++) {
        const auto& pr = result.processResults[i];
        if (i > 0) out << ",";
        out << "{\"id\":" << pr.id
            << ",\"arrivalTime\":" << pr.arrivalTime
            << ",\"burstTime\":" << pr.burstTime
            << ",\"priority\":" << pr.priority
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
