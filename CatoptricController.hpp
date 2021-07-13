/* CatoptricController.hpp
 * The CatoptricController class defines the event loop for the overall
 * catoptric lighting system and input processing.
 * Author: Scott Sirri scottsirri@wustl.edu
 */

#pragma once

#include <vector>
#include <string>
#include "CatoptricSurface.hpp"
#include "ErrCodes.hpp"

#define STR_EQUAL 0
#define DIR_LEN_INIT -10

class CatoptricController {

    private:
        CatoptricSurface surface;

        CatoptricController();

        std::vector<std::string> checkForNewCSV();
        std::string getUserInput(std::string inputMessage);
        int getNumFiles(std::string dir);
        int extractFirstIntFromFile(std::istream& filStream);
        int renameMoveFile(std::string src, std::string dest);

    public:
        void run();
};
