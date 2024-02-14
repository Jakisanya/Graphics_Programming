// --- Declares all mesh parser functions --- \\

#ifndef CLIONPROJECTS_XMLPARSER_H
#define CLIONPROJECTS_XMLPARSER_H
#include <vector>

std::vector<float> parseMeshXMLVertexData(const char* xmlFilePath);
std::tuple<std::vector<std::vector<unsigned int>>,
        std::vector<std::vector<unsigned int>>,
        std::vector<std::vector<unsigned int>>> parseMeshXMLIndexData(const char* xmlFilePath);
#endif // CLIONPROJECTS_XMLPARSER_H
