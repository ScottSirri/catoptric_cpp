#include "SerialFSM.cpp"
#include "prep_serial.cpp"
#include <sys/ioctl.h>
#include <string>
#include <vector>
#include <unistd.h>

#define MAX_CMDS_OUT 2
#define FLUSH_IN_OUT 2
#define NUM_MSG_ELEMS 8
#define NUM_MOTORS 2

#define MSG_MAGIC_NUM '!'
#define ACK_KEY 'A'

using namespace std;

typedef int MotorState[NUM_MOTORS];

struct Message {
    const int magic_num = MSG_MAGIC_NUM, ack_key = ACK_KEY;
    int row_num;
    int mirror_id, which_motor, direction;
    int count_high, count_low;

    Message(int row_in, int mirror_in, int motor_in, int dir_in, int chigh_in, int clow_in) {
        row_num = row_in;
        mirror_id = mirror_in;
        direction = dir_in;
        count_high = chigh_in;
        count_low = clow_in;
    }
};

class CatoptricRow {

    int serial_fd;
    int rowNumber, numMirrors;
    vector<MotorState> motorStates;
	vector<Message> commandQueue;
    SerialFSM fsm;

    CatoptricRow(int rowNumber_in, int numMirrors_in, char *serialPort_in) {
		rowNumber = rowNumber_in;
		numMirrors = numMirrors_in;

		// Init Motor States
		for(int i = 0; i < numMirrors; ++i) {
            int arr[] = { 0, 0 };
		    motorStates.push_back( arr );
        }

		// Setup Serial
		_setup(serialPort_in);
    }

	void _setup(char *serialPort_in) {
        serial_fd = prep_serial(serialPort_in); // Returns open fd for serial port
        ioctl(serial_fd, TCFLSH, FLUSH_IN_OUT); // Flushes the input and output buffers
        sleep(2); // Why does this function sleep?
        const char *row_str = to_string(rowNumber).c_str();
		fsm = SerialFSM(const_cast<char*>(row_str));
    }

	void resetSerialBuffer() {
        ioctl(serial_fd, TCFLSH, FLUSH_IN_OUT);
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
	void stepMotor(int mirror_id, int which_motor, int direction, float delta_pos) {
		int delta_pos_int = ((int) delta_pos) * (513.0/360.0);
		int countLow = ((int) delta_pos_int) & 255;
		int countHigh = (((int) delta_pos_int) >> 8) & 255;
	    
		Message message (rowNumber, mirror_id, which_motor, direction, countHigh, countLow);
		commandQueue.push_back(message);
    }

	void sendMessageToArduino(Message message) {
		for(int i = 0; i < NUM_MSG_ELEMS; ++i) {
			char bCurrent = message[i];
            write(serial_fd, &bCurrent, 1);
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
		int currentState = motorStates[mirror-1][motor];
		
		int delta = newState - currentState;
		int direction = delta < 0 ? 1 : 0;

		stepMotor(mirror, motor, direction, abs(delta));
		motorStates[mirror-1][motor] = newState;
    }*/

	void reset() {
		for(int i = 0; i < numMirrors; ++i) {
			stepMotor(i+1, 1, 0, 200);
			stepMotor(i+1, 0, 0, 200);
			motorStates[i][0] = 0;
			motorStates[i][1] = 0;
        }
    }
};
