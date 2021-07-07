#include "SerialFSM.hpp"
#include "prep_serial.hpp"
#include <sys/ioctl.h>
#include <string>
#include <termios.h>
#include <vector>
#include <unistd.h>
#include "CatoptricRow.hpp"

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
    row_num = row_in;
    mirror_id = mirror_in;
    which_motor = motor_in;
    direction = dir_in;
    count_high = chigh_in;
    count_low = clow_in;
}

Message::Message(int mirrorRow, int mirrorColumn, int motorNumber, 
        int position) {
    row_num = mirrorRow;
    mirror_id = mirrorColumn;
    which_motor = motorNumber;
    new_pos = position;
}

/* Convert the message into a serialized byte vector that can be sent to
 * the Arduino.
 */
vector<char> Message::to_vec() {

    string str;

    try {
        str = to_string(MSG_MAGIC_NUM) + to_string(ACK_KEY) + 
            to_string(row_num) + to_string(mirror_id) + to_string(which_motor) +
            to_string(direction) + to_string(count_high) + to_string(count_low);
    } catch (...) {
        printf("to_string error: %s\n", strerror(errno));
        return vector<char>();
    }

    vector<char> msgVec(str.begin(), str.end());

    return msgVec;
}

CatoptricRow::CatoptricRow() {}

CatoptricRow::CatoptricRow(int rowNumberIn, int numMirrorsIn, 
        const char *serialPortIn) {

	rowNumber = rowNumberIn;
	numMirrors = numMirrorsIn;

	// Init Motor States
	for(int i = 0; i < numMirrors; ++i) {
        MotorState state = MotorState();
	    motorStates.push_back(state);
    }

	// Setup Serial
	setup(serialPortIn);

    string row_str = to_string(rowNumber);
    const char *row_cstr = row_str.c_str();
	fsm = SerialFSM(row_cstr);
}

/* Prepare the correpsonding serial port for IO (termios).
 */
int CatoptricRow::setup(const char *serialPortIn) {
    
    // Returns fd for configured serial port
    serial_fd = prep_serial(serialPortIn); 
    
    // Flush residual data in buffer
    if(tcflush(serial_fd, TCIOFLUSH) < 0) {
        printf("tcflush error: %s\n", strerror(errno));
        return ERR_TCFLUSH;
    }
    
    sleep(SETUP_SLEEP_TIME); // Why does this sleep?

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
            fsm.clearMessage();
        }
    }

    // If the number of pending commands is < max limit and is > 0
	if(fsmCommandsOut() < MAX_CMDS_OUT && commandQueue.size() > 0) { 
		Message message = commandQueue.back();
        commandQueue.pop_back();
		sendMessageToArduino(message);
    }
}

/* Push a Message onto the commandQueue to update a mirror's position.
 */
void CatoptricRow::step_motor(int mirror_id, int which_motor, 
        int direction, float delta_pos) {
	int delta_pos_int = ((int) delta_pos) * (513.0/360.0);
	int countLow = ((int) delta_pos_int) & 255;
	int countHigh = (((int) delta_pos_int) >> 8) & 255;
    
	Message message (rowNumber, mirror_id, which_motor, direction, 
            countHigh, countLow);
	commandQueue.push_back(message);
}

/* Transmit the passed Message to the Arduino.
 */
void CatoptricRow::sendMessageToArduino(Message message) {

    vector<char> message_vec = message.to_vec();
    char bCurrent;

	for(int i = 0; i < NUM_MSG_ELEMS; ++i) {
		bCurrent = message_vec[i];
        if(write(serial_fd, &bCurrent, 1) < 0) {
            printf("write error: %s\n", strerror(errno));
            return;
        }
    }

	fsm.currentCommandsToArduino += 1;
}

/* Push a Message onto the commandQueue and update motorStates. 
 * TODO : There may be redundancy between step_motor() and reorientMirrorAxis()?
 */
void CatoptricRow::reorientMirrorAxis(Message command) {
    int mirror = command.mirror_id;
    int motor = command.which_motor;
    int newState = command.new_pos;
    int currentState = -1;
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

    step_motor(mirror, motor, direction, delta);
    motorStates[mirror - 1].motor[motor] = newState;
}

/* Reset/'zero' the orientation of all mirrors in this row.
 */
void CatoptricRow::reset() {
	for(int i = 0; i < numMirrors; ++i) {
		step_motor(i+1, 1, 0, 200);
		step_motor(i+1, 0, 0, 200);
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

