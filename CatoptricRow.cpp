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
MotorState::MotorState(int pan_in, int tilt_in) {
    motor[PAN_IND] = pan_in;
    motor[TILT_IND] = tilt_in;
}

Message::Message(int row_in, int mirror_in, int motor_in, int dir_in, 
        int chigh_in, int clow_in): magic_num(MSG_MAGIC_NUM), ack_key(ACK_KEY) {
    row_num = row_in;
    mirror_id = mirror_in;
    which_motor = motor_in;
    direction = dir_in;
    count_high = chigh_in;
    count_low = clow_in;
}

/* Convert the message into a byte stream that can be serialized and sent to
 * the Arduino.
 */
vector<char> Message::to_vec() {
    vector<char> vec;
    string str;
    int i;

    try {
        str = to_string(magic_num) + to_string(ack_key) + to_string(row_num) + 
            to_string(mirror_id) + to_string(which_motor) + 
            to_string(direction) + to_string(count_high) + to_string(count_low);
    } catch (...) {
        printf("to_string error: %s\n", strerror(errno));
        return vec;
    }

    for(i = 0; i < str.length(); ++i) vec.push_back(str[i]);

    return vec;
}

CatoptricRow::CatoptricRow() {}

CatoptricRow::CatoptricRow(int rowNumber_in, int numMirrors_in, 
        const char *serial_port_in) {
	rowNumber = rowNumber_in;
	numMirrors = numMirrors_in;

	// Init Motor States
	for(int i = 0; i < numMirrors; ++i) {
        MotorState state = MotorState();
	    motor_states.push_back(state);
    }

	// Setup Serial
	_setup(serial_port_in);

    string row_str = to_string(rowNumber);
    const char *row_cstr = row_str.c_str();
	fsm = SerialFSM(row_cstr);
}

int CatoptricRow::_setup(const char *serial_port_in) {
    serial_fd = prep_serial(serial_port_in); // Returns open fd for serial port
    if(tcflush(serial_fd, TCIOFLUSH) < 0) {
        printf("tcflush error: %s\n", strerror(errno));
        return ERR_TCFLUSH;
    }
    sleep(2); // Why does this function sleep?

    return RET_SUCCESS;
}

int CatoptricRow::resetSerialBuffer() {
    if(tcflush(serial_fd, TCIOFLUSH) < 0) {
        printf("tcflush error: %s\n", strerror(errno));
        return ERR_TCFLUSH;
    }

    return RET_SUCCESS;
}

void CatoptricRow::update() {
    char input;
    while(read(serial_fd, &input, 1) > 0) {
        fsm.Execute(input);
        if(fsm.messageReady) {
            printf("Incoming message:%s\n", fsm.message);
            fsm.clearMessage();
        }
    }

	if (getCurrentCommandsOut() < MAX_CMDS_OUT && 
            commandQueue.size() > 0) { // If num pending commands is > 0 and < max limit
		Message message = commandQueue.back();
        commandQueue.pop_back();
		sendMessageToArduino(message);
    }
}

// Is the last param displacement or final position?
void CatoptricRow::step_motor(int mirror_id, int which_motor, int direction, float delta_pos) {
	int delta_pos_int = ((int) delta_pos) * (513.0/360.0);
	int countLow = ((int) delta_pos_int) & 255;
	int countHigh = (((int) delta_pos_int) >> 8) & 255;
    
	Message message (rowNumber, mirror_id, which_motor, direction, 
            countHigh, countLow);
	commandQueue.push_back(message);
}

void CatoptricRow::sendMessageToArduino(Message message) {
    vector<char> message_vec = message.to_vec();
    char b_current;

	for(int i = 0; i < NUM_MSG_ELEMS; ++i) {
		b_current = message_vec[i];
        if(write(serial_fd, &b_current, 1) < 0) {
            printf("write error: %s\n", strerror(errno));
            return;
        }
    }

	fsm.currentCommandsToArduino += 1;
}

int CatoptricRow::getCurrentCommandsOut() {
	return fsm.currentCommandsToArduino;
}

int CatoptricRow::getCurrentNackCount() {
	return fsm.nackCount;
}

int CatoptricRow::getCurrentAckCount() {
	return fsm.ackCount;
}

void CatoptricRow::reorientMirrorAxis(Message command) {
    int mirror = command.mirror_id;
    int motor = command.which_motor;
    int newState = command.new_pos;
    int currentState = -1;
    if(motor == PAN_IND) {
        currentState = motor_states[mirror - 1].motor[PAN_IND];
    } else if(motor == TILT_IND) {
        currentState = motor_states[mirror - 1].motor[TILT_IND];
    } else {
        printf("Invalid 'motor' value in reorientMirrorAxis\n");
        return;
    }

    int delta = newState - currentState;
    int direction = delta <  0 ? 1 : 0;
    if(delta < 0) delta *= -1;

    step_motor(mirror, motor, direction, delta);
    motor_states[mirror - 1].motor[motor] = newState;
}

void CatoptricRow::reset() {
	for(int i = 0; i < numMirrors; ++i) {
		step_motor(i+1, 1, 0, 200);
		step_motor(i+1, 0, 0, 200);
		motor_states[i].motor[PAN_IND] = 0;
		motor_states[i].motor[TILT_IND] = 0;
    }
}

int CatoptricRow::getRowNumber() {
    return rowNumber;
}
