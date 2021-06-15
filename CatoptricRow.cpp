#include "SerialFSM.cpp"
#include "prep_serial.cpp"
#include <sys/ioctl.h>
#include <unistd.h>

#define MAX_CMDS_OUT 2
#define FLUSH_IN_OUT 2
#define MSG_ELEMS 8
#define NUM_MOTORS 2

typedef int[NUM_MOTORS] MotorState;
typedef int[MSG_ELEMS] Message;

class CatoptricRow {

    int serial_fd;
    int rowNumber, numMirrors;
    vector<MotorState> motorStates;
	vector<Message> commandQueue;
    SerialFSM fsm;

    CatoptricRow(rowNumber_in, numMirrors_in, serialPort_in) {
		rowNumber = rowNumber_in;
		numMirrors = numMirrors_in;

		// Init Motor States
		for(int i = 0; i < numMirrors; ++i) {
		    motorStates[i].push_back( { 0, 0 } );
        }

		// Setup Serial
		self._setup(serialPort_in);
    }

	void _setup(serialPort_in) {
        serial_fd = prep_serial(serialPort_in); // Returns open fd for serial port
        ioctl(serial_fd, TCFLSH, FLUSH_IN_OUT); // Flushes the input and output buffers
        sleep(2); // Why does this function sleep?
		fsm = SerialFSM(rowNumber);
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
			Message message = commandQueue.pop();
			sendMessageToArduino(message);
        }
    }

    // Is the last param displacement or final position?
	void stepMotor(int mirror_id, int which_motor, int direction, float c_float) {
		int c_int = ((int) c_float) * (513.0/360.0);
		int countLow = ((int) c_int) & 255;
		int countHigh = (((int) c_int) >> 8) & 255;
	    
		Message message(33, 65, rowNumber, mirror_id, which_motor, direction, countHigh, countLow);
		commandQueue.push_back(message);
    }

	void sendMessageToArduino(Message message) {
		for(int i = 0; i < MSG_ELEMS; ++i) {
			char bCurrent = message[i];
            write(serial_fd, &bCurrent, 1);
        }

		fsm.currentCommandsToArduino += 1;
	}

	void getCurrentCommandsOut() {
		return fsm.currentCommandsToArduino;
    }

	void getCurrentNackCount() {
		return fsm.nackCount;
    }

	void getCurrentAckCount() {
		return fsm.ackCount;
    }

    // What is 'command'? What data type?
	void reorientMirrorAxis(command) {
		mirror = int(command[1])
		motor = int(command[2])
		newState = int(command[3])
		currentState = motorStates[mirror-1][motor]
		
		delta = newState - currentState
		direction = delta < 0 ? 1 : 0;

		self.stepMotor(mirror, motor, direction, abs(delta))
		self.motorStates[mirror-1][motor] = newState
    }

	void reset() {
		for i in range(self.numMirrors):
			self.stepMotor(i+1, 1, 0, 200)
			self.stepMotor(i+1, 0, 0, 200)
			self.motorStates[i][0] = 0
			self.motorStates[i][1] = 0
    }
}
