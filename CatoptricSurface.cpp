#include <stdlib.h>
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
    numRows = serialPorts.size();
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
                serialNumber = serialInfoLine.substr(SERIAL_INFO_PREFIX.size(), SERIAL_NUM_LEN);
            }

            int row = serialPortOrder.getRow(serialNumber);
            if(row == ERR_QUERY_FAILED) {
                printf("Unrecognized device: serialNumber %s\n", serialNumber.c_str());
            } else {
                serialPorts.push_back(SerialPort(serialNumber, row, serialInfoLine));
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

/*
class CatoptricSurface():

	def __init__(self):
		self.serialPorts = self.getOrderedSerialPorts()
		self.numRows = len(self.serialPorts)
		self.rowInterfaces = dict()

		self.setupRowInterfaces()
		self.reset() 


	# Initializes a Row Interface for each available arduino
	def setupRowInterfaces(self):

		for sP in self.serialPorts:
			name = serialPortOrder[sP.serial_number]
			port = sP.device

			rowLength = 0
			if (name >= 1 and name < 12):
				rowLength = 16						### THIS SHOULD BE 16, NOT 10 ###
			elif (name >= 12 and name < 17):
				rowLength = 24
			elif (name >= 17 and name < 28):
				rowLength = 17
			elif (name >= 28 and name < 33):
				rowLength = 25
			else: #Test setup arduinos
				rowLength = 2

			print (" -- Initializing Catoptric Row %d with %d mirrors" % (name, rowLength))

			self.rowInterfaces[name] = CatoptricRow(name, rowLength, port)
	

	# Returns a list of serial ports, ordered according to the serialPortOrder dictionary
	def getOrderedSerialPorts(self):

		# Get all serial ports
		allPorts = serial.tools.list_ports.comports()
		#print(allPorts)
		
		# Get only ports with arduinos attached
		arduinoPorts = [p for p in allPorts if p.pid == 67]
		print (" -- %d Arduinos Found:" % len(arduinoPorts)) 

		# Sort arduino ports by row
		try:
			arduinoPorts.sort(key= lambda x: serialPortOrder[x.serial_number])
		except:
			print(" -- One or more arduino serial number unrecognized")
        
		for a in arduinoPorts:
			try:
				print ("    Arduino #%s : Row #%d" % (a.serial_number, serialPortOrder[a.serial_number]))
			except:
				print ("    Arduino #%s : Unrecognized Serial Number" % a.serial_number)

		return arduinoPorts
	

	def reset(self):
		print (" -- Resetting all mirrors to default position")
		for r in self.rowInterfaces:
			self.rowInterfaces[r].reset()
		
		self.run()


	def getCSV(self, path):
		# Delete old CSV data
		self.csvData  = []

		# Read in CSV contents
		with open(path, newline='') as csvfile:
			reader = csv.reader(csvfile, delimiter=',')
			for row in reader:
				x = []
				for i in range(0, len(row)):
					x.append(row[i])
				self.csvData.append(x)
	

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
