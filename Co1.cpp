//
// Created by yunpa38 on 2022-10-12.
// Implementation for "Co1".
//
#include "Co1.h"
int quarantine_PRU = 10;

/*
 * sort all traffic flows: according to the offset / randomly
 */
void Co1(string& cfg, int f){
    // Step 1: parse the meta information from cfg file
    std::vector<std::string> parsedCfg = split(cfg, ' ');
    // for(auto item: parsedCfg) std::cout << item << std::endl;
    int indexOfTF = stoi(parsedCfg[0]);
    int indexOfSetting = stoi(parsedCfg[1]);
    int numberOfTFs = stoi(parsedCfg[2]);
    int numberOfPilots = stoi(parsedCfg[3]);
    int hyperPeriod = stoi(parsedCfg[4]);
    int numberOfSlots = hyperPeriod;
    int indexOfFolder = stoi(parsedCfg[5]);

    // Step 2: create traffic flow list
    std::vector<TrafficFlow> trafficFlowList;
    int numberOfAttributes = 5;     // start index of arguments for all traffic flows
    int readingOffset = 6;          // for each traffic flow, 6 arguments are needed

    for(int tf = 0; tf < numberOfTFs; tf++){
        trafficFlowList.emplace_back(
                TrafficFlow(stoi(parsedCfg[numberOfAttributes + readingOffset * tf + 1]),
                            stoi(parsedCfg[numberOfAttributes + readingOffset * tf + 2]),
                            stoi(parsedCfg[numberOfAttributes + readingOffset * tf + 3]),
                            stoi(parsedCfg[numberOfAttributes + readingOffset * tf + 4]),
                            stoi(parsedCfg[numberOfAttributes + readingOffset * tf + 5]),
                            stoi(parsedCfg[numberOfAttributes + readingOffset * tf + 6]),
                            tf));
    }
    // std::cout << trafficFlowList;
    // Step 2.2: sort all traffic flows according to the scheduling flexibility & payload
    //  ratio between payload and transmission period
    sort(trafficFlowList.begin(), trafficFlowList.end(), compareSchedulingFlexibility);
    std::cout << trafficFlowList;

    // Step 3: create resource grid
    ResourceGrid rg(numberOfSlots, numberOfPilots);
    // testResourceGrid();

    auto startTime = std::chrono::steady_clock::now();
    // Step 4: schedule for all data packets
    int scheduleIntervalStart, scheduleIntervalEnd;
    int comparsionCount = 0;
    for(int tfIndex = 0; tfIndex < numberOfTFs; tfIndex++){
        std::vector<std::pair<int, int>> schedulingIntervals;
        for(int i = 0; i < numberOfSlots / trafficFlowList[tfIndex].transmissionPeriod; i++){
            schedulingIntervals.emplace_back(
                    std::make_pair(ceil(trafficFlowList[tfIndex].initialOffset +
                                        trafficFlowList[tfIndex].transmissionPeriod * i),floor(trafficFlowList[tfIndex].initialOffset +
                                                                                               trafficFlowList[tfIndex].transmissionPeriod * i + trafficFlowList[tfIndex].latencyRequirement - 1)));
        }
//        for(auto si: schedulingIntervals){
//            std::cout << si.first << " " << si.second << std::endl;
//        }
        trafficFlowList[tfIndex].schedulingInterval = schedulingIntervals;
        // first, consider the first interval
        scheduleIntervalStart = schedulingIntervals[0].first;
        scheduleIntervalEnd = schedulingIntervals[0].second;
        // for each flat, calculate the minimal height to identify the shape
        // Format: left, right, bottom, height
        int lowestSettingForAllFlat [] = {0, 0, 0, 99999};

        // width max
        int WidthMax;
        int freeSpace = scheduleIntervalEnd - scheduleIntervalStart + 1;
        int h = ceil((double) trafficFlowList[tfIndex].payload / freeSpace);
        if (h == 1) {
            WidthMax = trafficFlowList[tfIndex].payload;
        } else {
            WidthMax = scheduleIntervalEnd - scheduleIntervalStart + 1;
        }
        // FUNCTION: calculate the minimum height for all shapes w.r.t the current flat
        int offsetMin = scheduleIntervalStart;
        int offsetMax = scheduleIntervalEnd;
        int bottomMin = 0;
        int widthMax = WidthMax;
        // cout << "Width: " << widthMax << endl;
        int transmissionPeriod = trafficFlowList[tfIndex].transmissionPeriod;

        for(int alternativeWidth = 1; alternativeWidth <= widthMax; alternativeWidth++){
            int tmpHeight = (int)ceil((double) trafficFlowList[tfIndex].payload / alternativeWidth);
            // cout << "Height: " << tmpHeight << endl;
            int tmpOffsetMax = offsetMax - alternativeWidth + 1;
            for(int alternativeOffset = tmpOffsetMax; alternativeOffset >= offsetMin; alternativeOffset--){
                for(int alternativeBottom = bottomMin; alternativeBottom < numberOfPilots; alternativeBottom++){
                    // check collision for all packets
                    bool allPacketsSuccessFlag = false;
                    for(int packets = 0; packets < trafficFlowList[tfIndex].numberOfConfigurations; packets++){
                        int packetLeft = alternativeOffset + transmissionPeriod * packets;
                        int packetRight = alternativeOffset + alternativeWidth - 1 + transmissionPeriod * packets;
                        int packetBottom = alternativeBottom;
                        int packetHeight = tmpHeight;
                        // collision check for one packet
                        bool collisionFlag = false;
                        for(int col = packetLeft; col <= packetRight; col++){
                            for(int row = packetBottom; row < packetBottom + packetHeight; row++){
                                comparsionCount++;
                                if(rg.data[col][row] != -1){
                                    // collision occurs
                                    collisionFlag = true;
                                    break;
                                }
                            }
                        }
                        if(collisionFlag){
                            // collision happened
                            break;
                        }
                        if(packets == trafficFlowList[tfIndex].numberOfConfigurations - 1){
                            allPacketsSuccessFlag = true;
                        }
                    }
                    if(allPacketsSuccessFlag){
                        // compare, keep the minimum one
                        if(alternativeBottom + tmpHeight < lowestSettingForAllFlat[2] + lowestSettingForAllFlat[3]){
                            lowestSettingForAllFlat[0] = alternativeOffset;
                            lowestSettingForAllFlat[1] = alternativeOffset + alternativeWidth - 1;
                            lowestSettingForAllFlat[2] = alternativeBottom;
                            lowestSettingForAllFlat[3] = tmpHeight;
                        }
                        break;
                    }
                }
            }
        }

        // after identifying the lowest setting, then update the resource grid
        // check the height
        if(lowestSettingForAllFlat[2] + lowestSettingForAllFlat[3] >= numberOfPilots){
            std::cout << "No solution1!!" << std::endl;
            return;
        }
        // update the information for all data packets
        for(int packets = 0; packets < trafficFlowList[tfIndex].numberOfConfigurations; packets++){
            rg.update(lowestSettingForAllFlat[0] + transmissionPeriod*packets,
                      lowestSettingForAllFlat[1] + transmissionPeriod*packets,
                      lowestSettingForAllFlat[2],
                      lowestSettingForAllFlat[3],
                      trafficFlowList[tfIndex].number);
        }
        // std::cout << "max pilot: " << topPilots << std::endl;
    }
    auto endTime = std::chrono::steady_clock::now();
    int elapsedTimeInMilliseconds =
            std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

    std::vector<int> topList;
    topList.push_back(0);
    std::string ss;

    // Step 5.4: display schedules
//    std::cout << std::endl;
//    std::cout << "Final schedules: " << std::endl;
//    std::vector<int> topList;
//    std::string ss;
    int maxPilot = 0;
    bool flagF = false;
    topList.clear();
    for(int rowIndex = numberOfPilots - 1; rowIndex >= 0; rowIndex--){
        ss = "";
        for(int colIndex = 0; colIndex < numberOfSlots; colIndex++){
            if(rg.data[colIndex][rowIndex] == -1){
                //ss += "0 ";
                continue;
            }
            else{
                // ss += std::to_string(rg.data[colIndex][rowIndex]);
                // ss += " ";
//                if(0 <= rg.data[colIndex][rowIndex] < distanceDataControl){
//
//                }
                // topList.push_back(rowIndex+1);
                maxPilot = rowIndex + 1;
                flagF = true;
                break;
            }
        }
        if(flagF) break;
    }
    // rg.printData();
    // int maxPilot = *std::max_element(topList.begin(), topList.end());
    std::cout << "Max pilot:" << maxPilot << std::endl;
    std::ofstream outlog;
    outlog.open("../Results/" + std::to_string(f) + "/out_" + std::to_string(indexOfTF) + ".txt");
    std::ofstream heatMapLog, heatMapLog_b;
    heatMapLog.open("../Results/heatmap.txt", std::ios_base::app);
    heatMapLog_b.open("../Results/heatmap_b.txt", std::ios_base::app);

    int i, j;
    int ru = 0;
    int ruu = 0;
    int rowru = 0;
    int rowrub = 0; // before control message
    // std::cout << "Current status of the resource grid: " << std::endl;
    // for(i = numberOfPilots - 1;i >= 0; i--){
    for(i = 0;i <= numberOfPilots - 1; i++){
        rowru = 0;
        rowrub = 0;
        for(j=0;j<numberOfSlots;j++){
            char parsed;
            if(rg.data[j][i] == -1){
                parsed = '0';
            }else{
                parsed = '1';
                rowru += 1;
                if(rg.data[j][i] <= distanceDataControl){
                    ru += 1;
                    rowrub += 1;
                }
                ruu += 1;
                // parsed = 'A'+rg.data[j][i];
            }
            outlog << parsed << " ";
        }
        outlog << std::endl;
        heatMapLog << (double) rowru / numberOfSlots << " ";
        heatMapLog_b << (double) rowrub / numberOfSlots << " ";
    }
    heatMapLog << std::endl;
    heatMapLog_b << std::endl;
    // std::cout << ru << std::endl;
    int totalru = 0;
    int totalPackets = 0;

    for(auto tf: trafficFlowList){
        totalru += tf.payload * tf.numberOfConfigurations;
        totalPackets += tf.numberOfConfigurations;
    }
    int totalConfigurations = numberOfTFs;
//    int flag=0;
//    if(totalru > ru){
//        flag = 1;
//    }
    // rg.printData();

    std::cout << "Time (ms): "<<elapsedTimeInMilliseconds << std::endl;
    double efficiency = (double) ru / (maxPilot * numberOfSlots);
    double utilization = (double) ruu / (maxPilot * numberOfSlots);
    std::cout << "Efficiency: " << efficiency << std::endl;
    std::fstream outLog;
    outLog.open("../Results/output1.txt", std::ios_base::app);
    outLog  << std::to_string(maxPilot) << " "
            << std::to_string(elapsedTimeInMilliseconds) << " "
            << std::to_string(efficiency) << " "
            << std::to_string(utilization) << " "
            << std::to_string(totalPackets) << " "
            << std::to_string(totalConfigurations) << " "
            << std::endl;
    outlog.clear();
    outlog.flush();
}

/*
 * parameter: f --> number of traffic flows
 */
void performCo1(int f){
    cfgInformation.clear();
    extractCfg(f);
    double pru_ratio_list[] = {1, 0.75, 0.5, 0.25, 0.1, 0};
    // double pru_ratio_list[] = {1};
    if(ifTest){
        Co1(testSample, f);
    } else{
        for (std::string &cfg: cfgInformation){
            Co1(cfg, f);
        }
    }
}