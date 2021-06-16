#include "SerialFSM.hpp"
#include "prep_serial.hpp"
#include <sys/ioctl.h>
#include <string>
#include <termios.h>
#include <vector>
#include <unistd.h>

#define MAX_CMDS_OUT 2
#define FLUSH_IN_OUT 2
#define NUM_MSG_ELEMS 8
#define NUM_MOTORS 2

#define MSG_MAGIC_NUM '!'
#define ACK_KEY 'A'

#define RET_SUCCESS 0
#define ERR_TCFLUSH -2

using namespace std;

struct MotorState {
    int pan, tilt;

    MotorState(): pan(0), tilt(0) {}
    MotorState(int pan_in, int tilt_in): pan(pan_in), tilt(tilt_in) {}
};

struct Message {
    const int magic_num, ack_key;
    int row_num;
    int mirror_id, which_motor, direction;
    int count_high, count_low;

    Message(int row_in, int mirror_in, int motor_in, int dir_in, int chigh_in, 
            int clow_in): magic_num(MSG_MAGIC_NUM), ack_key(ACK_KEY) {
        row_num = row_in;
        mirror_id = mirror_in;
        direction = dir_in;
        count_high = chigh_in;
        count_low = clow_in;
    }

    vector<char> to_vec() {
        vector<char> vec;
        string str;
        int i;

        str = to_string(magic_num) + to_string(ack_key) + to_string(row_num) + 
            to_string(mirror_id) + to_string(which_motor) + to_string(direction) + 
            to_string(count_high) + to_string(count_low);
        for(i = 0; i < str.length(); ++i) vec.push_back(str[i]);

        return vec;
    }
		//Message message (rowNumber, mirror_id, which_motor, direction, countHigh, countLow);
};

class CatoptricRow {

    int serial_fd;
    int rowNumber, numMirrors;
    vector<MotorState> motor_states;
	vector<Message> commandQueue;
    SerialFSM fsm;

    CatoptricRow(int rowNumber_in, int numMirrors_in, char *serial_port_in) {
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
		fsm = SerialFSM(const_cast<char*>(row_cstr));
    }

	int _setup(char *serial_port_in) {
        serial_fd = prep_serial(serial_port_in); // Returns open fd for serial port
        if(tcflush(serial_fd, TCIOFLUSH) < 0) {
            printf("tcflush error: %s\n", strerror(errno));
            return ERR_TCFLUSH;
        }
        sleep(2); // Why does this function sleep?

        return RET_SUCCESS;
    }

	int resetSerialBuffer() {
        if(tcflush(serial_fd, TCIOFLUSH) < 0) {
            printf("tcflush error: %s\n", strerror(errno));
            return ERR_TCFLUSH;
        }

        return RET_SUCCESS;
    }
	
	void update() {
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
	void step_motor(int mirror_id, int which_motor, int direction, float delta_pos) {
		int delta_pos_int = ((int) delta_pos) * (513.0/360.0);
		int countLow = ((int) delta_pos_int) & 255;
		int countHigh = (((int) delta_pos_int) >> 8) & 255;
	    
		Message message (rowNumber, mirror_id, which_motor, direction, countHigh, countLow);
		commandQueue.push_back(message);
    }

	void sendMessageToArduino(Message message) {
        vector<char> message_vec = message.to_vec();
        char b_current;

		for(int i = 0; i < NUM_MSG_ELEMS; ++i) {
			b_current = message_vec[i];
            write(serial_fd, &b_current, 1);
        }

		fsm.currentCommandsToArduino += 1;
	}

	int getCurrentCommandsOut() {
		return fsm.currentCommandsToArduino;
    }

	int getCurrentNackCount() {
		return fsm.nackCount;
    }

	int getCurrentAckCount() {
		return fsm.ackCount;
    }

    // What is 'command'? What data type is it supposed to be?
	/*void reorientMirrorAxis(Message command) {
		int mirror = (int) command[1];
		int motor = (int) command[2];
		int newState = (int) command[3];
		int currentState = motor_states[mirror-1][motor];
		
		int delta = newState - currentState;
		int direction = delta < 0 ? 1 : 0;

		step_motor(mirror, motor, direction, abs(delta));
		motor_states[mirror-1][motor] = newState;
    }*/

	void reset() {
		for(int i = 0; i < numMirrors; ++i) {
			step_motor(i+1, 1, 0, 200);
			step_motor(i+1, 0, 0, 200);
			motor_states[i].pan = 0;
			motor_states[i].tilt = 0;
        }
    }
};
