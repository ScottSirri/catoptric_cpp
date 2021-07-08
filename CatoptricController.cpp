
#include "CatoptricController.hpp"

CatoptricController::CatoptricController() {
    surface = CatoptricSurface();
}

/* Check csv/new directory for any newly deposited CSVs.
 * Uses 'system' function to execute command-line instructions for filesystem
 * inspection.
 */
vector<string> CatoptricController::checkForNewCSV() {

    // Create file listing contents of csv/new
    string directoryStr = "csv/new";
    string ls_cmd = "ls " + directoryStr + " > " + LS_CSV_FILENAME;
    int ret;
    if((ret = system(ls_cmd.c_str())) != SYSTEM_SUCCESS) {
        printf("System function error: return  %d, %s\n", ret, strerror(errno));
    }

    ifstream ls_file_stream;
    string ls_line;
    string csv_ending = ".csv";
    vector<string> newCSVs;

    ls_file_stream.open(LS_CSV_FILENAME, ios_base::in);
    
    while(ls_file_stream.good() && !ls_file_stream.eof() && 
            getline(ls_file_stream, ls_line)) {
        // If line from ls ends in ".csv"
        if(ls_line.compare(ls_line.length() - csv_ending.length(), 
                    csv_ending.length(), csv_ending) == CMP_EQUAL) {
            newCSVs.push_back(ls_line);
        }
    }

    return newCSVs;
}

int extractFirstIntFromFile(istream& filStream) {
    string line;
    getline(filStream, line);

    stringstream lineStream(line);
    string wc_elem;

    while(getline(lineStream, wc_elem, ' ')) {
        if(wc_elem.empty() != true) {
            int stoi_ret;
            try {
                stoi_ret = stoi(wc_elem);
            } catch(...) {
                printf("extractFirstIntFromFile parsing error in stoi: %s\n", 
                        strerror(errno));
                stoi_ret = ERR_STOI;
            }

            return stoi_ret;
        }
    }

    printf("extractFirstIntFromFile error, no int found\n");
    return ERR_NO_INT;
}

void CatoptricController::run() {
    while(CONTROLLER_RUNNING) {
        printf("\n-------------------------\n\n");
        string csv = "";
        vector<string> csvList = checkForNewCSV();
        string inputMessage = "\'Reset\' mirrors or uploa a file to run: ";

        if(csvList.size() > 0) {
            csv = csvList[0];
            printf(" -- Found csv file \'%s\'\n", csv.c_str());
            inputMessage = "\'Reset\' mirrors or \'Run\' file: ";
        }

        string userInput;
        printf("%s", inputMessage.c_str());
        cin >> userInput;
        printf("\n\n");

        // Transform user input to lowercase characters
        transform(userInput.begin(), userInput.end(), 
                userInput.begin(), ::tolower);

        if(0 == userInput.compare("reset")) {
            surface.reset();
            printf(" -- Reset Complete\n");
        } else if(csvList.size() > 0 && userInput.compare("run") == STR_EQUAL) {
            printf(" -- Running \'%s\'\n", csv.c_str());
            surface.updateByCSV(csv); // TODO : Need to alter csv before?
            printf(" -- \'%s\' ran successfully\n", csv.c_str());

            char ls_wc_cmd[CMD_LEN];
            snprintf(ls_wc_cmd, CMD_LEN, "ls ./csv/archive | wc > %s", 
                    LS_WC_FILENAME);
            if(system(ls_wc_cmd) != SYSTEM_SUCCESS) {
                printf("Error in system function for command \'%s\': %s\n", 
                        ls_wc_cmd, strerror(errno));
                return;
            }

            int archiveLength = -1;
            ifstream fs;
            fs.open(LS_WC_FILENAME, ios_base::in);
            if(fs.fail()) {
                printf("LS_WC file fail to open: %s\n", strerror(errno));
                return;
            } else if(fs.good() && !fs.eof()) {
                /* First int in output from 'wc' is number lines, i.e. number
                   files listed by ls */
                archiveLength = extractFirstIntFromFile(fs);
            }

            if(archiveLength == ERR_NO_INT || archiveLength == ERR_STOI) return;

            // Rename + move CSV file to archive
            string newName = "./csv/archive/" + to_string(archiveLength) + 
                "_" + csv;
            string mov_cmd = "mov " + csv + " " + newName;
            if(system(mov_cmd.c_str()) != SYSTEM_SUCCESS) {
                printf("Error in system function for command \'%s\': %s\n", 
                        mov_cmd.c_str(), strerror(errno));
                return;
                
            }
            printf(" -- \'%s\' moved to archive\n", csv.c_str());
            
        }
    }
}
