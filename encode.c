#include <stdio.h>
#include <stdbool.h>


int main() {
    int M[257][17] = {0};

    int layer = 1;

    for(int i = 0; i < 256; i++){
        for(int t = 0; t < 33; t++){
            for(int n_i = 0; n_i < 16; n_i++){
                if(i != -1) {                    
                    if(i <= 16) {
                        if(n_i == 0 && t == 16*layer + 16 - i + 1 && i != 0) {
                          M[i][n_i] = t; 
                        } 
                    } else { 
                        if(i % 16 == 0) { 
                            if(n_i == (int) i / 16 - 1 && t % 16 == 1) {
                                M[i][n_i] = t - 16 + 16*layer;
                            } 
                        } else {
                            if(n_i == (int) i / 16) {
                                if (t == 16*layer + (16 - (i - (i / 16) * 16)) % 16 + 1) {
                                     M[i][n_i] = t; 
                                } 
                            } else { 
                                if((n_i == (int) i / 16 - 1) && (t == 16*layer + (16 - (16 - i - (i / 16) * 16)) % 16 + 1)) {
                                    M[i][n_i] = t; 
                                }
                            }  
                        }
                    }
                }
            }
        }
    }

    for(int i = 0; i < 256; i++){
        printf("[");
        for(int j = 0; j < 16; j++) {
            if(j==15) printf("%d]\n", M[i][j]);
            else printf("%d, ", M[i][j]);
        }
    }
    return 0;
}