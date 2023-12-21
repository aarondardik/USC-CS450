#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFSIZE 4096

int main() {

    FILE *fptrCE;

    if ((fptrCE = fopen("serverCE.c", "r")) == NULL) {
        printf("Error opening file\n");
        exit(1);
    }

    FILE *fptrEE;

    if ((fptrEE = fopen("serverEE.c", "w")) == NULL) {
        printf("Error opening file\n");
        exit(1);
    }

    char buff_input[BUFFSIZE];

    int cont = 1;
    while (fgets(buff_input, BUFFSIZE, fptrCE) != NULL) {//

        if(cont == 14){
            char * line = "#define SERV_NAME \"EE\"\n";
            fwrite(line, sizeof(char), strlen(line), fptrEE);
            cont++;
        }
        else if(cont == 15) {
            char *line = "#define FILENAME \"EE.txt\"\n";
            fwrite(line, sizeof(char), strlen(line), fptrEE);
            cont++;
        }
        else if(cont == 16) {
            char *line = "#define SERV_UDP_PORT   22420\n";
            fwrite(line, sizeof(char), strlen(line), fptrEE);
            cont++;
        }
        else{
            fwrite(buff_input, sizeof(char), strlen(buff_input), fptrEE);
            cont++;
        }
    }
    fclose(fptrCE);
    fclose(fptrEE);
}