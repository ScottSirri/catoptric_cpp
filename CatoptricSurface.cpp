#include <iostream>
#include <algorithm>    // transform
#include <stdlib.h>
#include <stdio.h>      // snprintf
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <string>
#include <errno.h> 
#include "CatoptricRow.hpp"
#include "CatoptricSurface.hpp"

using namespace std;

int main() {
    return 0;
}

SerialPort::SerialPort() {
    serialNumber = string();
    row = UNDEF_ORDER;
}

SerialPort::SerialPort(string numIn, int rowIn, string deviceIn) {
    serialNumber = numIn;
    row = rowIn;
    device = deviceIn;
}

SerialPort::SerialPort(string numIn, int rowIn) {
    serialNumber = numIn;
    row = rowIn;
    device = string();
}

/* Returns the name of the port with the passed row number,
 * if any. Returns empty string otherwise.
 */
string SerialPortDict::getSerialNumber(int row) {
    for(int i = 0; i < dict.size(); ++i) {
        if(dict[i].row == row) return dict[i].serialNumber;
    }

    return string();
}

/* Returns the row number for the serial port associated
 * with the passed name. Returns ERR_QUERY_FAILED otherwise.
 */
int SerialPortDict::getRow(string serialNumber) {
    for(int i = 0; i < dict.size(); ++i) {
        if(dict[i].serialNumber.compare(serialNumber) == STR_EQ) {
            return dict[i].row;
        }
    }

    return ERR_QUERY_FAILED;
}

/* Adds a new serial port object the the CatoptricSurface object's
 * serial port dictionary.
 */
void SerialPortDict::addPort(SerialPort port) {
    dict.push_back(port);
}

/* Reads file containing surface dimensions and initializes the corresponding
 * vector with row lengths.
 * File containing surface dimensions (row lengths) is formatted as intervals
 * of row numbers, each interval associated with a row length (conducive to
 * rectangular-ish arrays).
 *      Two integers (separated by a space) per line.
 *      First integer is the last (one-indexed) row number in that interval.
 *          The first row number in that interval is implicitly one greater than
 *          the end of the previous interval (first interval starts at 1).
 *          Both bounds are INCLUSIVE.
 *      Second integer is the length of that row.
 */
int SurfaceDimensions::initDimensions(string filePath) {

    defaultRowLen = DEFAULT_ROW_LEN;
    ifstream dimensionsFile(filePath);

    if(dimensionsFile) {

        string dimensionsLine;
        int rowNumStart = 1, rowNumEnd, rowLen;

        while(getline(dimensionsFile, dimensionsLine)) {
            
            istringstream iss(dimensionsLine);

            try {
                iss >> rowNumEnd;
                iss >> rowLen;
            } catch(...) {
                printf("istringstream error- invalid dimensions file contents: "
                        "%s\n", strerror(errno));
                return ERR_DIMS_FILE;
            }

            // Requires space linear in # rows, but easy indexing operation
            for(int i = 0; i < rowNumEnd - rowNumStart + 1; ++i) {
                rowLengths.push_back(rowLen);
            }

            rowNumStart = rowNumEnd + 1;
        }
    }

    dimensionsFile.close();

    return RET_SUCCESS;
}

/* Returns the length of the row with the passed row number.
 * Row numbers are ONE-INDEXED! (not my fault)
 */
int SurfaceDimensions::getLength(int rowNumber) {
    if(1 <= rowNumber && rowNumber <= rowLengths.size()) {
        return rowLengths[rowNumber - 1];
    } else {
        return defaultRowLen;
    }
}

CatoptricSurface::CatoptricSurface() {

    SERIAL_INFO_PREFIX = SERIAL_INFO_PREFIX_MACRO;

    serialPortOrder.addPort(SerialPort("8543931323035121E170", 1));
    serialPortOrder.addPort(SerialPort("8543931323035130C092", 2));
    serialPortOrder.addPort(SerialPort("85439313230351610262", 3));
    serialPortOrder.addPort(SerialPort("75435353934351D052C0", 4));
    serialPortOrder.addPort(SerialPort("85436323631351509171", 5));
    serialPortOrder.addPort(SerialPort("75435353934351F0C020", 6));
    serialPortOrder.addPort(SerialPort("8543931333035160E081", 7));
    serialPortOrder.addPort(SerialPort("85439313230351818090", 8));
    serialPortOrder.addPort(SerialPort("755333434363519171F0", 9));
    serialPortOrder.addPort(SerialPort("8543931333035160F102", 10));
    serialPortOrder.addPort(SerialPort("8543931323035161B021", 11));
    serialPortOrder.addPort(SerialPort("85439313330351D03160", 12));
    serialPortOrder.addPort(SerialPort("85439303133351716221", 13));
    serialPortOrder.addPort(SerialPort("85436323631351300201", 14));
    serialPortOrder.addPort(SerialPort("75435353934351E07072", 15));
    serialPortOrder.addPort(SerialPort("8543931323035170D0C2", 16));
    serialPortOrder.addPort(SerialPort("854393133303518072A1", 33));

    serialPorts = getOrderedSerialPorts();
    numRowsConnected = serialPorts.size();
    
    dimensions.initDimensions(DIMENSIONS_FILENAME);
    setupRowInterfaces();
    reset();
}

/* Returns a vector of SerialPort objects each representing a connected Arduino,
 * sorted by serial number. 
 */
vector<SerialPort> CatoptricSurface::getOrderedSerialPorts() {

    string path = "/dev/serial/by-id";
    string ls_id_cmd = "ls " + path + " > " + LS_ID_FILENAME;

    int ret = system(ls_id_cmd.c_str());
    if(ret == NO_DEVICES) {
        printf("No devices detected in /dev/serial/by-id\n");
        return vector<SerialPort>();
    }

    // Read VFS entries for serial ports from 'ls' output
    vector<SerialPort> serialPorts = readSerialPorts();

    // Sort serial ports by their serial number
    sort(serialPorts.begin(), serialPorts.end(), serialCompObj);

    for(SerialPort sp : serialPorts) {
        printf("Arduino #%s : Row #%d\n", sp.serialNumber.c_str(), sp.row);
    }

    return serialPorts;
}

/* Read the file containing 'ls' output to scan VFS entries.
 * Return vector of SerialPort objects representing only connected Arduinos.
 */
vector<SerialPort> CatoptricSurface::readSerialPorts() {

    vector<SerialPort> serialPorts;

    fstream serialInfoFile;
    serialInfoFile.open(LS_ID_FILENAME, ios::in);
    if(serialInfoFile.is_open()) {
        string serialInfoLine;
        while(getline(serialInfoFile, serialInfoLine)) {
            string serialNumber;
            
            /* Format of Arduino Uno's file name in /dev/serial/by-id is
             *     usb-Arduino__www.arduino.cc__0043_XXXXXXXXXXXXXXXXXXX-YYYY
             * for serial number X's (SERIAL_INFO_PREFIX contains the prefix)
             */

            if(serialInfoLine.find(SERIAL_INFO_PREFIX) != string::npos) {
                serialNumber = serialInfoLine.substr(SERIAL_INFO_PREFIX.size(),
                        SERIAL_NUM_LEN);
            }

            int row = serialPortOrder.getRow(serialNumber);
            if(row == ERR_QUERY_FAILED) {
                printf("Unrecognized device: serialNumber %s\n", 
                        serialNumber.c_str());
            } else {
                serialPorts.push_back(SerialPort(serialNumber, row, 
                            serialInfoLine));
            }
        }

    }
    serialInfoFile.close();

    return serialPorts;
}

/* Initializes a CatoptricRow object for each available Arduino.
 * Initializes rowInterfaces accordingly.
 */
void CatoptricSurface::setupRowInterfaces() {
    for(SerialPort sp : serialPorts) {

        string port = sp.device;
        int rowNum = sp.row;
        int rowLen = dimensions.getLength(rowNum);

		printf(" -- Initializing Catoptric Row %d with %d mirrors\n", 
                rowNum, rowLen);
	    rowInterfaces[rowNum] = CatoptricRow(rowNum, rowLen, port.c_str());
    }
}

/* Reset the orientation of every mirror and resume running.
 */
void CatoptricSurface::reset() {
    printf(" -- Resetting all mirrors to default position\n");
    for(CatoptricRow cr : rowInterfaces) {
        cr.reset(); // Reset whole row
    }

    run();
}

/* Clear the old cached CSV data.
 * Read the new passed CSV file and insert cells into csvData (omit delimiters).
 */
void CatoptricSurface::getCSV(string path) {
    csvData.clear(); // Clear old, cached CSV data

    bool readData = false;

    ifstream fs(path.c_str());
    while(fs && !fs.eof()) {
        readData = true;
        // Get vector of next line's elements
        vector<string> nextLinesCells = getNextLineAndSplitIntoTokens(fs);
        // Append the next line's cells onto csvData
        csvData.insert(csvData.end(), nextLinesCells.begin(), 
                nextLinesCells.end());
   }

   if(!readData) printf("Didn't read data from CSV %s\n", path.c_str());

   fs.close();
}

/* Returns a vector of all cells in the next unread line from the CSV
 * corresponding to the passed i(f)stream.
 */
vector<string> CatoptricSurface::getNextLineAndSplitIntoTokens(istream& str) {
    
    vector<string> result;
    string line, cell;
    
    getline(str,line);
    stringstream lineStream(line);

    while(getline(lineStream, cell, ',')) {
        result.push_back(cell);
    }

    if(result.size() == 0) {
        printf("Failed to read data from CSV line in "
                "getNextLineAndSplitIntoTokens\n");
    }

    return result;
}

/* Read passed CSV.
 * Push a sequence of Message objects onto the commandQueue according to the
 * contents of the CSV.
 * Resume running.
 */
void CatoptricSurface::updateByCSV(string path) {

    getCSV(path);   // Read in CSV contents to csvData

    for(CatoptricRow cr : rowInterfaces) {
        cr.resetSerialBuffer();
    }

    // Synthesize a Message object per CSV line (four cells per line)
    for(int csvLineInd = 0; csvLineInd < csvData.size(); csvLineInd += 4) {
        
        int rowRead, mirrorColumn, motorNumber, position;
        parseCSVLine(csvLineInd, rowRead, mirrorColumn, motorNumber, position);

        Message msg(rowRead, mirrorColumn, motorNumber, position);

        bool foundRow = false;
        for(int rowInd = 0; rowInd < NUM_ROWS; ++rowInd) {
            if(rowRead == rowInterfaces[rowInd].getRowNumber()) {
                foundRow = true;
                rowInterfaces[rowRead].reorientMirrorAxis(msg);
                break;
            }
        }

        if(!foundRow) {
            printf("Line %d of CSV is addressed to a row that does not exist"
                    ": %d", csvLineInd, rowRead);
        }
    }

    run();
}

/* Assign fields for new Message based on specified line from csvData.
 */
void CatoptricSurface::parseCSVLine(int csvLineInd, int& rowRead, 
        int& mirrorColumn, int& motorNumber, int& position) {
    try {
        rowRead      = stoi(csvData[csvLineInd]);
        mirrorColumn = stoi(csvData[csvLineInd + 1]);
        motorNumber  = stoi(csvData[csvLineInd + 2]);
        position     = stoi(csvData[csvLineInd + 3]);
    } catch(const std::invalid_argument& e) {
        printf("Invalid CSV data passed to stoi in updateByCSV: %s\n", 
                e.what());
        return;
    }
}

/* Each row reads incoming data and updates its SerialFSM object, sends queued
 * message to its Arduino.
 * Sleep and print update message.
 */
void CatoptricSurface::run() {

    printf("\n\n");
    int commandsOut = 1, updates = 0;

    while(commandsOut > 0) {

        /* Each row reads incoming data and updates SerialFSM objects, 
           sends messages from the back of respective commandQueue */
        for(CatoptricRow& cr : rowInterfaces) cr.update();

        commandsOut = 0;
        int commandsQueue = 0, ackCount = 0, nackCount = 0;
        
        for(CatoptricRow& cr : rowInterfaces) {
            commandsOut += cr.fsmCommandsOut();
            ackCount += cr.fsmAckCount();
            nackCount += cr.fsmNackCount();
            commandsQueue += cr.commandQueue.size();
        }

        updates++;
        sleep(SLEEP_TIME);
        printf("\r%d commands out | %d commands in queue | %d acks | %d nacks "
                "| %d cycles \r", commandsOut, commandsQueue, ackCount, 
                nackCount, updates);

        for(CatoptricRow& cr : rowInterfaces) {
            cr.fsm.ackCount = 0;
            cr.fsm.nackCount = 0;
        }

        printf("\n\n");
    }
}

