#include <stdio.h>
#include <string>
#include <filesystem>
#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>
#include <chrono>
#include <iomanip>
#include <sstream>
#include "utils/fileUtils/fileUtils.h"
#include "utils/csvFile/CSVFileReader.h"
#include "FVProcessor.h"

// Example processor implementations
class AverageProcessor : public FVProcessor::IProcessor {
private:
    double m_sum = 0.0;
    int m_count = 0;
    
public:
    void reset() override {
        m_sum = 0.0;
        m_count = 0;
    }
    
    void notifyValue(double f) override {
        m_sum += f;
        m_count++;
    }
    
    double getResult() override {
        return (m_count > 0) ? (m_sum / m_count) : 0.0;
    }
};

class MaxProcessor : public FVProcessor::IProcessor {
private:
    double m_max = std::numeric_limits<double>::lowest();
    bool m_hasValue = false;
    
public:
    void reset() override {
        m_max = std::numeric_limits<double>::lowest();
        m_hasValue = false;
    }
    
    void notifyValue(double f) override {
        if (!m_hasValue || f > m_max) {
            m_max = f;
            m_hasValue = true;
        }
    }
    
    double getResult() override {
        return m_hasValue ? m_max : 0.0;
    }
};

class VariabilityProcessor : public FVProcessor::IProcessor {
private:
    double m_prevValue = -1;
    AverageProcessor m_avgProcessor;

public:
    void reset() override {
        m_prevValue = -1;
        m_avgProcessor.reset();
    }

    void notifyValue(double f) override {
        if (m_prevValue != -1)
        {
            double fSum = (m_prevValue + f);
            if (fSum > 0)
            {
                double fVariability = abs(m_prevValue - f) / fSum;
                m_avgProcessor.notifyValue(fVariability * 100);
            }
        }
        m_prevValue = f;
    }

    double getResult() override {
        return m_avgProcessor.getResult();
    }
};

int main(int argc, char* argv[])
{
    // Get the folder path from command line arguments or use default
    std::string folderPath;
    if (argc < 2) {
        folderPath = "exampleInput";
        printf("No folder path provided, using default: %s\n", folderPath.c_str());
    }
    else {
        folderPath = argv[1];
    }

    printf("Processing folder: %s\n", folderPath.c_str());
    
    // Find the specified folder
    std::filesystem::path foundPath;
    if (FileUtils::findTheFolder(folderPath, foundPath)) {
        printf("Found folder at: %s\n", foundPath.string().c_str());
    } else {
        printf("Error: Could not find folder '%s'\n", folderPath.c_str());
        return 1;
    }

    // Process all CSV files in the found folder
    std::vector<std::filesystem::path> csvFiles;
    
    // Find all CSV files in the directory
    try {
        for (const auto& entry : std::filesystem::directory_iterator(foundPath)) {
            if (entry.is_regular_file()) {
                std::string extension = entry.path().extension().string();
                // Convert extension to lowercase for case-insensitive comparison
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                if (extension == ".csv") {
                    csvFiles.push_back(entry.path());
                }
            }
        }
    }
    catch (const std::filesystem::filesystem_error& ex) {
        printf("Error accessing directory: %s\n", ex.what());
        return 1;
    }
    
    if (csvFiles.empty()) {
        printf("No CSV files found in folder: %s\n", foundPath.string().c_str());
        return 0;
    }
    
    printf("Found %zu CSV file(s):\n", csvFiles.size());
    
    // Find the allOutputs folder
    std::filesystem::path outputFolderPath;
    if (FileUtils::findTheFolder("allOutputs", outputFolderPath)) {
        printf("Found output folder at: %s\n", outputFolderPath.string().c_str());
    } else {
        printf("Error: Could not find 'allOutputs' folder\n");
        return 1;
    }
    
    // Generate filename with current timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &time_t);
    
    std::ostringstream filename;
    filename << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".csv";
    
    // Create the full output file path
    std::filesystem::path outputFilePath = outputFolderPath / filename.str();
    
    try {
        FVProcessor processor(outputFilePath.string());
        
        // Add processors for different metrics
        processor.addOutput("MsBetweenDisplayChange", "AvgFrameTimeMs", 
                          std::make_shared<AverageProcessor>());
        processor.addOutput("MsPCLatency", "AvgLatencyMs", 
                          std::make_shared<AverageProcessor>());
        processor.addOutput("MsBetweenDisplayChange", "FrameVariabilityPercent",
            std::make_shared<VariabilityProcessor>());
        
        // Process each CSV file found
        for (const auto& csvPath : csvFiles) {
            printf("Processing %s with FVProcessor...\n", csvPath.filename().string().c_str());
            processor.addInputCSVFile(csvPath.string());
        }
        
        printf("FVProcessor output written to: %s\n", outputFilePath.string().c_str());
    }
    catch (const std::exception& ex) {
        printf("FVProcessor error: %s\n", ex.what());
    }

    return 0;
}
