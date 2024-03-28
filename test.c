#include <stdio.h>

typedef struct 
{
    char *ha;
    int da;
}Data1;

typedef struct 
{
    Data1 data1;
    double ba;
}Data2;



int main()
{
    char tmp1[2] = {1, 2};
    int tmp2[2] = {1, 2};
    Data2 data_2;


    int *buf2 = data_2.data1.da;
    char *buf1 = data_2.data1.ha;

    
    printf("%d, %d\n", data_2.data1.da[0], data_2.data1.da[1]);
    return 0;
}