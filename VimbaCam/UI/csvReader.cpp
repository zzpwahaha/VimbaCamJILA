#include "csvReader.h"

csvReader::csvReader(std::string pathname) : 
    data(pathname)
    , success(false)
{
    if (data.good())
    {
        success = true;
        parse();
        ncol = header.size();
        std::vector<std::vector<double>> parsedCSVDouble(strVec2DblVec(parsedCSV));
        dataDouble = transpose(parsedCSVDouble);
    }
    
};

csvReader::csvReader(std::wstring pathname) : 
     data(pathname)
    , success(false)
{
    if (data.good())
    {
        success = true;
        parse();
        ncol = header.size();
        std::vector<std::vector<double>> parsedCSVDouble(strVec2DblVec(parsedCSV));
        dataDouble = transpose(parsedCSVDouble);
    }

};

csvReader::~csvReader() { data.close(); }

void csvReader::parse() {
    std::vector<std::vector<std::string>> tmp;
    std::string line;
    while (std::getline(data, line))
    {

        std::string cell;
        std::vector<std::string> parsedRow;
        std::stringstream lineStream(line);
        while (std::getline(lineStream, cell, ','))
        {
            parsedRow.push_back(cell);
        }
        tmp.push_back(parsedRow);
    }
    std::copy(tmp.begin() + 1, tmp.end(), std::back_inserter(parsedCSV));
    std::copy(tmp[0].begin(), tmp[0].end(), std::back_inserter(header));
}

template <class T>
std::vector<T> csvReader::getColumn(const std::vector<std::vector<T>>& vector2D, size_t col) {
    //https://codereview.stackexchange.com/questions/114658/return-row-or-column-from-a-2d-array
    std::vector<T> column;
    column.reserve(vector2D.size());
    for (size_t i = 0; i < vector2D.size(); ++i)
    {
        column.push_back(vector2D[i][col]);
    }
    return column;
}

std::vector<double> 
csvReader::strVec2DblVec(const std::vector<std::string>& stringVector) {
    std::vector<double> doubleVector(stringVector.size());
    std::transform(stringVector.begin(), stringVector.end(), doubleVector.begin(), [](const std::string& val)
        {
            return stod(val);
        });
    return doubleVector;
}

std::vector<std::vector<double>> 
csvReader::strVec2DblVec(const std::vector<std::vector<std::string>>& stringVector) {
    std::vector<std::vector<double>> doubleVector;
    for (size_t i = 0; i < stringVector.size(); ++i)
    {
        std::vector<double> tmp(stringVector[i].size());
        std::transform(stringVector[i].begin(), stringVector[i].end(), tmp.begin(), [](const std::string& val)
            {
                return stod(val);
            });
        doubleVector.push_back(tmp);
    }
    
    return doubleVector;
}

template <class P>
std::vector<std::vector<P>> csvReader::transpose(std::vector<std::vector<P>>& vec2D) {
    std::vector<std::vector<P>> 
        vec(vec2D[0].size(), std::vector<P>(vec2D.size()));
    
    for (size_t i = 0; i < vec2D[0].size(); ++i)
    {
        for (size_t j = 0; j < vec2D.size(); ++j)
        {
            vec[i][j] = vec2D[j][i];
        }
    }
    return vec;
}