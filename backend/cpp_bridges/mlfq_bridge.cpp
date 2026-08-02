// ================================================================================
// mlfq_bridge.cpp — JSON bridge for Multilevel Feedback Queue (MLFQ) scheduling.
//
// Differences from mlq_bridge.cpp:
//   1. Processes do NOT carry a queueIndex — MLFQ always starts every
//      process at queue 0, so there's nothing to read from the request
//      for that.
//   2. The request includes one extra top-level integer, "agingThreshold",
//      which we pass as the 3rd argument to runMLFQScheduling(...).
//   3. Each result also reports finalQueueLevel and queueMovements, which
//      MLFQ.cpp adds specifically so the frontend can show "this process
//      sank to queue 2" style insights.
// See fcfs_bridge.cpp for the full explanation of the #define main trick
// and the FastAPI <-> bridge flow.
// ================================================================================

#define main mlfq_original_main
#include "../cpp/MLFQ.cpp"
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

    int agingThreshold = root["agingThreshold"].asInt();

    SimulationResult result = runMLFQScheduling(processInputs, queueConfigs, agingThreshold);

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
            << ",\"finalQueueLevel\":" << pr.finalQueueLevel
            << ",\"queueMovements\":" << pr.queueMovements
            << "}";
    }
    out << "],";
    writeCommonSummaryFields(out, result);
    out << "}";

    std::cout << out.str();
    return 0;
}
