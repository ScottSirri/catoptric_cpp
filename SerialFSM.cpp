/*
 * This class implements the FSM per Arduino on a Pi. Keeps track of the Pi's
 * state in the process of receiving data from the Arduino-- namely, acks and
 * debug messages at this point.
 */

#include "SerialFSM.hpp"

using namespace std;

SerialFSM::SerialFSM(string name_in) {

    name = name_in;
    
    currentCommandsToArduino = 0;
    nackCount = 0;
    ackCount = 0;
    currentState = 0;

    resetVariables();
}

SerialFSM::SerialFSM() {
    
    currentCommandsToArduino = 0;
    nackCount = 0;
    ackCount = 0;
    currentState = 0;

    resetVariables();
}

/* FSM control logic function.
 */
void SerialFSM::Execute(char c) {
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


void SerialFSM::resetVariables() {

    ackX = 0;
    ackY = 0;
    ackM = 0;
    countHigh = 0;
    countLow = 0;
    count = 0;

    clearMsg(); 
}

/* Clear any currently cached message, including deallocation of its memory. 
 */
void SerialFSM::clearMsg() {

    if(message != NULL) {
        free(message);
        message = NULL;
    }

    messageEnd = message;
    messageReady = false;
}

char SerialFSM::getMagicNum(char c) {
    resetVariables();
    if (c == '!') {
        return GET_KEY;
    } else {
        return GET_MAGIC_NUM;
    }
}
    
char SerialFSM::getKey(char c) {
    if (c == 'a') {
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

char SerialFSM::getAckKey(char c) {
    if (c == 'A') {
        return GET_ACK_X;
    } else {
        return GET_MAGIC_NUM;
    }
}
    
char SerialFSM::getAckX(char c) {
    if (c <= 32) {
        ackX = c;
        return GET_ACK_Y;
    } else {
        return GET_MAGIC_NUM;
    }
}

char SerialFSM::getAckY(char c) {
    if (c <= 32) {
        ackY = c;
        return GET_ACK_M;
    } else {
        return GET_MAGIC_NUM;
    }
}

char SerialFSM::getAckM(char c) {
    if (c <= 2) {
        ackM = c;
        currentCommandsToArduino -= 1; // Successful ack completed
        return GET_MAGIC_NUM;
    } else {
        return GET_MAGIC_NUM;
    }
}

char SerialFSM::getNumCharHigh(char c) {
    countHigh = c;
    return GET_NUM_CHAR_LOW;
}

char SerialFSM::getNumCharLow(char c) {

    countLow = c;
    count = (countHigh << 8) + countLow;

    clearMsg(); // Avoid leak

    // Null term + safety buffer character
    message = (char *) calloc(count + 2, sizeof(char)); 
    messageEnd = message;

    return GET_CHAR;
}

char SerialFSM::getChar(char c) {

    if(count > 1) {
        *messageEnd = c;
        messageEnd++;
        count -= 1;
        return GET_CHAR;
    } else {
        *messageEnd = c;
        messageEnd++;
        *messageEnd = '\0';
        messageReady = true;
        return GET_MAGIC_NUM;
    }
}

char SerialFSM::getNackKey(char c) {
    if (c == 'B') {
        // TODO - Process Nack
        return GET_MAGIC_NUM;
    } else {
        return GET_MAGIC_NUM;
    }
}
