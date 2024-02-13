//
// Created by jorda on 13/02/2024.
//

#include "xmlparser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "libraries/rapidxml-master/rapidxml.hpp"
#include "libraries/rapidxml-master/rapidxml_utils.hpp"
#include <vector>

std::vector<float> parseMeshXML(const char* xmlFilePath) {
    rapidxml::file<> xmlFile(xmlFilePath);
    if (!xmlFile.data()) {
        std::cerr << "Failed to open XML file." << std::endl;
    }

    rapidxml::xml_document<> doc;
    try {
        doc.parse<0>(xmlFile.data());;  // Parse the XML data
    } catch (rapidxml::parse_error &e) {
        std::cerr << "Parse error: " << e.what() << std::endl;
    }

    rapidxml::xml_node<>* vertexAttributeNode = doc.first_node()->first_node("attribute");
    rapidxml::xml_node<>* colourAttributeNode = vertexAttributeNode->next_sibling("attribute");
    if (colourAttributeNode) {
        // Iterate through the content of the <attribute> node
        std::vector<float> vertexData;

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

        /*
        // Output the parsed floats
        for (int i{0}; i < vertexData.size(); i++) {
            if (i % 7 == 0) std::cout << "\n";
            std::cout << vertexData[i] << " ";
        }
        */
        return vertexData;

    } else {
        std::cerr << "Attribute node not found." << std::endl;
    }


}

int main() {


    return 0;
}