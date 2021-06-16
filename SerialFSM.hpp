
#define MAX_MSG_LEN 255

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

    SerialFSM(char *name_in);
    SerialFSM();
    void Execute(char c); 
    void resetVariables();
    void clearMessage();

    private:
    char getMagicNum(char c);
    char getKey(char c); 
    char getAckKey(char c);
    char getAckX(char c);
    char getAckM(char c);
    char getNumCharHigh(char c);
    char getNumCharLow(char c);
    char getChar(char c);
    char getNackKey(char c);
};
