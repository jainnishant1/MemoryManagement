#include <iostream>
#include<bits/stdc++.h>
using namespace std;

#include "memmgr.h"
#include "complex.h"


int main(int argc, char* argv[]){

std::cout<<1<<endl;
Complex* array[1000];

std::cout<<1<<endl;
clock_t start = std::clock();

for(int i = 0;i < 500000; i++){
    
    std::cout<<1<<endl;
    for(int j = 0; j < 4; j++){
        array[j] = new Complex (i, j);
    }

    for(int j = 0; j < 4; j++){
        delete array[j];
    }
}
 std::cout << ((double)((std::clock() - start) * 1000)) / CLOCKS_PER_SEC << " milliseconds" << std::endl;

return 0;
}
