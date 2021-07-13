
#include <sys/ioctl.h>
#include <string>
#include <termios.h>
#include <vector>
#include <unistd.h>

#include "CatoptricRow.hpp"
#include "SerialFSM.hpp"
#include "prep_serial.hpp"
#include "ErrCodes.hpp"

using namespace std;

MotorState::MotorState() {
    motor[PAN_IND] = 0;
    motor[TILT_IND] = 0;
}

MotorState::MotorState(int panIn, int tiltIn) {
    motor[PAN_IND] = panIn;
    motor[TILT_IND] = tiltIn;
}

Message::Message(int row_in, int mirror_in, int motor_in, int dir_in, 
        int chigh_in, int clow_in) {
    rowNum = row_in;
    mirrorID = mirror_in;
    whichMotor = motor_in;
    direction = dir_in;
    countHigh = chigh_in;
    countLow = clow_in;
}

Message::Message(int mirrorRow, int mirrorColumn, int motorNumber, 
        int position) {
    rowNum = mirrorRow;
    mirrorID = mirrorColumn;
    whichMotor = motorNumber;
    newPos = position;
}

/* Convert the message into a serialized byte vector that can be sent to
 * the Arduino.
 */
vector<char> Message::toVec() {

    string msgStr = toStr();
    vector<char> msgVec(msgStr.begin(), msgStr.end());

    return msgVec;
}

/* Convert the Message object into a string containing what will be transmitted.
 */
string Message::toStr() {

    string str;
    try {
        str = to_string(MSG_MAGIC_NUM) + to_string(ACK_KEY) + 
            to_string(rowNum) + to_string(mirrorID) + to_string(whichMotor) +
            to_string(direction) + to_string(countHigh) + to_string(countLow);
    } catch (...) {
        printf("to_string error: %s\n", strerror(errno));
        return string();
    }

    return str;
}

CatoptricRow::CatoptricRow() {}

CatoptricRow::CatoptricRow(int rowNumberIn, int numMirrorsIn, 
        const char *serialPortIn) {

	rowNumber = rowNumberIn;
	numMirrors = numMirrorsIn;

	// Initalize a MotorState object for each motor
	for(int i = 0; i < numMirrors; ++i) {
        MotorState state = MotorState();
	    motorStates.push_back(state);
    }

	// Configure the relevant serial port for communication
	setupSerial(serialPortIn);

    // Create FSM for communication with this Arduino.
    string rowStr = to_string(rowNumber);
    const char *rowCStr = rowStr.c_str();
	fsm = SerialFSM(rowCStr);
}

/* Prepare the correpsonding serial port for IO (termios).
 */
int CatoptricRow::setupSerial(const char *serialPortIn) {
    
    // Returns fd for configured serial port
    serial_fd = prep_serial(serialPortIn); 
    
    // Flush residual data in buffer
    if(tcflush(serial_fd, TCIOFLUSH) < 0) {
        printf("tcflush error: %s\n", strerror(errno));
        return ERR_TCFLUSH;
    }
    
    sleep(SETUP_SLEEP_TIME); // Why does this function sleep?

    return RET_SUCCESS;
}

/* Flush the serial port buffer.
 */
int CatoptricRow::resetSerialBuffer() {
    if(tcflush(serial_fd, TCIOFLUSH) < 0) {
        printf("tcflush error: %s\n", strerror(errno));
        return ERR_TCFLUSH;
    }

    return RET_SUCCESS;
}

/* Read all input from the corresponding serial port and update the CatoptricRow
 * object's SerialFSM object.
 * Send a Message object from the back of the commandQueue to the Arduino.
 */
void CatoptricRow::update() {
    
    char input;
    while(read(serial_fd, &input, 1) > 0) {
        fsm.Execute(input);
        if(fsm.messageReady) {
            printf("Incoming message:%s\n", fsm.message);
            fsm.clearMsg();
        }
    }

    // If the number of pending commands is < max limit and is > 0
	if(fsmCommandsOut() < MAX_CMDS_OUT && commandQueue.size() > 0) { 
		Message message = commandQueue.back();
        commandQueue.pop_back();
		sendMessageToArduino(message);
    }
}

/* Transmit the passed Message to the Arduino.
 */
void CatoptricRow::sendMessageToArduino(Message message) {

    vector<char> message_vec = message.toVec();
    char bCurrent;

	for(int i = 0; i < NUM_MSG_ELEMS; ++i) {
		bCurrent = message_vec[i];
        if(write(serial_fd, &bCurrent, 1) < 0) {
            printf("write error: %s\n", strerror(errno));
            return;
        }
    }

    printf("Successfully sent message to Arduino:%s\n", 
            message.toStr().c_str());

	fsm.currentCommandsToArduino += 1; // New sent message, awaiting ack
}

/* Push a Message onto the commandQueue to update a mirror's position.
 */
void CatoptricRow::stepMotor(int mirrorID, int whichMotor, 
        int direction, float deltaPos) {

    // I assume there's 513 steps in the motor?
	int deltaPosInt = (int) (deltaPos * (513.0/360.0));
	int countLow = ((int) deltaPosInt) & 255;
	int countHigh = (((int) deltaPosInt) >> 8) & 255;
    
    // mirrorID could just as well be named columnNumber
	Message message (rowNumber, mirrorID, whichMotor, direction, 
            countHigh, countLow);
	commandQueue.push_back(message);
}

/* Push a Message onto the commandQueue and update motorStates. 
 * TODO : There may be functional redundancy between stepMotor() and 
 *        reorientMirrorAxis()?
 */
void CatoptricRow::reorientMirrorAxis(Message command) {

    int mirror = command.mirrorID;
    int motor = command.whichMotor;
    int newState = command.newPos;
    int currentState = -1; // Placeholder value to silence compiler warnings

    if(motor == PAN_IND) {
        currentState = motorStates[mirror - 1].motor[PAN_IND];
    } else if(motor == TILT_IND) {
        currentState = motorStates[mirror - 1].motor[TILT_IND];
    } else {
        printf("Invalid 'motor' value in reorientMirrorAxis\n");
        return;
    }

    int delta = newState - currentState;
    int direction = delta <  0 ? 1 : 0;
    if(delta < 0) delta *= -1;

    stepMotor(mirror, motor, direction, delta);
    motorStates[mirror - 1].motor[motor] = newState;
}

/* Reset/'zero' the orientation of all mirrors in this row.
 */
void CatoptricRow::reset() {
	for(int i = 0; i < numMirrors; ++i) {
        // Column numbers seem to not be 0-indexed on the Arduino?
		stepMotor(i + 1, 1, 0, 200);
		stepMotor(i + 1, 0, 0, 200);
		motorStates[i].motor[PAN_IND] = 0;
		motorStates[i].motor[TILT_IND] = 0;
    }
}

/* Get the SerialFSM's currentCommandsToArduino */
int CatoptricRow::fsmCommandsOut() {
	return fsm.currentCommandsToArduino;
}

/* Get the SerialFSM's nackCount */
int CatoptricRow::fsmNackCount() {
	return fsm.nackCount;
}

/* Get the SerialFSM's ackCount */
int CatoptricRow::fsmAckCount() {
	return fsm.ackCount;
}

/* Get the CatoptricRow's rowNumber */
int CatoptricRow::getRowNumber() {
    return rowNumber;
}

