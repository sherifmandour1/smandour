#ifndef TCP_CLIENT_H_
#define TCP_CLIENT_H_

#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define TCP_CLIENT_BAD_SOCKET -1
#define TCP_CLIENT_DEFAULT_PORT "8081"
#define TCP_CLIENT_DEFAULT_HOST "localhost"

// Contains all of the information needed to create to connect to the server and send it a message.
typedef struct Config {
    char *port;
    char *host;
    char *file;
} Config;

// Parses the options given to the program. It will return a Config struct with the necessary
// information filled in. argc and argv are provided by main. If an error occurs in processing the
// arguments and options (such as an invalid option), this function will print the correct message
// and then exit.
Config tcp_client_parse_arguments(int argc, char *argv[]);

////////////////////////////////////////////////////
///////////// SOCKET RELATED FUNCTIONS /////////////
////////////////////////////////////////////////////

// Creates a TCP socket and connects it to the specified host and port. It returns the socket file
// descriptor.
int tcp_client_connect(Config config);

// Using the the action and message provided by the command line, format the data to follow the
// protocol and send it to the server. Return 1 (EXIT_FAILURE) if an error occurs, otherwise return
// 0 (EXIT_SUCCESS).
int tcp_client_send_request(int sockfd, char *action, char *message);

// Receive the response from the server. The caller must provide a function pointer that handles the
// response and returns a true value if all responses have been handled, otherwise it returns a
// false value. After the response is handled by the handle_response function pointer, the response
// data can be safely deleted. The string passed to the function pointer must be null terminated.
// Return 1 (EXIT_FAILURE) if an error occurs, otherwise return 0 (EXIT_SUCCESS).
int tcp_client_receive_response(int sockfd, int (*handle_response)(char *));

// Close the socket when your program is done running.
void tcp_client_close(int sockfd);

////////////////////////////////////////////////////
////////////// FILE RELATED FUNCTIONS //////////////
////////////////////////////////////////////////////

// Given a file name, open that file for reading, returning the file object.
FILE *tcp_client_open_file(char *file_name);

// Gets the next line of a file, filling in action and message. This function should be similar
// design to getline() (https://linux.die.net/man/3/getline). *action and message must be allocated
// by the function and freed by the caller.* When this function is called, action must point to the
// action string and the message must point to the message string.
int tcp_client_get_line(FILE *fd, char **action, char **message);

// Close the file when your program is done with the file.
void tcp_client_close_file(FILE *fd);

#endif
