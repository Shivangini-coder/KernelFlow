// ================================================================================
// json_writer.hpp
// ================================================================================
// WHY THIS FILE EXISTS:
// Every algorithm's SimulationResult struct shares the SAME summary fields
// (ganttChart, contextSwitches, the three averages, cpuUtilization,
// throughput, totalTime) even though the per-process fields differ
// slightly (e.g. MLQ adds queueIndex, MLFQ adds finalQueueLevel). To avoid
// copy-pasting the same "write the Gantt chart as JSON" code into all 10
// bridge files, that shared part lives here as a template function. Each
// bridge still writes its OWN processResults array by hand (just a few
// lines), since that part genuinely differs per algorithm.
// ================================================================================

#pragma once
#include <sstream>
#include <string>

// Writes the ganttChart array and every summary/aggregate field that is
// identical across all algorithms. `result` can be any SimulationResult
// struct (from any of the 10 .cpp files) as long as it has these exact
// field names — which it does, since every file was written to the same
// shared contract.
template <typename SimulationResultT>
void writeCommonSummaryFields(std::ostringstream& out, const SimulationResultT& result) {

    // --- Gantt chart array ---
    out << "\"ganttChart\":[";
    for (size_t i = 0; i < result.ganttChart.size(); i++) {
        const auto& segment = result.ganttChart[i];
        if (i > 0) out << ",";
        out << "{\"processId\":" << segment.processId
            << ",\"startTime\":" << segment.startTime
            << ",\"endTime\":" << segment.endTime << "}";
    }
    out << "],";

    // --- Aggregate / summary stats ---
    out << "\"contextSwitches\":" << result.contextSwitches << ","
        << "\"averageWaitingTime\":" << result.averageWaitingTime << ","
        << "\"averageTurnaroundTime\":" << result.averageTurnaroundTime << ","
        << "\"averageResponseTime\":" << result.averageResponseTime << ","
        << "\"cpuUtilization\":" << result.cpuUtilization << ","
        << "\"throughput\":" << result.throughput << ","
        << "\"totalTime\":" << result.totalTime;
}
