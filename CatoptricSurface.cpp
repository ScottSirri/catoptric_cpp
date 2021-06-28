#include <stdlib.h>
#include <sstream>
#include <fstream>
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

/*
class CatoptricSurface():

	def updateByCSV(self, path):
		self.getCSV(path)

		for r in self.rowInterfaces:
			self.rowInterfaces[r].resetSerialBuffer()

		for i in range(0, len(self.csvData)):
			line = self.csvData[i]
			if (int(line[0]) in self.rowInterfaces):
				self.rowInterfaces[int(line[0])].reorientMirrorAxis(line)
			else:
				print("line %d of csv is addressed to a row that does not exist: %d" % (i, int(line[0])))
		
		self.run()


	def run(self):
		print ("\n")

		commandsOut = 1
		updates = 0
		while (commandsOut > 0):

			for r in self.rowInterfaces:
				self.rowInterfaces[r].update()

			commandsOut = 0
			commandsQueue = 0
			ackCount = 0
			nackCount = 0
			
			for row in self.rowInterfaces:
				commandsOut += self.rowInterfaces[row].getCurrentCommandsOut()
				ackCount += self.rowInterfaces[row].getCurrentAckCount()
				nackCount += self.rowInterfaces[row].getCurrentNackCount()
				commandsQueue += self.rowInterfaces[row].commandQueue.qsize()
			updates += 1
			time.sleep(1)

			sys.stdout.write("\r%d commands out | %d commands in queue | %d acks | %d nacks | %d cycles \r" % (commandsOut, commandsQueue, ackCount, nackCount, updates))
		
		for r in self.rowInterfaces:
			self.rowInterfaces[r].fsm.ackCount = 0
			self.rowInterfaces[r].fsm.nackCount = 0

		print ("\n")

	

			


		
class CatoptricController():
	def __init__(self):
		self.surface = CatoptricSurface()

	def checkForNewCSV(self):
		directory = 'csv/new'
		newCSVs = [os.path.join(directory, name) for name in os.listdir(directory) 
			if os.path.isfile(os.path.join(directory, name)) 
			and name.endswith('.csv')]
		return newCSVs


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
