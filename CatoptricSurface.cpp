#include <iostream>
#include <filesystem>
#include <stdlib.h>
#include <stdio.h>      // sprintf
#include <sstream>
#include <fstream>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include <cstring>  // #include <string>
#include <cerrno>   // #include <errno.h>
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

/* Returns the name of the port with the passed order integer,
 * if any. Returns empty string otherwise.
 */
string SerialPortDict::getSerialNumber(int row) {
    for(int i = 0; i < dict.size(); ++i) {
        if(dict[i].row == row) return dict[i].serialNumber;
    }

    return string();
}

/* Returns the order integer for the serial port associated
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

CatoptricSurface::CatoptricSurface() {

    SERIAL_INFO_PREFIX = "usb-Arduino__www.arduino.cc__0043_";
    //rowInterfaces = CatoptricRow[NUM_ROWS];

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
    
    setupRowInterfaces();
    reset();
}

vector<SerialPort> CatoptricSurface::getOrderedSerialPorts() {
    string path = "/dev/serial/by-id";
    string cmd = "ls " + path + " > .serialInfo";
    int ret = system(cmd.c_str());
    if(ret == NO_DEVICES) {
        printf("No devices detected in /dev/serial/by-id\n");
        return vector<SerialPort>();
    }

    vector<SerialPort> serialPorts;

    fstream serialInfoFile;
    serialInfoFile.open(".serialInfo", ios::in);
    if(serialInfoFile.is_open()) {
        string serialInfoLine;
        while(getline(serialInfoFile, serialInfoLine)) {
            string serialNumber;
            
            /* Format of Arduino Uno's file name in /dev/serial/by-id is
             *     usb-Arduino__www.arduino.cc__0043_XXXXXXXXXXXXXXXXXXX-YYYY
             * for serial number X's
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

    // Sort serial ports by their serial number
    sort(serialPorts.begin(), serialPorts.end(), serialCompObj);

    for(SerialPort sp : serialPorts) {
        printf("Arduino #%s : Row #%d\n", sp.serialNumber.c_str(), sp.row);
    }

    return serialPorts;
}

void CatoptricSurface::setupRowInterfaces() {
    for(SerialPort sp : serialPorts) {
        int row = sp.row;
        string port = sp.device;

        int rowLength = 0;
        if(row >= 1 && row < 12) {
            rowLength = 16;
        } else if(row >= 12 && row < 17) {
            rowLength = 24;
        } else if(row >= 17 && row < 28) {
            rowLength = 17;
        } else if(row >= 28 && row < 33) {
            rowLength = 25;
        } else {    // Test setup arduinos
            rowLength = 2;
        }

		printf(" -- Initializing Catoptric Row %d with %d mirrors\n", 
                row, rowLength);
	    rowInterfaces[row] = CatoptricRow(row, rowLength, port.c_str());
    }
}

void CatoptricSurface::reset() {
    printf(" -- Resetting all mirrors to default position\n");
    for(CatoptricRow cr : rowInterfaces) {
        cr.reset(); // Reset each row
    }

    run();
}

vector<string> getNextLineAndSplitIntoTokens(istream& str) {
    vector<string> result;
    string line;
    getline(str,line);

    stringstream lineStream(line);
    string cell;

    while(getline(lineStream, cell, ',')) {
        result.push_back(cell);
    }

    return result;
}

void CatoptricSurface::getCSV(string path) {
    csvData.clear(); // Delete old CSV data

    bool readData = false;

    ifstream fs;
    fs.open(path.c_str(), ios_base::in);
    while(fs.good() && !fs.eof()) {
        readData = true;
        // Get vector of next line's elements
        vector<string> nextLine = getNextLineAndSplitIntoTokens(fs);
        // Append the elements of the next line onto csvData
        csvData.insert(csvData.end(), nextLine.begin(), nextLine.end());
   }

   if(!readData) printf("Didn't read data from CSV %s\n", path.c_str());
}

void CatoptricSurface::updateByCSV(string path) {
    getCSV(path);

    for(CatoptricRow cr : rowInterfaces) {
        cr.resetSerialBuffer();
    }

    int rowRead = -1;
    for(int csvLineInd = 0; csvLineInd < csvData.size(); csvLineInd += 4) {
        try {
            rowRead = stoi(csvData[csvLineInd]);
        } catch(...) {
            printf("updateByCSV parsing error in stoi\n");
            return;
        }

        for(int rowInd = 0; rowInd < rowInterfaces.size(); ++rowInd) {
            if(rowRead == rowInterfaces[rowInd].row) {
                foundRow = true;
                rowInterfaces[rowRead].reorientMirrorAxis();
                break;
            }
        }

        if(!foundRow) {
            printf("Line %d of CSV is addressed to a row that does not exist
                    : %d", csvLineInd, rowRead);
        }
    }

    run();
}

void CatoptricSurface::run() {
    printf("\n\n");

    int commandsOut = 1;
    int updates = 0;
    while(commandsOut > 0) {
        for(CatoptricRow& cr : rowInterfaces) cr.update();

        commandsOut = 0;
        int commandsQueue = 0;
        int ackCount = 0;
        int nackCount = 0;
        
        for(CatoptricRow& cr : rowInterfaces) {
            commandsOut += cr.getCurrentCommandsOut();
            ackCount += cr.getCurrentAckCount();
            nackCount += cr.getCurrentNackCount();
            commandsQueue += cr.commandQueue.size();
        }
        updates++;
        // TODO: Do we really need to sleep here? Why, and is this a good way?
        this_thread::sleep_for (chrono::seconds(1));
        printf("\r%d commands out | %d commands in queue | %d acks | %d nacks 
                | %d cycles \r", commandsOut, commandsQueue, ackCount, 
                nackCount, updates);

        for(CatoptricRow& cr : rowInterfaces) {
            cr.fsm.ackCount = 0;
            cr.fsm.nackCount = 0;
        }

        printf("\n\n");
    }
}

CatoptricController::CatoptricController() {
    surface = CatoptricSurface();
}

vector<string> CatoptricController::checkForNewCSV() {
    string directoryStr = "csv/new";
    vector<string> newCSVs;
    
    string filePath, ending = ".csv";
    // For each file in directory csv/new
    for (const auto & entry : filesystem::directory_iterator(directoryStr)) {
        filePath = entry.path();
        // Check if each file's name ends in CSV extension
        if(0 == filePath.compare(filePath.length() - ending.length(), 
                    ending.length(), ending)) {
            newCSVs.push_back(filePath);
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
        if(wc_elem.empty() != true) return stoi(wc_elem);
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
        printf("%s", inputMessage);
        cin >> userInput;
        printf("\n\n");
        userInput = userInput.tolower();
        if(0 == userInput.compare("reset")) {
            surface.reset();
            printf(" -- Reset Complete\n");
        } else if(csvList.size() > 0 && 0 == userInput.compare("run")) {
            printf(" -- Running \'%s\'\n", csv.c_str());
            surface.updateByCSV(csv); // TODO : Need to alter csv before?
            printf(" -- \'%s\' ran successfully\n", csv.c_str());

            char ls_wc_cmd[CMD_LEN];
            snprintf(ls_wc_cmd, CMD_LEN, "ls -l ./csv/archive | wc > %s", 
                    LS_WC_FILENAME);
            if(0 != system(ls_wc_cmd)) {
                printf("Error in system function for command \'%s\': %s\n", 
                        ls_wc_cmd, strerror(errno));
                return;
            }

            int archiveLength;
            ifstream fs;
            fs.open(LS_WC_FILENAME, ios_base::in);
            if(fs.good() && !fs.eof()) {
                // 'ls -l' output has one extra line for number of blocks,
                // so subtract one for number of files in directory
                archiveLength = extractFirstIntFromFile(fs) - 1;
            }

            // Account for -1 offset to check for error code
            if(archiveLength + 1 == ERR_NO_INT) return;

            string newName = "./csv/archive/" + to_string(archiveLength) + 
                "_" + csv;
            /* os.rename(csv, newName)
             * Execute system call 'mv %s %s' w str formatting for csv, newName
             */
            string mov_cmd = "mov " + csv + " " + newName;
            if(system(mov_cmd) != 0) {
                printf("Error in system function for command \'%s\': %s\n", 
                        mov_cmd, strerror(errno));
                return;
                
            }
            printf(" -- \'%s\' moved to archive\n", csv.c_str());
            
        }
    }
}

/*
class CatoptricController():

	def run(self):
		while True:
			print ("\n-------------------------\n")
			csv = ""
			csvList = self.checkForNewCSV()
			inputMessage = bcolors.OKBLUE + "'Reset' mirrors or upload a file to run: " + bcolors.ENDC
			
			if (len(csvList) > 0):
				csv = csvList[0] #This could be more intelligent
				print (" -- Found csv file '%s'\n" % os.path.basename(csv))
				inputMessage = bcolors.OKBLUE + "'Reset' mirrors or 'Run' file: " + bcolors.ENDC

			c = input(inputMessage)
			print ("\n")

			if (c.lower() == 'reset'):
				self.surface.reset()
				print (" -- Reset Complete")

			elif (len(csvList) > 0 and c.lower() == 'run'):
				fileName = os.path.basename(csv)
				print (" -- Running '%s'" % fileName)

				self.surface.updateByCSV(csv)
				print (" -- '%s' ran successfully" % fileName)

				archiveLength = len([name for name in os.listdir('./csv/archive')])
				newName = './csv/archive/' + str(archiveLength) + "_" + fileName
				os.rename(csv, newName)
				print (" -- '%s' moved to archive" % fileName)


				




if __name__ == '__main__':
	c = CatoptricController()
	c.run()
*/
