#include <vector>
#include <string>

#define STR_EQUAL 0

class CatoptricController {

    private:
        CatoptricSurface surface;

        CatoptricController();

        std::vector<std::string> checkForNewCSV();
        void run();
};
