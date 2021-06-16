/*
 * This class represents the FSM for a Raspberry Pi managing an Arduino
 */

#include <stdio.h>
#include "SerialFSM.hpp"

// Define FSMstates
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


/*int main() {

    return 0;
}*/

class SerialFSM {

    public:

    char *name; // TODO: unknown purpose?
    char message[MAX_MSG_LEN], *messageEnd; // messageEnd points to the null terminator of message string
    int messageReady;
    int currentCommandsToArduino; // TODO: unknown purpose? Is need in CatoptricRow.cpp
    int nackCount, ackCount;
    int count, countHigh, countLow; // Character count for message, temporary vars
    int ackX, ackY, ackM; // Ack vars for X, Y, and Mirror
    int currentState;

    SerialFSM(char *name_in) {
        name = name_in;
        //super(SerialFSM, self).__init__()
        currentCommandsToArduino = 0;
        nackCount = 0;
        ackCount = 0;

        currentState = 0;
        //self.setupStates()
        resetVariables();
    }

    SerialFSM() {
        //super(SerialFSM, self).__init__()
        currentCommandsToArduino = 0;
        nackCount = 0;
        ackCount = 0;

        currentState = 0;
        //self.setupStates()
        resetVariables();
    }

    void Execute(char c) {
        switch(currentState) {
            case GET_MAGIC_NUM:
            {
                currentState = getMagicNum(c);
                break;
            }
            case GET_KEY:
            {
                currentState = getKey(c);
                break;
            }
            case GET_NUM_CHAR_HIGH:
            {
                currentState = getNumCharHigh(c);
                break;
            }
            case GET_NUM_CHAR_LOW:
            {
                currentState = getNumCharLow(c);
                break;
            }
            case GET_CHAR:
            {
                currentState = getChar(c);
                break;
            }
            case GET_ACK_KEY:
            {
                currentState = getAckKey(c);
                break;
            }
            case GET_ACK_X:
            {
                currentState = getAckX(c);
                break;
            }
            case GET_ACK_Y:
            {
                currentState = getAckY(c);
                break;
            }
            case GET_ACK_M:
            {
                currentState = getAckM(c);
                break;
            }
            case GET_NACK_KEY:
            {
                currentState = getNackKey(c);
                break;
            }
            default:
            {
                printf("Invalid FSM state\n");
            }
        }
    }


    void resetVariables() {
        ackX = 0;
        ackY = 0;
        ackM = 0;
        countHigh = 0;
        countLow = 0;
        count = 0;
        messageEnd = message;
        *messageEnd = '\0';
        messageReady = false; 
    }

    void clearMessage() {
        messageEnd = message;
        *messageEnd = '\0';
        messageReady = false;
    }

    private:

/* 
 * STATES
 */

    char getMagicNum(char c) {
        resetVariables();
        if (c == '!') {
            return GET_KEY;
        } else {
            return GET_MAGIC_NUM;
        }
    }
        
    char getKey(char c) {
        if (c == 'a') {
            currentCommandsToArduino -= 1;
            ackCount += 1;
            return GET_ACK_KEY;
        } else if (c == 'b') {
            nackCount += 1;
            return GET_NACK_KEY;
        } else if (c == 'c') {
            return GET_NUM_CHAR_HIGH;
        } else {
            return GET_MAGIC_NUM;
        }
    }

    char getAckKey(char c) {
        if (c == 'A') {
            return GET_ACK_X;
        } else {
            return GET_MAGIC_NUM;
        }
    }
        
    char getAckX(char c) {
        if (c <= 32) {
            ackX = c;
            return GET_ACK_Y;
        } else {
            return GET_MAGIC_NUM;
        }
    }

    char getAckY(char c) {
        if (c <= 32) {
            ackY = c;
            return GET_ACK_M;
        } else {
            return GET_MAGIC_NUM;
        }
    }

    char getAckM(char c) {
        if (c <= 2) {
            ackM = c;
            // print("Mirror (%d, %d), Motor %d moved to new state" % (self.ackX, self.ackY, self.ackM));
            return GET_MAGIC_NUM;
        } else {
            return GET_MAGIC_NUM;
        }
    }

    char getNumCharHigh(char c) {
        countHigh = c;
        return GET_NUM_CHAR_LOW;
    }

    char getNumCharLow(char c) {
        countLow = c;
        count = (countHigh << 8) + countLow;
        messageEnd = message;
        return GET_CHAR;
    }

    char getChar(char c) {
        if (count <= 1) {
            *messageEnd = c;
            messageEnd++;
            *messageEnd = '\0';
            messageReady = true;
            return GET_MAGIC_NUM;
        } else {
            *messageEnd = c;
            messageEnd++;
            count -= 1;
            return GET_CHAR;
        }
    }

    char getNackKey(char c) {
        if (c == 'B') {
            // TODO - Process Nack
            return GET_MAGIC_NUM;
        } else {
            return GET_MAGIC_NUM;
        }
    }
};
