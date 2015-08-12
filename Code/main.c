

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

float **train_data;
int *lable;
int main()
{
    int row = 0;
    int col = 0;

    char buffer[10000];
    FILE *fp = fopen("../../NeuralNetC/iris.data", "rb");
    while (1) {
        if (!fgets(buffer, sizeof buffer, fp)) break;
        //printf("%s\n", buffer);
        char seps[] = ", \t\n\r";
        char *token;
        token = strtok(buffer, seps);
        int i = 0;
        while (token != NULL) {
            printf("%d, %s\n", i, token);
            if(i==0) row = atoi(token);
            if(i==1) col = atoi(token);
            token = strtok(NULL, seps);
            i++;
        }
        printf("%d %d", row, col);
    }
}

//float **train_data;
//int *lable;
//
//int main(int argc, const char * argv[]) {
//    freopen("../../NeuralNetC/iris.data", "r", stdin);
//    //freopen("test.out", "w", stdout);
//    
//    int row;
//    int col;
//    
//    scanf("%d", &row);
//    scanf("%d", &col);
//    
//    printf("%d %d", row, col);
//    
//    train_data = (float **) malloc (row * sizeof(float*));
//    lable = (int *) malloc (row * sizeof(int));
//    char label_str[255];
//    for (int i = 0; i < row; i++) {
//        train_data[i] = (float *) malloc ((col - 1) * sizeof(float));
//        for (int j = 0; j < col - 1; j++) {
//            scanf("%f", &train_data[i][j]);
//        }
//        scanf("%s",label_str);
//    }
//    
//    for (int i = 0; i < row; i++) {
//        for (int j = 0; j < col - 1; j++) {
//            printf("%f ", train_data[i][j]);
//        }
//        
//    }
//    
//    fclose(stdin);
//    return 0;
//}
