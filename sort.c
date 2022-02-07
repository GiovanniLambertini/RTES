#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char **argv) {  
    int i, j, min_index;
    char* temp;

    for (i = 1; i < argc; i++) {
        min_index=i;

        for (j = i+1; j < argc; j++) 
            if (strcmp(argv[min_index] , argv[j]) > 0)
                  min_index=j;

        temp = argv[i];
        argv[i] = argv[min_index];
        argv[min_index] = temp;
    }

    for (i = 0; i < argc; i++)
        printf("%s ", argv[i]);  

    printf ("\n");

    return 0;  
}
