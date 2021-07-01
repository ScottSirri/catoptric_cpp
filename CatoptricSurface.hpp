#pragma once

#include "CatoptricRow.hpp"
#include "SerialFSM.hpp"
#include <string>

#define STR_EQ 0
#define UNDEF_ORDER -4

#define ERR_QUERY_FAILED -3
#define ERR_NO_INT -7
#define ERR_STOI -9
#define NO_DEVICES 512
#define SERIAL_NUM_LEN 20
#define NUM_ROWS 32
#define CONTROLLER_RUNNING 1
#define CMD_LEN 100
#define SYSTEM_SUCCESS 0
#define CMP_EQUAL 0

#define LS_ID_FILENAME ".serialInfo"
#define LS_WC_FILENAME ".numFiles"
#define LS_CSV_FILENAME ".csvSearch"

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
    // Used to sort serial ports by their Arduino's serial number
    bool operator() (SerialPort a, SerialPort b) {
        return a.serialNumber.compare(b.serialNumber) < 0;
    }
} serialCompObj;

class CatoptricSurface {

    private:
        int numRowsConnected;
        SerialPortDict serialPortOrder;
        CatoptricRow rowInterfaces[NUM_ROWS];
        static std::string SERIAL_INFO_PREFIX;
        std::vector<SerialPort> serialPorts;
        std::vector<std::string> csvData;

        /* Initializes a Row Interface for each available arduino */
        void setupRowInterfaces();
        /* Returns a list of serial ports corresponding to Arduinos, ordered 
           according to the serialPortOrder dictionary */
        std::vector<SerialPort> getOrderedSerialPorts();
        void getCSV(std::string path);
        void run();

    public:
        CatoptricSurface();

        void reset();
        void updateByCSV(std::string path);

};

class CatoptricController {

    CatoptricSurface surface;

    CatoptricController();

    std::vector<std::string> checkForNewCSV();
	void run();
};

int main();
