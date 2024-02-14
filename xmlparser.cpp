//
// Created by jorda on 13/02/2024.
//

#include <iostream>
#include <sstream>
#include "libraries/rapidxml-master/rapidxml.hpp"
#include "libraries/rapidxml-master/rapidxml_utils.hpp"
#include "xmlparser.h"
#include <vector>
#include <cstring>

std::vector<float> parseMeshXMLVertexData(const char* xmlFilePath) {
    rapidxml::file<> xmlFile(xmlFilePath);
    if (!xmlFile.data()) {
        std::cerr << "Failed to open XML file." << std::endl;
    }

    std::cout << "Parsing XML Mesh Vertex Data..." << "\n";
    rapidxml::xml_document<> doc;
    try {
        doc.parse<0>(xmlFile.data());;  // Parse the XML data
    } catch (rapidxml::parse_error &e) {
        std::cerr << "Parse error: " << e.what() << std::endl;
    }

    rapidxml::xml_node<>* vertexAttributeNode = doc.first_node()->first_node("attribute");
    rapidxml::xml_node<>* colourAttributeNode = vertexAttributeNode->next_sibling("attribute");
    std::vector<float> vertexData;

    if (colourAttributeNode) {
        // Split the content by lines
        std::istringstream iss(vertexAttributeNode->value());
        std::istringstream iss2(colourAttributeNode->value());
        std::string vertexLine;
        std::string colourLine;
        while (std::getline(iss, vertexLine) && std::getline(iss2, colourLine)) {
            std::istringstream vertexLineStream(vertexLine);
            std::istringstream colourLineStream(colourLine);

            // Parse floats in each line
            float floatValue1, floatValue2;
            while (vertexLineStream >> floatValue1) {
                vertexData.push_back(floatValue1);
            }
            while (colourLineStream >> floatValue2) {
                vertexData.push_back(floatValue2);
            }
        }
    } else {
        std::cerr << "Attribute node not found." << std::endl;
    }

    return vertexData;
}

std::tuple<std::vector<std::vector<unsigned int>>,
std::vector<std::vector<unsigned int>>,
std::vector<std::vector<unsigned int>>> parseMeshXMLIndexData(const char* xmlFilePath) {
    rapidxml::file<> xmlFile(xmlFilePath);
    if (!xmlFile.data()) {
        std::cerr << "Failed to open XML file." << std::endl;
    }

    std::cout << "Parsing XML Mesh Index Data..." << "\n";
    rapidxml::xml_document<> doc;
    try {
        doc.parse<0>(xmlFile.data());;  // Parse the XML data
    } catch (rapidxml::parse_error &e) {
        std::cerr << "Parse error: " << e.what() << std::endl;
    }

    rapidxml::xml_node<>* vertexAttributeNode = doc.first_node()->first_node("attribute");
    rapidxml::xml_node<>* colourAttributeNode = vertexAttributeNode->next_sibling("attribute");
    std::vector<std::vector<unsigned int>> triStripIndices;
    std::vector<std::vector<unsigned int>> triIndices;
    std::vector<std::vector<unsigned int>> triFanIndices;

    rapidxml::xml_node<>* indicesSibling = colourAttributeNode->next_sibling("indices");
    while (indicesSibling != nullptr) {
        std::istringstream iss(indicesSibling->value());
        std::string indicesLine;
        unsigned int indexValue;
        if (std::strcmp(indicesSibling->first_attribute()->value(), "tri-strip") == 0) {
            std::vector<unsigned int> triStrip;
            while (iss >> indexValue) {
                triStrip.push_back(indexValue);
            }
            triStripIndices.push_back(triStrip);
        }
        if (std::strcmp(indicesSibling->first_attribute()->value(), "triangles") == 0) {
            std::vector<unsigned int> triangles;
            while (iss >> indexValue) {
                triangles.push_back(indexValue);
            }
            triIndices.push_back(triangles);
        }
        if (std::strcmp(indicesSibling->first_attribute()->value(), "tri-fan") == 0) {
            std::vector<unsigned int> triFan;
            while (iss >> indexValue) {
                triFan.push_back(indexValue);
            }
            triFanIndices.push_back(triFan);
        }
        indicesSibling = indicesSibling->next_sibling("indices");
    }

    return std::make_tuple(triStripIndices, triIndices, triFanIndices);
}

/*
int main() {
    std::vector<float> vertexData = parseMeshXMLVertexData(R"(C:\Users\jorda\OneDrive - Queen Mary, University of London\Cpp_Projects\OpenGL_Learnings\SmallGimbal.xml)");
    auto indexData = parseMeshXMLIndexData(R"(C:\Users\jorda\OneDrive - Queen Mary, University of London\Cpp_Projects\OpenGL_Learnings\SmallGimbal.xml)");

    std::cout << "VertexData: " << "\n";
    for (int i{0}; i < vertexData.size(); i++) {
        if (i % 7 == 0) std::cout << "\n";
        std::cout << vertexData[i] << " ";
    }

    std::cout << "\n\n";

    std::vector<std::vector<unsigned int>> triStripIndices;
    std::vector<std::vector<unsigned int>> triIndices;
    std::vector<std::vector<unsigned int>> triFanIndices;
    std::tie(triStripIndices, triIndices, triFanIndices) = indexData;
    std::cout << "No. of triStripIndices vectors: " << triStripIndices.size() << "\n";
    std::cout << "No. of triIndices vectors: " << triIndices.size() << "\n";
    std::cout << "No. of triFanIndices vectors: " << triFanIndices.size() << "\n\n";

    if (!triStripIndices.empty()) {
        for (std::vector<unsigned int> &vec: triStripIndices) {
            std::cout << "Tri-strip \n";
            for (unsigned int val: vec) {
                std::cout << val << " ";
            }
            std::cout << "\n";
        }
    }

    if (!triIndices.empty()) {
        for (std::vector<unsigned int> &vec: triIndices) {
            std::cout << "Tri \n";
            for (unsigned int val: vec) {
                std::cout << val << " ";
            }
            std::cout << "\n";
        }
    }

    if (!triFanIndices.empty()) {
        for (std::vector<unsigned int> &vec: triFanIndices) {
            std::cout << "Tri-fan \n";
            for (unsigned int val: vec) {
                std::cout << val << " ";
            }
            std::cout << "\n";
        }
    }
}
*/