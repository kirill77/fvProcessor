#pragma once

#include <string>
#include <memory>
#include <vector>
#include "../utils/csvFile/CSVFileReader.h"
#include "../utils/csvFile/CSVFileWriter.h"

class FVProcessor
{
public:
    FVProcessor(const std::string& outputCsvPath);

    struct IProcessor
    {
        virtual void reset() = 0;
        virtual void notifyValue(double f) = 0;
        virtual double getResult() = 0;
    };

    // at first the caller adds different outputs they want
    void addOutput(const std::string& inputColumnName,
        const std::string& outputName,
        std::shared_ptr<IProcessor> pProcessor);

    // then the caller calls this function one or more times to process one or more input CSV files
    void addInputCSVFile(const std::string& inputCsvPath);

private:
    struct OutputType
    {
        std::string m_inputColumnName;
        std::string m_outputTypeName;
        std::shared_ptr<IProcessor> m_pProcessor;
    };
    std::vector<OutputType> m_outputs;
    std::string m_outputCsvPath;
    std::unique_ptr<CSVFileWriter> m_outputCSV;
};

