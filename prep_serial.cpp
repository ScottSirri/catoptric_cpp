
/* Must first execute 'sudo adduser $USER dialout' on Pi
 */

#include "prep_serial.hpp"

/* Takes a string containing the serial port to be configured.
 * Creates a termios struct, configures it for output, 
 * and returns the file descriptor.
 */
int prep_serial(const char *port_str) {

    int serial_port = open(port_str, O_RDWR);
    if(serial_port < 0) {
        printf("Error opening serial port %s: %s\n", port_str, strerror(errno));
    }

    struct termios tty;
    // Retrieve current termios configs
    if(tcgetattr(serial_port, &tty) != 0) {
        printf("tcgetattr error on serial port %s: %s\n", 
                port_str, strerror(errno));
    }

    // Do not generate/detect parity bit
    tty.c_cflag &= ~PARENB;
    // Use only one stop bit
    tty.c_cflag &= ~CSTOPB;
    // 8 data bits per byte
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    // Disable hardware RTS/CTS flow control
    tty.c_cflag &= ~CRTSCTS;
    // Disable modem-specific signal lines and SIGHUP signals on disconnect
    tty.c_cflag |= CREAD | CLOCAL;

    // Disable canonical mode, where lines are delimited by newline chars
    tty.c_lflag &= ~ICANON;
    // Disable echo, erasure, and newline echo just in case
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ECHONL; 
    // Disable interpretation of INTR, QUIT, and SUSP characters
    tty.c_lflag &= ~ISIG;

    // Disable software flow control
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    // Disable any special handling of bytes upon reception by serial port
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);

    // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~OPOST; 
    // Prevent conversion of newline to carriage return/line feed
    tty.c_oflag &= ~ONLCR; 
    
    // Wait for up to 10 deciseconds, return as soon as any data is received
    tty.c_cc[VTIME] = 10;
    tty.c_cc[VMIN] = 0;

    // Set in/out baud rate to be 9600
    cfsetispeed(&tty, B9600);
    cfsetospeed(&tty, B9600);

    // Apply termios struct changes
    if(tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        printf("tcsetattr error on serial port %s: %s\n", 
                port_str, strerror(errno));
    }

    return serial_port;
}
