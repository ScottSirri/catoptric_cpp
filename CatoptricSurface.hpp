#pragma once

#include "CatoptricRow.hpp"
#include "SerialFSM.hpp"
#include <string>

#define STR_EQ 0
#define UNDEF_ORDER -4

#define ERR_QUERY_FAILED -3

struct SerialPort {
    std::string portName;
    int order; 

    SerialPort();
    SerialPort(std::string nameIn, int orderIn);
};

class SerialPortDict {
    
    private:
        std::vector<SerialPort> dict;

    public:
        std::string getPortName(int order);
        int getOrder(std::string portName);
        void addPort(SerialPort port);
};

class CatoptricSurface {

    SerialPortDict serialPortOrder;
    vector<SerialPort> serialPorts;
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
