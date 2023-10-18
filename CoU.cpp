//
// Created by yunpa38 on 2022-10-12.
// Implementation for "Co1".
//
#include "CoU.h"

/*
 * Used to store the scheduling information for each group
 */
struct GroupSchedulingStruct{
    int left;       // real left, but only for the first packet in the group
    int right;      // real right, but only for the first packet in the group
    int bottom;
    int height;
    int period;             // period of the packets
    int startPacketIndex;   // used to calculate the basic offset of the current group
    int numberOfPackets;    // covered number of packets in the group
};

/*
 * A higher layer of structure to store the temporary scheduling information
 * When trying to combine the two consecutive groups, use one "HyperGroupSchedulingStruct"
 * to store the information for the current number of groups
 * The selection of the combination having the smallest impacts on the scheduling is performed
 * in the main process.
 */
struct HyperGroupSchedulingStruct{
    vector<GroupSchedulingStruct> groupSchedulingList;
    double objectiveFunctionValue;
    bool reducedControl;
    int gss1offset;
    int gss2offset;
    int gsscombinedossfet;
    vector<pair<int, int>> PRUList;
};

/*
 * For combination profit:
 *      - calculate the increase of PRUs
 */
int combinationPRUs(GroupSchedulingStruct& gssCombined,
                    GroupSchedulingStruct& gss1,
                    GroupSchedulingStruct& gss2,
                    ResourceGrid& rg){
    int groupOffsetIndex, groupEndIndex, groupTop, groupPeriod, groupLeft, groupRight;
    int numberOfWastedResourceUnits = 0;    // return value; newly wasted resource units after combination
    int combinationWastedResourceUnits = 0;
    int previousWastedResourceUnits = 0;    // for gss1 and gss2
    int rowIndex, colIndex;

    // calculate the previous wasted resource units [gss1 and gss2]
    // for gss1
    groupLeft = gss1.left;
    groupRight = gss1.right;
    groupTop = gss1.bottom;
    groupPeriod = gss1.period;
    for(int packets = 0; packets < gss1.numberOfPackets; packets++){
        groupOffsetIndex = groupLeft + packets * groupPeriod;
        groupEndIndex = groupRight + packets * groupPeriod;
        for(rowIndex = 0; rowIndex < groupTop; rowIndex++){
            for(colIndex = groupOffsetIndex; colIndex <= groupEndIndex; colIndex++){
                if(rg.data[colIndex][rowIndex] == -1){
                    previousWastedResourceUnits++;
                }
            }
        }
    }
    // gss2
    groupLeft = gss2.left;
    groupRight = gss2.right;
    groupTop = gss2.bottom;
    groupPeriod = gss2.period;
    for(int packets = 0; packets < gss2.numberOfPackets; packets++){
        groupOffsetIndex = groupLeft + packets * groupPeriod;
        groupEndIndex = groupRight + packets * groupPeriod;
        for(rowIndex = 0; rowIndex < groupTop; rowIndex++){
            for(colIndex = groupOffsetIndex; colIndex <= groupEndIndex; colIndex++){
                if(rg.data[colIndex][rowIndex] == -1){
                    previousWastedResourceUnits++;
                }
            }
        }
    }

    // gssCombined
    groupLeft = gssCombined.left;
    groupRight = gssCombined.right;
    groupTop = gssCombined.bottom;
    groupPeriod = gssCombined.period;
    for(int packets = 0; packets < gssCombined.numberOfPackets; packets++){
        groupOffsetIndex = groupLeft + packets * groupPeriod;
        groupEndIndex = groupRight + packets * groupPeriod;
        for(rowIndex = 0; rowIndex < groupTop; rowIndex++){
            for(colIndex = groupOffsetIndex; colIndex <= groupEndIndex; colIndex++){
                if(rg.data[colIndex][rowIndex] == -1){
                    combinationWastedResourceUnits++;
                }
            }
        }
    }
    // cout << "PRUs: combined: " << combinationWastedResourceUnits << " gss12: " << previousWastedResourceUnits<< endl;
    return combinationWastedResourceUnits - previousWastedResourceUnits;
}

/*
 * For combination profit:
 *      - calculate the increase of DRUs (for the increase of payload)
 */
int combinationDRUs(GroupSchedulingStruct& gssCombined,
                    GroupSchedulingStruct& gss1,
                    GroupSchedulingStruct& gss2){
    int width, height, drugss1, drugss2, drucombined;
    // DRU for gss 1
    width = gss1.right - gss1.left + 1;
    height = gss1.height;
    drugss1 = width * height * gss1.numberOfPackets;

    // DRUs for gss 2
    width = gss2.right - gss2.left + 1;
    height = gss2.height;
    drugss2 = width * height * gss2.numberOfPackets;

    // DRUs for combined
    width = gssCombined.right - gssCombined.left + 1;
    height = gssCombined.height;
    drucombined = width * height * gssCombined.numberOfPackets;

    return drucombined - (drugss1 + drugss2);
}

/*
 * For combination profit:
 *      - calculate the decline of control message
 *      - if gss1 and gss2 can be represented by one configuration (must be the same with gss combined), return 0
 *      - otherwise, return the size of the control message
 *
 *      The problem: how to identify two gss belonging to one configuration
 *      Idea: 1) offsets of gss1 and gss2 are the same with gsscombined
 *              2) width
 *              3) height
 *
 *      return value:
 *          - true: the combination would reduce one control message (gss1 and gss2 can not be represented by 1 conf.)
 *          - false: the combination has no contribution to the reduction of control message
 *
 */
bool combinationCRUs(GroupSchedulingStruct& gssCombined,
                     GroupSchedulingStruct& gss1,
                     GroupSchedulingStruct& gss2){

    // width
    int widthgss1 = gss1.right - gss1.left + 1;
    int widthgss2 = gss2.right - gss2.left + 1;
    int widthgsscombined = gssCombined.right - gssCombined.left + 1;
    if(widthgss1!= widthgsscombined || widthgss2 != widthgsscombined){
        return true;
    }else{
        // height
        if(gss1.height != gssCombined.height || gss2.height != gssCombined.height){
            return true;
        }else{
            // the offset of each data packet in gsscombined = ... in gss1 + ... in gss2
            // gss1
            for(int i = 0; i < gss1.numberOfPackets; i++){
                int ogss1 = gss1.left + gss1.period * i;
                int ogsscombined = gssCombined.left + gssCombined.period * i;
                if(ogss1 != ogsscombined){
                    return true;
                }
            }
            // gss2
            for(int i = 0; i < gss2.numberOfPackets; i++){
                int ogss2 = gss2.left + gss2.period * i;
                int ogsscombined = gssCombined.left + gssCombined.period * (i + gss1.numberOfPackets);
                if(ogss2 != ogsscombined){
                    return true;
                }
            }
            return false;
        }
    }
}

/*
 * assess the quality of the combination
 * return value: - (decline of control message - increase of PRUs - increase of DRUs)
 */
double combinationObjectiveFunction(GroupSchedulingStruct& gssCombined,
                                    GroupSchedulingStruct& gss1,
                                    GroupSchedulingStruct& gss2,
                                    ResourceGrid& rg,
                                    int controlMessageSize,
                                    double ratio_PRU){
    // TODO: combination objective:
    // 1) newly increased hard-utilized resource;
    // 2) new increased wasted resources (payload)
    // 3): the decrease of control message
    int increaseOfPRUs = combinationPRUs(gssCombined, gss1, gss2, rg);
    int increaseOfDRUs = combinationDRUs(gssCombined, gss1, gss2);
    bool declineControlMessage = combinationCRUs(gssCombined, gss1, gss2);
//    cout << " gss1 o: " << gss1.left << " w: " << gss1.right - gss1.left + 1 << " h: " << gss1.height << endl;
//    cout << " gss2 o: " << gss2.left << " w: " << gss2.right - gss2.left + 1 << " h: " << gss2.height << endl;
//    cout << " gssc o: " << gssCombined.left <<
//    " w: " << gssCombined.right - gssCombined.left + 1 <<
//    " h: " << gssCombined.height << endl;
//    cout << "cov: "<< increaseOfPRUs << " " << increaseOfDRUs << endl;
    if(declineControlMessage){
        return increaseOfPRUs * ratio_PRU + increaseOfDRUs - controlMessageSize;
    }else{
        return increaseOfPRUs * ratio_PRU + increaseOfDRUs;
    }
}

// 4) increase of RB
// 5) closeness between gssCombinedTop and currentRGTop
double combinationRBsV3(GroupSchedulingStruct& gssCombined,
                                GroupSchedulingStruct& gss1,
                                GroupSchedulingStruct& gss2,
                                int currentRGTop){
    double gssTop = gssCombined.bottom + gssCombined.height;
//    double gssPreviousTop1 = gss1.bottom + gss1.height;
//    double gssPreviousTop2 = gss2.bottom + gss2.height;
    if(gssTop > currentRGTop){
        return RBIncreaseRatio*(gssTop - currentRGTop);
    }
//    else{
//        // close to currentRGTop --> better --> smaller value
//        return RBIncreaseRatio * (currentRGTop - gssTop);
//    }
//    if(gssTop > gssPreviousTop1){
//        tmpIncreaseRB += gss1.numberOfPackets * (gssTop - gssPreviousTop1);
//    }
//    if(gssTop > gssPreviousTop2){
//        tmpIncreaseRB += gss2.numberOfPackets * (gssTop - gssPreviousTop2);
//    }
    // cout << tmpIncreaseRB << endl;
}

double combinationObjectiveFunctionV3(GroupSchedulingStruct& gssCombined,
                                    GroupSchedulingStruct& gss1,
                                    GroupSchedulingStruct& gss2,
                                    ResourceGrid& rg,
                                    int controlMessageSize,
                                    double ratio_PRU,
                                    int currentRGTop){
    // TODO: combination objective:
    // 1) newly increased hard-utilized resource;
    // 2) new increased wasted resources (payload)
    // 3) the decrease of control message
    // 4) increase of RB
    // 5) closeness between gssCombinedTop and currentRGTop
    int increaseOfPRUs = combinationPRUs(gssCombined, gss1, gss2, rg);
    int increaseOfDRUs = combinationDRUs(gssCombined, gss1, gss2);
    bool declineControlMessage = combinationCRUs(gssCombined, gss1, gss2);
    // double RBs = combinationRBsV3(gssCombined, gss1, gss2, currentRGTop);


//    cout << " gss1 o: " << gss1.left << " w: " << gss1.right - gss1.left + 1 << " h: " << gss1.height << endl;
//    cout << " gss2 o: " << gss2.left << " w: " << gss2.right - gss2.left + 1 << " h: " << gss2.height << endl;
//    cout << " gssc o: " << gssCombined.left <<
//    " w: " << gssCombined.right - gssCombined.left + 1 <<
//    " h: " << gssCombined.height << endl;
//    cout << "cov: "<< increaseOfPRUs << " " << increaseOfDRUs << endl;
//    if(declineControlMessage){
//        return increaseOfPRUs * ratio_PRU + increaseOfDRUs - controlMessageSize + RBs;
//    }else{
//        return increaseOfPRUs * ratio_PRU + increaseOfDRUs + RBs;
//    }
    if(declineControlMessage){
        return increaseOfPRUs * ratio_PRU + increaseOfDRUs - controlMessageSize;
    }else{
        return increaseOfPRUs * ratio_PRU + increaseOfDRUs;
    }
    // return RBs;

        // return increaseofRB;
}

/*
 * Function: for a given group scheduling list, return the objective function
 * suppose that the scheduling has been applied
 */
//int groupSchedulingObjectiveFunction(vector<GroupSchedulingStruct>& groupSchedulingList,
//                                                 ResourceGrid& rg, TrafficFlow& tf){
//    int groupOffsetIndex, groupEndIndex, groupTop, groupPeriod, groupPackets, groupLeft, groupRight;
//    int rowIndex, colIndex;
//    int wastageResourceUnits = 0;
//
//    for(auto gss: groupSchedulingList){
//        // calculate the objective function
//
//        groupLeft = gss.left;
//        groupRight = gss.right;
//        groupTop = gss.bottom;
//        groupPackets = gss.numberOfPackets;
//        groupPeriod = gss.period;
//
//        for(int packets = 0; packets < groupPackets; packets++){
//            groupOffsetIndex = groupLeft + packets * groupPeriod;
//            groupEndIndex = groupRight + packets * groupPeriod;
//            // resource units wastage...
//            for(rowIndex = 0; rowIndex < groupTop; rowIndex++){
//                for(colIndex = groupOffsetIndex; colIndex <= groupEndIndex; colIndex++){
//                    if(rg.data[colIndex][rowIndex] == -1){
//                        wastageResourceUnits++;
//                    }
//                }
//            }
//        }
//
//    }
//    // calculate the sum of wasted resource units and control messages
//    // TODO: balance the wastage of resource units and the involved control overhead
//    // TODO: the increase of the resource units larger than the payload
//
//    return  wastageResourceUnits + (groupSchedulingList.size()-1) * tf.controlOverhead;
//}

/*
 * FUNCTION: perform CoN algorithm (with higher degree of exploration using combination based approach)
 * compare to V0, use Co1 to replace CoU at the very beginning (the schedule information for each packet)
 */

void subCoNV1(TrafficFlow& tf, ResourceGrid& rg, double rwi, double ratio_PRU){
    std::vector<std::pair<int, int>> schedulingIntervals;
    for(int i = 0; i < rg.numberOfSlots / tf.transmissionPeriod; i++){
        schedulingIntervals.emplace_back(
                std::make_pair(ceil(tf.initialOffset + tf.transmissionPeriod * i),
                               floor(tf.initialOffset +tf.transmissionPeriod * i + tf.latencyRequirement - 1)));
    }

//    for(auto si: schedulingIntervals){
//        std::cout << si.first << " " << si.second << std::endl;
//    }
    tf.schedulingInterval = schedulingIntervals;
    vector<GroupSchedulingStruct> groupSchedulingList;
    vector<HyperGroupSchedulingStruct> hyperSchedulingList;
    int packetIndex = 0;
    rwi = 1;
    // step 1: for all packets, group size = 1, find the most appropriate location for all packets
    // this is the optimal solution [the highest resource efficiency]
    // auto startTime_initialCo1 = std::chrono::steady_clock::now();
    for(auto intervals: schedulingIntervals) {
        // Initialization of the current group scheduling struct
        // cout << "intervals..." << endl;
        GroupSchedulingStruct gss{};
        gss.left = 0;
        gss.right = 0;
        gss.bottom = 0;
        gss.height = 999999;
        gss.period = tf.transmissionPeriod;
        gss.numberOfPackets = 1;
        gss.startPacketIndex = packetIndex;
        packetIndex++;

        int lowestSettingForAllFlat [] = {0, 0, 0, 99999};

        // width max
        int WidthMax;
        int freeSpace = intervals.second - intervals.first + 1;
        int h = ceil((double) tf.payload / freeSpace);
        if (h == 1) {
            WidthMax = tf.payload;
        } else {
            WidthMax = intervals.second - intervals.first + 1;
        }
        // FUNCTION: calculate the minimum height for all shapes w.r.t the current flat
        int offsetMin = intervals.first;
        int offsetMax = intervals.second;
        int bottomMin = 0;
        int widthMax = WidthMax;
        // cout << "Width: " << widthMax << endl;
        int transmissionPeriod = tf.transmissionPeriod;

        for (int alternativeWidth = 1; alternativeWidth <= widthMax; alternativeWidth++) {
            int tmpHeight = (int) ceil((double) tf.payload / alternativeWidth);
            // cout << "Height: " << tmpHeight << endl;
            if(tmpHeight >= rg.numberOfPilots) continue;
            int tmpOffsetMax = offsetMax - alternativeWidth + 1;
            for (int alternativeOffset = tmpOffsetMax; alternativeOffset >= offsetMin; alternativeOffset--) {
                for (int alternativeBottom = bottomMin; alternativeBottom < rg.numberOfPilots; alternativeBottom++) {
                    // check collision for all packets
                    bool allPacketsSuccessFlag = false;
                    for (int packets = 0; packets < 1; packets++) {
                        int packetLeft = alternativeOffset + transmissionPeriod * packets;
                        int packetRight = alternativeOffset + alternativeWidth - 1 + transmissionPeriod * packets;
                        int packetBottom = alternativeBottom;
                        int packetHeight = tmpHeight;
                        // collision check for one packet
                        bool collisionFlag = false;
                        for (int col = packetLeft; col <= packetRight; col++) {
                            for (int row = packetBottom; row < packetBottom + packetHeight; row++) {
                                if (rg.data[col][row] != -1) {
                                    // collision occurs
                                    collisionFlag = true;
                                    break;
                                }
                            }
                        }
                        if (collisionFlag) {
                            // collision happened
                            break;
                        }
                        if (packets == 0) {
                            allPacketsSuccessFlag = true;
                        }
                    }
                    if (allPacketsSuccessFlag) {
                        // compare, keep the minimum one
                        if (alternativeBottom + tmpHeight < lowestSettingForAllFlat[2] + lowestSettingForAllFlat[3]) {
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
        gss.left = lowestSettingForAllFlat[0];
        gss.right = lowestSettingForAllFlat[1];
        gss.bottom = lowestSettingForAllFlat[2];
        gss.height = lowestSettingForAllFlat[3];
        // cout << "l: " << gss.left << " r: " << gss.right << " b: " << gss.bottom << " h: " << gss.height << endl;
        groupSchedulingList.push_back(gss);
    }
    // initial part -- has impact, but no big

    // auto startTime_interation = std::chrono::steady_clock::now();
    // int groupSchedulingObjectiveFunctionValue = groupSchedulingObjectiveFunction(groupSchedulingList, rg, tf);
    double scheduleProfit = 0;
    hyperSchedulingList.push_back(
            {groupSchedulingList, scheduleProfit});

    // cout << "step 2" << endl;
    /*
     * Step 2: try to combine two adjacent groups, and find the most efficient combination, then store
     * the newly generated scheduling info to the hyperSchedulingList
     * The current scheduling info is in groupSchedulingList
     *
     * IMPORTANT: establish the group combination objective function
     *              [wasted resource units -- hard to utilize in the future]
    */
    int totalNumberOfPackets = tf.numberOfConfigurations;
    // n groups --> n-1 groups --> n-2 groups --> .... --> 1 groups
    // total n-1 hyper scheduling info
    for (int hyperIteration = totalNumberOfPackets - 1; hyperIteration >= 1; hyperIteration--) {
        // cout << "\n hyper iteration.." << hyperIteration << endl;
        // for each hyper-iteration, try to select the most appropriate combination of groups
        // cout << "----------new combination loop----------------" << endl;
        int minGroupIndex = -1;     // record the index of the first group in the combination, the next is trivial
        double minCombinationObjectiveValue = 99999;
        int minGss1Offset, minGss2Offset, minGssCombinedOffset;
        bool minReduceControl;

        GroupSchedulingStruct minGss{};
        for (int combinationIndex = 0; combinationIndex < hyperIteration; combinationIndex++) {
            // cout << "combinationIndex: " << combinationIndex << endl;
            // try to combine groups, assess the combination objective function, and select one
            int startPacketIndex = groupSchedulingList[combinationIndex].startPacketIndex;
            int numberOfCoveredPackets = groupSchedulingList[combinationIndex].numberOfPackets +
                                         groupSchedulingList[combinationIndex + 1].numberOfPackets;

            // try to combine....
            GroupSchedulingStruct gss{};
            gss.startPacketIndex = startPacketIndex;
            gss.numberOfPackets = numberOfCoveredPackets;
            int scheduleIntervalStart = schedulingIntervals[0].first;
            int scheduleIntervalEnd = schedulingIntervals[0].second;
            // for each flat, calculate the minimal height to identify the shape
            // Format: left, right, bottom, height, period
            int lowestSettingForAllFlat[] = {0, 0, 0, 99999, 0};

            // width max
            int WidthMax;
            int freeSpace = scheduleIntervalEnd - scheduleIntervalStart + 1;
            int h = ceil((double) tf.payload / freeSpace);
            if (h == 1) {
                WidthMax = tf.payload;
            } else {
                WidthMax = scheduleIntervalEnd - scheduleIntervalStart + 1;
            }

            // FUNCTION: calculate the minimum height for all shapes w.r.t the current flat
            int offsetMin = scheduleIntervalStart;
            int offsetMax = scheduleIntervalEnd;
            int bottomMin = 0;
            int widthMax;
            int theoritcalWidth = (int) ceil(rwi * (double) tf.payload);
            if (WidthMax <= theoritcalWidth) widthMax = WidthMax;
            else widthMax = theoritcalWidth;

            // cout << "Width: " << widthMax << endl;
            int transmissionPeriod = tf.transmissionPeriod;
            int startPacket = 0;
            int endPacket = numberOfCoveredPackets;
            int numberOfPacketsInTheSegment = numberOfCoveredPackets;
            // record the number of packets been assigned resources
            int assignedNumberOfPackets = startPacketIndex;

            for (int alternativeWidth = 1; alternativeWidth <= widthMax; alternativeWidth++) {
                int tmpHeight = (int) ceil((double) tf.payload / alternativeWidth);
                // cout << "Height: " << tmpHeight << endl;
                int tmpOffsetMax = offsetMax - alternativeWidth + 1;
                for (int alternativeOffset = tmpOffsetMax; alternativeOffset >= offsetMin; alternativeOffset--) {
                    // decide the period of packets
                    int minP = transmissionPeriod -
                               (int) floor((double) (alternativeOffset - offsetMin) / numberOfPacketsInTheSegment);
                    int maxP = transmissionPeriod + (int) floor(
                            (double) (offsetMax - (alternativeOffset + alternativeWidth - 1)) /
                            numberOfPacketsInTheSegment);
                    for (int alternativePeriod = minP; alternativePeriod <= maxP; alternativePeriod++) {
                        for (int alternativeBottom = bottomMin;
                             alternativeBottom < rg.numberOfPilots; alternativeBottom++) {
                            // check collision for all packets
                            bool allPacketsSuccessFlag = false;
                            for (int packets = startPacket; packets < endPacket; packets++) {
                                int packetLeft = assignedNumberOfPackets * transmissionPeriod
                                                 + alternativeOffset + alternativePeriod * packets;
                                int packetRight = assignedNumberOfPackets * transmissionPeriod +
                                                  alternativeOffset + alternativeWidth - 1 +
                                                  alternativePeriod * packets;
                                int packetBottom = alternativeBottom;
                                int packetHeight = tmpHeight;
                                // collision check for one packet
                                bool collisionFlag = false;
                                for (int col = packetLeft; col <= packetRight; col++) {
                                    for (int row = packetBottom; row < packetBottom + packetHeight; row++) {
                                        if (rg.data[col][row] != -1) {
                                            // collision occurs
                                            // cout << "collision: row: " << row << " col: " << col << endl;
                                            // rg.printData();
                                            collisionFlag = true;
                                            break;
                                        }
                                    }
                                    if (collisionFlag) {
                                        // collision happened
                                        break;
                                    }
                                }
                                if (collisionFlag) {
                                    // collision happened
                                    break;
                                }
                                if (packets == endPacket - 1) {
                                    allPacketsSuccessFlag = true;
                                }
                            }
                            if (allPacketsSuccessFlag) {
                                // compare, keep the minimum one
                                if (alternativeBottom + tmpHeight <
                                    lowestSettingForAllFlat[2] + lowestSettingForAllFlat[3]) {
                                    lowestSettingForAllFlat[0] = assignedNumberOfPackets * transmissionPeriod
                                                                 + alternativeOffset;
                                    lowestSettingForAllFlat[1] = assignedNumberOfPackets * transmissionPeriod
                                                                 + alternativeOffset + alternativeWidth - 1;
                                    lowestSettingForAllFlat[2] = alternativeBottom;
                                    lowestSettingForAllFlat[3] = tmpHeight;
                                    lowestSettingForAllFlat[4] = alternativePeriod;
                                    // update gss
                                    gss.left = lowestSettingForAllFlat[0];
                                    gss.right = lowestSettingForAllFlat[1];
                                    gss.bottom = lowestSettingForAllFlat[2];
                                    gss.height = lowestSettingForAllFlat[3];
                                    gss.period = lowestSettingForAllFlat[4];
                                }
                                break;
                            }
                        }
                    }
                }
            }
            // obtain a new group scheduling structure, calculate the combination objective function
            // cout << "pre object value..." << endl;
            double combinationProfit =
                    combinationObjectiveFunction(gss,
                                                 groupSchedulingList[combinationIndex],
                                                 groupSchedulingList[combinationIndex + 1],
                                                 rg,
                                                 tf.controlOverhead,
                                                 ratio_PRU);
            bool currentReducedCURs = combinationCRUs(gss,
                                                      groupSchedulingList[combinationIndex],
                                                      groupSchedulingList[combinationIndex + 1]);
//            cout << "combination objective value: " << combinationProfit
//            << "\t" << currentReducedCURs << endl;
            if (combinationProfit < minCombinationObjectiveValue) {
                // record the current combination
                minCombinationObjectiveValue = combinationProfit;
                minGroupIndex = combinationIndex;
                minGss = gss;
                minGss1Offset = groupSchedulingList[combinationIndex].left;
                minGss2Offset = groupSchedulingList[combinationIndex + 1].left;
                minGssCombinedOffset = gss.left;
                minReduceControl = currentReducedCURs;
            }
        }
        // obtain the current optimal combination, update the current group scheduling list
//        cout << "original list: " << endl;
//        for (auto ele: groupSchedulingList){
//            cout << ele.left << " ";
//        }
//        cout << endl;
        groupSchedulingList.erase(groupSchedulingList.begin() + minGroupIndex);
//        cout << "delete first..." << endl;
//        for (auto ele: groupSchedulingList){
//            cout << ele.left << " ";
//        }
//        cout << endl;
        groupSchedulingList.erase(groupSchedulingList.begin() + minGroupIndex);
//        cout << "delete second..." << endl;
//        for (auto ele: groupSchedulingList){
//            cout << ele.left << " ";
//        }
//        cout << endl;
        groupSchedulingList.insert(groupSchedulingList.begin() + minGroupIndex, minGss);
//        cout << "add new one..." << endl;
//        for (auto ele: groupSchedulingList){
//            cout << ele.left << " ";
//        }
//        cout << endl;
        //groupSchedulingObjectiveFunctionValue = groupSchedulingObjectiveFunction(groupSchedulingList, rg, tf);
        scheduleProfit += minCombinationObjectiveValue;
        hyperSchedulingList.push_back(
                {groupSchedulingList, scheduleProfit,
                 minReduceControl,
                 minGss1Offset,
                 minGss2Offset,
                 minGssCombinedOffset});
        // cout << "group scheduling objective function: " << groupSchedulingObjectiveFunctionValue << endl;
    }
//    auto endTime = std::chrono::steady_clock::now();
//    int elapsedTimeInMilliseconds =
//            std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime_interation).count();
//    cout << "iteration part: " << elapsedTimeInMilliseconds << endl;

    // update part: pruRatio = 0, consume less time, due to fewer configurations
    // cout << "Having all hyper scheduling list:..." << endl;
    // select the one with the smallest objective function
    int minHyperSchedulingIndex = 0, minHyperSchedulingObjectiveValue = 999999999;
    for (int hyperSchedulingIndex = 0; hyperSchedulingIndex < hyperSchedulingList.size(); hyperSchedulingIndex++) {
        // cout << "size of group scheduling.." << hyperSchedulingList[hyperSchedulingIndex].groupSchedulingList.size() << endl;
        if (hyperSchedulingList[hyperSchedulingIndex].objectiveFunctionValue < minHyperSchedulingObjectiveValue) {
            minHyperSchedulingIndex = hyperSchedulingIndex;
            minHyperSchedulingObjectiveValue = hyperSchedulingList[hyperSchedulingIndex].objectiveFunctionValue;
        }
    }
    int numberOfActualConfigurations = 1;
    set<int> configurationOffsets;
    // cout << "hyperschedule size: " << hyperSchedulingList.size() << " min index: " << minHyperSchedulingIndex << endl;
    for(int iSchedules = hyperSchedulingList.size() - 1; iSchedules > minHyperSchedulingIndex; iSchedules--){
        if(hyperSchedulingList[iSchedules].reducedControl){
            configurationOffsets.insert(hyperSchedulingList[iSchedules].gss1offset);
            configurationOffsets.insert(hyperSchedulingList[iSchedules].gss2offset);
//            cout << "add offsets: " << hyperSchedulingList[iSchedules].gss1offset
//            << " " << hyperSchedulingList[iSchedules].gss2offset << endl;
            numberOfActualConfigurations++;
        }else{
            configurationOffsets.insert(hyperSchedulingList[iSchedules].gsscombinedossfet);
        }
    }
    configurationOffsets.insert(hyperSchedulingList[minHyperSchedulingIndex].gsscombinedossfet);
//    cout << "min schedule index: " << minHyperSchedulingIndex << endl;
//    cout << "Number of configurations: " << numberOfActualConfigurations << endl;
    // update the resource grid
    // cout << "size of set: " << configurationOffsets.size() << endl;
    int groupOffsetIndex, groupEndIndex, groupPeriod, groupPackets, groupLeft, groupRight;
    tf.numberOfConfigurationActual = numberOfActualConfigurations - 1;
    tf.offsetConfigurationsActual.assign(configurationOffsets.begin(), configurationOffsets.end());
    sort(tf.offsetConfigurationsActual.begin(), tf.offsetConfigurationsActual.end());
    tf.offsetConfigurationsActual.erase(tf.offsetConfigurationsActual.begin());

    int gssIndex = 0;
    for (auto gss: hyperSchedulingList[minHyperSchedulingIndex].groupSchedulingList) {
        groupLeft = gss.left;
        groupRight = gss.right;
        groupPackets = gss.numberOfPackets;
        groupPeriod = gss.period;
//        if (gssIndex > 0) {
//            tf.offsetConfigurationsActual.push_back(groupLeft);
//        }
        gssIndex++;
        // cout << "aaa" << endl;
        for (int packets = 0; packets < groupPackets; packets++) {
            groupOffsetIndex = groupLeft + packets * groupPeriod;
            groupEndIndex = groupRight + packets * groupPeriod;
            rg.update(groupOffsetIndex, groupEndIndex, gss.bottom, gss.height, tf.number);
            scheduleInfoElement s{groupOffsetIndex, groupEndIndex, gss.bottom, gss.height};
            tf.schedulingInfo.push_back(s);
            // rg.printData();
        }
        // cout << "bbb" << endl;
    }
}

void subCoNV3(TrafficFlow& tf, ResourceGrid& rg, double rwi, double ratio_PRU, int& currentRGTop, std::ofstream& outlog, int tfnumber){
    std::vector<std::pair<int, int>> schedulingIntervals;
    for(int i = 0; i < rg.numberOfSlots / tf.transmissionPeriod; i++){
        schedulingIntervals.emplace_back(
                std::make_pair(ceil(tf.initialOffset + tf.transmissionPeriod * i),
                               floor(tf.initialOffset +tf.transmissionPeriod * i + tf.latencyRequirement - 1)));
    }

//    for(auto si: schedulingIntervals){
//        std::cout << si.first << " " << si.second << std::endl;
//    }
    tf.schedulingInterval = schedulingIntervals;
    vector<GroupSchedulingStruct> groupSchedulingList;
    vector<HyperGroupSchedulingStruct> hyperSchedulingList;
    int packetIndex = 0;
    rwi = 1;
    // step 1: for all packets, group size = 1, find the most appropriate location for all packets
    // this is the optimal solution [the highest resource efficiency]
    // auto startTime_initialCo1 = std::chrono::steady_clock::now();
    for(auto intervals: schedulingIntervals) {
        // Initialization of the current group scheduling struct
        // cout << "intervals..." << endl;
        GroupSchedulingStruct gss{};
        gss.left = 0;
        gss.right = 0;
        gss.bottom = 0;
        gss.height = 999999;
        gss.period = tf.transmissionPeriod;
        gss.numberOfPackets = 1;
        gss.startPacketIndex = packetIndex;
        packetIndex++;

        int lowestSettingForAllFlat [] = {0, 0, 0, 99999};

        // width max
        int WidthMax;
        int freeSpace = intervals.second - intervals.first + 1;
        int h = ceil((double) tf.payload / freeSpace);
        if (h == 1) {
            WidthMax = tf.payload;
        } else {
            WidthMax = intervals.second - intervals.first + 1;
        }
        // FUNCTION: calculate the minimum height for all shapes w.r.t the current flat
        int offsetMin = intervals.first;
        int offsetMax = intervals.second;
        int bottomMin = 0;
        int widthMax = WidthMax;
        // cout << "Width: " << widthMax << endl;
        int transmissionPeriod = tf.transmissionPeriod;

        for (int alternativeWidth = 1; alternativeWidth <= widthMax; alternativeWidth++) {
            int tmpHeight = (int) ceil((double) tf.payload / alternativeWidth);
            // cout << "Height: " << tmpHeight << endl;
            if(tmpHeight >= rg.numberOfPilots) continue;
            int tmpOffsetMax = offsetMax - alternativeWidth + 1;
            for (int alternativeOffset = tmpOffsetMax; alternativeOffset >= offsetMin; alternativeOffset--) {
                for (int alternativeBottom = bottomMin; alternativeBottom < rg.numberOfPilots; alternativeBottom++) {
                    // check collision for all packets
                    bool allPacketsSuccessFlag = false;
                    for (int packets = 0; packets < 1; packets++) {
                        int packetLeft = alternativeOffset + transmissionPeriod * packets;
                        int packetRight = alternativeOffset + alternativeWidth - 1 + transmissionPeriod * packets;
                        int packetBottom = alternativeBottom;
                        int packetHeight = tmpHeight;
                        // collision check for one packet
                        bool collisionFlag = false;
                        for (int col = packetLeft; col <= packetRight; col++) {
                            for (int row = packetBottom; row < packetBottom + packetHeight; row++) {
                                if (rg.data[col][row] != -1) {
                                    // collision occurs
                                    collisionFlag = true;
                                    break;
                                }
                            }
                        }
                        if (collisionFlag) {
                            // collision happened
                            break;
                        }
                        if (packets == 0) {
                            allPacketsSuccessFlag = true;
                        }
                    }
                    if (allPacketsSuccessFlag) {
                        // compare, keep the minimum one
                        if (alternativeBottom + tmpHeight < lowestSettingForAllFlat[2] + lowestSettingForAllFlat[3]) {
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
        gss.left = lowestSettingForAllFlat[0];
        gss.right = lowestSettingForAllFlat[1];
        gss.bottom = lowestSettingForAllFlat[2];
        gss.height = lowestSettingForAllFlat[3];
        // cout << "l: " << gss.left << " r: " << gss.right << " b: " << gss.bottom << " h: " << gss.height << endl;
        if(gss.bottom+gss.height > currentRGTop){
            currentRGTop = gss.bottom + gss.height;
        }
        groupSchedulingList.push_back(gss);
        // update the current top index of RBs
    }


    int groupOffsetIndex, groupEndIndex, groupPeriod, groupPackets, groupLeft, groupRight;
    if(output_enabled){
        // update co1 per packet
        for (auto gss: groupSchedulingList) {
            groupLeft = gss.left;
            groupRight = gss.right;
            groupPackets = gss.numberOfPackets;
            groupPeriod = gss.period;
            for (int packets = 0; packets < groupPackets; packets++) {
                groupOffsetIndex = groupLeft + packets * groupPeriod;
                groupEndIndex = groupRight + packets * groupPeriod;
                rg.update(groupOffsetIndex, groupEndIndex, gss.bottom, gss.height, tf.number);
            }
        }
        outlog << char('A' + tfnumber) << " co1 per packet" << endl;
        int i, j;
        for(i = 0;i <= rg.numberOfPilots - 1; i++){
            for(j=0;j<rg.numberOfSlots;j++){
                char parsed;
                if(rg.data[j][i] == -1){
                    parsed = '0';
                }else{
                    parsed = 'A'+rg.data[j][i];
                }
                outlog << parsed << " ";
            }
            outlog << std::endl;
        }
        outlog << endl << endl << endl << endl;
        // rewind the update of rg
        for (auto gss: groupSchedulingList) {
            groupLeft = gss.left;
            groupRight = gss.right;
            groupPackets = gss.numberOfPackets;
            groupPeriod = gss.period;
            for (int packets = 0; packets < groupPackets; packets++) {
                groupOffsetIndex = groupLeft + packets * groupPeriod;
                groupEndIndex = groupRight + packets * groupPeriod;
                rg.update(groupOffsetIndex, groupEndIndex, gss.bottom, gss.height, -1);
            }
        }
    }

    // initial part -- has impact, but no big

    // auto startTime_interation = std::chrono::steady_clock::now();
    // int groupSchedulingObjectiveFunctionValue = groupSchedulingObjectiveFunction(groupSchedulingList, rg, tf);
    double scheduleProfit = 0;
    hyperSchedulingList.push_back(
            {groupSchedulingList, scheduleProfit});

    // cout << "step 2" << endl;
    /*
     * Step 2: try to combine two adjacent groups, and find the most efficient combination, then store
     * the newly generated scheduling info to the hyperSchedulingList
     * The current scheduling info is in groupSchedulingList
     *
     * IMPORTANT: establish the group combination objective function
     *              [wasted resource units -- hard to utilize in the future]
    */
    int totalNumberOfPackets = tf.numberOfConfigurations;
    // n groups --> n-1 groups --> n-2 groups --> .... --> 1 groups
    // total n-1 hyper scheduling info
    for (int hyperIteration = totalNumberOfPackets - 1; hyperIteration >= 1; hyperIteration--) {
        // cout << "\n hyper iteration.." << hyperIteration << endl;
        // for each hyper-iteration, try to select the most appropriate combination of groups
        // cout << "----------new combination loop----------------" << endl;
        int minGroupIndex = -1;     // record the index of the first group in the combination, the next is trivial
        double minCombinationObjectiveValue = 99999;
        int minGss1Offset, minGss2Offset, minGssCombinedOffset;
        bool minReduceControl;

        GroupSchedulingStruct minGss{};
        for (int combinationIndex = 0; combinationIndex < hyperIteration; combinationIndex++) {
            // cout << "combinationIndex: " << combinationIndex << endl;
            // try to combine groups, assess the combination objective function, and select one
            int startPacketIndex = groupSchedulingList[combinationIndex].startPacketIndex;
            int numberOfCoveredPackets = groupSchedulingList[combinationIndex].numberOfPackets +
                                         groupSchedulingList[combinationIndex + 1].numberOfPackets;

            // try to combine....
            GroupSchedulingStruct gss{};
            gss.startPacketIndex = startPacketIndex;
            gss.numberOfPackets = numberOfCoveredPackets;
            int scheduleIntervalStart = schedulingIntervals[0].first;
            int scheduleIntervalEnd = schedulingIntervals[0].second;
            // for each flat, calculate the minimal height to identify the shape
            // Format: left, right, bottom, height, period
            int lowestSettingForAllFlat[] = {0, 0, 0, 99999, 0};

            // width max
            int WidthMax;
            int freeSpace = scheduleIntervalEnd - scheduleIntervalStart + 1;
            int h = ceil((double) tf.payload / freeSpace);
            if (h == 1) {
                WidthMax = tf.payload;
            } else {
                WidthMax = scheduleIntervalEnd - scheduleIntervalStart + 1;
            }

            // FUNCTION: calculate the minimum height for all shapes w.r.t the current flat
            int offsetMin = scheduleIntervalStart;
            int offsetMax = scheduleIntervalEnd;
            int bottomMin = 0;
            int widthMax;
            int theoritcalWidth = (int) ceil(rwi * (double) tf.payload);
            if (WidthMax <= theoritcalWidth) widthMax = WidthMax;
            else widthMax = theoritcalWidth;

            // cout << "Width: " << widthMax << endl;
            int transmissionPeriod = tf.transmissionPeriod;
            int startPacket = 0;
            int endPacket = numberOfCoveredPackets;
            int numberOfPacketsInTheSegment = numberOfCoveredPackets;
            // record the number of packets been assigned resources
            int assignedNumberOfPackets = startPacketIndex;

            for (int alternativeWidth = 1; alternativeWidth <= widthMax; alternativeWidth++) {
                int tmpHeight = (int) ceil((double) tf.payload / alternativeWidth);
                // cout << "Height: " << tmpHeight << endl;
                int tmpOffsetMax = offsetMax - alternativeWidth + 1;
                for (int alternativeOffset = tmpOffsetMax; alternativeOffset >= offsetMin; alternativeOffset--) {
                    // decide the period of packets
                    int minP = transmissionPeriod -
                               (int) floor((double) (alternativeOffset - offsetMin) / numberOfPacketsInTheSegment);
                    int maxP = transmissionPeriod + (int) floor(
                            (double) (offsetMax - (alternativeOffset + alternativeWidth - 1)) /
                            numberOfPacketsInTheSegment);
                    for (int alternativePeriod = minP; alternativePeriod <= maxP; alternativePeriod++) {
                        for (int alternativeBottom = bottomMin;
                             alternativeBottom < rg.numberOfPilots; alternativeBottom++) {
                            // check collision for all packets
                            bool allPacketsSuccessFlag = false;
                            for (int packets = startPacket; packets < endPacket; packets++) {
                                int packetLeft = assignedNumberOfPackets * transmissionPeriod
                                                 + alternativeOffset + alternativePeriod * packets;
                                int packetRight = assignedNumberOfPackets * transmissionPeriod +
                                                  alternativeOffset + alternativeWidth - 1 +
                                                  alternativePeriod * packets;
                                int packetBottom = alternativeBottom;
                                int packetHeight = tmpHeight;
                                // collision check for one packet
                                bool collisionFlag = false;
                                for (int col = packetLeft; col <= packetRight; col++) {
                                    for (int row = packetBottom; row < packetBottom + packetHeight; row++) {
                                        if (rg.data[col][row] != -1) {
                                            // collision occurs
                                            // cout << "collision: row: " << row << " col: " << col << endl;
                                            // rg.printData();
                                            collisionFlag = true;
                                            break;
                                        }
                                    }
                                    if (collisionFlag) {
                                        // collision happened
                                        break;
                                    }
                                }
                                if (collisionFlag) {
                                    // collision happened
                                    break;
                                }
                                if (packets == endPacket - 1) {
                                    allPacketsSuccessFlag = true;
                                }
                            }
                            if (allPacketsSuccessFlag) {
                                // compare, keep the minimum one
                                if (alternativeBottom + tmpHeight <
                                    lowestSettingForAllFlat[2] + lowestSettingForAllFlat[3]) {
                                    lowestSettingForAllFlat[0] = assignedNumberOfPackets * transmissionPeriod
                                                                 + alternativeOffset;
                                    lowestSettingForAllFlat[1] = assignedNumberOfPackets * transmissionPeriod
                                                                 + alternativeOffset + alternativeWidth - 1;
                                    lowestSettingForAllFlat[2] = alternativeBottom;
                                    lowestSettingForAllFlat[3] = tmpHeight;
                                    lowestSettingForAllFlat[4] = alternativePeriod;
                                    // update gss
                                    gss.left = lowestSettingForAllFlat[0];
                                    gss.right = lowestSettingForAllFlat[1];
                                    gss.bottom = lowestSettingForAllFlat[2];
                                    gss.height = lowestSettingForAllFlat[3];
                                    gss.period = lowestSettingForAllFlat[4];
                                }
                                break;
                            }
                        }
                    }
                }
            }
            // obtain a new group scheduling structure, calculate the combination objective function
            // cout << "pre object value..." << endl;
            double combinationProfit =
                    combinationObjectiveFunctionV3(gss,
                                                 groupSchedulingList[combinationIndex],
                                                 groupSchedulingList[combinationIndex + 1],
                                                 rg,
                                                 tf.controlOverhead,
                                                 ratio_PRU,
                                                 currentRGTop);
            bool currentReducedCURs = combinationCRUs(gss,
                                                      groupSchedulingList[combinationIndex],
                                                      groupSchedulingList[combinationIndex + 1]);
//            cout << "combination objective value: " << combinationProfit
//            << "\t" << currentReducedCURs << endl;
            if (combinationProfit < minCombinationObjectiveValue) {
                // record the current combination
                minCombinationObjectiveValue = combinationProfit;
                minGroupIndex = combinationIndex;
                minGss = gss;
                minGss1Offset = groupSchedulingList[combinationIndex].left;
                minGss2Offset = groupSchedulingList[combinationIndex + 1].left;
                minGssCombinedOffset = gss.left;
                minReduceControl = currentReducedCURs;
            }
        }
        // obtain the current optimal combination, update the current group scheduling list
//        cout << "original list: " << endl;
//        for (auto ele: groupSchedulingList){
//            cout << ele.left << " ";
//        }
//        cout << endl;
        groupSchedulingList.erase(groupSchedulingList.begin() + minGroupIndex);
//        cout << "delete first..." << endl;
//        for (auto ele: groupSchedulingList){
//            cout << ele.left << " ";
//        }
//        cout << endl;
        groupSchedulingList.erase(groupSchedulingList.begin() + minGroupIndex);
//        cout << "delete second..." << endl;
//        for (auto ele: groupSchedulingList){
//            cout << ele.left << " ";
//        }
//        cout << endl;
        groupSchedulingList.insert(groupSchedulingList.begin() + minGroupIndex, minGss);
//        cout << "add new one..." << endl;
//        for (auto ele: groupSchedulingList){
//            cout << ele.left << " ";
//        }
//        cout << endl;
        //groupSchedulingObjectiveFunctionValue = groupSchedulingObjectiveFunction(groupSchedulingList, rg, tf);
        scheduleProfit += minCombinationObjectiveValue;
        hyperSchedulingList.push_back(
                {groupSchedulingList, scheduleProfit,
                 minReduceControl,
                 minGss1Offset,
                 minGss2Offset,
                 minGssCombinedOffset});


//        // print out every candidate schedule per hyperIteration
//        for (auto gss: groupSchedulingList) {
//            groupLeft = gss.left;
//            groupRight = gss.right;
//            groupPackets = gss.numberOfPackets;
//            groupPeriod = gss.period;
//            for (int packets = 0; packets < groupPackets; packets++) {
//                groupOffsetIndex = groupLeft + packets * groupPeriod;
//                groupEndIndex = groupRight + packets * groupPeriod;
//                rg.update(groupOffsetIndex, groupEndIndex, gss.bottom, gss.height, tf.number);
//            }
//        }
//
//        outlog << char('A' + tfnumber) << " candidate schedule: " << std::to_string(minCombinationObjectiveValue) << endl;
//        int i, j;
//        for(i = 0;i <= rg.numberOfPilots - 1; i++){
//            for(j=0;j<rg.numberOfSlots;j++){
//                char parsed;
//                if(rg.data[j][i] == -1){
//                    parsed = '0';
//                }else{
//                    parsed = 'A'+rg.data[j][i];
//                }
//                outlog << parsed << " ";
//            }
//            outlog << std::endl;
//        }
//        outlog << endl << endl << endl << endl;
//        // rewind the update of rg
//        for (auto gss: groupSchedulingList) {
//            groupLeft = gss.left;
//            groupRight = gss.right;
//            groupPackets = gss.numberOfPackets;
//            groupPeriod = gss.period;
//            for (int packets = 0; packets < groupPackets; packets++) {
//                groupOffsetIndex = groupLeft + packets * groupPeriod;
//                groupEndIndex = groupRight + packets * groupPeriod;
//                rg.update(groupOffsetIndex, groupEndIndex, gss.bottom, gss.height, -1);
//            }
//        }


        // cout << "group scheduling objective function: " << groupSchedulingObjectiveFunctionValue << endl;
    }
//    auto endTime = std::chrono::steady_clock::now();
//    int elapsedTimeInMilliseconds =
//            std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime_interation).count();
//    cout << "iteration part: " << elapsedTimeInMilliseconds << endl;

    // update part: pruRatio = 0, consume less time, due to fewer configurations
    // cout << "Having all hyper scheduling list:..." << endl;
    // select the one with the smallest objective function
    int minHyperSchedulingIndex = 0, minHyperSchedulingObjectiveValue = 999999999;
    for (int hyperSchedulingIndex = 0; hyperSchedulingIndex < hyperSchedulingList.size(); hyperSchedulingIndex++) {
        // cout << "size of group scheduling.." << hyperSchedulingList[hyperSchedulingIndex].groupSchedulingList.size() << endl;
        if (hyperSchedulingList[hyperSchedulingIndex].objectiveFunctionValue < minHyperSchedulingObjectiveValue) {
            minHyperSchedulingIndex = hyperSchedulingIndex;
            minHyperSchedulingObjectiveValue = hyperSchedulingList[hyperSchedulingIndex].objectiveFunctionValue;
        }
    }

    // TODO: refine the setting of each gss


    int numberOfActualConfigurations = 1;
    set<int> configurationOffsets;
    // cout << "hyperschedule size: " << hyperSchedulingList.size() << " min index: " << minHyperSchedulingIndex << endl;
    for(int iSchedules = hyperSchedulingList.size() - 1; iSchedules > minHyperSchedulingIndex; iSchedules--){
        if(hyperSchedulingList[iSchedules].reducedControl){
            configurationOffsets.insert(hyperSchedulingList[iSchedules].gss1offset);
            configurationOffsets.insert(hyperSchedulingList[iSchedules].gss2offset);
//            cout << "add offsets: " << hyperSchedulingList[iSchedules].gss1offset
//            << " " << hyperSchedulingList[iSchedules].gss2offset << endl;
            numberOfActualConfigurations++;
        }else{
            configurationOffsets.insert(hyperSchedulingList[iSchedules].gsscombinedossfet);
        }
    }
    configurationOffsets.insert(hyperSchedulingList[minHyperSchedulingIndex].gsscombinedossfet);
//    cout << "min schedule index: " << minHyperSchedulingIndex << endl;
//    cout << "Number of configurations: " << numberOfActualConfigurations << endl;
    // update the resource grid
    // cout << "size of set: " << configurationOffsets.size() << endl;
    // int groupOffsetIndex, groupEndIndex, groupPeriod, groupPackets, groupLeft, groupRight;
    tf.numberOfConfigurationActual = numberOfActualConfigurations - 1;
    tf.offsetConfigurationsActual.assign(configurationOffsets.begin(), configurationOffsets.end());
    sort(tf.offsetConfigurationsActual.begin(), tf.offsetConfigurationsActual.end());
    tf.offsetConfigurationsActual.erase(tf.offsetConfigurationsActual.begin());

    int gssIndex = 0;
    for (auto gss: hyperSchedulingList[minHyperSchedulingIndex].groupSchedulingList) {
        groupLeft = gss.left;
        groupRight = gss.right;
        groupPackets = gss.numberOfPackets;
        groupPeriod = gss.period;
//        if (gssIndex > 0) {
//            tf.offsetConfigurationsActual.push_back(groupLeft);
//        }
        // update the current top index of RBs
        if(gss.bottom+gss.height > currentRGTop){
            currentRGTop = gss.bottom + gss.height;
        }
        gssIndex++;
        // cout << "aaa" << endl;
        for (int packets = 0; packets < groupPackets; packets++) {
            groupOffsetIndex = groupLeft + packets * groupPeriod;
            groupEndIndex = groupRight + packets * groupPeriod;
            rg.update(groupOffsetIndex, groupEndIndex, gss.bottom, gss.height, tf.number);
            scheduleInfoElement s{groupOffsetIndex, groupEndIndex, gss.bottom, gss.height};
            tf.schedulingInfo.push_back(s);
            // rg.printData();
        }
        // cout << "bbb" << endl;
    }
}

// compare to V1, we schedule the control messages after the scheduling of each TF immediately
void CoNDynamicGroupV2(string& cfg, int f, double ratio_PRU){
    // Step 1: parse the meta information from cfg file
    std::vector<std::string> parsedCfg = split(cfg, ' ');
    // for(auto item: parsedCfg) std::cout << item << std::endl;
    int indexOfTF = stoi(parsedCfg[0]);
    int indexOfSetting = stoi(parsedCfg[1]);
    int numberOfTFs = stoi(parsedCfg[2]);
    int numberOfPilots = stoi(parsedCfg[3]);
    // int numberOfPilots = 150;
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
    int scheduleIntervalStart, scheduleIntervalEnd;

    for(int tfIndex = 0; tfIndex < numberOfTFs; tfIndex++) {
        if(dynamicPRURatio){
            ratio_PRU = (double)(tfIndex) / numberOfTFs;
            ratio_PRU = 0.5 + 0.3 * ratio_PRU;
//        double exp_k = 1;
//        ratio_PRU = exp(-1 * exp_k * ratio_PRU);
            // ratio_PRU = 0.05 + 0.95 * ratio_PRU;
        }

        double percentage = (double)tfIndex / numberOfTFs;
        if(percentage < 0.5){
            relaxedWidthIndicator = relaxedWidthIndicatorList[0];
        }else if(percentage < 0.75){
            relaxedWidthIndicator = relaxedWidthIndicatorList[1];
        }else {
            relaxedWidthIndicator = relaxedWidthIndicatorList[2];
        }

        subCoNV1(trafficFlowList[tfIndex], rg, relaxedWidthIndicator, ratio_PRU);

        std::vector<FreeSpaceElement> freeSpaceList;
        int localHeight;
        for(int checkIndex = 0; checkIndex < numberOfSlots; checkIndex ++){
            bool flag = true;
            int startFrequencyPointer = numberOfPilots - 1;
            for(int frequencyPointer = numberOfPilots - 1; frequencyPointer >= 0; frequencyPointer--){
                if(rg.data[checkIndex][frequencyPointer] != -1 && flag){
                    localHeight = startFrequencyPointer - frequencyPointer;
                    if(localHeight >= minControlMessageHeight){
                        freeSpaceList.push_back({checkIndex, frequencyPointer+1, localHeight});
                    }
                    flag = false;
                }else{
                    if(frequencyPointer == 0 && flag){
                        localHeight = startFrequencyPointer - frequencyPointer + 1;
                        if(localHeight >= minControlMessageHeight){
                            freeSpaceList.push_back({checkIndex, frequencyPointer, localHeight});
                        }
                    }
                }
                if(rg.data[checkIndex][frequencyPointer] == -1 && !flag){
                    startFrequencyPointer = frequencyPointer;
                    flag = true;
                }
            }
        }

        int numberNeededControlMessages = trafficFlowList[tfIndex].numberOfConfigurationActual;
        if(numberNeededControlMessages == 0) continue;
        std::vector<std::pair<int, int>> controlSchedulingInterval;
        for(int i = 0; i < trafficFlowList[tfIndex].numberOfConfigurationActual; i++){
            controlSchedulingInterval.emplace_back(std::make_pair(0, trafficFlowList[tfIndex].offsetConfigurationsActual[i] - 1));
        }
        int currentStartIndex = 0;  // for freespacelist, this is the available index
        int currentEndIndex = 0;
        for(auto intervalControl: controlSchedulingInterval){
            int endSlot = intervalControl.second;
            int startSlot = std::max(0, endSlot - distanceControlConfiguration);
            // identify the current range of freeSpaceList according to endSlot and startSlot
            for(int i = currentEndIndex; i < freeSpaceList.size(); i++){
                if(freeSpaceList[i].ele[0] <= endSlot){
                    continue;
                }else{
                    currentEndIndex = i-1;
                    break;
                }
            }
            for(int i = currentStartIndex; i < freeSpaceList.size(); i++){
                if(freeSpaceList[i].ele[0] < startSlot){
                    continue;
                }else{
                    currentStartIndex = i;
                    break;
                }
            }
            // format of free space element: 0 column_index     1 bottom    2 available space(height)
            // find the most appropriate free space according to: a. bottom(small)  b. height(small) c. close to end
            int currentCol, currentBottom, currentHeight, freeSpaceListIndex;
//            currentCol = freeSpaceList[currentEndIndex].ele[0];
//            currentBottom = freeSpaceList[currentEndIndex].ele[1];
//            currentHeight = freeSpaceList[currentEndIndex].ele[2];
            currentBottom = 99999;

            // std::cout << "before: " << currentEndIndex << " " <<currentCol << " " << currentBottom << " " << currentHeight << std::endl;
            // std::cout << "current start index: " << currentStartIndex << " current end index: " << currentEndIndex << std::endl;

            for(int i=currentEndIndex; i>=currentStartIndex;i--){
//                std::cout << i << " "   << freeSpaceList[i].ele[0] << " "
//                                        << freeSpaceList[i].ele[1] << " "
//                                        << freeSpaceList[i].ele[2] << std::endl;
                // if height not enough, skip
                if(freeSpaceList[i].ele[2] < trafficFlowList[tfIndex].controlOverhead) continue;

                if(currentBottom > freeSpaceList[i].ele[1]){
                    currentCol = freeSpaceList[i].ele[0];
                    currentBottom = freeSpaceList[i].ele[1];
                    currentHeight = freeSpaceList[i].ele[2];
                    freeSpaceListIndex = i;
                }else if(currentBottom == freeSpaceList[i].ele[1]){
                    if(currentHeight > freeSpaceList[i].ele[2]){
                        currentCol = freeSpaceList[i].ele[0];
                        currentBottom = freeSpaceList[i].ele[1];
                        currentHeight = freeSpaceList[i].ele[2];
                        freeSpaceListIndex = i;
                    }
                }
                // std::cout << currentCol << " " << currentBottom << " " << currentHeight << std::endl;
            }
            // std::cout << currentCol << " " << currentBottom << " " << currentHeight << std::endl;
            // check height
            if(currentHeight < trafficFlowList[tfIndex].controlOverhead){
//                std::cout << currentHeight << " " << trafficFlowList[tfIndex].controlOverhead << " "
//                            << tfIndex << std::endl;
                std::cout << "No solution2!" << std::endl;
                return;
            }
            // std::cout << "rrr" << std::endl;
            // update resource grid
            rg.update(currentCol,
                      currentCol,
                      currentBottom,
                      trafficFlowList[tfIndex].controlOverhead,
                      trafficFlowList[tfIndex].number + distanceDataControl);
            // update free space list
            freeSpaceList[freeSpaceListIndex].ele[1] += trafficFlowList[tfIndex].controlOverhead;
            freeSpaceList[freeSpaceListIndex].ele[2] -= trafficFlowList[tfIndex].controlOverhead;
        }
    }

    std::vector<int> topList;
    topList.push_back(0);
    std::string ss;


    auto endTime = std::chrono::steady_clock::now();
    int elapsedTimeInMilliseconds =
            std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

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
    int totalConfigurations = 0;
    for(const auto& tf: trafficFlowList){
        totalru += tf.payload * tf.numberOfConfigurations;
        totalPackets += tf.numberOfConfigurations;
        totalConfigurations += (tf.numberOfConfigurationActual + 1);
    }
//    int flag=0;
//    if(totalru > ru){
//        flag = 1;
//    }
    cout << endl;

    std::cout << "Time (ms): "<<elapsedTimeInMilliseconds << std::endl;
    double efficiency = (double) ru / (maxPilot * numberOfSlots);
    double utilization = (double) ruu / (maxPilot * numberOfSlots);
    std::cout << "Efficiency: " << efficiency << std::endl;
    std::fstream outLog;
    if(!dynamicPRURatio){
        outLog.open("../Results/output_0_" + std::to_string(ratio_PRU) + ".txt", std::ios_base::app);
    }else{
        outLog.open("../Results/output_0.txt", std::ios_base::app);
    }


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

// In v3, the increase of RB is considered.
void CoNDynamicGroupV3(string& cfg, int f, double ratio_PRU){
    // record the current maximum index of RB allocated
    int currentRGTop = 0;
    // Step 1: parse the meta information from cfg file
    std::vector<std::string> parsedCfg = split(cfg, ' ');
    // for(auto item: parsedCfg) std::cout << item << std::endl;
    int indexOfTF = stoi(parsedCfg[0]);
    int indexOfSetting = stoi(parsedCfg[1]);
    int numberOfTFs = stoi(parsedCfg[2]);
    int numberOfPilots = stoi(parsedCfg[3]);
    // int numberOfPilots = 150;
    int hyperPeriod = stoi(parsedCfg[4]);
    int numberOfSlots = hyperPeriod;
    int indexOfFolder = stoi(parsedCfg[5]);

    std::ofstream outlog;
    outlog.open("../Results/" + std::to_string(f) + "/out_" + std::to_string(indexOfTF) + ".txt");

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
    int scheduleIntervalStart, scheduleIntervalEnd;

    for(int tfIndex = 0; tfIndex < numberOfTFs; tfIndex++) {
        if(dynamicPRURatio){
            ratio_PRU = (double)(tfIndex) / numberOfTFs;
//            if(tfIndex < numberOfTFs / 2){
//                ratio_PRU = ratio_PRU;
//            }else{
//                ratio_PRU = 2 - ratio_PRU;
//            }
//        double exp_k = 1;
//        ratio_PRU = exp(-1 * exp_k * ratio_PRU);
            // ratio_PRU = 0.05 + 0.95 * ratio_PRU;
        }

        double percentage = (double)tfIndex / numberOfTFs;
        if(percentage < 0.5){
            relaxedWidthIndicator = relaxedWidthIndicatorList[0];
        }else if(percentage < 0.75){
            relaxedWidthIndicator = relaxedWidthIndicatorList[1];
        }else {
            relaxedWidthIndicator = relaxedWidthIndicatorList[2];
        }

        subCoNV3(trafficFlowList[tfIndex], rg, relaxedWidthIndicator, ratio_PRU, currentRGTop, outlog, trafficFlowList[tfIndex].number);

        std::vector<FreeSpaceElement> freeSpaceList;
        int localHeight;
        for(int checkIndex = 0; checkIndex < numberOfSlots; checkIndex ++){
            bool flag = true;
            int startFrequencyPointer = numberOfPilots - 1;
            for(int frequencyPointer = numberOfPilots - 1; frequencyPointer >= 0; frequencyPointer--){
                if(rg.data[checkIndex][frequencyPointer] != -1 && flag){
                    localHeight = startFrequencyPointer - frequencyPointer;
                    if(localHeight >= minControlMessageHeight){
                        freeSpaceList.push_back({checkIndex, frequencyPointer+1, localHeight});
                    }
                    flag = false;
                }else{
                    if(frequencyPointer == 0 && flag){
                        localHeight = startFrequencyPointer - frequencyPointer + 1;
                        if(localHeight >= minControlMessageHeight){
                            freeSpaceList.push_back({checkIndex, frequencyPointer, localHeight});
                        }
                    }
                }
                if(rg.data[checkIndex][frequencyPointer] == -1 && !flag){
                    startFrequencyPointer = frequencyPointer;
                    flag = true;
                }
            }
        }

        int numberNeededControlMessages = trafficFlowList[tfIndex].numberOfConfigurationActual;
        if(numberNeededControlMessages == 0) continue;
        std::vector<std::pair<int, int>> controlSchedulingInterval;
        for(int i = 0; i < trafficFlowList[tfIndex].numberOfConfigurationActual; i++){
            controlSchedulingInterval.emplace_back(std::make_pair(0, trafficFlowList[tfIndex].offsetConfigurationsActual[i] - 1));
        }
        int currentStartIndex = 0;  // for freespacelist, this is the available index
        int currentEndIndex = 0;
        for(auto intervalControl: controlSchedulingInterval){
            int endSlot = intervalControl.second;
            int startSlot = std::max(0, endSlot - distanceControlConfiguration);
            // identify the current range of freeSpaceList according to endSlot and startSlot
            for(int i = currentEndIndex; i < freeSpaceList.size(); i++){
                if(freeSpaceList[i].ele[0] <= endSlot){
                    continue;
                }else{
                    currentEndIndex = i-1;
                    break;
                }
            }
            for(int i = currentStartIndex; i < freeSpaceList.size(); i++){
                if(freeSpaceList[i].ele[0] < startSlot){
                    continue;
                }else{
                    currentStartIndex = i;
                    break;
                }
            }
            // format of free space element: 0 column_index     1 bottom    2 available space(height)
            // find the most appropriate free space according to: a. bottom(small)  b. height(small) c. close to end
            int currentCol, currentBottom, currentHeight, freeSpaceListIndex;
//            currentCol = freeSpaceList[currentEndIndex].ele[0];
//            currentBottom = freeSpaceList[currentEndIndex].ele[1];
//            currentHeight = freeSpaceList[currentEndIndex].ele[2];
            currentBottom = 99999;

            // std::cout << "before: " << currentEndIndex << " " <<currentCol << " " << currentBottom << " " << currentHeight << std::endl;
            // std::cout << "current start index: " << currentStartIndex << " current end index: " << currentEndIndex << std::endl;

            for(int i=currentEndIndex; i>=currentStartIndex;i--){
//                std::cout << i << " "   << freeSpaceList[i].ele[0] << " "
//                                        << freeSpaceList[i].ele[1] << " "
//                                        << freeSpaceList[i].ele[2] << std::endl;
                // if height not enough, skip
                if(freeSpaceList[i].ele[2] < trafficFlowList[tfIndex].controlOverhead) continue;

                if(currentBottom > freeSpaceList[i].ele[1]){
                    currentCol = freeSpaceList[i].ele[0];
                    currentBottom = freeSpaceList[i].ele[1];
                    currentHeight = freeSpaceList[i].ele[2];
                    freeSpaceListIndex = i;
                }else if(currentBottom == freeSpaceList[i].ele[1]){
                    if(currentHeight > freeSpaceList[i].ele[2]){
                        currentCol = freeSpaceList[i].ele[0];
                        currentBottom = freeSpaceList[i].ele[1];
                        currentHeight = freeSpaceList[i].ele[2];
                        freeSpaceListIndex = i;
                    }
                }
                // std::cout << currentCol << " " << currentBottom << " " << currentHeight << std::endl;
            }
            // std::cout << currentCol << " " << currentBottom << " " << currentHeight << std::endl;
            // check height
            if(currentHeight < trafficFlowList[tfIndex].controlOverhead){
//                std::cout << currentHeight << " " << trafficFlowList[tfIndex].controlOverhead << " "
//                            << tfIndex << std::endl;
                std::cout << "No solution2!" << std::endl;
                return;
            }
            // std::cout << "rrr" << std::endl;
            // update resource grid
            rg.update(currentCol,
                      currentCol,
                      currentBottom,
                      trafficFlowList[tfIndex].controlOverhead,
                      trafficFlowList[tfIndex].number + distanceDataControl);
            // update free space list
            freeSpaceList[freeSpaceListIndex].ele[1] += trafficFlowList[tfIndex].controlOverhead;
            freeSpaceList[freeSpaceListIndex].ele[2] -= trafficFlowList[tfIndex].controlOverhead;
        }
    }

    std::vector<int> topList;
    topList.push_back(0);
    std::string ss;


    auto endTime = std::chrono::steady_clock::now();
    int elapsedTimeInMilliseconds =
            std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

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
//    std::ofstream outlog;
//    outlog.open("../Results/" + std::to_string(f) + "/out_" + std::to_string(indexOfTF) + ".txt");
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
    int totalConfigurations = 0;
    for(const auto& tf: trafficFlowList){
        totalru += tf.payload * tf.numberOfConfigurations;
        totalPackets += tf.numberOfConfigurations;
        totalConfigurations += (tf.numberOfConfigurationActual + 1);
    }
//    int flag=0;
//    if(totalru > ru){
//        flag = 1;
//    }
    cout << endl;

    std::cout << "Time (ms): "<<elapsedTimeInMilliseconds << std::endl;
    double efficiency = (double) ru / (maxPilot * numberOfSlots);
    double utilization = (double) ruu / (maxPilot * numberOfSlots);
    std::cout << "Efficiency: " << efficiency << std::endl;
    std::fstream outLog;
    if(!dynamicPRURatio){
        outLog.open("../Results/output_0_" + std::to_string(ratio_PRU) + ".txt", std::ios_base::app);
    }else{
        outLog.open("../Results/output_0.txt", std::ios_base::app);
    }


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

// compare to V2, the newest version, reconsidering the definition of PRU (future of the current TF)
/*
 * Just for demonstration
 * we need first print out the RU allocation status for every TF
 */
void CoNDynamicGroupVTest(string& cfg, int f, double ratio_PRU){
    // record the current maximum index of RB allocated
    int currentRGTop = 0;
    // Step 1: parse the meta information from cfg file
    std::vector<std::string> parsedCfg = split(cfg, ' ');
    // for(auto item: parsedCfg) std::cout << item << std::endl;
    int indexOfTF = stoi(parsedCfg[0]);
    int indexOfSetting = stoi(parsedCfg[1]);
    int numberOfTFs = stoi(parsedCfg[2]);
    int numberOfPilots = stoi(parsedCfg[3]);
    // int numberOfPilots = 150;
    int hyperPeriod = stoi(parsedCfg[4]);
    int numberOfSlots = hyperPeriod;
    int indexOfFolder = stoi(parsedCfg[5]);

    std::ofstream outlog;
    outlog.open("../Results/" + std::to_string(f) + "/out_" + std::to_string(indexOfTF) + ".txt");

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
    int scheduleIntervalStart, scheduleIntervalEnd;

    for(int tfIndex = 0; tfIndex < numberOfTFs; tfIndex++) {
        if(dynamicPRURatio){
            ratio_PRU = (double)(tfIndex) / numberOfTFs;
            ratio_PRU = 0.5 + 0.3 * ratio_PRU;
//        double exp_k = 1;
//        ratio_PRU = exp(-1 * exp_k * ratio_PRU);
            // ratio_PRU = 0.05 + 0.95 * ratio_PRU;
        }

        double percentage = (double)tfIndex / numberOfTFs;
        if(percentage < 0.5){
            relaxedWidthIndicator = relaxedWidthIndicatorList[0];
        }else if(percentage < 0.75){
            relaxedWidthIndicator = relaxedWidthIndicatorList[1];
        }else {
            relaxedWidthIndicator = relaxedWidthIndicatorList[2];
        }

        subCoNV3(trafficFlowList[tfIndex], rg, relaxedWidthIndicator, ratio_PRU, currentRGTop, outlog, trafficFlowList[tfIndex].number);

        outlog << char('A' + trafficFlowList[tfIndex].number) << endl;
        int i, j;
        for(i = 0;i <= numberOfPilots - 1; i++){
            for(j=0;j<numberOfSlots;j++){
                char parsed;
                if(rg.data[j][i] == -1){
                    parsed = '0';
                }else{
                    parsed = 'A'+rg.data[j][i];
                }
                outlog << parsed << " ";
            }
            outlog << std::endl;
        }
        outlog << endl << endl << endl << endl;

        std::vector<FreeSpaceElement> freeSpaceList;
        int localHeight;
        for(int checkIndex = 0; checkIndex < numberOfSlots; checkIndex ++){
            bool flag = true;
            int startFrequencyPointer = numberOfPilots - 1;
            for(int frequencyPointer = numberOfPilots - 1; frequencyPointer >= 0; frequencyPointer--){
                if(rg.data[checkIndex][frequencyPointer] != -1 && flag){
                    localHeight = startFrequencyPointer - frequencyPointer;
                    if(localHeight >= minControlMessageHeight){
                        freeSpaceList.push_back({checkIndex, frequencyPointer+1, localHeight});
                    }
                    flag = false;
                }else{
                    if(frequencyPointer == 0 && flag){
                        localHeight = startFrequencyPointer - frequencyPointer + 1;
                        if(localHeight >= minControlMessageHeight){
                            freeSpaceList.push_back({checkIndex, frequencyPointer, localHeight});
                        }
                    }
                }
                if(rg.data[checkIndex][frequencyPointer] == -1 && !flag){
                    startFrequencyPointer = frequencyPointer;
                    flag = true;
                }
            }
        }

        int numberNeededControlMessages = trafficFlowList[tfIndex].numberOfConfigurationActual;
        if(numberNeededControlMessages == 0) continue;
        std::vector<std::pair<int, int>> controlSchedulingInterval;
        for(int i = 0; i < trafficFlowList[tfIndex].numberOfConfigurationActual; i++){
            controlSchedulingInterval.emplace_back(std::make_pair(0, trafficFlowList[tfIndex].offsetConfigurationsActual[i] - 1));
        }
        int currentStartIndex = 0;  // for freespacelist, this is the available index
        int currentEndIndex = 0;
        for(auto intervalControl: controlSchedulingInterval){
            int endSlot = intervalControl.second;
            int startSlot = std::max(0, endSlot - distanceControlConfiguration);
            // identify the current range of freeSpaceList according to endSlot and startSlot
            for(int i = currentEndIndex; i < freeSpaceList.size(); i++){
                if(freeSpaceList[i].ele[0] <= endSlot){
                    continue;
                }else{
                    currentEndIndex = i-1;
                    break;
                }
            }
            for(int i = currentStartIndex; i < freeSpaceList.size(); i++){
                if(freeSpaceList[i].ele[0] < startSlot){
                    continue;
                }else{
                    currentStartIndex = i;
                    break;
                }
            }
            // format of free space element: 0 column_index     1 bottom    2 available space(height)
            // find the most appropriate free space according to: a. bottom(small)  b. height(small) c. close to end
            int currentCol, currentBottom, currentHeight, freeSpaceListIndex;
//            currentCol = freeSpaceList[currentEndIndex].ele[0];
//            currentBottom = freeSpaceList[currentEndIndex].ele[1];
//            currentHeight = freeSpaceList[currentEndIndex].ele[2];
            currentBottom = 99999;

            // std::cout << "before: " << currentEndIndex << " " <<currentCol << " " << currentBottom << " " << currentHeight << std::endl;
            // std::cout << "current start index: " << currentStartIndex << " current end index: " << currentEndIndex << std::endl;

            for(int i=currentEndIndex; i>=currentStartIndex;i--){
//                std::cout << i << " "   << freeSpaceList[i].ele[0] << " "
//                                        << freeSpaceList[i].ele[1] << " "
//                                        << freeSpaceList[i].ele[2] << std::endl;
                // if height not enough, skip
                if(freeSpaceList[i].ele[2] < trafficFlowList[tfIndex].controlOverhead) continue;

                if(currentBottom > freeSpaceList[i].ele[1]){
                    currentCol = freeSpaceList[i].ele[0];
                    currentBottom = freeSpaceList[i].ele[1];
                    currentHeight = freeSpaceList[i].ele[2];
                    freeSpaceListIndex = i;
                }else if(currentBottom == freeSpaceList[i].ele[1]){
                    if(currentHeight > freeSpaceList[i].ele[2]){
                        currentCol = freeSpaceList[i].ele[0];
                        currentBottom = freeSpaceList[i].ele[1];
                        currentHeight = freeSpaceList[i].ele[2];
                        freeSpaceListIndex = i;
                    }
                }
                // std::cout << currentCol << " " << currentBottom << " " << currentHeight << std::endl;
            }
            // std::cout << currentCol << " " << currentBottom << " " << currentHeight << std::endl;
            // check height
            if(currentHeight < trafficFlowList[tfIndex].controlOverhead){
//                std::cout << currentHeight << " " << trafficFlowList[tfIndex].controlOverhead << " "
//                            << tfIndex << std::endl;
                std::cout << "No solution2!" << std::endl;
                return;
            }
            // std::cout << "rrr" << std::endl;
            // update resource grid
            rg.update(currentCol,
                      currentCol,
                      currentBottom,
                      trafficFlowList[tfIndex].controlOverhead,
                      trafficFlowList[tfIndex].number + distanceDataControl);
            // update free space list
            freeSpaceList[freeSpaceListIndex].ele[1] += trafficFlowList[tfIndex].controlOverhead;
            freeSpaceList[freeSpaceListIndex].ele[2] -= trafficFlowList[tfIndex].controlOverhead;
        }

    }

    std::vector<int> topList;
    topList.push_back(0);
    std::string ss;


    auto endTime = std::chrono::steady_clock::now();
    int elapsedTimeInMilliseconds =
            std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

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
                parsed = 'A'+rg.data[j][i];
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
    int totalConfigurations = 0;
    for(const auto& tf: trafficFlowList){
        totalru += tf.payload * tf.numberOfConfigurations;
        totalPackets += tf.numberOfConfigurations;
        totalConfigurations += (tf.numberOfConfigurationActual + 1);
    }
//    int flag=0;
//    if(totalru > ru){
//        flag = 1;
//    }
    cout << endl;

    std::cout << "Time (ms): "<<elapsedTimeInMilliseconds << std::endl;
    double efficiency = (double) ru / (maxPilot * numberOfSlots);
    double utilization = (double) ruu / (maxPilot * numberOfSlots);
    std::cout << "Efficiency: " << efficiency << std::endl;
    std::fstream outLog;
    if(!dynamicPRURatio){
        outLog.open("../Results/output_0_" + std::to_string(ratio_PRU) + ".txt", std::ios_base::app);
    }else{
        outLog.open("../Results/output_0.txt", std::ios_base::app);
    }


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

void updateRGViaSchedulingList(TrafficFlow& tf,
                               vector<GroupSchedulingStruct>& copiedGroupSchedulingList,
                               ResourceGrid& rg){
    /*
     * Step 1.1: for the current TF with gss1 & gss2
     */
    /*
     * Step 1.1.1: schedule all gss in copiedGroupSchedulingList
     */
    int gssIndex = 0, groupLeft, groupRight, groupPackets, groupPeriod, groupOffsetIndex, groupEndIndex;
    for (auto gss: copiedGroupSchedulingList) {
        groupLeft = gss.left;
        groupRight = gss.right;
        groupPackets = gss.numberOfPackets;
        groupPeriod = gss.period;
//        if (gssIndex > 0) {
//            tf.offsetConfigurationsActual.push_back(groupLeft);
//        }
        // update the current top index of RBs
//        if(gss.bottom+gss.height > currentRGTop){
//            currentRGTop = gss.bottom + gss.height;
//        }
        gssIndex++;
        // cout << "aaa" << endl;
        for (int packets = 0; packets < groupPackets; packets++) {
            groupOffsetIndex = groupLeft + packets * groupPeriod;
            groupEndIndex = groupRight + packets * groupPeriod;
            // for RG, "-2" means tmp RU allocation, this assignment can be withdrawn
            rg.update(groupOffsetIndex, groupEndIndex, gss.bottom, gss.height, -2);
            scheduleInfoElement s{groupOffsetIndex, groupEndIndex, gss.bottom, gss.height};
        }
    }
    /*
     * Step 1.1.2: schedule all corresponding control messages: scan all gss; generate all offsets
     */
    int numberOfActualConfigurations = 1;
    vector<int> configurationOffsets;
    configurationOffsets.push_back(copiedGroupSchedulingList[0].left);
    int currentL = copiedGroupSchedulingList[0].left,
            currentH = copiedGroupSchedulingList[0].height,
            currentW = copiedGroupSchedulingList[0].right - copiedGroupSchedulingList[0].left + 1,
            currentB = copiedGroupSchedulingList[0].bottom,
            currentP = copiedGroupSchedulingList[0].period,
            currentPackets = copiedGroupSchedulingList[0].numberOfPackets;
    // if currentPackets == 1, then currentP is invalid; otherwise, currentP is calculated by the difference of offsets
    for(gssIndex = 1; gssIndex < copiedGroupSchedulingList.size() - 1; gssIndex++){
        // compare h, w, b
        if(currentH != copiedGroupSchedulingList[gssIndex].height){
            configurationOffsets.push_back(copiedGroupSchedulingList[gssIndex].left);
            numberOfActualConfigurations++;
            currentL = copiedGroupSchedulingList[gssIndex].left;
            currentH = copiedGroupSchedulingList[gssIndex].height;
            currentW = copiedGroupSchedulingList[gssIndex].right - copiedGroupSchedulingList[gssIndex].left + 1;
            currentB = copiedGroupSchedulingList[gssIndex].bottom;
            currentP = copiedGroupSchedulingList[gssIndex].period;
            currentPackets = copiedGroupSchedulingList[gssIndex].numberOfPackets;
            continue;
        }
        if(currentW != (copiedGroupSchedulingList[gssIndex].right - copiedGroupSchedulingList[gssIndex].left + 1)){
            configurationOffsets.push_back(copiedGroupSchedulingList[gssIndex].left);
            numberOfActualConfigurations++;
            currentL = copiedGroupSchedulingList[gssIndex].left;
            currentH = copiedGroupSchedulingList[gssIndex].height;
            currentW = copiedGroupSchedulingList[gssIndex].right - copiedGroupSchedulingList[gssIndex].left + 1;
            currentB = copiedGroupSchedulingList[gssIndex].bottom;
            currentP = copiedGroupSchedulingList[gssIndex].period;
            currentPackets = copiedGroupSchedulingList[gssIndex].numberOfPackets;
            continue;
        }
        if(currentB != copiedGroupSchedulingList[gssIndex].bottom){
            configurationOffsets.push_back(copiedGroupSchedulingList[gssIndex].left);
            numberOfActualConfigurations++;
            currentL = copiedGroupSchedulingList[gssIndex].left;
            currentH = copiedGroupSchedulingList[gssIndex].height;
            currentW = copiedGroupSchedulingList[gssIndex].right - copiedGroupSchedulingList[gssIndex].left + 1;
            currentB = copiedGroupSchedulingList[gssIndex].bottom;
            currentP = copiedGroupSchedulingList[gssIndex].period;
            currentPackets = copiedGroupSchedulingList[gssIndex].numberOfPackets;
            continue;
        }
        // p
        if(currentPackets == 1){
            currentP = copiedGroupSchedulingList[gssIndex].left - currentL;
        }
        if(currentP != copiedGroupSchedulingList[gssIndex].period){
            configurationOffsets.push_back(copiedGroupSchedulingList[gssIndex].left);
            numberOfActualConfigurations++;
            currentL = copiedGroupSchedulingList[gssIndex].left;
            currentH = copiedGroupSchedulingList[gssIndex].height;
            currentW = copiedGroupSchedulingList[gssIndex].right - copiedGroupSchedulingList[gssIndex].left + 1;
            currentB = copiedGroupSchedulingList[gssIndex].bottom;
            currentP = copiedGroupSchedulingList[gssIndex].period;
            currentPackets = copiedGroupSchedulingList[gssIndex].numberOfPackets;
            continue;
        }
        // until now, one configuration can be reduced, we need to update the setting of the current configuration
        currentPackets += copiedGroupSchedulingList[gssIndex].numberOfPackets;
    }
    /*
     * Step 1.1.3: schedule all control messages according to the offsets
     */
    std::vector<FreeSpaceElement> freeSpaceList;
    int localHeight;
    for(int checkIndex = 0; checkIndex < rg.numberOfSlots; checkIndex ++){
        bool flag = true;
        int startFrequencyPointer = rg.numberOfPilots - 1;
        for(int frequencyPointer = rg.numberOfPilots - 1; frequencyPointer >= 0; frequencyPointer--){
            if(rg.data[checkIndex][frequencyPointer] != -1 && flag){
                localHeight = startFrequencyPointer - frequencyPointer;
                if(localHeight >= minControlMessageHeight){
                    freeSpaceList.push_back({checkIndex, frequencyPointer+1, localHeight});
                }
                flag = false;
            }else{
                if(frequencyPointer == 0 && flag){
                    localHeight = startFrequencyPointer - frequencyPointer + 1;
                    if(localHeight >= minControlMessageHeight){
                        freeSpaceList.push_back({checkIndex, frequencyPointer, localHeight});
                    }
                }
            }
            if(rg.data[checkIndex][frequencyPointer] == -1 && !flag){
                startFrequencyPointer = frequencyPointer;
                flag = true;
            }
        }
    }

    int numberNeededControlMessages = numberOfActualConfigurations - 1;
    if(numberNeededControlMessages > 0){
        std::vector<std::pair<int, int>> controlSchedulingInterval;
        int i;
        for(i = 1; i <= numberNeededControlMessages; i++){
            controlSchedulingInterval.emplace_back(std::make_pair(0, configurationOffsets[i] - 1));
        }
        int currentStartIndex = 0;  // for freespacelist, this is the available index
        int currentEndIndex = 0;
        for(auto intervalControl: controlSchedulingInterval){
            int endSlot = intervalControl.second;
            int startSlot = std::max(0, endSlot - distanceControlConfiguration);
            // identify the current range of freeSpaceList according to endSlot and startSlot
            for(i = currentEndIndex; i < freeSpaceList.size(); i++){
                if(freeSpaceList[i].ele[0] <= endSlot){
                    continue;
                }else{
                    currentEndIndex = i-1;
                    break;
                }
            }
            for(i = currentStartIndex; i < freeSpaceList.size(); i++){
                if(freeSpaceList[i].ele[0] < startSlot){
                    continue;
                }else{
                    currentStartIndex = i;
                    break;
                }
            }
            // format of free space element: 0 column_index     1 bottom    2 available space(height)
            // find the most appropriate free space according to: a. bottom(small)  b. height(small) c. close to end
            int currentCol, currentBottom, currentHeight, freeSpaceListIndex;
//            currentCol = freeSpaceList[currentEndIndex].ele[0];
//            currentBottom = freeSpaceList[currentEndIndex].ele[1];
//            currentHeight = freeSpaceList[currentEndIndex].ele[2];
            currentBottom = 99999;

            // std::cout << "before: " << currentEndIndex << " " <<currentCol << " " << currentBottom << " " << currentHeight << std::endl;
            // std::cout << "current start index: " << currentStartIndex << " current end index: " << currentEndIndex << std::endl;

            for(i=currentEndIndex; i>=currentStartIndex;i--){
//                std::cout << i << " "   << freeSpaceList[i].ele[0] << " "
//                                        << freeSpaceList[i].ele[1] << " "
//                                        << freeSpaceList[i].ele[2] << std::endl;
                // if height not enough, skip
                if(freeSpaceList[i].ele[2] < tf.controlOverhead) continue;

                if(currentBottom > freeSpaceList[i].ele[1]){
                    currentCol = freeSpaceList[i].ele[0];
                    currentBottom = freeSpaceList[i].ele[1];
                    currentHeight = freeSpaceList[i].ele[2];
                    freeSpaceListIndex = i;
                }else if(currentBottom == freeSpaceList[i].ele[1]){
                    if(currentHeight > freeSpaceList[i].ele[2]){
                        currentCol = freeSpaceList[i].ele[0];
                        currentBottom = freeSpaceList[i].ele[1];
                        currentHeight = freeSpaceList[i].ele[2];
                        freeSpaceListIndex = i;
                    }
                }
                // std::cout << currentCol << " " << currentBottom << " " << currentHeight << std::endl;
            }
            // std::cout << currentCol << " " << currentBottom << " " << currentHeight << std::endl;
            // check height
            if(currentHeight < tf.controlOverhead){
//                std::cout << currentHeight << " " << trafficFlowList[tfIndex].controlOverhead << " "
//                            << tfIndex << std::endl;
                std::cout << "No solution2!" << std::endl;
            }
            // std::cout << "rrr" << std::endl;
            // update resource grid
            rg.update(currentCol,
                      currentCol,
                      currentBottom,
                      tf.controlOverhead,
                      -2);
            // update free space list
            freeSpaceList[freeSpaceListIndex].ele[1] += tf.controlOverhead;
            freeSpaceList[freeSpaceListIndex].ele[2] -= tf.controlOverhead;
        }
    }
}

/*
 * calculate the difference of RUs for gss12 and gssCombined
 */
int diffRGGss12GssCombined(ResourceGrid& rgGss12, ResourceGrid& rgGssCombined){
    // for all RUs in the area of {hyperperiod * topPilot}, count the number of RUs with -1
    /*
     * step 1: calculate the topPilots of two rgs
     */
    // gss12
    int topPilotGss12 = 0, topPilotGssCombined = 0;
    std::vector<int> topList;
    topList.push_back(0);
    bool flagF = false;
    topList.clear();
    for(int rowIndex = rgGss12.numberOfPilots - 1; rowIndex >= 0; rowIndex--){
        for(int colIndex = 0; colIndex < rgGss12.numberOfSlots; colIndex++){
            if(rgGss12.data[colIndex][rowIndex] == -1){
                continue;
            }
            else{
                topPilotGss12 = rowIndex + 1;
                flagF = true;
                break;
            }
        }
        if(flagF) break;
    }
    // gssCombined
    flagF = false;
    topList.clear();
    for(int rowIndex = rgGssCombined.numberOfPilots - 1; rowIndex >= 0; rowIndex--){
        for(int colIndex = 0; colIndex < rgGssCombined.numberOfSlots; colIndex++){
            if(rgGssCombined.data[colIndex][rowIndex] == -1){
                continue;
            }
            else{
                topPilotGssCombined = rowIndex + 1;
                flagF = true;
                break;
            }
        }
        if(flagF) break;
    }

    // unallocated RUs of rgGss12
//    int unAllocatedRUsGss12 = 0;
//    for(int rowIndex = topPilotGss12; rowIndex >= 0; rowIndex--){
//        for(int colIndex = 0; colIndex < rgGss12.numberOfSlots; colIndex++){
//            if(rgGss12.data[colIndex][rowIndex] == -1){
//                unAllocatedRUsGss12 += 1;
//            }
//        }
//    }
//
//    // cout << "gss12 pilots: " << topPilotGss12 << " gsscombined pilots: " << topPilotGssCombined << endl;
//    // unallocated RUs of rgGssCombined
//    int unAllocatedRUsGssCombined = 0;
//    for(int rowIndex = topPilotGssCombined; rowIndex >= 0; rowIndex--){
//        for(int colIndex = 0; colIndex < rgGssCombined.numberOfSlots; colIndex++){
//            if(rgGssCombined.data[colIndex][rowIndex] == -1){
//                unAllocatedRUsGssCombined += 1;
//            }
//        }
//    }
    // cout << "RU combined: " << unAllocatedRUsGssCombined << "RU 12: " << unAllocatedRUsGss12 << endl;
    // return unAllocatedRUsGssCombined - unAllocatedRUsGss12;
    return topPilotGssCombined - topPilotGss12;

}

/*
 * All "Future" related approach:
 * For the current TF, each time when doing the combination process, schedule all the next TFs to calculate the penalty
 * In this process, try to establish the connection between the number of PRU and the current penalty
 */
double combinationObjectiveFunctionFuture(  vector<TrafficFlow>& trafficFlowList,
                                            TrafficFlow& tf,
                                            int currentTFIndex,
                                            vector<GroupSchedulingStruct>& groupSchedulingList,
                                            int combinationIndex,
                                            GroupSchedulingStruct& gssCombined,
                                            GroupSchedulingStruct& gss1,
                                            GroupSchedulingStruct& gss2,
                                            ResourceGrid& rg,
                                            int controlMessageSize){
    // int increaseOfPRUs = combinationPRUs(gssCombined, gss1, gss2, rg);

    ResourceGrid tmpRGGss12(rg);
    ResourceGrid tmpRGGssCombined(rg);
//    cout << "gss1: top:" << std::to_string(gss1.bottom + gss1.height)
//    << " gss2: top: " << std::to_string(gss2.bottom + gss2.height) <<
//    " gsscombined: top: " << std::to_string(gssCombined.bottom + gssCombined.height) << endl;
    /*
     * for the current combination process, calculate the penalty:
     * difference: resource grid of gss1 + gss2 vs. resource grid of gssCombined (number of unallocated RUs/increase of RBs)
     */
    vector<GroupSchedulingStruct> copiedGroupSchedulingList(groupSchedulingList);
    /*
     * step 1, for gss1 & gss2 calculate the finished resource grid, each TF is scheduled by co1 per data packet
     * here, we need to schedule the current TF firstly including the control messages, then the other TFs
     */
    updateRGViaSchedulingList(tf, copiedGroupSchedulingList, tmpRGGss12);

    /*
     * Step 1.2: schedule all the other TFs
     */
    /*
     * Step 1.2.1: using co1 for all the rest TFs per data transmission
     */
    for(int tfIndex = currentTFIndex+1; tfIndex < trafficFlowList.size(); tfIndex++){
        vector<std::pair<int, int>> schedulingIntervals;
        for(int i = 0; i < tmpRGGss12.numberOfSlots / trafficFlowList[tfIndex].transmissionPeriod; i++){
            schedulingIntervals.emplace_back(
                    std::make_pair(ceil(trafficFlowList[tfIndex].initialOffset + trafficFlowList[tfIndex].transmissionPeriod * i),
                                   floor(trafficFlowList[tfIndex].initialOffset +trafficFlowList[tfIndex].transmissionPeriod * i + trafficFlowList[tfIndex].latencyRequirement - 1)));
        }
        tf.schedulingInterval = schedulingIntervals;
        vector<GroupSchedulingStruct> tmp_groupSchedulingList;
        int packetIndex = 0;
        // step 1: for all packets, group size = 1, find the most appropriate location for all packets
        // this is the optimal solution [the highest resource efficiency]
        // auto startTime_initialCo1 = std::chrono::steady_clock::now();
        for(auto intervals: schedulingIntervals) {
            // Initialization of the current group scheduling struct
            // cout << "intervals..." << endl;
            GroupSchedulingStruct gss{};
            gss.left = 0;
            gss.right = 0;
            gss.bottom = 0;
            gss.height = 999999;
            gss.period = trafficFlowList[tfIndex].transmissionPeriod;
            gss.numberOfPackets = 1;
            gss.startPacketIndex = packetIndex;
            packetIndex++;

            int lowestSettingForAllFlat [] = {0, 0, 0, 99999};

            // width max
            int WidthMax;
            int freeSpace = intervals.second - intervals.first + 1;
            int h = ceil((double) trafficFlowList[tfIndex].payload / freeSpace);
            if (h == 1) {
                WidthMax = trafficFlowList[tfIndex].payload;
            } else {
                WidthMax = intervals.second - intervals.first + 1;
            }
            // FUNCTION: calculate the minimum height for all shapes w.r.t the current flat
            int offsetMin = intervals.first;
            int offsetMax = intervals.second;
            int bottomMin = 0;
            int widthMax = WidthMax;
            // cout << "Width: " << widthMax << endl;
            int transmissionPeriod = trafficFlowList[tfIndex].transmissionPeriod;

            for (int alternativeWidth = 1; alternativeWidth <= widthMax; alternativeWidth++) {
                int tmpHeight = (int) ceil((double) trafficFlowList[tfIndex].payload / alternativeWidth);
                // cout << "Height: " << tmpHeight << endl;
                if(tmpHeight >= tmpRGGss12.numberOfPilots) continue;
                int tmpOffsetMax = offsetMax - alternativeWidth + 1;
                for (int alternativeOffset = tmpOffsetMax; alternativeOffset >= offsetMin; alternativeOffset--) {
                    for (int alternativeBottom = bottomMin; alternativeBottom < tmpRGGss12.numberOfPilots; alternativeBottom++) {
                        // check collision for all packets
                        bool allPacketsSuccessFlag = false;
                        for (int packets = 0; packets < 1; packets++) {
                            int packetLeft = alternativeOffset + transmissionPeriod * packets;
                            int packetRight = alternativeOffset + alternativeWidth - 1 + transmissionPeriod * packets;
                            int packetBottom = alternativeBottom;
                            int packetHeight = tmpHeight;
                            // collision check for one packet
                            bool collisionFlag = false;
                            for (int col = packetLeft; col <= packetRight; col++) {
                                for (int row = packetBottom; row < packetBottom + packetHeight; row++) {
                                    if (tmpRGGss12.data[col][row] != -1) {
                                        // collision occurs
                                        collisionFlag = true;
                                        break;
                                    }
                                }
                            }
                            if (collisionFlag) {
                                // collision happened
                                break;
                            }
                            if (packets == 0) {
                                allPacketsSuccessFlag = true;
                            }
                        }
                        if (allPacketsSuccessFlag) {
                            // compare, keep the minimum one
                            if (alternativeBottom + tmpHeight < lowestSettingForAllFlat[2] + lowestSettingForAllFlat[3]) {
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
            gss.left = lowestSettingForAllFlat[0];
            gss.right = lowestSettingForAllFlat[1];
            gss.bottom = lowestSettingForAllFlat[2];
            gss.height = lowestSettingForAllFlat[3];
            tmp_groupSchedulingList.push_back(gss);
        }
        updateRGViaSchedulingList(trafficFlowList[tfIndex], tmp_groupSchedulingList, tmpRGGss12);
    }

    /*
     * Step 2: schedule for gssCombined
     */

    /*
     * step 1, for gssCombined calculate the finished resource grid, each TF is scheduled by co1 per data packet
     * here, we need to schedule the current TF firstly including the control messages, then the other TFs
     */
    copiedGroupSchedulingList.erase(copiedGroupSchedulingList.begin() + combinationIndex);
    copiedGroupSchedulingList.erase(copiedGroupSchedulingList.begin() + combinationIndex);
    copiedGroupSchedulingList.insert(copiedGroupSchedulingList.begin() + combinationIndex, gssCombined);
    updateRGViaSchedulingList(tf, copiedGroupSchedulingList, tmpRGGssCombined);
    /*
     * Step 1.2: schedule all the other TFs
     */
    /*
     * Step 1.2.1: using co1 for all the rest TFs per data transmission
     */
    for(int tfIndex = currentTFIndex+1; tfIndex < trafficFlowList.size(); tfIndex++){
        vector<std::pair<int, int>> schedulingIntervals;
        for(int i = 0; i < tmpRGGssCombined.numberOfSlots / trafficFlowList[tfIndex].transmissionPeriod; i++){
            schedulingIntervals.emplace_back(
                    std::make_pair(ceil(trafficFlowList[tfIndex].initialOffset + trafficFlowList[tfIndex].transmissionPeriod * i),
                                   floor(trafficFlowList[tfIndex].initialOffset +trafficFlowList[tfIndex].transmissionPeriod * i + trafficFlowList[tfIndex].latencyRequirement - 1)));
        }
        tf.schedulingInterval = schedulingIntervals;
        vector<GroupSchedulingStruct> tmp_groupSchedulingList;
        int packetIndex = 0;
        // step 1: for all packets, group size = 1, find the most appropriate location for all packets
        // this is the optimal solution [the highest resource efficiency]
        // auto startTime_initialCo1 = std::chrono::steady_clock::now();
        for(auto intervals: schedulingIntervals) {
            // Initialization of the current group scheduling struct
            // cout << "intervals..." << endl;
            GroupSchedulingStruct gss{};
            gss.left = 0;
            gss.right = 0;
            gss.bottom = 0;
            gss.height = 999999;
            gss.period = trafficFlowList[tfIndex].transmissionPeriod;
            gss.numberOfPackets = 1;
            gss.startPacketIndex = packetIndex;
            packetIndex++;

            int lowestSettingForAllFlat [] = {0, 0, 0, 99999};

            // width max
            int WidthMax;
            int freeSpace = intervals.second - intervals.first + 1;
            int h = ceil((double) trafficFlowList[tfIndex].payload / freeSpace);
            if (h == 1) {
                WidthMax = trafficFlowList[tfIndex].payload;
            } else {
                WidthMax = intervals.second - intervals.first + 1;
            }
            // FUNCTION: calculate the minimum height for all shapes w.r.t the current flat
            int offsetMin = intervals.first;
            int offsetMax = intervals.second;
            int bottomMin = 0;
            int widthMax = WidthMax;
            // cout << "Width: " << widthMax << endl;
            int transmissionPeriod = trafficFlowList[tfIndex].transmissionPeriod;

            for (int alternativeWidth = 1; alternativeWidth <= widthMax; alternativeWidth++) {
                int tmpHeight = (int) ceil((double) trafficFlowList[tfIndex].payload / alternativeWidth);
                // cout << "Height: " << tmpHeight << endl;
                if(tmpHeight >= tmpRGGssCombined.numberOfPilots) continue;
                int tmpOffsetMax = offsetMax - alternativeWidth + 1;
                for (int alternativeOffset = tmpOffsetMax; alternativeOffset >= offsetMin; alternativeOffset--) {
                    for (int alternativeBottom = bottomMin; alternativeBottom < tmpRGGssCombined.numberOfPilots; alternativeBottom++) {
                        // check collision for all packets
                        bool allPacketsSuccessFlag = false;
                        for (int packets = 0; packets < 1; packets++) {
                            int packetLeft = alternativeOffset + transmissionPeriod * packets;
                            int packetRight = alternativeOffset + alternativeWidth - 1 + transmissionPeriod * packets;
                            int packetBottom = alternativeBottom;
                            int packetHeight = tmpHeight;
                            // collision check for one packet
                            bool collisionFlag = false;
                            for (int col = packetLeft; col <= packetRight; col++) {
                                for (int row = packetBottom; row < packetBottom + packetHeight; row++) {
                                    if (tmpRGGssCombined.data[col][row] != -1) {
                                        // collision occurs
                                        collisionFlag = true;
                                        break;
                                    }
                                }
                            }
                            if (collisionFlag) {
                                // collision happened
                                break;
                            }
                            if (packets == 0) {
                                allPacketsSuccessFlag = true;
                            }
                        }
                        if (allPacketsSuccessFlag) {
                            // compare, keep the minimum one
                            if (alternativeBottom + tmpHeight < lowestSettingForAllFlat[2] + lowestSettingForAllFlat[3]) {
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
            gss.left = lowestSettingForAllFlat[0];
            gss.right = lowestSettingForAllFlat[1];
            gss.bottom = lowestSettingForAllFlat[2];
            gss.height = lowestSettingForAllFlat[3];
            tmp_groupSchedulingList.push_back(gss);
        }
        updateRGViaSchedulingList(trafficFlowList[tfIndex], tmp_groupSchedulingList, tmpRGGssCombined);
    }

    /*
     * Step 3: calculate all unallocated RUs: the difference is PRU
     */

    return diffRGGss12GssCombined(tmpRGGss12, tmpRGGssCombined);
}
void subCoNFuture(vector<TrafficFlow>& trafficFlowList,
                  TrafficFlow& tf,
                  int currentTFIndex,
                  double rwi,
                  ResourceGrid& rg,
                  int& currentRGTop,
                  std::ofstream& outlog,
                  int tfnumber,
                  std::ofstream& combinationDiff){
    std::vector<std::pair<int, int>> schedulingIntervals;
    for(int i = 0; i < rg.numberOfSlots / tf.transmissionPeriod; i++){
        schedulingIntervals.emplace_back(
                std::make_pair(ceil(tf.initialOffset + tf.transmissionPeriod * i),
                               floor(tf.initialOffset +tf.transmissionPeriod * i + tf.latencyRequirement - 1)));
    }

//    for(auto si: schedulingIntervals){
//        std::cout << si.first << " " << si.second << std::endl;
//    }
    tf.schedulingInterval = schedulingIntervals;
    vector<GroupSchedulingStruct> groupSchedulingList;
    vector<HyperGroupSchedulingStruct> hyperSchedulingList;
    int packetIndex = 0;
    // step 1: for all packets, group size = 1, find the most appropriate location for all packets
    // this is the optimal solution [the highest resource efficiency]
    // auto startTime_initialCo1 = std::chrono::steady_clock::now();
    for(auto intervals: schedulingIntervals) {
        // Initialization of the current group scheduling struct
        // cout << "intervals..." << endl;
        GroupSchedulingStruct gss{};
        gss.left = 0;
        gss.right = 0;
        gss.bottom = 0;
        gss.height = 999999;
        gss.period = tf.transmissionPeriod;
        gss.numberOfPackets = 1;
        gss.startPacketIndex = packetIndex;
        packetIndex++;

        int lowestSettingForAllFlat [] = {0, 0, 0, 99999};

        // width max
        int WidthMax;
        int freeSpace = intervals.second - intervals.first + 1;
        int h = ceil((double) tf.payload / freeSpace);
        if (h == 1) {
            WidthMax = tf.payload;
        } else {
            WidthMax = intervals.second - intervals.first + 1;
        }
        // FUNCTION: calculate the minimum height for all shapes w.r.t the current flat
        int offsetMin = intervals.first;
        int offsetMax = intervals.second;
        int bottomMin = 0;
        int widthMax = WidthMax;
        // cout << "Width: " << widthMax << endl;
        int transmissionPeriod = tf.transmissionPeriod;

        for (int alternativeWidth = 1; alternativeWidth <= widthMax; alternativeWidth++) {
            int tmpHeight = (int) ceil((double) tf.payload / alternativeWidth);
            // cout << "Height: " << tmpHeight << endl;
            if(tmpHeight >= rg.numberOfPilots) continue;
            int tmpOffsetMax = offsetMax - alternativeWidth + 1;
            for (int alternativeOffset = tmpOffsetMax; alternativeOffset >= offsetMin; alternativeOffset--) {
                for (int alternativeBottom = bottomMin; alternativeBottom < rg.numberOfPilots; alternativeBottom++) {
                    // check collision for all packets
                    bool allPacketsSuccessFlag = false;
                    for (int packets = 0; packets < 1; packets++) {
                        int packetLeft = alternativeOffset + transmissionPeriod * packets;
                        int packetRight = alternativeOffset + alternativeWidth - 1 + transmissionPeriod * packets;
                        int packetBottom = alternativeBottom;
                        int packetHeight = tmpHeight;
                        // collision check for one packet
                        bool collisionFlag = false;
                        for (int col = packetLeft; col <= packetRight; col++) {
                            for (int row = packetBottom; row < packetBottom + packetHeight; row++) {
                                if (rg.data[col][row] != -1) {
                                    // collision occurs
                                    collisionFlag = true;
                                    break;
                                }
                            }
                        }
                        if (collisionFlag) {
                            // collision happened
                            break;
                        }
                        if (packets == 0) {
                            allPacketsSuccessFlag = true;
                        }
                    }
                    if (allPacketsSuccessFlag) {
                        // compare, keep the minimum one
                        if (alternativeBottom + tmpHeight < lowestSettingForAllFlat[2] + lowestSettingForAllFlat[3]) {
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
        gss.left = lowestSettingForAllFlat[0];
        gss.right = lowestSettingForAllFlat[1];
        gss.bottom = lowestSettingForAllFlat[2];
        gss.height = lowestSettingForAllFlat[3];
        // cout << "l: " << gss.left << " r: " << gss.right << " b: " << gss.bottom << " h: " << gss.height << endl;
        if(gss.bottom+gss.height > currentRGTop){
            currentRGTop = gss.bottom + gss.height;
        }
        groupSchedulingList.push_back(gss);
        // update the current top index of RBs
    }


    int groupOffsetIndex, groupEndIndex, groupPeriod, groupPackets, groupLeft, groupRight;
    if(output_enabled){
        // update co1 per packet
        for (auto gss: groupSchedulingList) {
            groupLeft = gss.left;
            groupRight = gss.right;
            groupPackets = gss.numberOfPackets;
            groupPeriod = gss.period;
            for (int packets = 0; packets < groupPackets; packets++) {
                groupOffsetIndex = groupLeft + packets * groupPeriod;
                groupEndIndex = groupRight + packets * groupPeriod;
                rg.update(groupOffsetIndex, groupEndIndex, gss.bottom, gss.height, tf.number);
            }
        }
        outlog << char('A' + tfnumber) << " co1 per packet" << endl;
        int i, j;
        for(i = 0;i <= rg.numberOfPilots - 1; i++){
            for(j=0;j<rg.numberOfSlots;j++){
                char parsed;
                if(rg.data[j][i] == -1){
                    parsed = '0';
                }else{
                    parsed = 'A'+rg.data[j][i];
                }
                outlog << parsed << " ";
            }
            outlog << std::endl;
        }
        outlog << endl << endl << endl << endl;
        // rewind the update of rg
        for (auto gss: groupSchedulingList) {
            groupLeft = gss.left;
            groupRight = gss.right;
            groupPackets = gss.numberOfPackets;
            groupPeriod = gss.period;
            for (int packets = 0; packets < groupPackets; packets++) {
                groupOffsetIndex = groupLeft + packets * groupPeriod;
                groupEndIndex = groupRight + packets * groupPeriod;
                rg.update(groupOffsetIndex, groupEndIndex, gss.bottom, gss.height, -1);
            }
        }
    }

    // initial part -- has impact, but no big

    // auto startTime_interation = std::chrono::steady_clock::now();
    // int groupSchedulingObjectiveFunctionValue = groupSchedulingObjectiveFunction(groupSchedulingList, rg, tf);
    double scheduleProfit = 0;
    hyperSchedulingList.push_back(
            {groupSchedulingList, scheduleProfit});

    // cout << "step 2" << endl;
    /*
     * Step 2: try to combine two adjacent groups, and find the most efficient combination, then store
     * the newly generated scheduling info to the hyperSchedulingList
     * The current scheduling info is in groupSchedulingList
     *
     * IMPORTANT: establish the group combination objective function
     *              [wasted resource units -- hard to utilize in the future]
    */
    int totalNumberOfPackets = tf.numberOfConfigurations;
    // n groups --> n-1 groups --> n-2 groups --> .... --> 1 groups
    // total n-1 hyper scheduling info
    for (int hyperIteration = totalNumberOfPackets - 1; hyperIteration >= 1; hyperIteration--) {
        // cout << "\n hyper iteration.." << hyperIteration << endl;
        // for each hyper-iteration, try to select the most appropriate combination of groups
        // cout << "----------new combination loop----------------" << endl;
        int minGroupIndex = -1;     // record the index of the first group in the combination, the next is trivial
        double minCombinationObjectiveValue = 99999;
        int minGss1Offset, minGss2Offset, minGssCombinedOffset;
        bool minReduceControl;

        GroupSchedulingStruct minGss{};
        for (int combinationIndex = 0; combinationIndex < hyperIteration; combinationIndex++) {
            // cout << "combinationIndex: " << combinationIndex << endl;
            // try to combine groups, assess the combination objective function, and select one
            int startPacketIndex = groupSchedulingList[combinationIndex].startPacketIndex;
            int numberOfCoveredPackets = groupSchedulingList[combinationIndex].numberOfPackets +
                                         groupSchedulingList[combinationIndex + 1].numberOfPackets;

            // try to combine....
            GroupSchedulingStruct gss{};
            gss.startPacketIndex = startPacketIndex;
            gss.numberOfPackets = numberOfCoveredPackets;
            int scheduleIntervalStart = schedulingIntervals[0].first;
            int scheduleIntervalEnd = schedulingIntervals[0].second;
            // for each flat, calculate the minimal height to identify the shape
            // Format: left, right, bottom, height, period
            int lowestSettingForAllFlat[] = {0, 0, 0, 99999, 0};

            // width max
            int WidthMax;
            int freeSpace = scheduleIntervalEnd - scheduleIntervalStart + 1;
            int h = ceil((double) tf.payload / freeSpace);
            if (h == 1) {
                WidthMax = tf.payload;
            } else {
                WidthMax = scheduleIntervalEnd - scheduleIntervalStart + 1;
            }

            // FUNCTION: calculate the minimum height for all shapes w.r.t the current flat
            int offsetMin = scheduleIntervalStart;
            int offsetMax = scheduleIntervalEnd;
            int bottomMin = 0;
            int widthMax;
            int theoritcalWidth = (int) ceil(rwi * (double) tf.payload);
            if (WidthMax <= theoritcalWidth) widthMax = WidthMax;
            else widthMax = theoritcalWidth;

            // cout << "Width: " << widthMax << endl;
            int transmissionPeriod = tf.transmissionPeriod;
            int startPacket = 0;
            int endPacket = numberOfCoveredPackets;
            int numberOfPacketsInTheSegment = numberOfCoveredPackets;
            // record the number of packets been assigned resources
            int assignedNumberOfPackets = startPacketIndex;

            for (int alternativeWidth = 1; alternativeWidth <= widthMax; alternativeWidth++) {
                int tmpHeight = (int) ceil((double) tf.payload / alternativeWidth);
                // cout << "Height: " << tmpHeight << endl;
                int tmpOffsetMax = offsetMax - alternativeWidth + 1;
                for (int alternativeOffset = tmpOffsetMax; alternativeOffset >= offsetMin; alternativeOffset--) {
                    // decide the period of packets
                    int minP = transmissionPeriod -
                               (int) floor((double) (alternativeOffset - offsetMin) / numberOfPacketsInTheSegment);
                    int maxP = transmissionPeriod + (int) floor(
                            (double) (offsetMax - (alternativeOffset + alternativeWidth - 1)) /
                            numberOfPacketsInTheSegment);
                    for (int alternativePeriod = minP; alternativePeriod <= maxP; alternativePeriod++) {
                        for (int alternativeBottom = bottomMin;
                             alternativeBottom < rg.numberOfPilots; alternativeBottom++) {
                            // check collision for all packets
                            bool allPacketsSuccessFlag = false;
                            for (int packets = startPacket; packets < endPacket; packets++) {
                                int packetLeft = assignedNumberOfPackets * transmissionPeriod
                                                 + alternativeOffset + alternativePeriod * packets;
                                int packetRight = assignedNumberOfPackets * transmissionPeriod +
                                                  alternativeOffset + alternativeWidth - 1 +
                                                  alternativePeriod * packets;
                                int packetBottom = alternativeBottom;
                                int packetHeight = tmpHeight;
                                // collision check for one packet
                                bool collisionFlag = false;
                                for (int col = packetLeft; col <= packetRight; col++) {
                                    for (int row = packetBottom; row < packetBottom + packetHeight; row++) {
                                        if (rg.data[col][row] != -1) {
                                            // collision occurs
                                            // cout << "collision: row: " << row << " col: " << col << endl;
                                            // rg.printData();
                                            collisionFlag = true;
                                            break;
                                        }
                                    }
                                    if (collisionFlag) {
                                        // collision happened
                                        break;
                                    }
                                }
                                if (collisionFlag) {
                                    // collision happened
                                    break;
                                }
                                if (packets == endPacket - 1) {
                                    allPacketsSuccessFlag = true;
                                }
                            }
                            if (allPacketsSuccessFlag) {
                                // compare, keep the minimum one
                                if (alternativeBottom + tmpHeight <
                                    lowestSettingForAllFlat[2] + lowestSettingForAllFlat[3]) {
                                    lowestSettingForAllFlat[0] = assignedNumberOfPackets * transmissionPeriod
                                                                 + alternativeOffset;
                                    lowestSettingForAllFlat[1] = assignedNumberOfPackets * transmissionPeriod
                                                                 + alternativeOffset + alternativeWidth - 1;
                                    lowestSettingForAllFlat[2] = alternativeBottom;
                                    lowestSettingForAllFlat[3] = tmpHeight;
                                    lowestSettingForAllFlat[4] = alternativePeriod;
                                    // update gss
                                    gss.left = lowestSettingForAllFlat[0];
                                    gss.right = lowestSettingForAllFlat[1];
                                    gss.bottom = lowestSettingForAllFlat[2];
                                    gss.height = lowestSettingForAllFlat[3];
                                    gss.period = lowestSettingForAllFlat[4];
                                }
                                break;
                            }
                        }
                    }
                }
            }
            // obtain a new group scheduling structure, calculate the combination objective function
            // cout << "pre object value..." << endl;
            double combinationProfit =
                    combinationObjectiveFunctionFuture(trafficFlowList,
                                                       tf,
                                                       currentTFIndex,
                                                       groupSchedulingList,
                                                       combinationIndex,
                                                       gss,
                                                   groupSchedulingList[combinationIndex],
                                                   groupSchedulingList[combinationIndex + 1],
                                                   rg,
                                                   tf.controlOverhead
                                                   );
            // cout << "combination cost: " << combinationProfit << endl;
            bool currentReducedCURs = combinationCRUs(gss,
                                                      groupSchedulingList[combinationIndex],
                                                      groupSchedulingList[combinationIndex + 1]);
//            cout << "cov: " << combinationProfit << endl;
//            << "\t" << currentReducedCURs << endl;
            if (combinationProfit < minCombinationObjectiveValue) {
                // record the current combination
                minCombinationObjectiveValue = combinationProfit;
                minGroupIndex = combinationIndex;
                minGss = gss;
                minGss1Offset = groupSchedulingList[combinationIndex].left;
                minGss2Offset = groupSchedulingList[combinationIndex + 1].left;
                minGssCombinedOffset = gss.left;
                minReduceControl = currentReducedCURs;
            }
        }

        // obtain the current optimal combination, update the current group scheduling list
//        cout << "original list: " << endl;
//        for (auto ele: groupSchedulingList){
//            cout << ele.left << " ";
//        }
//        cout << endl;
        groupSchedulingList.erase(groupSchedulingList.begin() + minGroupIndex);
//        cout << "delete first..." << endl;
//        for (auto ele: groupSchedulingList){
//            cout << ele.left << " ";
//        }
//        cout << endl;
        groupSchedulingList.erase(groupSchedulingList.begin() + minGroupIndex);
//        cout << "delete second..." << endl;
//        for (auto ele: groupSchedulingList){
//            cout << ele.left << " ";
//        }
//        cout << endl;
        groupSchedulingList.insert(groupSchedulingList.begin() + minGroupIndex, minGss);
//        cout << "add new one..." << endl;
//        for (auto ele: groupSchedulingList){
//            cout << ele.left << " ";
//        }
//        cout << endl;
        //groupSchedulingObjectiveFunctionValue = groupSchedulingObjectiveFunction(groupSchedulingList, rg, tf);
        // cout << "min combination cost: " << minCombinationObjectiveValue << endl;
        scheduleProfit += minCombinationObjectiveValue;
        // cout << "--------------------schedule cost: " << scheduleProfit << endl;
        hyperSchedulingList.push_back(
                {groupSchedulingList, scheduleProfit,
                 minReduceControl,
                 minGss1Offset,
                 minGss2Offset,
                 minGssCombinedOffset});


//        // print out every candidate schedule per hyperIteration
//        for (auto gss: groupSchedulingList) {
//            groupLeft = gss.left;
//            groupRight = gss.right;
//            groupPackets = gss.numberOfPackets;
//            groupPeriod = gss.period;
//            for (int packets = 0; packets < groupPackets; packets++) {
//                groupOffsetIndex = groupLeft + packets * groupPeriod;
//                groupEndIndex = groupRight + packets * groupPeriod;
//                rg.update(groupOffsetIndex, groupEndIndex, gss.bottom, gss.height, tf.number);
//            }
//        }
//
//        outlog << char('A' + tfnumber) << " candidate schedule: " << std::to_string(minCombinationObjectiveValue) << endl;
//        int i, j;
//        for(i = 0;i <= rg.numberOfPilots - 1; i++){
//            for(j=0;j<rg.numberOfSlots;j++){
//                char parsed;
//                if(rg.data[j][i] == -1){
//                    parsed = '0';
//                }else{
//                    parsed = 'A'+rg.data[j][i];
//                }
//                outlog << parsed << " ";
//            }
//            outlog << std::endl;
//        }
//        outlog << endl << endl << endl << endl;
//        // rewind the update of rg
//        for (auto gss: groupSchedulingList) {
//            groupLeft = gss.left;
//            groupRight = gss.right;
//            groupPackets = gss.numberOfPackets;
//            groupPeriod = gss.period;
//            for (int packets = 0; packets < groupPackets; packets++) {
//                groupOffsetIndex = groupLeft + packets * groupPeriod;
//                groupEndIndex = groupRight + packets * groupPeriod;
//                rg.update(groupOffsetIndex, groupEndIndex, gss.bottom, gss.height, -1);
//            }
//        }


        // cout << "group scheduling objective function: " << groupSchedulingObjectiveFunctionValue << endl;
    }
//    auto endTime = std::chrono::steady_clock::now();
//    int elapsedTimeInMilliseconds =
//            std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime_interation).count();
//    cout << "iteration part: " << elapsedTimeInMilliseconds << endl;

    // update part: pruRatio = 0, consume less time, due to fewer configurations
    // cout << "Having all hyper scheduling list:..." << endl;
    // select the one with the smallest objective function
    int minHyperSchedulingIndex = 0;
    double minHyperSchedulingObjectiveValue = 999999999;
    for (int hyperSchedulingIndex = 0; hyperSchedulingIndex < hyperSchedulingList.size(); hyperSchedulingIndex++) {
        // cout << "size of group scheduling.." << hyperSchedulingList[hyperSchedulingIndex].groupSchedulingList.size() << endl;
        if (hyperSchedulingList[hyperSchedulingIndex].objectiveFunctionValue < minHyperSchedulingObjectiveValue) {
            minHyperSchedulingIndex = hyperSchedulingIndex;
            minHyperSchedulingObjectiveValue = hyperSchedulingList[hyperSchedulingIndex].objectiveFunctionValue;
        }
    }
    // cout << "----------------------------------------------min schedule cost: " <<minHyperSchedulingObjectiveValue << endl;
    combinationDiff << minHyperSchedulingObjectiveValue << " ";


    int numberOfActualConfigurations = 1;
    set<int> configurationOffsets;
    // cout << "hyperschedule size: " << hyperSchedulingList.size() << " min index: " << minHyperSchedulingIndex << endl;
    for(unsigned long iSchedules = hyperSchedulingList.size() - 1; iSchedules > minHyperSchedulingIndex; iSchedules--){
        if(hyperSchedulingList[iSchedules].reducedControl){
            configurationOffsets.insert(hyperSchedulingList[iSchedules].gss1offset);
            configurationOffsets.insert(hyperSchedulingList[iSchedules].gss2offset);
//            cout << "add offsets: " << hyperSchedulingList[iSchedules].gss1offset
//            << " " << hyperSchedulingList[iSchedules].gss2offset << endl;
            numberOfActualConfigurations++;
        }else{
            configurationOffsets.insert(hyperSchedulingList[iSchedules].gsscombinedossfet);
        }
    }
    configurationOffsets.insert(hyperSchedulingList[minHyperSchedulingIndex].gsscombinedossfet);
//    cout << "min schedule index: " << minHyperSchedulingIndex << endl;
//    cout << "Number of configurations: " << numberOfActualConfigurations << endl;
    // update the resource grid
    // cout << "size of set: " << configurationOffsets.size() << endl;
    // int groupOffsetIndex, groupEndIndex, groupPeriod, groupPackets, groupLeft, groupRight;
    tf.numberOfConfigurationActual = numberOfActualConfigurations - 1;
    tf.offsetConfigurationsActual.assign(configurationOffsets.begin(), configurationOffsets.end());
    sort(tf.offsetConfigurationsActual.begin(), tf.offsetConfigurationsActual.end());
    tf.offsetConfigurationsActual.erase(tf.offsetConfigurationsActual.begin());

    int gssIndex = 0;
    for (auto gss: hyperSchedulingList[minHyperSchedulingIndex].groupSchedulingList) {
        groupLeft = gss.left;
        groupRight = gss.right;
        groupPackets = gss.numberOfPackets;
        groupPeriod = gss.period;
//        if (gssIndex > 0) {
//            tf.offsetConfigurationsActual.push_back(groupLeft);
//        }
        // update the current top index of RBs
        if(gss.bottom+gss.height > currentRGTop){
            currentRGTop = gss.bottom + gss.height;
        }
        gssIndex++;
        // cout << "aaa" << endl;
        for (int packets = 0; packets < groupPackets; packets++) {
            groupOffsetIndex = groupLeft + packets * groupPeriod;
            groupEndIndex = groupRight + packets * groupPeriod;
            rg.update(groupOffsetIndex, groupEndIndex, gss.bottom, gss.height, tf.number);
            scheduleInfoElement s{groupOffsetIndex, groupEndIndex, gss.bottom, gss.height};
            tf.schedulingInfo.push_back(s);
            // rg.printData();
        }
        // cout << "bbb" << endl;
    }
}
/*
 * Just for demonstration
 * we need first print out the RU allocation status for every TF
 */
void CoNDynamicGroupFuture(string& cfg, int f){
    // record the current maximum index of RB allocated
    int currentRGTop = 0;
    // Step 1: parse the meta information from cfg file
    std::vector<std::string> parsedCfg = split(cfg, ' ');
    // for(auto item: parsedCfg) std::cout << item << std::endl;
    int indexOfTF = stoi(parsedCfg[0]);
    int indexOfSetting = stoi(parsedCfg[1]);
    int numberOfTFs = stoi(parsedCfg[2]);
    int numberOfPilots = stoi(parsedCfg[3]);
    // int numberOfPilots = 150;
    int hyperPeriod = stoi(parsedCfg[4]);
    int numberOfSlots = hyperPeriod;
    int indexOfFolder = stoi(parsedCfg[5]);

    std::ofstream outlog, combinationDiff;
    outlog.open("../Results/" + std::to_string(f) + "/out_" + std::to_string(indexOfTF) + ".txt");
    combinationDiff.open("../Results/combinationDiff.txt", std::ios_base::app);

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
    int scheduleIntervalStart, scheduleIntervalEnd;

    for(int tfIndex = 0; tfIndex < numberOfTFs; tfIndex++) {
        double percentage = (double)tfIndex / numberOfTFs;
        if(percentage < 0.5){
            relaxedWidthIndicator = relaxedWidthIndicatorList[0];
        }else if(percentage < 0.75){
            relaxedWidthIndicator = relaxedWidthIndicatorList[1];
        }else {
            relaxedWidthIndicator = relaxedWidthIndicatorList[2];
        }

        subCoNFuture(trafficFlowList,
                     trafficFlowList[tfIndex],
                     tfIndex,
                     relaxedWidthIndicator,
                     rg,
                     currentRGTop,
                     outlog,
                     trafficFlowList[tfIndex].number,
                     combinationDiff);
        outlog << char('A' + trafficFlowList[tfIndex].number) << endl;
        int i, j;
        for(i = 0;i <= numberOfPilots - 1; i++){
            for(j=0;j<numberOfSlots;j++){
                char parsed;
                if(rg.data[j][i] == -1){
                    parsed = '0';
                }else{
                    parsed = 'A'+rg.data[j][i];
                }
                outlog << parsed << " ";
            }
            outlog << std::endl;
        }
        outlog << endl << endl << endl << endl;

        std::vector<FreeSpaceElement> freeSpaceList;
        int localHeight;
        for(int checkIndex = 0; checkIndex < numberOfSlots; checkIndex ++){
            bool flag = true;
            int startFrequencyPointer = numberOfPilots - 1;
            for(int frequencyPointer = numberOfPilots - 1; frequencyPointer >= 0; frequencyPointer--){
                if(rg.data[checkIndex][frequencyPointer] != -1 && flag){
                    localHeight = startFrequencyPointer - frequencyPointer;
                    if(localHeight >= minControlMessageHeight){
                        freeSpaceList.push_back({checkIndex, frequencyPointer+1, localHeight});
                    }
                    flag = false;
                }else{
                    if(frequencyPointer == 0 && flag){
                        localHeight = startFrequencyPointer - frequencyPointer + 1;
                        if(localHeight >= minControlMessageHeight){
                            freeSpaceList.push_back({checkIndex, frequencyPointer, localHeight});
                        }
                    }
                }
                if(rg.data[checkIndex][frequencyPointer] == -1 && !flag){
                    startFrequencyPointer = frequencyPointer;
                    flag = true;
                }
            }
        }

        int numberNeededControlMessages = trafficFlowList[tfIndex].numberOfConfigurationActual;
        if(numberNeededControlMessages == 0) continue;
        std::vector<std::pair<int, int>> controlSchedulingInterval;
        for(i = 0; i < trafficFlowList[tfIndex].numberOfConfigurationActual; i++){
            controlSchedulingInterval.emplace_back(std::make_pair(0, trafficFlowList[tfIndex].offsetConfigurationsActual[i] - 1));
        }
        int currentStartIndex = 0;  // for freespacelist, this is the available index
        int currentEndIndex = 0;
        for(auto intervalControl: controlSchedulingInterval){
            int endSlot = intervalControl.second;
            int startSlot = std::max(0, endSlot - distanceControlConfiguration);
            // identify the current range of freeSpaceList according to endSlot and startSlot
            for(i = currentEndIndex; i < freeSpaceList.size(); i++){
                if(freeSpaceList[i].ele[0] <= endSlot){
                    continue;
                }else{
                    currentEndIndex = i-1;
                    break;
                }
            }
            for(i = currentStartIndex; i < freeSpaceList.size(); i++){
                if(freeSpaceList[i].ele[0] < startSlot){
                    continue;
                }else{
                    currentStartIndex = i;
                    break;
                }
            }
            // format of free space element: 0 column_index     1 bottom    2 available space(height)
            // find the most appropriate free space according to: a. bottom(small)  b. height(small) c. close to end
            int currentCol, currentBottom, currentHeight, freeSpaceListIndex;
//            currentCol = freeSpaceList[currentEndIndex].ele[0];
//            currentBottom = freeSpaceList[currentEndIndex].ele[1];
//            currentHeight = freeSpaceList[currentEndIndex].ele[2];
            currentBottom = 99999;

            // std::cout << "before: " << currentEndIndex << " " <<currentCol << " " << currentBottom << " " << currentHeight << std::endl;
            // std::cout << "current start index: " << currentStartIndex << " current end index: " << currentEndIndex << std::endl;

            for(i=currentEndIndex; i>=currentStartIndex;i--){
//                std::cout << i << " "   << freeSpaceList[i].ele[0] << " "
//                                        << freeSpaceList[i].ele[1] << " "
//                                        << freeSpaceList[i].ele[2] << std::endl;
                // if height not enough, skip
                if(freeSpaceList[i].ele[2] < trafficFlowList[tfIndex].controlOverhead) continue;

                if(currentBottom > freeSpaceList[i].ele[1]){
                    currentCol = freeSpaceList[i].ele[0];
                    currentBottom = freeSpaceList[i].ele[1];
                    currentHeight = freeSpaceList[i].ele[2];
                    freeSpaceListIndex = i;
                }else if(currentBottom == freeSpaceList[i].ele[1]){
                    if(currentHeight > freeSpaceList[i].ele[2]){
                        currentCol = freeSpaceList[i].ele[0];
                        currentBottom = freeSpaceList[i].ele[1];
                        currentHeight = freeSpaceList[i].ele[2];
                        freeSpaceListIndex = i;
                    }
                }
                // std::cout << currentCol << " " << currentBottom << " " << currentHeight << std::endl;
            }
            // std::cout << currentCol << " " << currentBottom << " " << currentHeight << std::endl;
            // check height
            if(currentHeight < trafficFlowList[tfIndex].controlOverhead){
//                std::cout << currentHeight << " " << trafficFlowList[tfIndex].controlOverhead << " "
//                            << tfIndex << std::endl;
                std::cout << "No solution2!" << std::endl;
                return;
            }
            // std::cout << "rrr" << std::endl;
            // update resource grid
            rg.update(currentCol,
                      currentCol,
                      currentBottom,
                      trafficFlowList[tfIndex].controlOverhead,
                      trafficFlowList[tfIndex].number + distanceDataControl);
            // update free space list
            freeSpaceList[freeSpaceListIndex].ele[1] += trafficFlowList[tfIndex].controlOverhead;
            freeSpaceList[freeSpaceListIndex].ele[2] -= trafficFlowList[tfIndex].controlOverhead;
        }

    }

    std::vector<int> topList;
    topList.push_back(0);
    std::string ss;


    auto endTime = std::chrono::steady_clock::now();
    int elapsedTimeInMilliseconds =
            std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

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

    std::ofstream heatMapLog, heatMapLog_b;
    heatMapLog.open("../Results/heatmap.txt", std::ios_base::app);
    heatMapLog_b.open("../Results/heatmap_b.txt", std::ios_base::app);

    int i, j;
    int ru = 0;
    int ruu = 0;
    int rowru = 0;
    int rowrub = 0; // before control message
    // std::cout << "Current status of the resource grid: " << std::endl;
     for (i = 0; i <= numberOfPilots - 1; i++) {
         rowru = 0;
         rowrub = 0;
         for (j = 0; j < numberOfSlots; j++) {
             char parsed;
             if (rg.data[j][i] == -1) {
                 parsed = '0';
             } else {
                 parsed = '1';
                 rowru += 1;
                 if (rg.data[j][i] <= distanceDataControl) {
                     ru += 1;
                     rowrub += 1;
                 }
                 ruu += 1;
                 parsed = 'A' + rg.data[j][i];
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
    int totalConfigurations = 0;
    for(const auto& tf: trafficFlowList){
        totalru += tf.payload * tf.numberOfConfigurations;
        totalPackets += tf.numberOfConfigurations;
        totalConfigurations += (tf.numberOfConfigurationActual + 1);
    }
//    int flag=0;
//    if(totalru > ru){
//        flag = 1;
//    }
    cout << endl;

    std::cout << "Time (ms): "<<elapsedTimeInMilliseconds << std::endl;
    double efficiency = (double) ru / (maxPilot * numberOfSlots);
    double utilization = (double) ruu / (maxPilot * numberOfSlots);
    std::cout << "Efficiency: " << efficiency << std::endl;
    std::fstream outLog;
    outLog.open("../Results/output_3.txt", std::ios_base::app);


    outLog  << std::to_string(maxPilot) << " "
            << std::to_string(elapsedTimeInMilliseconds) << " "
            << std::to_string(efficiency) << " "
            << std::to_string(utilization) << " "
            << std::to_string(totalPackets) << " "
            << std::to_string(totalConfigurations) << " "
            << std::endl;
    combinationDiff << endl;
    outlog.clear();
    outlog.flush();

}


void dumpPRU(vector<GroupSchedulingStruct>& groupSchedulingList, vector<pair<int, int>>& PRUs, ResourceGrid& rg){
    int groupOffsetIndex, groupEndIndex, groupTop, groupPeriod, groupLeft, groupRight;
    int rowIndex, colIndex;

    for(auto gss: groupSchedulingList){
        groupLeft = gss.left;
        groupRight = gss.right;
        groupTop = gss.bottom;
        groupPeriod = gss.period;
        for(int packets = 0; packets < gss.numberOfPackets; packets++){
            groupOffsetIndex = groupLeft + packets * groupPeriod;
            groupEndIndex = groupRight + packets * groupPeriod;
            for(rowIndex = 0; rowIndex < groupTop; rowIndex++){
                for(colIndex = groupOffsetIndex; colIndex <= groupEndIndex; colIndex++){
                    if(rg.data[colIndex][rowIndex] == -1){
                        PRUs.push_back(std::make_pair(colIndex, rowIndex));
                    }
                }
            }
        }
    }
}

void subCoNPRUTest(TrafficFlow& tf, ResourceGrid& rg, double rwi, double ratio_PRU, int& currentRGTop, std::ofstream& outlog, int tfnumber, vector<vector<pair<int, int>>>& PRUsAllTF){
    std::vector<std::pair<int, int>> schedulingIntervals;
    for(int i = 0; i < rg.numberOfSlots / tf.transmissionPeriod; i++){
        schedulingIntervals.emplace_back(
                std::make_pair(ceil(tf.initialOffset + tf.transmissionPeriod * i),
                               floor(tf.initialOffset +tf.transmissionPeriod * i + tf.latencyRequirement - 1)));
    }

//    for(auto si: schedulingIntervals){
//        std::cout << si.first << " " << si.second << std::endl;
//    }
    tf.schedulingInterval = schedulingIntervals;
    vector<GroupSchedulingStruct> groupSchedulingList;
    vector<HyperGroupSchedulingStruct> hyperSchedulingList;
    int packetIndex = 0;
    rwi = 1;
    // step 1: for all packets, group size = 1, find the most appropriate location for all packets
    // this is the optimal solution [the highest resource efficiency]
    // auto startTime_initialCo1 = std::chrono::steady_clock::now();
    for(auto intervals: schedulingIntervals) {
        // Initialization of the current group scheduling struct
        // cout << "intervals..." << endl;
        GroupSchedulingStruct gss{};
        gss.left = 0;
        gss.right = 0;
        gss.bottom = 0;
        gss.height = 999999;
        gss.period = tf.transmissionPeriod;
        gss.numberOfPackets = 1;
        gss.startPacketIndex = packetIndex;
        packetIndex++;

        int lowestSettingForAllFlat [] = {0, 0, 0, 99999};

        // width max
        int WidthMax;
        int freeSpace = intervals.second - intervals.first + 1;
        int h = ceil((double) tf.payload / freeSpace);
        if (h == 1) {
            WidthMax = tf.payload;
        } else {
            WidthMax = intervals.second - intervals.first + 1;
        }
        // FUNCTION: calculate the minimum height for all shapes w.r.t the current flat
        int offsetMin = intervals.first;
        int offsetMax = intervals.second;
        int bottomMin = 0;
        int widthMax = WidthMax;
        // cout << "Width: " << widthMax << endl;
        int transmissionPeriod = tf.transmissionPeriod;

        for (int alternativeWidth = 1; alternativeWidth <= widthMax; alternativeWidth++) {
            int tmpHeight = (int) ceil((double) tf.payload / alternativeWidth);
            // cout << "Height: " << tmpHeight << endl;
            if(tmpHeight >= rg.numberOfPilots) continue;
            int tmpOffsetMax = offsetMax - alternativeWidth + 1;
            for (int alternativeOffset = tmpOffsetMax; alternativeOffset >= offsetMin; alternativeOffset--) {
                for (int alternativeBottom = bottomMin; alternativeBottom < rg.numberOfPilots; alternativeBottom++) {
                    // check collision for all packets
                    bool allPacketsSuccessFlag = false;
                    for (int packets = 0; packets < 1; packets++) {
                        int packetLeft = alternativeOffset + transmissionPeriod * packets;
                        int packetRight = alternativeOffset + alternativeWidth - 1 + transmissionPeriod * packets;
                        int packetBottom = alternativeBottom;
                        int packetHeight = tmpHeight;
                        // collision check for one packet
                        bool collisionFlag = false;
                        for (int col = packetLeft; col <= packetRight; col++) {
                            for (int row = packetBottom; row < packetBottom + packetHeight; row++) {
                                if (rg.data[col][row] != -1) {
                                    // collision occurs
                                    collisionFlag = true;
                                    break;
                                }
                            }
                        }
                        if (collisionFlag) {
                            // collision happened
                            break;
                        }
                        if (packets == 0) {
                            allPacketsSuccessFlag = true;
                        }
                    }
                    if (allPacketsSuccessFlag) {
                        // compare, keep the minimum one
                        if (alternativeBottom + tmpHeight < lowestSettingForAllFlat[2] + lowestSettingForAllFlat[3]) {
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
        gss.left = lowestSettingForAllFlat[0];
        gss.right = lowestSettingForAllFlat[1];
        gss.bottom = lowestSettingForAllFlat[2];
        gss.height = lowestSettingForAllFlat[3];
        // cout << "l: " << gss.left << " r: " << gss.right << " b: " << gss.bottom << " h: " << gss.height << endl;
        if(gss.bottom+gss.height > currentRGTop){
            currentRGTop = gss.bottom + gss.height;
        }
        groupSchedulingList.push_back(gss);
        // update the current top index of RBs
    }


    int groupOffsetIndex, groupEndIndex, groupPeriod, groupPackets, groupLeft, groupRight;
    if(output_enabled){
        // update co1 per packet
        for (auto gss: groupSchedulingList) {
            groupLeft = gss.left;
            groupRight = gss.right;
            groupPackets = gss.numberOfPackets;
            groupPeriod = gss.period;
            for (int packets = 0; packets < groupPackets; packets++) {
                groupOffsetIndex = groupLeft + packets * groupPeriod;
                groupEndIndex = groupRight + packets * groupPeriod;
                rg.update(groupOffsetIndex, groupEndIndex, gss.bottom, gss.height, tf.number);
            }
        }
        outlog << char('A' + tfnumber) << " co1 per packet" << endl;
        int i, j;
        for(i = 0;i <= rg.numberOfPilots - 1; i++){
            for(j=0;j<rg.numberOfSlots;j++){
                char parsed;
                if(rg.data[j][i] == -1){
                    parsed = '0';
                }else{
                    parsed = 'A'+rg.data[j][i];
                }
                outlog << parsed << " ";
            }
            outlog << std::endl;
        }
        outlog << endl << endl << endl << endl;
        // rewind the update of rg
        for (auto gss: groupSchedulingList) {
            groupLeft = gss.left;
            groupRight = gss.right;
            groupPackets = gss.numberOfPackets;
            groupPeriod = gss.period;
            for (int packets = 0; packets < groupPackets; packets++) {
                groupOffsetIndex = groupLeft + packets * groupPeriod;
                groupEndIndex = groupRight + packets * groupPeriod;
                rg.update(groupOffsetIndex, groupEndIndex, gss.bottom, gss.height, -1);
            }
        }
    }

    // initial part -- has impact, but no big

    // auto startTime_interation = std::chrono::steady_clock::now();
    // int groupSchedulingObjectiveFunctionValue = groupSchedulingObjectiveFunction(groupSchedulingList, rg, tf);
    double scheduleProfit = 0;
    hyperSchedulingList.push_back(
            {groupSchedulingList, scheduleProfit});

    // cout << "step 2" << endl;
    /*
     * Step 2: try to combine two adjacent groups, and find the most efficient combination, then store
     * the newly generated scheduling info to the hyperSchedulingList
     * The current scheduling info is in groupSchedulingList
     *
     * IMPORTANT: establish the group combination objective function
     *              [wasted resource units -- hard to utilize in the future]
    */
    int totalNumberOfPackets = tf.numberOfConfigurations;
    // n groups --> n-1 groups --> n-2 groups --> .... --> 1 groups
    // total n-1 hyper scheduling info
    for (int hyperIteration = totalNumberOfPackets - 1; hyperIteration >= 1; hyperIteration--) {
        // cout << "\n hyper iteration.." << hyperIteration << endl;
        // for each hyper-iteration, try to select the most appropriate combination of groups
        // cout << "----------new combination loop----------------" << endl;
        int minGroupIndex = -1;     // record the index of the first group in the combination, the next is trivial
        double minCombinationObjectiveValue = 99999;
        int minGss1Offset, minGss2Offset, minGssCombinedOffset;
        bool minReduceControl;

        GroupSchedulingStruct minGss{};
        for (int combinationIndex = 0; combinationIndex < hyperIteration; combinationIndex++) {
            // cout << "combinationIndex: " << combinationIndex << endl;
            // try to combine groups, assess the combination objective function, and select one
            int startPacketIndex = groupSchedulingList[combinationIndex].startPacketIndex;
            int numberOfCoveredPackets = groupSchedulingList[combinationIndex].numberOfPackets +
                                         groupSchedulingList[combinationIndex + 1].numberOfPackets;

            // try to combine....
            GroupSchedulingStruct gss{};
            gss.startPacketIndex = startPacketIndex;
            gss.numberOfPackets = numberOfCoveredPackets;
            int scheduleIntervalStart = schedulingIntervals[0].first;
            int scheduleIntervalEnd = schedulingIntervals[0].second;
            // for each flat, calculate the minimal height to identify the shape
            // Format: left, right, bottom, height, period
            int lowestSettingForAllFlat[] = {0, 0, 0, 99999, 0};

            // width max
            int WidthMax;
            int freeSpace = scheduleIntervalEnd - scheduleIntervalStart + 1;
            int h = ceil((double) tf.payload / freeSpace);
            if (h == 1) {
                WidthMax = tf.payload;
            } else {
                WidthMax = scheduleIntervalEnd - scheduleIntervalStart + 1;
            }

            // FUNCTION: calculate the minimum height for all shapes w.r.t the current flat
            int offsetMin = scheduleIntervalStart;
            int offsetMax = scheduleIntervalEnd;
            int bottomMin = 0;
            int widthMax;
            int theoritcalWidth = (int) ceil(rwi * (double) tf.payload);
            if (WidthMax <= theoritcalWidth) widthMax = WidthMax;
            else widthMax = theoritcalWidth;

            // cout << "Width: " << widthMax << endl;
            int transmissionPeriod = tf.transmissionPeriod;
            int startPacket = 0;
            int endPacket = numberOfCoveredPackets;
            int numberOfPacketsInTheSegment = numberOfCoveredPackets;
            // record the number of packets been assigned resources
            int assignedNumberOfPackets = startPacketIndex;

            for (int alternativeWidth = 1; alternativeWidth <= widthMax; alternativeWidth++) {
                int tmpHeight = (int) ceil((double) tf.payload / alternativeWidth);
                // cout << "Height: " << tmpHeight << endl;
                int tmpOffsetMax = offsetMax - alternativeWidth + 1;
                for (int alternativeOffset = tmpOffsetMax; alternativeOffset >= offsetMin; alternativeOffset--) {
                    // decide the period of packets
                    int minP = transmissionPeriod -
                               (int) floor((double) (alternativeOffset - offsetMin) / numberOfPacketsInTheSegment);
                    int maxP = transmissionPeriod + (int) floor(
                            (double) (offsetMax - (alternativeOffset + alternativeWidth - 1)) /
                            numberOfPacketsInTheSegment);
                    for (int alternativePeriod = minP; alternativePeriod <= maxP; alternativePeriod++) {
                        for (int alternativeBottom = bottomMin;
                             alternativeBottom < rg.numberOfPilots; alternativeBottom++) {
                            // check collision for all packets
                            bool allPacketsSuccessFlag = false;
                            for (int packets = startPacket; packets < endPacket; packets++) {
                                int packetLeft = assignedNumberOfPackets * transmissionPeriod
                                                 + alternativeOffset + alternativePeriod * packets;
                                int packetRight = assignedNumberOfPackets * transmissionPeriod +
                                                  alternativeOffset + alternativeWidth - 1 +
                                                  alternativePeriod * packets;
                                int packetBottom = alternativeBottom;
                                int packetHeight = tmpHeight;
                                // collision check for one packet
                                bool collisionFlag = false;
                                for (int col = packetLeft; col <= packetRight; col++) {
                                    for (int row = packetBottom; row < packetBottom + packetHeight; row++) {
                                        if (rg.data[col][row] != -1) {
                                            // collision occurs
                                            // cout << "collision: row: " << row << " col: " << col << endl;
                                            // rg.printData();
                                            collisionFlag = true;
                                            break;
                                        }
                                    }
                                    if (collisionFlag) {
                                        // collision happened
                                        break;
                                    }
                                }
                                if (collisionFlag) {
                                    // collision happened
                                    break;
                                }
                                if (packets == endPacket - 1) {
                                    allPacketsSuccessFlag = true;
                                }
                            }
                            if (allPacketsSuccessFlag) {
                                // compare, keep the minimum one
                                if (alternativeBottom + tmpHeight <
                                    lowestSettingForAllFlat[2] + lowestSettingForAllFlat[3]) {
                                    lowestSettingForAllFlat[0] = assignedNumberOfPackets * transmissionPeriod
                                                                 + alternativeOffset;
                                    lowestSettingForAllFlat[1] = assignedNumberOfPackets * transmissionPeriod
                                                                 + alternativeOffset + alternativeWidth - 1;
                                    lowestSettingForAllFlat[2] = alternativeBottom;
                                    lowestSettingForAllFlat[3] = tmpHeight;
                                    lowestSettingForAllFlat[4] = alternativePeriod;
                                    // update gss
                                    gss.left = lowestSettingForAllFlat[0];
                                    gss.right = lowestSettingForAllFlat[1];
                                    gss.bottom = lowestSettingForAllFlat[2];
                                    gss.height = lowestSettingForAllFlat[3];
                                    gss.period = lowestSettingForAllFlat[4];
                                }
                                break;
                            }
                        }
                    }
                }
            }
            // obtain a new group scheduling structure, calculate the combination objective function
            // cout << "pre object value..." << endl;
            double combinationProfit =
                    combinationObjectiveFunctionV3(gss,
                                                   groupSchedulingList[combinationIndex],
                                                   groupSchedulingList[combinationIndex + 1],
                                                   rg,
                                                   tf.controlOverhead,
                                                   ratio_PRU,
                                                   currentRGTop);
            bool currentReducedCURs = combinationCRUs(gss,
                                                      groupSchedulingList[combinationIndex],
                                                      groupSchedulingList[combinationIndex + 1]);

//            cout << "combination objective value: " << combinationProfit
//            << "\t" << currentReducedCURs << endl;
            if (combinationProfit < minCombinationObjectiveValue) {
                // record the current combination
                minCombinationObjectiveValue = combinationProfit;
                minGroupIndex = combinationIndex;
                minGss = gss;
                minGss1Offset = groupSchedulingList[combinationIndex].left;
                minGss2Offset = groupSchedulingList[combinationIndex + 1].left;
                minGssCombinedOffset = gss.left;
                minReduceControl = currentReducedCURs;
            }
        }
        // obtain the current optimal combination, update the current group scheduling list
//        cout << "original list: " << endl;
//        for (auto ele: groupSchedulingList){
//            cout << ele.left << " ";
//        }
//        cout << endl;
        groupSchedulingList.erase(groupSchedulingList.begin() + minGroupIndex);
//        cout << "delete first..." << endl;
//        for (auto ele: groupSchedulingList){
//            cout << ele.left << " ";
//        }
//        cout << endl;
        groupSchedulingList.erase(groupSchedulingList.begin() + minGroupIndex);
//        cout << "delete second..." << endl;
//        for (auto ele: groupSchedulingList){
//            cout << ele.left << " ";
//        }
//        cout << endl;
        groupSchedulingList.insert(groupSchedulingList.begin() + minGroupIndex, minGss);
//        cout << "add new one..." << endl;
//        for (auto ele: groupSchedulingList){
//            cout << ele.left << " ";
//        }
//        cout << endl;
        //groupSchedulingObjectiveFunctionValue = groupSchedulingObjectiveFunction(groupSchedulingList, rg, tf);
        // dump the PRUs
        vector<pair<int, int>> PRUs;
        dumpPRU(groupSchedulingList, PRUs, rg);
        scheduleProfit += minCombinationObjectiveValue;
        hyperSchedulingList.push_back(
                {groupSchedulingList, scheduleProfit,
                 minReduceControl,
                 minGss1Offset,
                 minGss2Offset,
                 minGssCombinedOffset,
                 PRUs});


//        // print out every candidate schedule per hyperIteration
//        for (auto gss: groupSchedulingList) {
//            groupLeft = gss.left;
//            groupRight = gss.right;
//            groupPackets = gss.numberOfPackets;
//            groupPeriod = gss.period;
//            for (int packets = 0; packets < groupPackets; packets++) {
//                groupOffsetIndex = groupLeft + packets * groupPeriod;
//                groupEndIndex = groupRight + packets * groupPeriod;
//                rg.update(groupOffsetIndex, groupEndIndex, gss.bottom, gss.height, tf.number);
//            }
//        }
//
//        outlog << char('A' + tfnumber) << " candidate schedule: " << std::to_string(minCombinationObjectiveValue) << endl;
//        int i, j;
//        for(i = 0;i <= rg.numberOfPilots - 1; i++){
//            for(j=0;j<rg.numberOfSlots;j++){
//                char parsed;
//                if(rg.data[j][i] == -1){
//                    parsed = '0';
//                }else{
//                    parsed = 'A'+rg.data[j][i];
//                }
//                outlog << parsed << " ";
//            }
//            outlog << std::endl;
//        }
//        outlog << endl << endl << endl << endl;
//        // rewind the update of rg
//        for (auto gss: groupSchedulingList) {
//            groupLeft = gss.left;
//            groupRight = gss.right;
//            groupPackets = gss.numberOfPackets;
//            groupPeriod = gss.period;
//            for (int packets = 0; packets < groupPackets; packets++) {
//                groupOffsetIndex = groupLeft + packets * groupPeriod;
//                groupEndIndex = groupRight + packets * groupPeriod;
//                rg.update(groupOffsetIndex, groupEndIndex, gss.bottom, gss.height, -1);
//            }
//        }


        // cout << "group scheduling objective function: " << groupSchedulingObjectiveFunctionValue << endl;
    }
//    auto endTime = std::chrono::steady_clock::now();
//    int elapsedTimeInMilliseconds =
//            std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime_interation).count();
//    cout << "iteration part: " << elapsedTimeInMilliseconds << endl;

    // update part: pruRatio = 0, consume less time, due to fewer configurations
    // cout << "Having all hyper scheduling list:..." << endl;
    // select the one with the smallest objective function
    int minHyperSchedulingIndex = 0;
    double minHyperSchedulingObjectiveValue = 999999999;
    for (int hyperSchedulingIndex = 0; hyperSchedulingIndex < hyperSchedulingList.size(); hyperSchedulingIndex++) {
        // cout << "size of group scheduling.." << hyperSchedulingList[hyperSchedulingIndex].groupSchedulingList.size() << endl;
        if (hyperSchedulingList[hyperSchedulingIndex].objectiveFunctionValue < minHyperSchedulingObjectiveValue) {
            minHyperSchedulingIndex = hyperSchedulingIndex;
            minHyperSchedulingObjectiveValue = hyperSchedulingList[hyperSchedulingIndex].objectiveFunctionValue;
        }
    }
    PRUsAllTF.push_back(hyperSchedulingList[minHyperSchedulingIndex].PRUList);

    // TODO: refine the setting of each gss


    int numberOfActualConfigurations = 1;
    set<int> configurationOffsets;
    // cout << "hyperschedule size: " << hyperSchedulingList.size() << " min index: " << minHyperSchedulingIndex << endl;
    for(unsigned long iSchedules = hyperSchedulingList.size() - 1; iSchedules > minHyperSchedulingIndex; iSchedules--){
        if(hyperSchedulingList[iSchedules].reducedControl){
            configurationOffsets.insert(hyperSchedulingList[iSchedules].gss1offset);
            configurationOffsets.insert(hyperSchedulingList[iSchedules].gss2offset);
//            cout << "add offsets: " << hyperSchedulingList[iSchedules].gss1offset
//            << " " << hyperSchedulingList[iSchedules].gss2offset << endl;
            numberOfActualConfigurations++;
        }else{
            configurationOffsets.insert(hyperSchedulingList[iSchedules].gsscombinedossfet);
        }
    }
    configurationOffsets.insert(hyperSchedulingList[minHyperSchedulingIndex].gsscombinedossfet);
//    cout << "min schedule index: " << minHyperSchedulingIndex << endl;
//    cout << "Number of configurations: " << numberOfActualConfigurations << endl;
    // update the resource grid
    // cout << "size of set: " << configurationOffsets.size() << endl;
    // int groupOffsetIndex, groupEndIndex, groupPeriod, groupPackets, groupLeft, groupRight;
    tf.numberOfConfigurationActual = numberOfActualConfigurations - 1;
    tf.offsetConfigurationsActual.assign(configurationOffsets.begin(), configurationOffsets.end());
    sort(tf.offsetConfigurationsActual.begin(), tf.offsetConfigurationsActual.end());
    tf.offsetConfigurationsActual.erase(tf.offsetConfigurationsActual.begin());

    int gssIndex = 0;
    for (auto gss: hyperSchedulingList[minHyperSchedulingIndex].groupSchedulingList) {
        groupLeft = gss.left;
        groupRight = gss.right;
        groupPackets = gss.numberOfPackets;
        groupPeriod = gss.period;
//        if (gssIndex > 0) {
//            tf.offsetConfigurationsActual.push_back(groupLeft);
//        }
        // update the current top index of RBs
        if(gss.bottom+gss.height > currentRGTop){
            currentRGTop = gss.bottom + gss.height;
        }
        gssIndex++;
        // cout << "aaa" << endl;
        for (int packets = 0; packets < groupPackets; packets++) {
            groupOffsetIndex = groupLeft + packets * groupPeriod;
            groupEndIndex = groupRight + packets * groupPeriod;
            rg.update(groupOffsetIndex, groupEndIndex, gss.bottom, gss.height, tf.number);
            scheduleInfoElement s{groupOffsetIndex, groupEndIndex, gss.bottom, gss.height};
            tf.schedulingInfo.push_back(s);
            // rg.printData();
        }
        // cout << "bbb" << endl;
    }
}

/*
 * Analyze the allocation status of each PRU dumped
 */

void analyzePRUStatus(vector<vector<pair<int, int>>>& PRUsAllTF, ResourceGrid& rg, std::ofstream& PRULog){
    for(auto eachTF: PRUsAllTF){
        // for each TF:
        int PRUofTF = eachTF.size();
        int finalPRU = 0;
        for(auto eachPRU: eachTF){
            if(rg.data[eachPRU.first][eachPRU.second] == -1){
                finalPRU += 1;
            }
        }
        double r = 0;
        if(PRUofTF == 0){
            r = 0;
        }else{
            r = (double)finalPRU/PRUofTF;
        }
        PRULog << std::to_string(r) << " ";
    }
    PRULog << endl;
}

/*
 * This method is to explore the utilization of PRUs
 * 1. apply the general approach to schedule all TFs
 * 2. For each TF, when finally selecting the schedule, dump the current PRUs (locations of all RUs)
 * 3. when finishing all TFs, check for the allocation status of all dumped RUs
 */
void CoNDynamicGroupPRUTest(string& cfg, int f, double ratio_PRU){
    // record the current maximum index of RB allocated
    int currentRGTop = 0;
    // Step 1: parse the meta information from cfg file
    std::vector<std::string> parsedCfg = split(cfg, ' ');
    // for(auto item: parsedCfg) std::cout << item << std::endl;
    int indexOfTF = stoi(parsedCfg[0]);
    int indexOfSetting = stoi(parsedCfg[1]);
    int numberOfTFs = stoi(parsedCfg[2]);
    int numberOfPilots = stoi(parsedCfg[3]);
    // int numberOfPilots = 150;
    int hyperPeriod = stoi(parsedCfg[4]);
    int numberOfSlots = hyperPeriod;
    int indexOfFolder = stoi(parsedCfg[5]);

    std::ofstream outlog;
    outlog.open("../Results/" + std::to_string(f) + "/out_" + std::to_string(indexOfTF) + ".txt");

    std::ofstream PRULog;
    PRULog.open("../Results/PRULog.txt", std::ios_base::app);

    vector<vector<pair<int, int>>> PRUsAllTF;

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
    int scheduleIntervalStart, scheduleIntervalEnd;

    for(int tfIndex = 0; tfIndex < numberOfTFs; tfIndex++) {
        if(dynamicPRURatio){
            ratio_PRU = (double)(tfIndex) / numberOfTFs;
            // ratio_PRU = 0.5 + 0.3 * ratio_PRU;
//        double exp_k = 1;
//        ratio_PRU = exp(-1 * exp_k * ratio_PRU);
            // ratio_PRU = 0.05 + 0.95 * ratio_PRU;
        }

        double percentage = (double)tfIndex / numberOfTFs;
        if(percentage < 0.5){
            relaxedWidthIndicator = relaxedWidthIndicatorList[0];
        }else if(percentage < 0.75){
            relaxedWidthIndicator = relaxedWidthIndicatorList[1];
        }else {
            relaxedWidthIndicator = relaxedWidthIndicatorList[2];
        }

        subCoNPRUTest(trafficFlowList[tfIndex], rg, relaxedWidthIndicator, ratio_PRU, currentRGTop, outlog, trafficFlowList[tfIndex].number, PRUsAllTF);

        outlog << char('A' + trafficFlowList[tfIndex].number) << endl;
        int i, j;
        for(i = 0;i <= numberOfPilots - 1; i++){
            for(j=0;j<numberOfSlots;j++){
                char parsed;
                if(rg.data[j][i] == -1){
                    parsed = '0';
                }else{
                    parsed = 'A'+rg.data[j][i];
                }
                outlog << parsed << " ";
            }
            outlog << std::endl;
        }
        outlog << endl << endl << endl << endl;

        std::vector<FreeSpaceElement> freeSpaceList;
        int localHeight;
        for(int checkIndex = 0; checkIndex < numberOfSlots; checkIndex ++){
            bool flag = true;
            int startFrequencyPointer = numberOfPilots - 1;
            for(int frequencyPointer = numberOfPilots - 1; frequencyPointer >= 0; frequencyPointer--){
                if(rg.data[checkIndex][frequencyPointer] != -1 && flag){
                    localHeight = startFrequencyPointer - frequencyPointer;
                    if(localHeight >= minControlMessageHeight){
                        freeSpaceList.push_back({checkIndex, frequencyPointer+1, localHeight});
                    }
                    flag = false;
                }else{
                    if(frequencyPointer == 0 && flag){
                        localHeight = startFrequencyPointer - frequencyPointer + 1;
                        if(localHeight >= minControlMessageHeight){
                            freeSpaceList.push_back({checkIndex, frequencyPointer, localHeight});
                        }
                    }
                }
                if(rg.data[checkIndex][frequencyPointer] == -1 && !flag){
                    startFrequencyPointer = frequencyPointer;
                    flag = true;
                }
            }
        }

        int numberNeededControlMessages = trafficFlowList[tfIndex].numberOfConfigurationActual;
        if(numberNeededControlMessages == 0) continue;
        std::vector<std::pair<int, int>> controlSchedulingInterval;
        for(int i = 0; i < trafficFlowList[tfIndex].numberOfConfigurationActual; i++){
            controlSchedulingInterval.emplace_back(std::make_pair(0, trafficFlowList[tfIndex].offsetConfigurationsActual[i] - 1));
        }
        int currentStartIndex = 0;  // for freespacelist, this is the available index
        int currentEndIndex = 0;
        for(auto intervalControl: controlSchedulingInterval){
            int endSlot = intervalControl.second;
            int startSlot = std::max(0, endSlot - distanceControlConfiguration);
            // identify the current range of freeSpaceList according to endSlot and startSlot
            for(int i = currentEndIndex; i < freeSpaceList.size(); i++){
                if(freeSpaceList[i].ele[0] <= endSlot){
                    continue;
                }else{
                    currentEndIndex = i-1;
                    break;
                }
            }
            for(int i = currentStartIndex; i < freeSpaceList.size(); i++){
                if(freeSpaceList[i].ele[0] < startSlot){
                    continue;
                }else{
                    currentStartIndex = i;
                    break;
                }
            }
            // format of free space element: 0 column_index     1 bottom    2 available space(height)
            // find the most appropriate free space according to: a. bottom(small)  b. height(small) c. close to end
            int currentCol, currentBottom, currentHeight, freeSpaceListIndex;
//            currentCol = freeSpaceList[currentEndIndex].ele[0];
//            currentBottom = freeSpaceList[currentEndIndex].ele[1];
//            currentHeight = freeSpaceList[currentEndIndex].ele[2];
            currentBottom = 99999;

            // std::cout << "before: " << currentEndIndex << " " <<currentCol << " " << currentBottom << " " << currentHeight << std::endl;
            // std::cout << "current start index: " << currentStartIndex << " current end index: " << currentEndIndex << std::endl;

            for(int i=currentEndIndex; i>=currentStartIndex;i--){
//                std::cout << i << " "   << freeSpaceList[i].ele[0] << " "
//                                        << freeSpaceList[i].ele[1] << " "
//                                        << freeSpaceList[i].ele[2] << std::endl;
                // if height not enough, skip
                if(freeSpaceList[i].ele[2] < trafficFlowList[tfIndex].controlOverhead) continue;

                if(currentBottom > freeSpaceList[i].ele[1]){
                    currentCol = freeSpaceList[i].ele[0];
                    currentBottom = freeSpaceList[i].ele[1];
                    currentHeight = freeSpaceList[i].ele[2];
                    freeSpaceListIndex = i;
                }else if(currentBottom == freeSpaceList[i].ele[1]){
                    if(currentHeight > freeSpaceList[i].ele[2]){
                        currentCol = freeSpaceList[i].ele[0];
                        currentBottom = freeSpaceList[i].ele[1];
                        currentHeight = freeSpaceList[i].ele[2];
                        freeSpaceListIndex = i;
                    }
                }
                // std::cout << currentCol << " " << currentBottom << " " << currentHeight << std::endl;
            }
            // std::cout << currentCol << " " << currentBottom << " " << currentHeight << std::endl;
            // check height
            if(currentHeight < trafficFlowList[tfIndex].controlOverhead){
//                std::cout << currentHeight << " " << trafficFlowList[tfIndex].controlOverhead << " "
//                            << tfIndex << std::endl;
                std::cout << "No solution2!" << std::endl;
                return;
            }
            // std::cout << "rrr" << std::endl;
            // update resource grid
            rg.update(currentCol,
                      currentCol,
                      currentBottom,
                      trafficFlowList[tfIndex].controlOverhead,
                      trafficFlowList[tfIndex].number + distanceDataControl);
            // update free space list
            freeSpaceList[freeSpaceListIndex].ele[1] += trafficFlowList[tfIndex].controlOverhead;
            freeSpaceList[freeSpaceListIndex].ele[2] -= trafficFlowList[tfIndex].controlOverhead;
        }

    }

    std::vector<int> topList;
    topList.push_back(0);
    std::string ss;


    auto endTime = std::chrono::steady_clock::now();
    int elapsedTimeInMilliseconds =
            std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

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
                parsed = 'A'+rg.data[j][i];
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
    int totalConfigurations = 0;
    for(const auto& tf: trafficFlowList){
        totalru += tf.payload * tf.numberOfConfigurations;
        totalPackets += tf.numberOfConfigurations;
        totalConfigurations += (tf.numberOfConfigurationActual + 1);
    }
//    int flag=0;
//    if(totalru > ru){
//        flag = 1;
//    }
    cout << endl;

    std::cout << "Time (ms): "<<elapsedTimeInMilliseconds << std::endl;
    double efficiency = (double) ru / (maxPilot * numberOfSlots);
    double utilization = (double) ruu / (maxPilot * numberOfSlots);
    std::cout << "Efficiency: " << efficiency << std::endl;
    std::fstream outLog;
    if(!dynamicPRURatio){
        outLog.open("../Results/output_0_" + std::to_string(ratio_PRU) + ".txt", std::ios_base::app);
    }else{
        outLog.open("../Results/output_0.txt", std::ios_base::app);
    }

    analyzePRUStatus(PRUsAllTF, rg, PRULog);


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
//void performCoU(int f){
//    cfgInformation.clear();
//    extractCfg(f);
//    if(ifTest){
//        CoNDynamicGroupV2(testSample, f, 1);
//    } else{
//        int count = 0;
//        for (std::string &cfg: cfgInformation){
//            if(count < 5){
//                cout << "CoU" << endl;
//                CoNDynamicGroupVTest(cfg, f, 1);
//            }
//            count+=1;
////            if(!dynamicPRURatio){
////                for (double pru_ratio: pru_ratio_list){
////                    cout << "CoU -- PRU_ratio: " << pru_ratio << endl;
////                    CoNDynamicGroupV2(cfg, f, pru_ratio);
////                }
////            }else{
////                CoNDynamicGroupV2(cfg, f, -1);
////            }
//
//        }
//    }
//}

void performCoU(int f){
    cfgInformation.clear();
    extractCfg(f);
    if(ifTest){
        CoNDynamicGroupFuture(testSample, f);
    } else{
        for (std::string &cfg: cfgInformation){
//            if(!dynamicPRURatio){
//                for (double pru_ratio: pru_ratio_list){
//                    cout << "CoU -- PRU_ratio: " << pru_ratio << endl;
//                    CoNDynamicGroupV2(cfg, f, pru_ratio);
//                }
//            }else{
//                CoNDynamicGroupV2(cfg, f, -1);
//            }
            // CoNDynamicGroupFuture(cfg, f);
            CoNDynamicGroupV3(cfg, f, 1);
            // CoNDynamicGroupVTest(cfg, f, 1);
//            for (double pru_ratio: pru_ratio_list){
//                    cout << "CoU -- PRU_ratio: " << pru_ratio << endl;
//                    CoNDynamicGroupV3(cfg, f, pru_ratio);
//            }
        }
    }
}