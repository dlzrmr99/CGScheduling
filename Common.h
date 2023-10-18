//
// Created by yunpa38 on 2022-10-25.
//
#pragma once
#ifndef CGSCHEDULINGCN_COMMON_H
#define CGSCHEDULINGCN_COMMON_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <chrono>
#include <numeric>
#include <set>
#include "z3++.h"

using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::set;
using std::pair;

using z3::context;
using z3::solver;
using z3::expr;
using z3::optimize;
using z3::func_decl;
using z3::function;
using z3::expr_vector;
using z3::sum;


// store all cfg related information
static std::vector<std::string> cfgInformation;

// only one setting
static std::string settingsName [] = {"CoU"};

// list of the folder name to distinguish the number of tested traffic flow, e.g., 10, 20, ...
static int folderName [] = {10, 20, 30, 40, 50, 60, 70};

// for test, establish a very simple test case
// static std::string testSample = "0 0 5 10 20 0 1 5 7 3 4 2 0 2 5 2 10 2 0 4 2 3 5 2 1 4 3 3 5 2 0 4 3 2 5 1";
static std::string testSample = "0 0 3 5 10 0 1 5 3 3 2 2 0 2 3 2 5 2 0 2 2 1 5 2";
// static std::string testSample = "0 0 1 5 5 0 1 5 1 1 1 2";
// static std::string testSample = "0 0 3 5 10 0 1 5 3 3 2 2 0 2 3 2 5 2 0 5 2 3 2 2";
// std::string testSample = "0 0 2 6 4 0 0 2 2 2 2 2 0 4 2 3 1 2";
static bool ifTest = false;
static bool dynamicPRURatio = false;
static bool output_enabled = false;
static double RBIncreaseRatio = 1;

static double relaxedWidthIndicator = 0.75;
// #define relaxedWidthIndicator 0.75
static double relaxedWidthIndicatorList [] = {0.5, 0.75, 1.00};
static int minControlMessageHeight = 1;
static int distanceControlConfiguration = 50;
static int distanceDataControl = 1000;
static double ratioResourceEfficiencyControlMessageSize = 0.01;

static double pru_ratio_list[] = {1, 0.75, 0.5, 0.25, 0.1, 0};

static double average(std::vector<double> const& v){
    if(v.empty()){
        return 0;
    }
    auto const count = static_cast<float>(v.size());
    return std::accumulate(v.begin(), v.end(), 0.0) / count;
}

// store the result of scheduling
struct returnResults{
    bool failOrSuccess;
    int maxPilots;
    double resourceEfficiency;  // %
    double time;    //  ms
};
static returnResults rr {false, -1, -1, -1};

struct scheduleInfoElement {
    int data[4];
};

static int vector_sum(vector<int>& v){
    int sum = 0;
    for(int ele: v){
        sum += ele;
    }
    return sum;
}

static int vector_max_idx(vector<int>& v){
    int max_idx = 0;
    int max_value = v[0];
    for(int i = 1; i < v.size(); i++){
        if(v[i] > max_value){
            max_idx = i;
            max_value = v[i];
        }
    }
    return max_idx;
}


class TrafficFlow {
public:
    // fundamental setting for each traffic flow
    int initialOffset, transmissionPeriod, payload, latencyRequirement;
    // maximum number of allowed configurations, for CoU, it is the number of packets
    int numberOfConfigurations;
    // the number of resource units needed for each control message
    int controlOverhead;
    // a number assigned to each traffic flow, used for debugging
    int number;

    // after having the schedule for each data packet, calculate the number of configurations
    int numberOfConfigurationActual = 0;
    // sort all traffic flows according to the scheduling emergency
    double ratioPayloadTransmissionPeriod = 0.0;
    // storing the scheduling information after
    // vector<vector<int>> schedulingInfo;
    // storing the schedules for all data packets
    std::vector<scheduleInfoElement> schedulingInfo;
    // storing the offsets for all configurations
    std::vector<int> offsetConfigurationsActual;
    // storing all scheduling windows
    std::vector<std::pair<int, int>> schedulingInterval;

    // constructor
    TrafficFlow(int it, int tp, int pl, int lr, int nc, int co, int n){
        initialOffset = it;
        transmissionPeriod = tp;
        payload = pl;
        latencyRequirement = lr;
        numberOfConfigurations = nc;
        controlOverhead = co;
        number = n;
        // calculate the ratioPayloadTransmissionPeriod
        ratioPayloadTransmissionPeriod = (double) payload / latencyRequirement;
    }

};

// Function: to sort traffic flow
static bool compareSchedulingFlexibility(const TrafficFlow& tf1, const TrafficFlow &tf2){
    // 1. ratio
    // cout << tf1.number << " io: "<<tf1.initialOffset << "  " << tf2.number << " io: " << tf2.initialOffset << endl;
    if(tf1.ratioPayloadTransmissionPeriod > tf2.ratioPayloadTransmissionPeriod) return true;
    if(tf1.ratioPayloadTransmissionPeriod < tf2.ratioPayloadTransmissionPeriod) return false;

    // 2. payload
    if(tf1.payload > tf2.payload){
        return true;
    }else{
        return false;
    }
}

static bool compareOffset(const TrafficFlow& tf1, const TrafficFlow &tf2){
    // 1. ratio
    // cout << tf1.number << " io: "<<tf1.initialOffset << "  " << tf2.number << " io: " << tf2.initialOffset << endl;
    if(tf1.initialOffset >= tf2.initialOffset){
        return false;
    }else{
        return true;
    }

    // 2. payload
//    if(tf1.payload > tf2.payload){
//        return true;
//    }else{
//        return false;
//    }
}


static std::ostream & operator << (std::ostream & os, const std::vector<TrafficFlow> & vec){
    os << std::endl << "List all traffic flows: " << std::endl;
    os << "IT\t"
          "TP\t"
          "PL\t"
          "LR\t"
          "NC\t"
          "CO\t"
          "NUM" << std::endl;
    for(const auto& elem : vec){
        os << elem.initialOffset << "\t"
           << elem.transmissionPeriod << "\t"
           << elem.payload << "\t"
           << elem.latencyRequirement << "\t"
           << elem.numberOfConfigurations << "\t"
           << elem.controlOverhead << "\t"
           << char('A' + elem.number) << std::endl;
    }
    return os;
}


struct StairListElement{
    int stair[3];
    /*
     * Format:
     * 1            2           3
     * stair_start, stair_end, start_distance
     */
};

struct  FlatListElement{
    int flat[5];
    /*
     * Format:
     * 1            2           3               4           5
     * stair_start, stair_end, start_distance, flat_start, flat_end
     */
};

struct FreeSpaceElement{
    int ele[3];
    /*
 * Format:
 * 1            2           3
 * col_index    bottom      height
 */
};


static std::ostream & operator << (std::ostream & os, const std::vector<FlatListElement> & vec)
{
    os << std::endl <<"List all flats: " << std::endl;
    for(auto elem : vec)
    {
        for(auto e:elem.flat){
            os << e << " ";
        }
        os<<std::endl;
    }
    return os;
}

static std::ostream & operator << (std::ostream & os, const std::vector<StairListElement> & vec)
{
    os << std::endl << "List all stairs: " << std::endl;
    for(auto elem : vec)
    {
        for(auto e:elem.stair){
            os << e << " ";
        }
        os<<std::endl;
    }
    return os;
}


class ResourceGrid{
public:
    int numberOfSlots, numberOfPilots, topPilots;
    int **data;
    ResourceGrid(int nos, int nop){
        numberOfSlots = nos;
        numberOfPilots = nop;
        topPilots = 1;
        data = new int* [numberOfSlots];
        for (int i = 0; i < numberOfSlots; i++){
            data[i] = new int[numberOfPilots];
        }
        int i, j;
        for(i=0;i<numberOfSlots;i++){
            for(j=0;j<numberOfPilots;j++){
                data[i][j] = -1;
            }
        }
    }
    ResourceGrid(ResourceGrid& rg){
        numberOfSlots = rg.numberOfSlots;
        numberOfPilots = rg.numberOfPilots;
        topPilots = 1;
        data = new int* [numberOfSlots];
        for (int i = 0; i < numberOfSlots; i++){
            data[i] = new int[numberOfPilots];
        }
        int i, j;
        for(i=0;i<numberOfSlots;i++){
            for(j=0;j<numberOfPilots;j++){
                data[i][j] = rg.data[i][j];
            }
        }
    }

    // toy case, print the basic setting of the resource grid
    std::string tostring() const{
        return "This is an ResourceGrid object with slots: " +
               std::to_string(numberOfSlots) +
               ", pilots: " + std::to_string(numberOfPilots) + "\n";
    }
    // for debug, print the current status of the resource gird for all resource units
    void printData() const{
        int i, j;
        std::cout << "Current status of the resource grid: " << std::endl;
        for(i = numberOfPilots - 1;i >= 0; i--){
            for(j=0;j<numberOfSlots;j++){
                char parsed;
                if(data[j][i] == -1){
                    parsed = '0';
                }else{
                    parsed = 'A'+data[j][i];
                }
                std::cout << parsed << " ";
            }
            std::cout << std::endl;
        }
    }

    // update the status
    void update(int left, int right, int bottom, int height, int tfno) const{
        int i,j;
        for(i = left; i <= right; i++){
            for(j = bottom; j < bottom + height; j++){
                data[i][j] = tfno;
            }
        }
    }

    // other possible functionalities
    // Function1: list all stairs
    // [start_index, end_index, distance to top]
    std::vector<FlatListElement> listStairs(int s, int e, int top_pilots){
        // check s, e, and top_pilots
        assert(s>=0);
        assert(e<=numberOfSlots-1);
        assert(top_pilots <= numberOfPilots - 1);

        bool stair = false;
        int stair_top = -1;
        std::vector<StairListElement> stairlist;
        int stair_end = -1;
        int stair_start = -1;
        int old_stair_top = -1;
        if(s == e){
            for(int frequency_pointer = top_pilots; frequency_pointer >= 0; frequency_pointer--){
                if(data[s][frequency_pointer] != -1){
                    stair_top = frequency_pointer;
                    break;
                } else{
                    if(frequency_pointer == 0){
                        stair_top = -1;
                    }
                }
            }
            stairlist.push_back({s, s, numberOfPilots - 1 - stair_top});
        }else{
            for(int time_pointer = e; time_pointer > s - 1; time_pointer--){
                if(!stair){
                    for(int frequency_pointer = top_pilots; frequency_pointer >= 0; frequency_pointer--){
                        if(data[time_pointer][frequency_pointer]!=-1){
                            stair = true;
                            stair_top = frequency_pointer;
                            stair_start = time_pointer;
                            stair_end = stair_start;
                            break;
                        }else{
                            if(frequency_pointer == 0){
                                stair = true;
                                stair_top = -1;
                                stair_start = time_pointer;
                                stair_end = stair_start;
                            }
                        }
                    }
                }else{
                    old_stair_top = stair_top;
                    for(int frequency_pointer = top_pilots; frequency_pointer >= 0; frequency_pointer--){
                        if(data[time_pointer][frequency_pointer]!=-1){
                            stair = true;
                            stair_top = frequency_pointer;
                            break;
                        }else{
                            if(frequency_pointer == 0){
                                stair = true;
                                stair_top = -1;
                            }
                        }
                    }
                    if(old_stair_top == stair_top){
                        stair_start = time_pointer;
                        if(time_pointer == s){
                            stairlist.push_back({s, stair_end, numberOfPilots - 1 - stair_top});
                        }
                    }else{
                        stairlist.push_back({stair_start, stair_end, numberOfPilots - 1 - old_stair_top});
                        stair_end = stair_start = time_pointer;
                        if(time_pointer == s){
                            stairlist.push_back({s, stair_end, numberOfPilots - 1 - stair_top});
                        }
                    }
                }
            }
        }
//        std::cout << "top pilots: " << top_pilots;
//        std::cout << stairlist;

        std::vector<FlatListElement> flatList;
        int old_start, old_end, current_distance, new_start, new_end, pointer_distance;
        for(auto ele: stairlist){
            old_start = ele.stair[0];
            old_end = ele.stair[1];
            current_distance = ele.stair[2];
            if(current_distance == 0){
                continue;
            }
            new_start = old_start;
            new_end = old_end;
            pointer_distance = -1;
            if(old_end == e){
                new_end = e;
            }else{
                for(int pointer = old_end + 1; pointer <= e; pointer++){
                    for(auto checkele: stairlist){
                        if(checkele.stair[0] <= pointer && pointer <= checkele.stair[1]){
                            pointer_distance = checkele.stair[2];
                        }
                    }
                    if(pointer_distance >= current_distance){
                        new_end = pointer;
                    }else{
                        new_end = pointer - 1;
                        break;
                    }
                }
            }
            if(old_start == s){
                new_start = s;
            }else{
                for(int pointer = old_start - 1; pointer > s - 1; pointer--){
                    for(auto checkele: stairlist){
                        if(checkele.stair[0] <= pointer <= checkele.stair[1]){
                            pointer_distance = checkele.stair[2];
                        }
                    }
                    if(pointer_distance >= current_distance){
                        new_start = pointer;
                    }else{
                        new_start = pointer + 1;
                        break;
                    }
                }
            }
            flatList.push_back({ele.stair[0], ele.stair[1], ele.stair[2], new_start, new_end});
        }
        return flatList;
    }
};


/*
 * Format of the cfg file
 * 0                1(useless here)     2               3                   4               5
 * index of TF      index of settings   number of TFs   number of pilots    hyper-period    folder_name
 * Then, info for each TF
 * 0                1                   2           3                   4                       5
 * initialOffset    transmissionPeriod  payload     latencyRequirement  numberOfConfigurations  controlOverhead
 * ....
 */
// extract the information from the cfg file
static void extractCfg(int f){
    std::ifstream myFile;
    std::string line;
    myFile.open("../DataGeneration/conf_" + std::to_string(f) + ".cfg");
    if(myFile.is_open()){
        while ((getline(myFile, line))){
            // std::cout << line << std::endl;
            cfgInformation.push_back(line);
        }
        myFile.close();
    }else{
        std::cout << "Unable to open the file!" << std::endl;
    }
//    std::cout << "cfgInformation = { ";
//    for (std::string &l: cfgInformation){
//        std::cout << l << std::endl;
//    }
//    std::cout << " }; \n";
}

// Function: split a std::string with specified delim
static std::vector<std::string> split (const std::string &s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss (s);
    std::string item;

    while (getline (ss, item, delim)) {
        result.push_back (item);
    }
    return result;
}

static void testResourceGrid(){
    ResourceGrid rg(20,6);
    std::cout << rg.tostring();
    rg.update(4, 6, 0, 1, 1);
    rg.update(6,6,1,1,1);
    rg.printData();
    std::vector<FlatListElement> flatlist = rg.listStairs(4, 6, 2);
    std::cout << flatlist << std::endl;
}


#endif //CGSCHEDULINGCN_COMMON_H
