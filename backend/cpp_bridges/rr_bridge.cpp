// ================================================================================
// rr_bridge.cpp — JSON bridge for Round Robin.
//
// The ONE difference from fcfs/sjf/ljf/srtf/lrtf bridges: Round Robin also
// needs a "timeQuantum" integer from the user, so we read root["timeQuantum"]
// out of the request JSON and pass it as the 2nd argument to
// runRoundRobinScheduling(...), exactly as RR_optimal.cpp's own comments
// describe. See fcfs_bridge.cpp for the full step-by-step explanation of
// the #define main trick and the FastAPI <-> bridge flow.
// ================================================================================

#define main rr_original_main
#include "../cpp/RR_optimal.cpp"
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

    // Round Robin's extra required field: the time quantum.
    int timeQuantum = root["timeQuantum"].asInt();

    SimulationResult result = runRoundRobinScheduling(processInputs, timeQuantum);

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
