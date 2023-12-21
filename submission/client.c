#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
//______________________________________________________________________________

#define BUFFSIZE 4096
#define MAIN_SERVER_IP_ADDR "127.0.0.1"
#define MAIN_SERV_TCP_PORT 24420
#define CLI_HEAD_SIZE 5 // Including the \0 at the end.
//______________________________________________________________________________

void setting_TCP_connection_client_serv(int * client_sock_fd_ptr,
                                        struct sockaddr_in * cli_sock_info_ptr,
                                        socklen_t * cli_sock_info_size_ptr){

    struct sockaddr_in serv_sock_info;

    serv_sock_info.sin_family = AF_INET;
    // short, network byte order
    serv_sock_info.sin_port = htons(MAIN_SERV_TCP_PORT);

    if(inet_pton(AF_INET, MAIN_SERVER_IP_ADDR,&(serv_sock_info.sin_addr)) != 1){
        printf("Error converting IP address\n");
        exit(1);
    }

    memset(serv_sock_info.sin_zero, 0, sizeof(serv_sock_info.sin_zero));

    int client_sock_fd;

    if((client_sock_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        printf("Error creating socket\n");
        exit(1);
    }

    int result_connect = connect(client_sock_fd,
                                 (struct sockaddr*)&serv_sock_info,
                                 sizeof(serv_sock_info));

    if (result_connect != 0){
        printf("Error connecting to server, errno: %d\n", errno);
        exit(1);
    }

    int rv_getSockName = getsockname(client_sock_fd,
                                     (struct sockaddr *) cli_sock_info_ptr,
                                     cli_sock_info_size_ptr);

    if(rv_getSockName == -1){
        printf("Error getting socket info\n");
        exit(1);
    }

    *client_sock_fd_ptr = client_sock_fd;
}
//______________________________________________________________________________

int send_all(int s, char *buf, int *len){

    int total = 0; // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) {
            break;
        }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
}
//______________________________________________________________________________

void build_message(char* msg, char* data){

    int len_data = strlen(data) + 1; // +1 to include the \0 at the end.

    char header[CLI_HEAD_SIZE];
    sprintf(header, "%d", len_data);

    strncpy(msg, header, CLI_HEAD_SIZE);
    strncpy(msg + CLI_HEAD_SIZE, data, len_data);
}
//______________________________________________________________________________

void send_entire_message(int sock_fd, char* msg){

    int len_msg = CLI_HEAD_SIZE + strlen(msg + CLI_HEAD_SIZE) + 1;

    int rv = send_all(sock_fd, msg, &len_msg);

    if(rv == -1){
        printf("Error sending the message to Main Server.\n");
        exit(1);
    }
}
//______________________________________________________________________________

int recv_all_TCP(int s, char *buf, int len){

    int total = 0; // how many bytes we've received
    int bytesleft = len; // how many we have left to receive
    int n;

    while(total < len) {
        n = recv(s, buf+total, bytesleft, 0);
        if (n == -1) {
            break;
        }
        if(n == 0){
            return 0; // Means that the client has disconnected.
        }
        total += n;
        bytesleft -= n;
    }
    return n==-1?-1:total; // return -1 on failure, or total on success.
}
//______________________________________________________________________________

void get_msg_data_TCP(char * msg_data, int child_serv_sock_fd){

    char incoming_msg[BUFFSIZE];
    memset(incoming_msg, 0, BUFFSIZE);

    int rv_header = recv_all_TCP(child_serv_sock_fd,incoming_msg,CLI_HEAD_SIZE);

    if (rv_header == 0) {
        printf("Client has disconnected. Server is going to exit. Bye!\n");
        exit(0);
    }

    if (rv_header == -1) {
        printf("Error receiving message from client.\n");
        exit(1);
    }

    // Since the first part of the message (header) is the size of the
    // message, we need to convert it to int to know how many bytes has
    // the actual data (no header).

    int size_of_msg_data = atoi(incoming_msg);

    char *ptr_to_data = incoming_msg + CLI_HEAD_SIZE;

    int rv_data = recv_all_TCP(child_serv_sock_fd, ptr_to_data,
                               size_of_msg_data);

    if (rv_data != size_of_msg_data) {
        printf("Error receiving message from client. Abortiinggg\n");
        exit(1);
    }

    memcpy(msg_data, ptr_to_data, size_of_msg_data);
}
//______________________________________________________________________________

void printFinalMsgOfIntersect(char * allTheStud,
                              char * studDontExist,
                              char * msgTotalIntersect){

    char finalMsg[BUFFSIZE];
    memset(finalMsg, 0, BUFFSIZE);

    strcat(finalMsg, "Course intervals ");
    strcat(finalMsg, msgTotalIntersect);
    strcat(finalMsg, " work for ");

    char cpyAllTheStud[BUFFSIZE];
    memset(cpyAllTheStud, 0, BUFFSIZE);
    strcpy(cpyAllTheStud, allTheStud);

    char * delim = " ";
    char * token = strtok(cpyAllTheStud, delim);

    while (token != NULL) {
        if(strstr(studDontExist, token) == NULL){
            strcat(finalMsg, token);
            strcat(finalMsg, ", ");
        }
        token = strtok(NULL, delim);
    }

    finalMsg[strlen(finalMsg) - 2] = '\0';
    printf("%s.\n\n", finalMsg);
}
//______________________________________________________________________________

int main(){

    printf("Client is up and running.\n\n");

    int client_sock_fd;
    struct sockaddr_in client_sock_info;
    socklen_t client_sock_info_size = sizeof(client_sock_info);
    setting_TCP_connection_client_serv(&client_sock_fd, &client_sock_info,
                                       &client_sock_info_size);

    char msg_to_serv[BUFFSIZE]; // This will include the header and the data.
    char user_names[BUFFSIZE]; // This is the data.

    memset(msg_to_serv, 0, BUFFSIZE);
    memset(user_names, 0, BUFFSIZE);

    for(;;) {

        printf("-----Start a new request-----\n");
        printf("Please enter the usernames to check GroupSync availability:\n");

        fgets(user_names, BUFFSIZE, stdin);

        if(strcmp(user_names, "mumbawa77\n") == 0){
            printf("Client is going to exit. Bye!\n");
            exit(0);
        }

        user_names[strlen(user_names) - 1] = '\0';

        build_message(msg_to_serv, user_names);

        send_entire_message(client_sock_fd, msg_to_serv);

        printf("Client finished sending the usernames to Main Server.\n");

        //----------------------------------------------------------------------
        // Receiving the response from the main server of students that don't
        // exist (if any), and printing their name (if any).
        char msgStudDontExist[BUFFSIZE]; // Header is not included here.
        memset(msgStudDontExist, 0, BUFFSIZE);

        get_msg_data_TCP(msgStudDontExist, client_sock_fd);

        if(strlen(msgStudDontExist) > 0){

            printf("Client received the reply from the Main Server using "
                   "TCP over port %d:\n", ntohs(client_sock_info.sin_port));

            printf("%s do not exist.\n", msgStudDontExist);
        }
        //----------------------------------------------------------------------
        // Receiving the response from the main server with the total
        // intersection.

        char msgTotalIntersection[BUFFSIZE]; // Header is not included here.
        memset(msgTotalIntersection, 0, BUFFSIZE);

        get_msg_data_TCP(msgTotalIntersection, client_sock_fd);

        printf("Client received the reply from the Main Server using "
               "TCP over port %d:\n", ntohs(client_sock_info.sin_port));

        if (strlen(msgTotalIntersection) > 0) {

            printFinalMsgOfIntersect(user_names, msgStudDontExist,
                                     msgTotalIntersection);
        } else {
            printf("No course interval intersection found.\n\n");
        }
    }

    close(client_sock_fd);
}
//______________________________________________________________________________
