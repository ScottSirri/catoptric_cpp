#pragma once

#include "CatoptricRow.hpp"
#include "SerialFSM.hpp"
#include <string>

#define STR_EQ 0
#define UNDEF_ORDER -4

#define RET_SUCCESS 0
#define ERR_QUERY_FAILED -3
#define ERR_DIMS_FILE -6
#define ERR_NO_INT -7
#define ERR_ROW_NUM -8
#define ERR_STOI -9
#define NO_DEVICES 512
#define SERIAL_NUM_LEN 20  /* Number characters in Arduino serial number */
#define NUM_ROWS 32
#define CONTROLLER_RUNNING 1
#define CMD_LEN 100
#define SYSTEM_SUCCESS 0
#define CMP_EQUAL 0
#define SLEEP_TIME 1    /* Seconds to sleep per CatoptricSurface::run() cycle */

#define DEFAULT_ROW_LEN 2

#define LS_ID_FILENAME ".serialInfo"    // Arbitrarily named
#define LS_WC_FILENAME ".numFiles"      // Arbitrarily named
#define LS_CSV_FILENAME ".csvSearch"    // Arbitrarily named
#define DIMENSIONS_FILENAME "surface_dimensions"    // Must be present in dir
#define SERIAL_INFO_PREFIX_MACRO "usb-Arduino__www.arduino.cc__0043_"

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

/* Encodes dimensions of surface in the form of map from row number to 
 * row length.
 */
class SurfaceDimensions {

    private:
        // Number of mirrors in a row, doesn't account for alignment nor spacing
        std::vector<int> rowLengths;
        // Typically defaulted to in functional prototype (e.g. 2x2 array)
        int defaultRowLen;

    public:
        int initDimensions(std::string filePath);
        int getLength(int rowNumber);
};

class CatoptricSurface {

    private:
        // Number of rows in surface
        int numRowsConnected;
        // Hard-coded dictionary of the setup's Arduinos and each serial number
        SerialPortDict serialPortOrder; 
        // Encodes the dimensions of surface (length of each row)
        SurfaceDimensions dimensions;
        // Vector of objects, each representing one row in surface
        CatoptricRow rowInterfaces[NUM_ROWS];
        // Vector of objects each representing a serial port open to an Arduino
        std::vector<SerialPort> serialPorts;
        // Vector of cells read in from the latest CSV
        std::vector<std::string> csvData;
        // Prefix for VFS entry of a serial port connected to an Arduino
        std::string SERIAL_INFO_PREFIX; 

        std::vector<SerialPort> getOrderedSerialPorts();
        std::vector<SerialPort> readSerialPorts();
        void setupRowInterfaces();
        void run();
        void getCSV(std::string path);
        std::vector<std::string> getNextLineAndSplitIntoTokens(
                std::istream& str);
        void parseCSVLine(int csvLineInd, int& rowRead, 
                int& mirrorColumn, int& motorNumber, int& position); 

    public:
        CatoptricSurface();

        void reset();
        void updateByCSV(std::string path);

};
