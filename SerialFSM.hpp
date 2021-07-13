#pragma once

#include <string>

// Define FSM states
#define GET_MAGIC_NUM 0
#define GET_KEY 1
#define GET_NUM_CHAR_HIGH 2
#define GET_NUM_CHAR_LOW 3
#define GET_CHAR 4 
#define GET_ACK_KEY 5
#define GET_ACK_X 6
#define GET_ACK_Y 7
#define GET_ACK_M 8
#define GET_NACK_KEY 9

class SerialFSM {

    public:
        std::string name; // TODO: unknown purpose?
        // messageEnd points to the null terminator of message string
        char *message, *messageEnd; 
        int messageReady;
        // I think this is the number of commands sent to the Arduino that 
        // haven't been ack'd yet
        int currentCommandsToArduino; 
        int nackCount, ackCount;
        // Character count for message, temporary vars
        int count, countHigh, countLow;
        int ackX, ackY, ackM; // Ack vars for X, Y, and Mirror
        int currentState;

        SerialFSM(std::string name_in);
        SerialFSM();
        void Execute(char c); 
        void resetVariables();
        void clearMsg();

    private:
        char getMagicNum(char c);
        char getKey(char c); 
        char getAckKey(char c);
        char getAckX(char c);
        char getAckY(char c);
        char getAckM(char c);
        char getNumCharHigh(char c);
        char getNumCharLow(char c);
        char getChar(char c);
        char getNackKey(char c);
};
