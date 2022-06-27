#pragma once
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

class csvReader {
    std::ifstream data;
    std::vector<std::vector<std::string>> parsedCSV;
    size_t ncol;
    std::vector<std::vector<double>> dataDouble;
    std::vector<std::string> header;
    bool success;
    

    void parse();
public:
    
    csvReader(std::string pathname);
    csvReader(std::wstring pathname);
    ~csvReader();
    size_t getNCol() { return ncol; }
    std::vector<std::vector<double>> getDataDouble() { return dataDouble; }
    std::vector<std::string> getHeader() { return header; }
    bool isSuccess() { return success; }
private:
    template <class T>
    std::vector<T> getColumn(const std::vector<std::vector<T>>& vector2D, size_t col);
    std::vector<double> strVec2DblVec(const std::vector<std::string>& stringVector);
    std::vector<std::vector<double>> strVec2DblVec(const std::vector<std::vector<std::string>>& stringVector);
    template <class P>
    std::vector<std::vector<P>> transpose(std::vector<std::vector<P>>&);
};