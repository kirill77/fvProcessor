#include "FVProcessor.h"
#include <stdexcept>
#include <algorithm>
#include <filesystem>

FVProcessor::FVProcessor(const std::string& outputCsvPath)
    : m_outputCsvPath(outputCsvPath)
{
    // CSV writer will be created when we have outputs defined and start processing
}

void FVProcessor::addOutput(const std::string& inputColumnName,
    const std::string& outputName,
    std::shared_ptr<IProcessor> pProcessor)
{
    OutputType output;
    output.m_inputColumnName = inputColumnName;
    output.m_outputTypeName = outputName;
    output.m_pProcessor = pProcessor;
    
    m_outputs.push_back(output);
}

void FVProcessor::addInputCSVFile(const std::string& inputCsvPath)
{
    // Initialize output CSV writer if not already done
    if (!m_outputCSV && !m_outputs.empty()) {
        std::vector<std::string> outputHeaders;
        outputHeaders.push_back("InputFileName");  // Add filename column first
        for (const auto& output : m_outputs) {
            outputHeaders.push_back(output.m_outputTypeName);
        }
        m_outputCSV = std::make_unique<CSVFileWriter>(m_outputCsvPath, outputHeaders);
        
        if (!m_outputCSV->isValid()) {
            throw std::runtime_error("Failed to create output CSV file: " + m_outputCsvPath);
        }
    }
    
    // Open input CSV file
    CSVFileReader reader(inputCsvPath);
    if (!reader.isValid()) {
        throw std::runtime_error("Failed to open input CSV file: " + inputCsvPath);
    }
    
    // Get headers and find column indices for our outputs
    const auto& headers = reader.getHeaders();
    std::vector<int> columnIndices;
    
    for (const auto& output : m_outputs) {
        auto it = std::find(headers.begin(), headers.end(), output.m_inputColumnName);
        if (it != headers.end()) {
            columnIndices.push_back(static_cast<int>(it - headers.begin()));
        } else {
            columnIndices.push_back(-1); // Column not found
        }
    }
    
    // Reset all processors before processing
    for (const auto& output : m_outputs) {
        output.m_pProcessor->reset();
    }
    
    // Process each row
    std::vector<std::string> row;
    while (reader.readRow(row)) {
        // Process each output
        for (size_t i = 0; i < m_outputs.size(); ++i) {
            if (columnIndices[i] >= 0 && 
                static_cast<size_t>(columnIndices[i]) < row.size()) {
                
                // Convert value to double and notify processor
                try {
                    double value = std::stod(row[columnIndices[i]]);
                    m_outputs[i].m_pProcessor->notifyValue(value);
                } catch (const std::exception&) {
                    // Skip invalid values
                }
            }
        }
    }
    
    // Write results to output CSV
    if (m_outputCSV) {
        // Extract just the filename from the full path
        std::filesystem::path inputPath(inputCsvPath);
        std::string filename = inputPath.filename().string();
        
        // Create a mixed row with filename first, then numeric results
        std::vector<std::string> mixedRow;
        mixedRow.push_back(filename);
        
        // Add numeric results as strings
        for (const auto& output : m_outputs) {
            mixedRow.push_back(std::to_string(output.m_pProcessor->getResult()));
        }
        
        m_outputCSV->addRow(mixedRow);
        m_outputCSV->flush();
    }
}
