#pragma once

#include <vector>
#include "SerialFSM.hpp"

#define MAX_CMDS_OUT 2
#define FLUSH_IN_OUT 2
#define NUM_MSG_ELEMS 8
#define NUM_MOTORS 2

#define MSG_MAGIC_NUM '!'
#define ACK_KEY 'A'

#define RET_SUCCESS 0
#define ERR_TCFLUSH -2
#define ERR_WRITE -3

struct MotorState {
    int pan, tilt;

    MotorState();
    MotorState(int pan_in, int tilt_in);
};

struct Message {
    const int magic_num, ack_key;
    int row_num;
    int mirror_id, which_motor, direction;
    int count_high, count_low;

    Message(int row_in, int mirror_in, int motor_in, int dir_in, int chigh_in, 
            int clow_in);

    std::vector<char> to_vec(); 
};

class CatoptricRow {

    int serial_fd;
    int rowNumber, numMirrors;
    std::vector<MotorState> motor_states;
    std::vector<Message> commandQueue;
    SerialFSM fsm;

    CatoptricRow(int rowNumber_in, int numMirrors_in, char *serial_port_in); 
	int _setup(char *serial_port_in);
	int resetSerialBuffer();
	void update();
	void step_motor(int mirror_id, int which_motor, int direction, float delta_pos); 
	void sendMessageToArduino(Message message);
	int getCurrentCommandsOut(); 
	int getCurrentNackCount();
	int getCurrentAckCount();

    // What is 'command'? What data type is it supposed to be?
	//void reorientMirrorAxis(Message command); 

	void reset();
};