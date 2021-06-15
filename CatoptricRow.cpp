#include "SerialFSM.cpp"
#include "prep_serial.cpp"
#include <sys/ioctl.h>
#include <unistd.h>

#define MAX_CMDS_OUT 2
#define FLUSH_IN_OUT 2

struct MotorState { int pan, tilt; } // I don't know if this the the right order

struct Message {
    int a = 33, b = 65;
    int rowNumber;
    int y, m, d;
    int countHigh, countLow;
}

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
		    motorStates[i].push_back(MotorState(0, 0));
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
                commandQueue.qsize() > 0) { // If num pending commands is > 0 and < max limit
			Message message = commandQueue.pop();
			sendMessageToArduino(message);
        }
    }

    // What are any of these parameters?
	void stepMotor(int y, int m, int d, float c_float) { // Is the last param a float?
		int c_int = (int) (c_float * (513.0/360.0));
		int countLow = ((int) c_float) & 255;
		int countHigh = (((int) c_float) >> 8) & 255;
	    
		Message message(33, 65, rowNumber, y, m, d, countHigh, countLow);
		commandQueue.push_back(message);
    }

	void sendMessageToArduino(Message message) {
		for i in range(0, len(message)):
			bCurrent = bytes( [int(message[i])] )
			self.serial.write(bCurrent)
		self.fsm.currentCommandsToArduino += 1
	}

	void getCurrentCommandsOut() {
		return self.fsm.currentCommandsToArduino
    }

	void getCurrentNackCount() {
		return self.fsm.nackCount
    }

	void getCurrentAckCount() {
		return self.fsm.ackCount
    }

	void reorientMirrorAxis(command) {
		mirror = int(command[1])
		motor = int(command[2])
		newState = int(command[3])
		currentState = self.motorStates[mirror-1][motor]
		
		delta = newState - currentState
		direction = 0
		if (delta < 0):
			direction = 1

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
