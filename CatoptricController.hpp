#include <vector>
#include <string>

class CatoptricController {

    private:
        CatoptricSurface surface;

        CatoptricController();

        std::vector<std::string> checkForNewCSV();
        void run();
};
