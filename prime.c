#include <stdio.h>
#include <stdlib.h>

void main(int argc, char *argv[]) {
    int i,j,n;
 
    n = atoi (argv[1]);    

    printf("I numeri primi sono:-\n");    

    for(i=2;i<=n;i++) {
        int numDivisori=0;

        for(j=1;j<=i;j++) {
            if(i%j==0) {
                numDivisori++;
            }
        }
     
        if(numDivisori==2) {
            printf("%d ",i);
        }
    }

    printf("\n");
}



