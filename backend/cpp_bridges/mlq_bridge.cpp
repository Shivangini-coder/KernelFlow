// ================================================================================
// mlq_bridge.cpp — JSON bridge for Multilevel Queue (MLQ) scheduling.
//
// Two differences from the simpler bridges:
//   1. Each process JSON object includes a "queueIndex" field (which queue
//      it's permanently placed in), which we copy onto ProcessInput.
//   2. The request also includes a "queues" array describing every queue's
//      own algorithm ("FCFS" or "ROUND_ROBIN") and time quantum. We convert
//      that into the vector<QueueConfig> that runMultilevelQueueScheduling
//      expects. See fcfs_bridge.cpp for the full explanation of the
//      #define main trick and the FastAPI <-> bridge flow.
// ================================================================================

#define main mlq_original_main
#include "../cpp/MLQ.cpp"
#undef main

#include "mini_json.hpp"
#include "json_writer.hpp"
#include <iostream>
#include <sstream>

int main() {
    std::string requestBody = readAllStdin();
    JsonValue root = JsonValue::parse(requestBody);

    // Build the process list, including each process's assigned queue.
    std::vector<ProcessInput> processInputs;
    for (const auto& processJson : root["processes"].asArray()) {
        ProcessInput input;
        input.id = processJson["id"].asInt();
        input.arrivalTime = processJson["arrivalTime"].asInt();
        input.burstTime = processJson["burstTime"].asInt();
        input.queueIndex = processJson["queueIndex"].asInt();
        processInputs.push_back(input);
    }

    // Build the queue configuration list: one entry per queue level, in
    // order (queue 0 first = highest priority).
    std::vector<QueueConfig> queueConfigs;
    for (const auto& queueJson : root["queues"].asArray()) {
        QueueConfig config;
        std::string algorithmName = queueJson["algorithm"].asString();
        config.algorithm = (algorithmName == "ROUND_ROBIN")
            ? SchedulingAlgorithm::ROUND_ROBIN
            : SchedulingAlgorithm::FCFS;
        config.timeQuantum = queueJson["timeQuantum"].asInt();
        queueConfigs.push_back(config);
    }

    SimulationResult result = runMultilevelQueueScheduling(processInputs, queueConfigs);

    std::ostringstream out;
    out << "{\"processResults\":[";
    for (size_t i = 0; i < result.processResults.size(); i++) {
        const auto& pr = result.processResults[i];
        if (i > 0) out << ",";
        out << "{\"id\":" << pr.id
            << ",\"arrivalTime\":" << pr.arrivalTime
            << ",\"burstTime\":" << pr.burstTime
            << ",\"queueIndex\":" << pr.queueIndex
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
