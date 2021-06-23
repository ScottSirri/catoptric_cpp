#pragma once

#include "CatoptricRow.hpp"
#include "SerialFSM.hpp"
#include <string>

#define STR_EQ 0
#define UNDEF_ORDER -4

#define ERR_QUERY_FAILED -3
#define NO_DEVICES 512
#define SERIAL_NUM_LEN 20

struct SerialPort {
    std::string serialNumber;   // Serial number (how to obtain in C++?)
    int row;            // Row of Arduino according to serialPortOrder
    std::string device; // Path to port (symlink in /dev/serial/by-id)

    SerialPort();
    SerialPort(std::string serialIn, int rowIn);
    SerialPort(std::string serialIn, int rowIn, std::string deviceIn);
};

class SerialPortDict {
    
    private:
        std::vector<SerialPort> dict;

    public:
        std::string getSerialNumber(int row);
        int getRow(std::string serialNumber);
        void addPort(SerialPort port);
};

struct serialComp {
    bool operator() (SerialPort a, SerialPort b) {
        return a.serialNumber.compare(b.serialNumber) < 0;
    }
} serialCompObj;

class CatoptricSurface {

    static std::string SERIAL_INFO_PREFIX;

    SerialPortDict serialPortOrder;
    std::vector<SerialPort> serialPorts;
    int numRows;


    CatoptricSurface();
	// Initializes a Row Interface for each available arduino
    void setupRowInterfaces();
	/* Returns a list of serial ports corresponding to Arduionos, ordered 
     * according to the serialPortOrder dictionary
     */
    std::vector<SerialPort> getOrderedSerialPorts();
	void reset();
	void getCSV(std::string path);
	void updateByCSV(std::string path);
	void run();
};

class CatoptricController {

    CatoptricController();

	void checkForNewCSV();
	void run();
};

int main();
