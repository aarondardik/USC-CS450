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

#define SERV_NAME "EE"
#define FILENAME "EE.txt"
#define SERV_UDP_PORT   22420
//______________________________________________________________________________
#define BUFFSIZE 4096
#define MAX_NUM_STUDENTS  1000
#define BUFFSIZE_FOR_NAME 50
#define BUFFSIZE_FOR_COURSE 5 // Actually only need 4 cus 3 digits(for 100)+'\0'
#define NUM_OF_DIFF_COURSES 200 // Actually it is (0-100)
#define MAIN_SERVER_IP_ADDR "127.0.0.1"
#define MAIN_SERV_UDP_PORT 23420
#define CCEE_HEAD_SIZE 6 // Including the \0 at the end.
//______________________________________________________________________________
typedef char studentName_t[BUFFSIZE_FOR_NAME];
typedef char course_t[BUFFSIZE_FOR_COURSE];

//______________________________________________________________________________

typedef struct studentCourses{
    char str_repr[BUFFSIZE];
    studentName_t studentName;
    course_t courses[NUM_OF_DIFF_COURSES];
    int numOfCourses;
} studentCourses_t;

//______________________________________________________________________________

typedef struct listOf_studCur{

    int num_of_students;
    studentCourses_t * theList[MAX_NUM_STUDENTS];
    studentName_t listOfOnlyNames[MAX_NUM_STUDENTS];

} listOf_studCur_t;

//______________________________________________________________________________

typedef struct intersectRes{

    int numOfCourses;
    char str_repr[BUFFSIZE];
    course_t courses[NUM_OF_DIFF_COURSES];
    char str_repr_aux[BUFFSIZE];

} intersectRes_t;
//______________________________________________________________________________

listOf_studCur_t listStud_Cour;
//______________________________________________________________________________

void set_servM_sockInfo(struct sockaddr_in * udp_sock_info_ptr,
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

    memcpy(udp_sock_info_ptr, &udp_sock_info, sizeof(udp_sock_info));
    memcpy(udp_sock_info_size_ptr, &udp_sock_info_size,
           sizeof(udp_sock_info_size));
}
//______________________________________________________________________________

int get_msg_data_UDP(char * msg_data, int udp_sock_fd,
                     struct sockaddr_in * from_sock_info_ptr,
                     socklen_t * from_sock_info_size_ptr){

    int maxSizeAllowed = CCEE_HEAD_SIZE + MAX_NUM_STUDENTS*BUFFSIZE_FOR_NAME;

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

void initializeArrayOfSets(char * msg, course_t * *setsOfCourses,
                           int * numOfSets){

    char cpy_msg_from_cli [BUFFSIZE];
    strcpy(cpy_msg_from_cli, msg);
    char * delim  = ", ";
    char * token  = strtok(cpy_msg_from_cli, delim);

    while(token != NULL){

        for(int i = 0; i < listStud_Cour.num_of_students; i++){

            if(strcmp(listStud_Cour.listOfOnlyNames[i], token) == 0){

                setsOfCourses[*numOfSets] = listStud_Cour.theList[i]->courses;
                break;
            }
        }
        (*numOfSets)++;
        token = strtok(NULL, delim);
    }
}
//______________________________________________________________________________

void parse_line_to_get_courses(studentCourses_t * student){

    char * line = student->str_repr;
    char numbAndCommasAndSqrBrk[BUFFSIZE];
    memset(numbAndCommasAndSqrBrk, '\0', BUFFSIZE);
    int j = 0; // index for numbAndCommasAndClsBrk

    for(int i = 0; i < strlen(line); i++){

        if(('0' <= line[i] && line[i] <= '9') || line[i] == ',' ||
           line[i] == ']'){
            memcpy(numbAndCommasAndSqrBrk + j, line + i, 1);
            j++;
        }
    }

    char onlyNumsAndCommas[BUFFSIZE];
    memset(onlyNumsAndCommas, 0, BUFFSIZE);

    char * token_elimSqrBrk;
    char * subtoken_elimComma;
    char * saveptrTok = numbAndCommasAndSqrBrk;
    char * delimTok  = "]";
    char * delimSubTok  = ",";
    int numOfTokens = 0;

    token_elimSqrBrk = strtok_r(numbAndCommasAndSqrBrk, delimTok,
                                &saveptrTok);

    while (token_elimSqrBrk != NULL){

        numOfTokens++;

        subtoken_elimComma = strtok_r(token_elimSqrBrk, delimSubTok,
                                      &token_elimSqrBrk);
        char leftSide[BUFFSIZE_FOR_COURSE];
        strcpy(leftSide, subtoken_elimComma);

        strcat(onlyNumsAndCommas, subtoken_elimComma);

        subtoken_elimComma = strtok_r(NULL, delimSubTok, &token_elimSqrBrk);

        strcat(onlyNumsAndCommas, ",");

        if(subtoken_elimComma != NULL){
            strcat(onlyNumsAndCommas, subtoken_elimComma);
            strcat(onlyNumsAndCommas, ",");

        } else{
            strcat(onlyNumsAndCommas, leftSide);
            strcat(onlyNumsAndCommas, ",");
        }
        token_elimSqrBrk = strtok_r(NULL, delimTok, &saveptrTok);
    }

    onlyNumsAndCommas[strlen(onlyNumsAndCommas)-1] = '\0';

    char * delim  = ",";
    char * token;
    int numOfCOurses = 0;

    course_t leftCourse;
    token = strtok(onlyNumsAndCommas, delim);
    memcpy(&leftCourse, token, strlen(token)+1);

    course_t rightCourse;

    for(int i = 0; i < numOfTokens; i++){

        if (i == 0){
            token = strtok(NULL, delim);
            memcpy(&rightCourse, token, strlen(token)+1);
        }
        else{
            token = strtok(NULL, delim);
            memcpy(&leftCourse, token, strlen(token)+1);

            token = strtok(NULL, delim);
            memcpy(&rightCourse, token, strlen(token)+1);
        }


        int lef_course_int = atoi(leftCourse);
        int right_course_int = atoi(rightCourse);
        int diff = right_course_int - lef_course_int;

        sprintf(student->courses[numOfCOurses], "%d", lef_course_int);

        numOfCOurses++;

        for(int k = 0; k < diff; k++){

            sprintf(student->courses[numOfCOurses], "%d",
                    atoi(student->courses[numOfCOurses-1])+1);

            numOfCOurses++;
        }
    }
    student->numOfCourses = numOfCOurses;


    //__________________________________________________________________________
//    char oeeee[BUFFSIZE];
//    memset(oeeee, 0, BUFFSIZE);
//    for(int i = 0; i < student->numOfCourses; i++){
//
//        strcat(oeeee, student->courses[i]);
//        strcat(oeeee, "|");
//    }
//    oeeee[strlen(oeeee)-1] = '\0';
//    char * eeeeeeeee = oeeee;
//    eeeeeeeee+1;
    //__________________________________________________________________________
}


//______________________________________________________________________________

int main(){

    printf("Server %s is up and running using UDP on port %d.\n",
           SERV_NAME, SERV_UDP_PORT);

    FILE * fptr;

    if((fptr = fopen(FILENAME, "r")) == NULL){
        printf("Error opening file\n");
        exit(1);
    }

    char buff_input[BUFFSIZE];

    memset(&listStud_Cour, 0, sizeof(listOf_studCur_t));
    listStud_Cour.num_of_students = 0; // Just for clarity (already done above).

    while(fgets(buff_input, BUFFSIZE, fptr) != NULL){

        studentCourses_t * studentCourses_ptr =
                           (studentCourses_t *)malloc(sizeof(studentCourses_t));

        memset(studentCourses_ptr, 0, sizeof(studentCourses_t));

        if(buff_input[strlen(buff_input) - 1] == '\n'){
            buff_input[strlen(buff_input) - 1] = '\0';
        }
        strcpy(studentCourses_ptr->str_repr, buff_input);

        char * posSemiColon = strchr(buff_input, ';');

        int len_studentName = posSemiColon - buff_input;

        strncpy(studentCourses_ptr->studentName, buff_input, len_studentName);

        //______________________________________________________________________
        //________Parsing the line to get the course of the student_____________

        parse_line_to_get_courses(studentCourses_ptr);

        //______________________________________________________________________
        //______________________________________________________________________

        studentCourses_ptr->studentName[len_studentName] = '\0';

        listStud_Cour.theList[listStud_Cour.num_of_students]=studentCourses_ptr;

        strcpy(listStud_Cour.listOfOnlyNames[listStud_Cour.num_of_students],
               studentCourses_ptr->studentName);

        listStud_Cour.num_of_students++;
    }

    //__________________________________________________________________________
    //__________________________________________________________________________
    //________This is just for testing purposes!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

//    for(int i = 0; i < listStud_Cour.num_of_students; i++){
//        printf("%s\n", listStud_Cour.theList[i]->str_repr);
//    }
//
//    for(int i=0; i < listStud_Cour.num_of_students; i++){
//        printf("%s\n", listStud_Cour.listOfOnlyNames[i]);
//    }

    //__________________________________________________________________________
    //__________________________________________________________________________
    //__________________________________________________________________________

    char msgToSend[CCEE_HEAD_SIZE + 3 + MAX_NUM_STUDENTS * BUFFSIZE_FOR_NAME];

    int lenLListOfNames = listStud_Cour.num_of_students * BUFFSIZE_FOR_NAME;

    int totalSizeOfTheData = lenLListOfNames + 3; // Excluding header.

    sprintf(msgToSend,"%d", totalSizeOfTheData);

    strcpy(msgToSend + CCEE_HEAD_SIZE, SERV_NAME);

    memcpy(msgToSend + CCEE_HEAD_SIZE + 3, listStud_Cour.listOfOnlyNames,
           lenLListOfNames);


    //________Creating UDP socket to communicate with the main server___________
    int udp_servCEEE_sock_fd;

    if((udp_servCEEE_sock_fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0){
        printf("Error creating socket\n");
        exit(1);
    }

    //___Setting the socket info of the UDP socket at the main server side
    struct sockaddr_in udp_servM_sock_info;
    socklen_t udp_servM_sock_info_size;

    set_servM_sockInfo(&udp_servM_sock_info, &udp_servM_sock_info_size);

    //__________________________________________________________________________
    //_____Sending the message of students in this server to main server________

    int totalNumOfBytesToSend = CCEE_HEAD_SIZE + 3 + lenLListOfNames;

    int totalNumOfBytesSent = sendto(udp_servCEEE_sock_fd, msgToSend,
                                     totalNumOfBytesToSend, 0,
                                     (struct sockaddr *)&udp_servM_sock_info,
                                     udp_servM_sock_info_size);

    if(totalNumOfBytesSent != totalNumOfBytesToSend){

        printf("Error sending message\n");
        exit(1);
    }

    printf("Server%s finished sending a list of usernames to Main Server.\n",
           SERV_NAME);

    printf("----------------------------------------------------------\n");

    //__________________________________________________________________________
    //__RECEIVING UDP message from serverM with stud to compute the intersection
    //__and SENDING the result back to serverM.

    for(;;){

        // Header is not included here.
        char msgFromServ_data[MAX_NUM_STUDENTS * BUFFSIZE_FOR_NAME];
        memset(msgFromServ_data, 0, MAX_NUM_STUDENTS * BUFFSIZE_FOR_NAME);

        get_msg_data_UDP(msgFromServ_data, udp_servCEEE_sock_fd,
                         &udp_servM_sock_info,
                         &udp_servM_sock_info_size);

        int printOrNot = strlen(msgFromServ_data) > 0 ? 1 : 0;

        if(printOrNot){
            printf("Server%s received the usernames from Main Server using UDP"
                   " over port %d.\n", SERV_NAME, SERV_UDP_PORT);

//            printf("oeeee message received from serverM: \"%s\"\n",
//                   msgFromServ_data);
        }

        intersectRes_t intersectionResult;
        memset(&intersectionResult, 0, sizeof(intersectRes_t));

        course_t * setsOfCourses[MAX_NUM_STUDENTS];
        memset(setsOfCourses, 0, MAX_NUM_STUDENTS * sizeof(course_t *));

        int numOfSets = 0;

        initializeArrayOfSets(msgFromServ_data, setsOfCourses, &numOfSets);

        compute_intersect(setsOfCourses, numOfSets, &intersectionResult);

        if(intersectionResult.numOfCourses == 0 && printOrNot){
            printf("Server%s did not find any course intersection"
                   " for %s\n", SERV_NAME, msgFromServ_data);
        }
        else if(printOrNot){
            printf("Found the intersection result: %s for %s.\n",
                   intersectionResult.str_repr, msgFromServ_data);
        }

        char msgToSendInterc[CCEE_HEAD_SIZE + 3 + sizeof(intersectRes_t)];

        memset(msgToSendInterc, 0, CCEE_HEAD_SIZE + 3 + sizeof(intersectRes_t));

        sprintf(msgToSendInterc,"%lu", 3 + sizeof(intersectRes_t));

        strcpy(msgToSendInterc + CCEE_HEAD_SIZE, SERV_NAME);

        memcpy(msgToSendInterc + CCEE_HEAD_SIZE + 3, &intersectionResult,
               sizeof(intersectRes_t));

        int numOfBytesToSend = CCEE_HEAD_SIZE + 3 + sizeof(intersectRes_t);

        int numOfBytesSent = sendto(udp_servCEEE_sock_fd, msgToSendInterc,
                                    numOfBytesToSend, 0,
                                    (struct sockaddr *)&udp_servM_sock_info,
                                    udp_servM_sock_info_size);

        if(numOfBytesToSend != numOfBytesSent){
            printf("Error sending message\n");
            exit(1);
        }

        if(printOrNot){
            printf("Server%s finished sending the response to Main Server.\n",
                   SERV_NAME);
            printf("----------------------------"
                   "------------------------------\n");
        }
    }

    //__________________________________________________________________________
    //__________________________________________________________________________
    fclose(fptr);
}