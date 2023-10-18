/*
 * Entrance of the project
 * Three algorithms are provided: Co1, CoU and SMT solutions
 */
#include "Co1.h"
#include "CoU.h"
#include "SMT.h"

int folder_name[] = {10, 20, 30, 40, 50, 60, 70, 80};
// int folder_name[] = {30};
int main(){
    for(auto f:folder_name){
        performCo1(f);
    }
//    for(auto f:folder_name){
//        performCoU(f);
//    }
    // SMT only applies when tests small scale systems
//    for(auto f:folder_name){
//        performSMT(f);
//    }
    return 0;
}
