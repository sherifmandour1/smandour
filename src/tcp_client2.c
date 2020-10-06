#include "tcp_client2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "log.h"
#define LINE_READ_ERROR -1
#define GET_LINE_ERROR -2
#define ZERO_VALUE 0
#define SPACE_CHAR ' '
#define ARRAY_INIT_NUM 1000
#define SIZEOF_LONGEST_ACTION 10
#define LAB1_BUFFER_SIZE 1024
#define MAX_ACCEPTABLE_BUFFER_CAPACITY_PERCENTAGE 0.7
#define TWO_VALUE 2
#define ONE_VALUE 1
#define NULL_TERMINATOR '\0'
#define READ_FROM_STDIN '-'
#define LENGTH_OF_LONGEST_ACTION 10
#define HANDLE_RESPONSE_ERROR -13

long validRequestCounter;
long totalCharNumber;

void printUsage()
{
    printf("Usage: tcp_client [--help] [-v] [-h HOST] [-p PORT] FILE\n\n");
    printf("Arguments:\n  FILE    A file name containing actions and messages to send to the server. If \"-\"is provided, stdin will be read.\n\n\n");
    printf("Options:\n  --help\n  -v, --v\n  --host HOSTNAME, -h HOSTNAME\n  --port PORT, -p PORT");
}
// Parses the options given to the program. It will return a Config struct with the necessary
// information filled in. argc andsend argv are provided by main. If an error occurs in processing the
// arguments and options (such as an invalid option), this function will print the correct message
// and then exit.
Config tcp_client_parse_arguments(int argc, char *argv[])
{
    Config localConfig;   
    int selectedOption = ZERO_VALUE;
    bool portReceived = false;
    bool hostReceived = false;
    bool verboseFlag = false;
    int optionIndex = ZERO_VALUE;
    char print[LAB1_BUFFER_SIZE] = "";
    log_set_quiet(true);
    struct option long_opts[] =
    {
        {"help", no_argument, ZERO_VALUE, ZERO_VALUE},
        {"verbose", no_argument, ZERO_VALUE, 'v'},
        {"host", required_argument, ZERO_VALUE, 'h'},
        {"port", required_argument, ZERO_VALUE, 'p'},
        {ZERO_VALUE,ZERO_VALUE,ZERO_VALUE,ZERO_VALUE}
    };    
    while((selectedOption = getopt_long(argc, argv, "vh:p:", long_opts, &optionIndex)) != TCP_CLIENT_BAD_SOCKET)
    {
        switch(selectedOption)
        {
            case 'v':
            verboseFlag = true;
            log_set_quiet(false);
            log_trace("About to parse terminal inputs..");
            break;
            case 'h':
            hostReceived = true;
            log_info("Received a host from client and the value is %s", optarg);
            localConfig.host = optarg;
            break;
            case 'p':
            log_info("Received port from the client and the value is %d" ,localConfig.port);
            portReceived = true;
            localConfig.port = optarg;
            sprintf(print, "this is the port %s", optarg);
            break;
            case '?':
            break;
            default:
            log_error("Unkonwn argument provided\n");
            printUsage();
            break;
        }
    }
    argc -= optind;
    argv += optind;
    if(!portReceived)
    {
        localConfig.port = TCP_CLIENT_DEFAULT_PORT;
        log_info("Did not receive port from user, now assigning it to default value");        
    }
    if(!hostReceived)
    {
        localConfig.host = TCP_CLIENT_DEFAULT_HOST;
        log_info("Did not receive host from user, now assigning it to default value");
    }
    if(argc == 1)
    {
        localConfig.file = argv[ZERO_VALUE];
        log_info("Received file name from user and it is %s", localConfig.file);
    }
    else
    {
        printUsage();
        exit(EXIT_SUCCESS);
    }
    return localConfig;
}

////////////////////////////////////////////////////
///////////// SOCKET RELATED FUNCTIONS /////////////
////////////////////////////////////////////////////

// Creates a TCP socket and connects it to the specified host and port. It returns the socket file
// descriptor.
int tcp_client_connect(Config config)
{
    log_trace("Attempting the connection to the sever");
    int serverSocket;
    struct hostent *he;
    struct sockaddr_in serverAddr;
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket == -1)
    {
        log_error("Socket creation was terminated");
        exit(EXIT_SUCCESS);
    }
    else
    {
        log_info("Socket created succesfully");
    }
      bzero(&serverAddr, sizeof(serverAddr));
      if ( (he = gethostbyname(config.host) ) == NULL ) 
      {
        exit(EXIT_SUCCESS);
      }

    /* copy the network address to sockaddr_in structure */
    memcpy(&serverAddr.sin_addr, he->h_addr_list[ZERO_VALUE], he->h_length);
    serverAddr.sin_port = htons(atoi(config.port)); //FIX ME

    
    if(connect(serverSocket, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) != ZERO_VALUE)
    {
        log_trace("Establishing connection was not complete");
        exit(EXIT_SUCCESS);
    }
    else
    {
        log_info("Successfully connected to the server");
    }
    return serverSocket;
}

// Using the the action and message provided by the command line, format the data to follow the
// protocol and send it to the server. Return 1 (EXIT_FAILURE) if an error occurs, otherwise return
// 0 (EXIT_SUCCESS).
int tcp_client_send_request(int sockfd, char *action, char *message)
{
    //message is formated by the standards of lab 1 going ACTION LENGTH MESSAGE
    //message is dynamically allocated and terminated by the null terminator to make it easier to send and recieve
    log_trace("Now about to try to send a message");
    int messageLen = strlen(message);
    unsigned int messageSize = snprintf(NULL, ZERO_VALUE, "%s %d %s", action, messageLen, message);
    char* finalRequest = malloc(sizeof(char) * messageSize);
    snprintf(finalRequest, messageSize + ONE_VALUE, "%s %d %s", action,messageLen, message);
    unsigned int charsSent = ZERO_VALUE;
    while(charsSent < messageSize)
    {
        int sentNow = write(sockfd, finalRequest, messageSize);
        charsSent += sentNow;
        if(sentNow == -1)
        {
            log_error("Sending malfunction");
            return EXIT_FAILURE;
        }
        log_info("This round %d characters have been send, out of %d characters total", charsSent, messageSize);
    }
    log_info("The entire message has been successfully sent");
    free(finalRequest);
    return EXIT_SUCCESS;
}

// Receive the response from the server. The caller must provide a function pointer that handles the
// response and returns a true value if all responses have been handled, otherwise it returns a
// false value. After the response is handled by the handle_response function pointer, the response
// data can be safely deleted. The string passed to the function pointer must be null terminated.
// Return 1 (EXIT_FAILURE) if an error occurs, otherwise return 0 (EXIT_SUCCESS).
int tcp_client_receive_response(int sockfd, int (*handle_response)(char *))
{
    //placehold variables for calculating and keeping track of what was sent and what is till yet to be
    log_trace("Now receiving responses from the server");
    int startingBufferSize = LAB1_BUFFER_SIZE;
    char* dynamicBuffer = malloc(sizeof(char) * startingBufferSize);
    char* messageReceived = NULL;
    char* foundSpace = NULL;
    int receievedAll = ZERO_VALUE;
    int responseStatus = ZERO_VALUE;
    //infinate loop that could only be broken if the response handler was not recieving anything
    //the loop is cheking to see if the messages received by the handler matches the length that was supposed to be receievd and keeps on going
    while(1)
    {
        if(receievedAll >= (startingBufferSize * MAX_ACCEPTABLE_BUFFER_CAPACITY_PERCENTAGE))
        {
            startingBufferSize *= TWO_VALUE;
            dynamicBuffer = realloc(dynamicBuffer, sizeof(char) * startingBufferSize);
            if(dynamicBuffer == NULL)
            {
                free(dynamicBuffer);
                return EXIT_FAILURE;
            }
        }
        int receivedChars = read(sockfd, (dynamicBuffer + receievedAll), (startingBufferSize - receievedAll - 1));
        if(receivedChars == -1)
        {
            return EXIT_FAILURE;
        }
        receievedAll += receivedChars;

        dynamicBuffer[receievedAll] = NULL_TERMINATOR;

        foundSpace = strchr(dynamicBuffer, SPACE_CHAR);

        if(foundSpace == NULL)
        {
            continue;
        }
        foundSpace += 1;

        while(foundSpace != NULL)
        {
            unsigned int originalLMessageLength = atoi(dynamicBuffer);

            unsigned int receivedMessageLength = strlen(foundSpace);

            if(receivedMessageLength < originalLMessageLength)
            {
                break;
            }
            //after receiving from the server allocate the memory for what was recieved
            messageReceived = malloc(sizeof(char) * (originalLMessageLength + ONE_VALUE));
            memcpy(messageReceived, foundSpace, originalLMessageLength);
            messageReceived[originalLMessageLength] = NULL_TERMINATOR;

            responseStatus = handle_response(messageReceived);
            free(messageReceived);
            if(responseStatus)
            {
                free(dynamicBuffer);
                return EXIT_SUCCESS;
                
            }
            else
            {
                receievedAll = strlen(originalLMessageLength + foundSpace);
            }
            memmove(dynamicBuffer, foundSpace + originalLMessageLength, receievedAll);
            dynamicBuffer[receievedAll] = NULL_TERMINATOR;

            foundSpace = strchr(dynamicBuffer, SPACE_CHAR);
            if(foundSpace != NULL)
            {
                foundSpace++;
            }
            
        }
    }
}
int handle_response(char* response)
{
    printf("%s\n", response);
    validRequestCounter--;
    totalCharNumber -=(strlen(response));
    if(validRequestCounter < ZERO_VALUE || totalCharNumber < ZERO_VALUE)
    {
        return HANDLE_RESPONSE_ERROR;
    }
    if(validRequestCounter == ZERO_VALUE && totalCharNumber == ZERO_VALUE)
    {
        return ONE_VALUE;
    }
    return ZERO_VALUE;
}
int main(int argc, char* argv[])
{
    Config myConfig;
    FILE *myFile;
    char* mainAction;
    char* mainMessage;
    int sockfd;
    log_trace("Calling parse arguments");
    //this function parses the arguments that have been passed in to the terminal or the progeam arguments
    //and then fills out the struct Config fields to be later used in other functions
    //function also accepts options and filters them
    myConfig = tcp_client_parse_arguments(argc, argv);
    log_trace("Config is now loaded with needed informaation");
    sockfd = tcp_client_connect(myConfig);
    log_trace("Socket is now connected");
    myFile = tcp_client_open_file(myConfig.file);//myOpenFile(myConfig.file, openFile);
    log_trace("The provided file should be handeled");
    while(tcp_client_get_line(myFile, &mainAction, &mainMessage) != GET_LINE_ERROR)
    {
        if(strcmp(mainAction, "invalid") != ZERO_VALUE )
        {
            validRequestCounter++;
            totalCharNumber += (strlen(mainMessage));
            if(tcp_client_send_request(sockfd, mainAction, mainMessage) == EXIT_FAILURE)
            {
                validRequestCounter = ZERO_VALUE;
                totalCharNumber = ZERO_VALUE;
                exit(EXIT_FAILURE);
            }
            free(mainAction);
            free(mainMessage);
        }
    }
    log_info("Successfully got all the lines of the file and send the requests in the valid format");
    tcp_client_close_file(myFile);
    log_trace("File has been closed");
    if(validRequestCounter)
    {
        tcp_client_receive_response(sockfd, (*handle_response));
    }
    log_trace("All valid responses have been receieved and processed");
    tcp_client_close(sockfd);
    log_trace("Done all the lords work and not exiting");
    return 0;
}




// Close the socket when your program is done running.
void tcp_client_close(int sockfd)
{
    log_trace("Closing the client socket");
    close(sockfd);
    log_info("Client socket successfully closed");
}

////////////////////////////////////////////////////
////////////// FILE RELATED FUNCTIONS //////////////
////////////////////////////////////////////////////

// Given a file name, open that file for reading, returning the file object.
FILE *tcp_client_open_file(char *file_name)
{
    log_trace("About to receieve or create the input file");
    FILE* tempPtr;
    if(strlen(file_name) < TWO_VALUE)
    {
        log_trace("Reading from the command line");
        tempPtr = stdin;
        
    }
    else
    {
        log_trace("Trying to open the file provided");
        tempPtr = fopen(file_name, "r");
        if (tempPtr == NULL) 
        { 
            log_error("Cannot open file \n"); 
            exit(EXIT_SUCCESS); 
        } 
        else
        {
            log_trace("file openned fine\n");
        }
    }
        
    return tempPtr;
}

// Gets the next line of a file, filling in action and message. This function should be similar
// design to getline() (https://linux.die.net/man/3/getline). *action and message must be allocated
// by the function and freed by the caller.* When this function is called, action must point to the
// action string and the message must point to the message string.
int tcp_client_get_line(FILE *fd, char **action, char **message)
{
    //placehold vairables for calculating the lenghts
    log_trace("Attempting to get new line");
    char* lineStroage = NULL;
    unsigned int bufferSize = 0;
    unsigned int lineRead = 0;
    char spaceChar = ' ';
    char* spacePtr = NULL;
    lineRead = getline(&lineStroage, &bufferSize, fd);
    lineRead--;
    if(lineRead != (unsigned int)LINE_READ_ERROR)
    {
        lineStroage[strcspn(lineStroage, "\n")] = NULL_TERMINATOR;
        spacePtr = strchr(lineStroage, spaceChar);
            if(spacePtr != ZERO_VALUE)
            {  
                //if the space was found then the request is more likely to be valid
                //calculate the action length and and then allocate memory for the length of both the action and the message
                //end both strings with a null terminator
                int actionLength = spacePtr - lineStroage;
                *action = malloc(sizeof(char) * (LENGTH_OF_LONGEST_ACTION * TWO_VALUE));
                *message = malloc(sizeof(char) * (lineRead - actionLength));
                memmove(*action,&lineStroage[ZERO_VALUE], actionLength);    
                memmove(*message, &lineStroage[actionLength + ONE_VALUE], (lineRead - actionLength)); 
                (*action)[actionLength] = NULL_TERMINATOR;
                (*message)[lineRead - actionLength - 1] = NULL_TERMINATOR;
                if(strcmp(*action, "reverse") == ZERO_VALUE || strcmp(*action, "uppercase") == ZERO_VALUE || strcmp(*action, "lowercase") == ZERO_VALUE || strcmp(*action, "shuffle") == ZERO_VALUE || strcmp(*action, "title-case") == ZERO_VALUE )
                {
                    log_info("Action and message were successully parsed and processed");
                    log_info("Action is valid and accepted");
                }
                else
                {
                    log_error("Action was invalid so skipping the line and not storing it");
                    *action = "invalid";
                    *message = "invalid";
                }
            }
            else
            {
                log_error("Request found does not match the needed format");
                log_error("Request skipped and not stored");
                *action = "invalid";
                *message = "invalid";
            }
    }
    return lineRead;
}

// Close the file when your program is done with the file.
void tcp_client_close_file(FILE *fd)
{
    fclose(fd);
    log_trace("The provided file has been successfully closed");
}
