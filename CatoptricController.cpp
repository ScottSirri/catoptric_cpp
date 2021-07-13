
/* CatoptricController.cpp
 * The CatoptricController class defines the event loop for the overall
 * catoptric lighting system and input processing.
 * Author: Scott Sirri scottsirri@wustl.edu
 */

#include <fstream> // ifstream
#include <iostream> // cin
#include <sstream> // stringstream
#include "CatoptricController.hpp"

using namespace std;

CatoptricController::CatoptricController() {
    surface = CatoptricSurface();
}

/* Repeatedly check for new CSV files and prompt user for input:
 * either reset the mirrors or execute a new CSV file.
 */
void CatoptricController::run() {

    while(CONTROLLER_RUNNING) { // Infinite event loop

        string csv = "";
        string inputMessage = "\'Reset\' mirrors or upload a file to run: ";

        // Retrieve any new CSV files
        vector<string> csvList = checkForNewCSV();

        printf("\n-------------------------\n\n");

        if(csvList.size() > 0) {
            csv = csvList[0];
            printf(" -- Found csv file \'%s\'\n", csv.c_str());
            inputMessage = "\'Reset\' mirrors or \'Run\' file: ";
        }

        string userInput = getUserInput(inputMessage);

        if(userInput.compare("reset") == STR_EQUAL) {
            surface.reset();
            printf(" -- Reset Complete\n");
        } else if(csvList.size() > 0 && userInput.compare("run") == STR_EQUAL) {
            
            printf(" -- Running \'%s\'\n", csv.c_str());
            surface.updateByCSV(csv);
            printf(" -- \'%s\' ran successfully\n", csv.c_str());

            // Find the number of files in csv/archive
            string archiveDir = "./csv/archive";
            int archiveLength = getNumFiles(archiveDir);
            if(archiveLength < 0) {
                printf("Error in getNumFiles\n");
                return;
            }

            // Rename + move CSV file to csv/archive
            string newName = "./csv/archive/" + to_string(archiveLength) + 
                "_" + csv;
            if(renameMoveFile(csv, newName) < 0) {
                printf("Error in renameMoveFile\n");
                return;
            }

            printf(" -- \'%s\' moved to archive\n", csv.c_str());
        }
    }
}

/* Check csv/new directory for any newly deposited CSVs.
 * Uses 'system' function to execute command-line instructions for filesystem
 * inspection.
 */
vector<string> CatoptricController::checkForNewCSV() {

    // Create file listing contents of csv/new
    string directoryStr = "./csv/new";
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

/* Retrieves user input and returns it lowercase.
 * */
string CatoptricController::getUserInput(string inputMessage) {
    string userInput;
    printf("%s", inputMessage.c_str());
    cin >> userInput;
    printf("\n\n");

    // Transform user input to lowercase characters
    transform(userInput.begin(), userInput.end(), 
            userInput.begin(), ::tolower);
    return userInput;
}

/* Return number of files in passed directory path (or error code).
 */
int CatoptricController::getNumFiles(string dir) {

    int dirLength = DIR_LEN_INIT;
    char ls_wc_cmd[CMD_LEN];
    snprintf(ls_wc_cmd, CMD_LEN, "ls %s | wc > %s", dir.c_str(), 
            LS_WC_FILENAME);

    if(system(ls_wc_cmd) != SYSTEM_SUCCESS) {
        printf("Error in system function for command \'%s\': %s\n", 
                ls_wc_cmd, strerror(errno));
        return ERR_SYSTEM;
    }

    ifstream fs;
    fs.open(LS_WC_FILENAME, ios_base::in);
    if(fs.good() && !fs.eof()) {
        /* First int in output from 'wc' is number lines, i.e. number
           files listed by ls */
        dirLength = extractFirstIntFromFile(fs);
    } else if(fs.fail()) {
        printf("LS_WC file fail to open: %s\n", strerror(errno));
        return ERR_FAIL_OPEN;
    }

    if(dirLength == ERR_NO_INT || dirLength == ERR_STOI) {
        printf("Error finding number files in dir\n");
    }

    return dirLength;
}

/* Extracts the first integer from a file containing only integers (and 
 * whitespace).
 * Exclusively employed on the output of 'wc' command to get the number
 * of lines in another file.
 */
int CatoptricController::extractFirstIntFromFile(istream& filStream) {

    string line;
    getline(filStream, line);

    stringstream lineStream(line);
    string wcElem;

    while(getline(lineStream, wcElem, ' ')) {
        if(wcElem.empty() != true) {
            int stoiRet;
            try {
                stoiRet = stoi(wcElem);
            } catch(...) {
                printf("extractFirstIntFromFile parsing error in stoi: %s\n", 
                        strerror(errno));
                stoiRet = ERR_STOI;
            }

            return stoiRet;
        }
    }

    printf("extractFirstIntFromFile error, no int found\n");
    return ERR_NO_INT;
}

/* Renames and moves the passed src file to dest.
 */
int CatoptricController::renameMoveFile(string src, string dest) {

    string mov_cmd = "mov " + src + " " + dest;
    if(system(mov_cmd.c_str()) != SYSTEM_SUCCESS) {
        printf("Error in system function for command \'%s\': %s\n", 
                mov_cmd.c_str(), strerror(errno));
        return ERR_SYSTEM;
    }

    return RET_SUCCESS;
}

