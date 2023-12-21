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
#define MAIN_SERV_UDP_PORT 23420
#define CE_SERV_UDP_PORT   21420
#define EE_SERV_UDP_PORT   22420
#define BACKLOG 100 // Actually for this project only 1 i needed, but who knows
#define CLI_HEAD_SIZE 5 // Including the \0 at the end.
#define CCEE_HEAD_SIZE 6 // Including the \0 at the end.
#define BUFFSIZE_FOR_NAME 50
#define BUFFSIZE_FOR_COURSE 5 // Actually only need 4 cus 3 digits(for 100)+'\0'
#define MAX_NUM_STUDENTS  1000
#define NUM_OF_DIFF_COURSES 200 // Actually it is (0-100)
//______________________________________________________________________________
typedef char studentName_t[BUFFSIZE_FOR_NAME];
typedef char course_t[BUFFSIZE_FOR_COURSE];
//______________________________________________________________________________

typedef struct listOfStudNames {
    int numOfStud;
    studentName_t theList[MAX_NUM_STUDENTS];
} listOfStudNames_t;
//______________________________________________________________________________

typedef struct intersectRes{

    int numOfCourses;
    char str_repr[BUFFSIZE];
    course_t courses[NUM_OF_DIFF_COURSES];
    char str_repr_aux[BUFFSIZE];

} intersectRes_t;
//______________________________________________________________________________

int udp_serv_sock_fd;
struct sockaddr_in udp_serv_sock_info;
socklen_t udp_serv_sock_info_size;

struct sockaddr_in udp_CE_serv_sock_info;
struct sockaddr_in udp_EE_serv_sock_info;

listOfStudNames_t listOfStudentsCE;
listOfStudNames_t listOfStudentsEE;
//______________________________________________________________________________

void set_UDP_socket(int * udp_sock_fd_ptr,
                    struct sockaddr_in * udp_sock_info_ptr,
                    socklen_t * udp_sock_info_size_ptr){

    struct sockaddr_in udp_sock_info;
    socklen_t udp_sock_info_size = sizeof(udp_sock_info);

    udp_sock_info.sin_family = AF_INET;
    // short, network byte order.
    udp_sock_info.sin_port = htons(MAIN_SERV_UDP_PORT);

    if(inet_pton(AF_INET, MAIN_SERVER_IP_ADDR,
                 &(udp_sock_info.sin_addr)) != 1){

        printf("Error converting IP address\n");
        exit(1);
    }

    memset(udp_sock_info.sin_zero,0,sizeof(udp_sock_info.sin_zero));

    int udp_sock_fd;

    if((udp_sock_fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0){
        printf("Error creating socket\n");
        exit(1);
    }

    if(bind(udp_sock_fd, (struct sockaddr *)&udp_sock_info,
            sizeof(udp_sock_info)) != 0){

        printf("Error binding socket\n");
        exit(1);
    }

    *udp_sock_fd_ptr = udp_sock_fd;
    memcpy(udp_sock_info_ptr, &udp_sock_info, sizeof(udp_sock_info));
    memcpy(udp_sock_info_size_ptr, &udp_sock_info_size,
           sizeof(udp_sock_info_size));
}


//______________________________________________________________________________

void setting_TCP_connect_client_serv(int * listen_serv_sock_fd_ptr,
                                     int * child_serv_sock_fd_ptr,
                                     struct sockaddr_in * client_sock_info_ptr,
                                     socklen_t * client_sock_info_size_ptr) {

    struct sockaddr_in listener_serv_sock_info;
    struct sockaddr_in client_sock_info;
    socklen_t client_sock_info_size = sizeof(client_sock_info);

    listener_serv_sock_info.sin_family = AF_INET;
    // short, network byte order.
    listener_serv_sock_info.sin_port = htons(MAIN_SERV_TCP_PORT);

    if(inet_pton(AF_INET, MAIN_SERVER_IP_ADDR,
                 &(listener_serv_sock_info.sin_addr)) != 1){

        printf("Error converting IP address\n");
        exit(1);
    }

    memset(listener_serv_sock_info.sin_zero,0,
           sizeof(listener_serv_sock_info.sin_zero));

    int listen_serv_sock_fd;

    if((listen_serv_sock_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        printf("Error creating socket\n");
        exit(1);
    }

    //__________________________________________________________________________
    // This is to force the socket to reuse the address immediately, in order
    // to avoid the pesky "Address already in use" error.
    int yes=1;
    if (setsockopt(listen_serv_sock_fd,SOL_SOCKET,SO_REUSEADDR,&yes,
                   sizeof(yes)) == -1) {
        perror("setsockopt");
        exit(1);
    }
    //__________________________________________________________________________


    if(bind(listen_serv_sock_fd, (struct sockaddr *)&listener_serv_sock_info,
            sizeof(listener_serv_sock_info)) != 0){

        printf("Error binding socket\n");
        exit(1);
    }

    if(listen(listen_serv_sock_fd, BACKLOG) != 0){

        printf("Error listening on socket\n");
        exit(1);
    }

    int child_serv_sock_fd = accept(listen_serv_sock_fd,
                                    (struct sockaddr *) &client_sock_info,
                                    &client_sock_info_size);

    if(child_serv_sock_fd < 0){
        printf("Error accepting connection\n");
        exit(1);
    }

    *child_serv_sock_fd_ptr = child_serv_sock_fd;
    *listen_serv_sock_fd_ptr = listen_serv_sock_fd;
    memcpy(client_sock_info_ptr, &client_sock_info, sizeof(client_sock_info));
    memcpy(client_sock_info_size_ptr, &client_sock_info_size,
           sizeof(client_sock_info_size));
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

int get_msg_data_UDP(char * msg_data, int udp_sock_fd,
                     struct sockaddr_in * from_sock_info_ptr,
                     socklen_t * from_sock_info_size_ptr){

    int maxSizeAllowed = CCEE_HEAD_SIZE+3+MAX_NUM_STUDENTS*BUFFSIZE_FOR_NAME;

    char incoming_msg[maxSizeAllowed];

    int rv_recv_udp_msg = recvfrom(udp_sock_fd, incoming_msg, maxSizeAllowed, 0,
                                   (struct sockaddr *)from_sock_info_ptr,
                                   from_sock_info_size_ptr);

    if (rv_recv_udp_msg == 0) {
        printf("Client has disconnected. Server is going to exit. Bye!\n");
        exit(1);
    }

    if (rv_recv_udp_msg == -1) {
        printf("Error receiving message from backend server.\n");
        exit(1);
    }

    // Since the first part of the message (header) is the size of the
    // message, we need to convert it to int to know how many bytes has
    // the actual data (no header).

    int size_of_msg_data = atoi(incoming_msg);

    if(size_of_msg_data != rv_recv_udp_msg - CCEE_HEAD_SIZE){
        printf("Error receiving message from backend server.\n");
        exit(1);
    }

    char *ptr_to_data = incoming_msg + CCEE_HEAD_SIZE;

    memcpy(msg_data, ptr_to_data, size_of_msg_data);

    return size_of_msg_data;
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

void scanClientRequest(char * msg_from_cli, char * stuDoNotExist,
                       char * studFoundInCE, char * studFoundInEE){

    char cpy_msg_from_cli [BUFFSIZE];
    strcpy(cpy_msg_from_cli, msg_from_cli);
    char * delim  = " ";
    char * token  = strtok(cpy_msg_from_cli, delim);
    int numStudDontExist = 0;
    int numStudFoundInCE = 0;
    int numStudFoundInEE = 0;

    while(token != NULL){

        int tokenExists = 0; //Either in CE or EE lists.

        for(int i = 0; i < listOfStudentsCE.numOfStud; i++){

            if(strcmp(listOfStudentsCE.theList[i], token) == 0){

                strcat(studFoundInCE, token);
                strcat(studFoundInCE, ", ");
                tokenExists = 1;
                numStudFoundInCE++;
                break;
            }
        }

        for(int i = 0; i < listOfStudentsEE.numOfStud; i++){

            if(strcmp(listOfStudentsEE.theList[i], token) == 0){

                strcat(studFoundInEE, token);
                strcat(studFoundInEE, ", ");
                tokenExists = 1;
                numStudFoundInEE++;
                break;
            }
        }

        if(!tokenExists){
            numStudDontExist++;
            strcat(stuDoNotExist, token);
            strcat(stuDoNotExist, ", ");
        }

        token = strtok(NULL, delim);
    }

    if(numStudDontExist > 0){
        stuDoNotExist[strlen(stuDoNotExist)-2] = '\0';
    }
    if(numStudFoundInCE > 0){
        studFoundInCE[strlen(studFoundInCE)-2] = '\0';
    }
    if(numStudFoundInEE > 0){
        studFoundInEE[strlen(studFoundInEE)-2] = '\0';
    }
}

//______________________________________________________________________________

void send_stud_toCEorEE(char * studFoundInCEorEE,
                        struct sockaddr_in * udp_CEorEE_serv_sock_info_ptr){

    char msg[CCEE_HEAD_SIZE + MAX_NUM_STUDENTS * BUFFSIZE];
    int len_msgData = strlen(studFoundInCEorEE) + 1;//+1 to include the \0
    sprintf(msg, "%d", len_msgData);
    memcpy(msg + CCEE_HEAD_SIZE, studFoundInCEorEE, len_msgData);

    int totalNumbOfBytesToSend = CCEE_HEAD_SIZE + len_msgData;

    int totalNumbOfBytesSent = sendto(udp_serv_sock_fd, msg,
                                      totalNumbOfBytesToSend,0,
                                      (struct sockaddr *)
                                      udp_CEorEE_serv_sock_info_ptr,
                                      sizeof(*udp_CEorEE_serv_sock_info_ptr));

    if(totalNumbOfBytesSent != totalNumbOfBytesToSend){
        printf("Error sending the message to backend server.\n");
        exit(1);
    }
}
//______________________________________________________________________________

void getStrReprOfIntersec(intersectRes_t * intersectRes){

    strcpy(intersectRes->str_repr,"[");

    course_t * courses = intersectRes->courses;
    // The line below is only for debugging purposes.
//    char * the_repr_aux    = intersectRes->str_repr_aux;

    int posNextLeft = 0;
    course_t left;
    course_t right;

    for( ; posNextLeft < intersectRes->numOfCourses; posNextLeft++){

        strcpy(left , courses[posNextLeft]);
        strcpy(right, courses[posNextLeft]);

        int leftNum  = atoi(left);
        int prevLeft = leftNum;

        for(int j = posNextLeft+1; j < intersectRes->numOfCourses; j++){

            int rightNum = atoi(courses[j]);

            if(prevLeft == rightNum-1){
                posNextLeft++;
                sprintf(right, "%d", rightNum);
                prevLeft++;
            }
            else{
                break;
            }
        }
//        if(strcmp(left, right) == 0){
//            strcat(intersectRes->str_repr, "[");
//            strcat(intersectRes->str_repr, left);
//            strcat(intersectRes->str_repr, "],");
//        }
//        else{
        strcat(intersectRes->str_repr, "[");
        strcat(intersectRes->str_repr, left);
        strcat(intersectRes->str_repr, ",");
        strcat(intersectRes->str_repr, right);
        strcat(intersectRes->str_repr, "],");
//        }
//        printf("[%s,%s] , ", left, right);
    }
    intersectRes->str_repr[strlen(intersectRes->str_repr)-1] = ']';
//    char * just_for_debugging = intersectRes->str_repr;
}
//______________________________________________________________________________

void compute_intersect(course_t * * setsOfCourses, int numOfSets,
                       intersectRes_t * resultIntersect){

    if(numOfSets == 0){
        return;
    }

    for(int i = 0; *(setsOfCourses[0][i]) != 0; i++){

        int isCommon = 1;

        for(int j = 1; j < numOfSets; j++){

            int k = 0;

            for(; *(setsOfCourses[j][k]) != 0; k++){

                if(strcmp(setsOfCourses[0][i], setsOfCourses[j][k]) == 0){
                    break;
                }
            }
            if(*(setsOfCourses[j][k]) == 0){
                isCommon = 0;
                break;
            }
        }
        if(isCommon == 1){
            strcpy(resultIntersect->courses[resultIntersect->numOfCourses],
                   setsOfCourses[0][i]);
            resultIntersect->numOfCourses++;
        }
    }

    // This for loop is not necessary although I use it for debugging purposes
    // of getStrReprOfIntersec function.
    for(int i = 0; i < resultIntersect->numOfCourses; i++){

        strcat(resultIntersect->str_repr_aux, resultIntersect->courses[i]);
        strcat(resultIntersect->str_repr_aux, "|");
    }

    if(resultIntersect->numOfCourses != 0){
        getStrReprOfIntersec(resultIntersect);
    }
}
//______________________________________________________________________________

void talk_to_client(int child_serv_sock_fd){

    for(;;){

        char msg_from_cl[BUFFSIZE]; // Header is NOT included here.
        memset(msg_from_cl, 0, BUFFSIZE);

        get_msg_data_TCP(msg_from_cl, child_serv_sock_fd);

        printf("Main Server received the request from client using "
               "TCP over port %d\n", MAIN_SERV_TCP_PORT);

        //----------------------------------------------------------------------
        // printf("Message received from client: \"%s\"\n", msg_from_cl);
        //----------------------------------------------------------------------

        char stuDoNotExist[BUFFSIZE];
        char studFoundInCE[BUFFSIZE];
        char studFoundInEE[BUFFSIZE];

        memset(stuDoNotExist, '\0', BUFFSIZE);
        memset(studFoundInCE, '\0', BUFFSIZE);
        memset(studFoundInEE, '\0', BUFFSIZE);

        scanClientRequest(msg_from_cl, stuDoNotExist, studFoundInCE,
                          studFoundInEE);

        //______________________________________________________________________
        //----------Sending student that don't exist to the client--------------

        if(strlen(stuDoNotExist) > 0){
            printf("%s do not exist. Send a reply to the client.\n",
                   stuDoNotExist);
        }

        char msg_to_cli[BUFFSIZE];
        memset(msg_to_cli, 0, BUFFSIZE);
        build_message(msg_to_cli, stuDoNotExist);
        send_entire_message(child_serv_sock_fd, msg_to_cli);

        //______________________________________________________________________
        //----Sending student to CE server for it to compute the intersection---

        if(strlen(studFoundInCE) > 0){
            printf("Found %s located at Server CE. Send to Server CE.\n",
                   studFoundInCE);
        }

        send_stud_toCEorEE(studFoundInCE, &udp_CE_serv_sock_info);

        //______________________________________________________________________
        //----Sending student to EE server for it to compute the intersection---
        if(strlen(studFoundInEE) > 0){
            printf("Found %s located at Server EE. Send to Server EE.\n",
                   studFoundInEE);
        }

        send_stud_toCEorEE(studFoundInEE, &udp_EE_serv_sock_info);

        //______________________________________________________________________
        //----Receiving the intersection from CE and EE servers-----------------

        // Header is NOT included here.
        char msgInterc_CEorEE[3 + sizeof(intersectRes_t)];
        intersectRes_t interRes_CE;
        intersectRes_t interRes_EE;

        memset(msgInterc_CEorEE, '\0', sizeof(msgInterc_CEorEE));
        memset(&interRes_CE, '\0', sizeof(intersectRes_t));
        memset(&interRes_EE, '\0', sizeof(intersectRes_t));

        for(int i = 0; i < 2; i++){

            get_msg_data_UDP(msgInterc_CEorEE, udp_serv_sock_fd, NULL,NULL);

            if(strcmp(msgInterc_CEorEE, "CE") == 0){

                memcpy(&interRes_CE, msgInterc_CEorEE + 3,
                       sizeof(intersectRes_t));

                if(studFoundInCE[0] != '\0'){
                    printf("Main Server received from server CE the "
                           "intersection result using UDP over"
                           " port %d:\n",MAIN_SERV_UDP_PORT);

                    if(interRes_CE.numOfCourses > 0){
                        printf("%s.\n", interRes_CE.str_repr);
                    } else{
                        printf("No intersection found.\n");
                    }
                }
            }
            else if(strcmp(msgInterc_CEorEE, "EE") == 0){

                memcpy(&interRes_EE, msgInterc_CEorEE + 3,
                       sizeof(intersectRes_t));

                if(studFoundInEE[0] != '\0'){
                    printf("Main Server received from server EE the "
                           "intersection result using UDP over"
                           " port %d:\n",MAIN_SERV_UDP_PORT);

                    if(interRes_EE.numOfCourses > 0){
                        printf("%s.\n", interRes_EE.str_repr);
                    } else{
                        printf("No intersection found.\n");
                    }
                }
            }
            else{
                printf("Error: The message received from the backend server "
                       "is not valid.\n");
                exit(1);
            }
        }
        //______________________________________________________________________
        //____Sending final intersection to cleint______________________________

        char msgToCliFinalIntersect[BUFFSIZE];
        intersectRes_t finalIntersect;

        memset(msgToCliFinalIntersect, 0, BUFFSIZE);
        memset(&finalIntersect, 0, sizeof(intersectRes_t));

        if(studFoundInCE[0] != '\0' && studFoundInEE[0] == '\0'){
            memcpy(&finalIntersect, &interRes_CE, sizeof(intersectRes_t));
        }
        else if(studFoundInCE[0] == '\0' && studFoundInEE[0] != '\0'){
            memcpy(&finalIntersect, &interRes_EE, sizeof(intersectRes_t));
        }
        else{
            course_t * setsOfCourses[2] = {interRes_CE.courses,
                                           interRes_EE.courses};

            compute_intersect(setsOfCourses, 2, &finalIntersect);
        }

        if(finalIntersect.numOfCourses > 0){
            printf("Found the intersection between the results from server"
                   " CE and EE:\n%s\n", finalIntersect.str_repr);
        } else{
            printf("No intersection found between CE and EE.\n");
        }

        build_message(msgToCliFinalIntersect, finalIntersect.str_repr);
        send_entire_message(child_serv_sock_fd, msgToCliFinalIntersect);

        printf("Main Server sent the result to the client.\n");
        printf("----------------------------------------------------------\n");
    }
}
//______________________________________________________________________________

int main(){

    printf("Main server is up and running.\n");

    memset(&listOfStudentsCE, '\0', sizeof(listOfStudNames_t));
    memset(&listOfStudentsEE, '\0', sizeof(listOfStudNames_t));

    //____First communication between backend servers and the main server
    //____Backend server tell to the main server their student lists.

    set_UDP_socket(&udp_serv_sock_fd, &udp_serv_sock_info,
                   &udp_serv_sock_info_size);

    struct sockaddr_in udp_CEorEE_serv_sock_info;
    socklen_t udp_CEorEE_serv_sock_info_size =sizeof(udp_CEorEE_serv_sock_info);

    // Header is NOT included here.
    char msg_from_CEorEE[3 + MAX_NUM_STUDENTS * BUFFSIZE_FOR_NAME];
    memset(msg_from_CEorEE, '\0', 3 + MAX_NUM_STUDENTS * BUFFSIZE_FOR_NAME);

    for(int i = 0; i<2; i++){

        // This size does not include the header, but includes if it is CE or EE
        int sizeOfMsgData = get_msg_data_UDP(msg_from_CEorEE, udp_serv_sock_fd,
                                             &udp_CEorEE_serv_sock_info,
                                             &udp_CEorEE_serv_sock_info_size);

        // This size, of course, is measured in bytes.
        int sizeOfListOfStudents = sizeOfMsgData - 3;

        if(strcmp(msg_from_CEorEE, "CE") == 0){

            memcpy(&udp_CE_serv_sock_info, &udp_CEorEE_serv_sock_info,
                   sizeof(udp_CE_serv_sock_info));

            memcpy(listOfStudentsCE.theList, msg_from_CEorEE + 3,
                   sizeOfListOfStudents);

            listOfStudentsCE.numOfStud = sizeOfListOfStudents/BUFFSIZE_FOR_NAME;

            printf("Main Server received the username list from server CE"
                   " using UDP over port %d\n", MAIN_SERV_UDP_PORT);
        }
        else if(strcmp(msg_from_CEorEE, "EE") == 0){

            memcpy(&udp_EE_serv_sock_info, &udp_CEorEE_serv_sock_info,
                   sizeof(udp_EE_serv_sock_info));

            memcpy(listOfStudentsEE.theList, msg_from_CEorEE + 3,
                sizeOfListOfStudents);

            listOfStudentsEE.numOfStud = sizeOfListOfStudents/BUFFSIZE_FOR_NAME;

            printf("Main Server received the username list from server EE"
                   " using UDP over port %d\n", MAIN_SERV_UDP_PORT);
        }
        else{
            printf("Error: The message received from CE or EE server is not "
                   "valid.\n");
            exit(1);
        }
    }

    printf("----------------------------------------------------------\n");

    //__________________________________________________________________________
    //_________Heyyyyyyyyy Remove this, this only for testing

//    printf("List of CE students:\n");
//    for(int i = 0; i<listOfStudentsCE.numOfStud; i++){
//        printf("%s\n", listOfStudentsCE.theList[i]);
//    }
//
//    printf("\nList of EE students:\n");
//    for(int i = 0; i<listOfStudentsEE.numOfStud; i++){
//        printf("%s\n", listOfStudentsEE.theList[i]);
//    }
//    printf("\n");

    //__________________________________________________________________________
    //__________________________________________________________________________

    int listen_serv_sock_fd;
    int child_serv_sock_fd;
    struct sockaddr_in client_sock_info; //for incoming connections with clients
    socklen_t client_sock_info_size; //for incoming connections with clients.


    setting_TCP_connect_client_serv(&listen_serv_sock_fd, &child_serv_sock_fd,
                                    &client_sock_info, &client_sock_info_size);

    talk_to_client(child_serv_sock_fd);

    close(child_serv_sock_fd);
    close(listen_serv_sock_fd);
}
//______________________________________________________________________________