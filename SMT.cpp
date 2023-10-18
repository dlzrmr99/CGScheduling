//
// Created by yunpa38 on 2022-10-12.
// Implementation for "SMT".
//
#include "SMT.h"
#include "Common.h"



void SMT(string& cfg){
    std::cout << "Beginning of SMT solution (C++)" << std::endl;
    // record the start time
    auto systemStartTime = std::chrono::steady_clock::now();

//    int elapsedTimeInMilliseconds =
//            std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

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

    std::ofstream logPilots, logSolvingTime, logFile, logFile_;
    logPilots.open("../Results/0/schedules.txt");
    logSolvingTime.open("../Results/0/solvingTime.txt");
    logFile.open("../Results/0/" + std::to_string(indexOfTF) + ".txt");
    logFile_.open("../Results/0/" + std::to_string(indexOfTF) +"_report.txt");
    // print out the setting of traffic flows
    cout << trafficFlowList << endl;

    // calculate maximum allowed number of configurations
    int sum_maximum_conf = 0;
    for(auto tf : trafficFlowList){
        sum_maximum_conf += tf.numberOfConfigurations;
    }

    string ss = "Start time: 0\n";
    cout << ss;
    logFile << ss;

    bool verbose = true;
    logFile << "============================INFORMATION OF TRAFFIC FLOWS==================================\n";
    for (int i = 0; i < trafficFlowList.size(); i++) {
        string s =
                "Index: " + std::to_string(i) + \
                    "\t Initial Offset: " + std::to_string(trafficFlowList[i].initialOffset) + \
            "\tTransmission Period: " + std::to_string(trafficFlowList[i].transmissionPeriod) + \
            "\tPayload: " + std::to_string(trafficFlowList[i].payload) + " resource units" + \
            "\tMaximum Delay Requirement: " + std::to_string(trafficFlowList[i].latencyRequirement) + \
            "\tMaximum number of configurations: " + std::to_string(trafficFlowList[i].numberOfConfigurations) + \
            "\tControl overhead: " + std::to_string(trafficFlowList[i].controlOverhead) + " resource units \n";
        logFile << s;
    }
    logFile << "============================SYSTEM PARAMETERS==================================\n";
    logFile << "Length of each time slot: " << std::to_string(0.5) << "\n";
    logFile << "Hyperperiod: " << std::to_string(hyperPeriod) << "\n";
    logFile << "Number of pilots: " << std::to_string(numberOfPilots) << "\n";
    cout << "Number of pilots: " << std::to_string(numberOfPilots) << "\n";

    int max_RBAllocationPatterns = int((numberOfPilots * (numberOfPilots + 1) / 2));
    // matrix of the resource allocation patterns
    vector<vector<int>> frequencyPattern;
        // auxiliary function for the optimization goal to know the number of RB used
    vector<int> frequencyPatternTop;
    for(int i = 0; i < numberOfPilots; i++){
        for(int s = 0; s < numberOfPilots - i; s++){
            frequencyPatternTop.push_back(numberOfPilots - s);
            vector<int> tmp;
            for(int idx = 0; idx < s; idx++){
                tmp.push_back(0);
            }
            for(int idx = s; idx <= s + i; idx++){
                tmp.push_back(1);
            }
            if(i == numberOfPilots - 1){
                frequencyPattern.push_back(tmp);
                continue;
            }
            for(int idx = i + 1; idx < numberOfPilots; idx++){
                tmp.push_back(0);
            }
            frequencyPattern.push_back(tmp);
        }
    }
    // the output of resource allocation pattern is correct
//    for(int i = numberOfPilots - 1;i >= 0; i--){
//        for(int j = 0; j < max_RBAllocationPatterns; j++){
//            cout << frequencyPattern[j][i] << " ";
//        }
//        cout << endl;
//    }

//    for(auto i: frequencyPatternTop){
//        cout << i << " ";
//    }
    // calculate the number of assigned resource blocks, which is used to fulfill the payload constraint
    vector<int> frequencyPatternRB;
    for(const auto& ele: frequencyPattern){
        int sum = 0;
        for(int i: ele){
            sum += i;
        }
        frequencyPatternRB.push_back(sum);
    }
//    for(auto ele: frequencyPatternRB){
//        cout << ele << " ";
//    }
    logFile << "Number of pilots: " << numberOfPilots << "\n";
    logFile << "Max allocation patterns: " << max_RBAllocationPatterns << "\n";
    logFile << "=========================ALLOCATION PATTERN=======================\n";
    for(int i = 0;i < numberOfPilots; i++){
        for(int j = 0; j < max_RBAllocationPatterns; j++){
            logFile << frequencyPattern[j][i] << " ";
        }
        logFile << endl;
    }

    // maxLen is for the maximum number of consecutively assigned time slots
    int maxLen;
    vector<int> latencyList;
    for(auto const& ele: trafficFlowList){
        latencyList.push_back(ele.latencyRequirement);
    }
    maxLen = trafficFlowList[vector_max_idx(latencyList)].latencyRequirement;
    cout << "Max len: " << maxLen << endl;
    logFile << "Max Len: \t" << maxLen << endl;

    // maxK is for the maximum number of repetitions, here, by default it's 1
    int maxK = 1;


    vector<int> neededRB, neededK;
    // the number of data packets may not equal to the number of configurations
    // the number of configurations is given by the input data
    vector<int> numberOfDataPackets, numberOfConfigurations;
    for(auto const& ele: trafficFlowList){
        neededRB.push_back(ele.payload);
        neededK.push_back(1);
        numberOfDataPackets.push_back(int(floor(hyperPeriod / ele.transmissionPeriod)));
        numberOfConfigurations.push_back(ele.numberOfConfigurations);
    }

    /*
     * establish the resource grid
     */
    // the hyperPeriod has been normalized to the length of each time slot as per the input data
    int numberOfTimeSlotResourceGrid = hyperPeriod;

    /*
    # For each traffic flow, it owns a series of scheduling configurations
    # for each scheduling configuration, it has seven features that have to be determined by the SMT solver:
    # 0     Resource block allocation scheme
    # 1     Offset (index of time slot for first transmission): 𝑜
    # 2     # of consecutive time slots: 𝑙𝑒𝑛
    # 3     # of repetitions: 𝑘∈{1, 2, 4, 8}
    # 4     # interval between repetitions (# of slots): 𝑞
    # 5     Periodicity: 𝑝
    # 6     repeat number: r
    # 7     valid indicator: v      then, SUM(r*v) == number of data packets per TF
     */

    int maxFeaturesPerConfiguration = 8;
    context smtContext;
    expr_vector tmp_and(smtContext);

    z3::set_param("parallel.enable", true);

    /*
     *  expr_vector x(c);
     *  std::stringstream x_name;
        x_name << "x_" << i;
        x.push_back(c.int_const(x_name.str().c_str()));
     */

    // vector<vector<vector<expr>>> configurationSeries;
    vector<vector<expr_vector>> configurationSeries;

    for(int i = 0; i < numberOfTFs; i++){
        vector<expr_vector> tmp;
        for(int j = 0; j < numberOfConfigurations[i]; j++){
            expr_vector tmp_(smtContext);
            for(int k = 0; k < maxFeaturesPerConfiguration; k++){
                tmp_.push_back(smtContext.int_const(("conf" +
                                                    std::to_string(i) +
                                                    "_" + std::to_string(j) +
                                                    "_" + std::to_string(k)).c_str()));

            }
            tmp.push_back(tmp_);
        }
        configurationSeries.push_back(tmp);
    }
        // until now, it is the same with the python version
//    for(int i = 0; i < numberOfTFs; i++){
//        for(int j = 0; j < numberOfConfigurations[i]; j++){
//            for(int k = 0; k < maxFeaturesPerConfiguration; k++){
//                cout << configurationSeries[i][j][k] << " ";
//            }
//        }
//    }

    int maxFeaturesPerConfigurationControl = 4;
    vector<int> numberControlConfigurations;
    for(auto const& ele: numberOfConfigurations){
        numberControlConfigurations.push_back(ele - 1);
    }

    vector<vector<expr_vector>> controlConfigurations;
    for(int i = 0; i < numberOfTFs; i++){
        vector<expr_vector> tmp;
        for(int j = 0; j < numberControlConfigurations[i]; j++){
            expr_vector tmp_(smtContext);
            for(int k = 0; k < maxFeaturesPerConfigurationControl; k++){
                tmp_.push_back(smtContext.int_const(("con_conf" +
                                                     std::to_string(i) +
                                                     "_" + std::to_string(j) +
                                                     "_" + std::to_string(k)).c_str()));
            }
            tmp.push_back(tmp_);
        }
        controlConfigurations.push_back(tmp);
    }

    optimize smtOptimize(smtContext);
    logFile << "===========================CREATE OPTIMIZER AND ADD CONSTRAINTS==================================\n";

    /*
     * Add constraint for all configuration related parameters
     */
    // 1. constraint for "RB allocation pattern"
    tmp_and.resize(0);
    for(int i = 0; i < numberOfTFs; i++){
        // for data transmission
        for(int j = 0; j < numberOfConfigurations[i]; j++){
            tmp_and.push_back(configurationSeries[i][j][0] >= 0
                            && configurationSeries[i][j][0] <= max_RBAllocationPatterns - 1);
        }
        // for control message
        for(int j = 0; j < numberControlConfigurations[i]; j++){
            tmp_and.push_back(controlConfigurations[i][j][0] >= 0 &&
                            controlConfigurations[i][j][0] <= max_RBAllocationPatterns - 1);
        }
    }
    smtOptimize.add(z3::mk_and(tmp_and));
    cout << "Adding constraints: range of allocation patterns" << endl;

    // 2. constraint for "total number of RBs"
    z3::sort z3Int = smtContext.int_sort();
    // func_decl g = function("g", I, I);
    func_decl translation_RB_Function = function("translation_RB_Function", z3Int, z3Int);
    tmp_and.resize(0);
    for(int i=0;i<max_RBAllocationPatterns;i++){
        tmp_and.push_back(translation_RB_Function(i) == frequencyPatternRB[i]);
    }
    smtOptimize.add(z3::mk_and(tmp_and));

    func_decl translation_RB_Top_Function = function("translation_RB_Top_Function", z3Int, z3Int);
    tmp_and.resize(0);
    for(int i=0;i<max_RBAllocationPatterns;i++){
        tmp_and.push_back(translation_RB_Top_Function(i) == frequencyPatternTop[i]);
    }
    smtOptimize.add(z3::mk_and(tmp_and));

    tmp_and.resize(0);
    for(int i = 0; i < numberOfTFs; i++){
        // for data transmission
        for(int j = 0; j < numberOfConfigurations[i]; j++){
            tmp_and.push_back(translation_RB_Function(configurationSeries[i][j][0])
                                                        * configurationSeries[i][j][2]
                                                        >= neededRB[i]);
        }
        // for control message
        for(int j = 0; j < numberControlConfigurations[i]; j++){
            tmp_and.push_back(translation_RB_Function(controlConfigurations[i][j][0])
                                                        * controlConfigurations[i][j][2]
                                                        == trafficFlowList[i].controlOverhead);
        }
    }
    smtOptimize.add(z3::mk_and(tmp_and));
    cout << "Adding constraints: assigned RBs >= needed RBs" << endl;

    // 3. constraint for "number of repetition"
    tmp_and.resize(0);
    for(int i = 0; i < numberOfTFs; i++){
        // for data transmission
        for(int j = 0; j < numberOfConfigurations[i]; j++){
            tmp_and.push_back(configurationSeries[i][j][3] == neededK[i]);
        }
    }
    smtOptimize.add(z3::mk_and(tmp_and));
    cout << "Adding constraints: assigned ks == Needed Ks" << endl;

    // 4. constraint for the sum of data packets
    for(int i = 0; i < numberOfTFs; i++){
        expr sumValidCountPerTF = configurationSeries[i][0][6] * configurationSeries[i][0][7];
        for(int j = 1; j < numberOfConfigurations[i]; j++){
            sumValidCountPerTF = sumValidCountPerTF + configurationSeries[i][j][6] * configurationSeries[i][j][7];
        }
        smtOptimize.add(sumValidCountPerTF == numberOfDataPackets[i]);
    }
    cout << "Adding constraints: sum of the data packets" << endl;

    // 5. constraints for "valid indicator": v
    tmp_and.resize(0);
    for(int i = 0; i < numberOfTFs; i++){
        // for data transmission
        for(int j = 0; j < numberOfConfigurations[i]; j++){
            tmp_and.push_back(configurationSeries[i][j][7] >= 0 && configurationSeries[i][j][7] <= 1);
        }
        // for control message
        for(int j = 0; j < numberControlConfigurations[i]; j++){
            tmp_and.push_back(controlConfigurations[i][j][3] >= 0 && controlConfigurations[i][j][3] <= 1);
        }
    }
    smtOptimize.add(z3::mk_and(tmp_and));
    cout << "Adding constraints: every 'valid indicator' == 0/1" << endl;

    // 6. the valid indicator of control message == the valid indicator of data packet configuration
    for(int i = 0; i < numberOfTFs; i++){
        // sum of the valid indicator of data packet configuration
        expr tmp_data_packet_configuration = configurationSeries[i][0][7];
        for(int j = 1; j < numberOfConfigurations[i]; j++){
            tmp_data_packet_configuration = tmp_data_packet_configuration + configurationSeries[i][j][7];
        }
        // sum of the valid indicator of control message
        // case 1: 0 control configuration
        if(numberControlConfigurations[i] == 0){
            smtOptimize.add(tmp_data_packet_configuration == 1);
        }
        continue;
        // case 2: >0 control configurations
        expr tmp_valid_indicator_control = controlConfigurations[i][0][3];
        for(int j = 1; j < numberControlConfigurations[i]; j++){
            tmp_valid_indicator_control = tmp_valid_indicator_control + controlConfigurations[i][j][3];
        }
        smtOptimize.add((tmp_valid_indicator_control + 1) == tmp_data_packet_configuration);
    }
    cout << "Adding constraints: the number of valid indicator w.r.t control messages & data packet configuration" << endl;

    // 7. basic constraints for "all parameters"
    // 7.1 offset
    tmp_and.resize(0);
    for(int i = 0; i < numberOfTFs; i++){
        // for data transmission
        for(int j = 0; j < numberOfConfigurations[i]; j++){
            tmp_and.push_back(configurationSeries[i][j][1] >= 0);
        }
        // for control
        for(int j = 0; j < numberControlConfigurations[i]; j++){
            tmp_and.push_back(controlConfigurations[i][j][1] >= 0);
        }
    }
    smtOptimize.add(z3::mk_and(tmp_and));

    // 7.2 len
    tmp_and.resize(0);
    for(int i = 0; i < numberOfTFs; i++){
        // for data transmission
        for(int j = 0; j < numberOfConfigurations[i]; j++){
            // todo: here max_len to be verified
            tmp_and.push_back(configurationSeries[i][j][2] >= 1 && configurationSeries[i][j][2] <= maxLen);
        }
        // for control
        for(int j = 0; j < numberControlConfigurations[i]; j++){
            tmp_and.push_back(controlConfigurations[i][j][2] == 1);
        }
    }
    smtOptimize.add(z3::mk_and(tmp_and));

    // 7.3 k
    tmp_and.resize(0);
    for(int i = 0; i < numberOfTFs; i++){
        // for data transmission
        for(int j = 0; j < numberOfConfigurations[i]; j++){
            tmp_and.push_back(configurationSeries[i][j][3] >= 1 && configurationSeries[i][j][3] <= maxK);
        }
    }
    smtOptimize.add(z3::mk_and(tmp_and));

    // 7.4 interval
    tmp_and.resize(0);
    for(int i = 0; i < numberOfTFs; i++){
        // for data transmission
        for(int j = 0; j < numberOfConfigurations[i]; j++){
            tmp_and.push_back(configurationSeries[i][j][4] == 0);
        }
    }
    smtOptimize.add(z3::mk_and(tmp_and));

    // 7.5 p
    tmp_and.resize(0);
    for(int i = 0; i < numberOfTFs; i++){
        // for data transmission
        for(int j = 0; j < numberOfConfigurations[i]; j++){
            tmp_and.push_back(configurationSeries[i][j][5] >= 1 &&
                            configurationSeries[i][j][5] < 2 * trafficFlowList[i].transmissionPeriod &&
                            configurationSeries[i][j][5] < hyperPeriod);
        }
    }
    smtOptimize.add(z3::mk_and(tmp_and));

    // 7.6 r: the number of consecutive data packets within one configuration
    tmp_and.resize(0);
    for(int i = 0; i < numberOfTFs; i++){
        // for data transmission
        for(int j = 0; j < numberOfConfigurations[i]; j++){
            tmp_and.push_back(configurationSeries[i][j][6] >= 1 &&
                            configurationSeries[i][j][6] <= numberOfDataPackets[i]);
        }
    }
    smtOptimize.add(z3::mk_and(tmp_and));
    cout << "Adding constraints: basic settings for all parameters!" << endl;

    /*
     *  # ========================== Resource mapping ===============================
        # building the connection between configurations and Resource grid
        # each traffic flow would occupy an empty and complete resource grid at first
        # and then all resource grids will be combined (perform addition for each resource block)
        # the goal is to keep all values lower than 1 which means that after considering all traffic flows that is valid
        # each resource block can be assigned only once

        # resource grid function arguments:
        # 1. numbering of traffic flows [every traffic flow has a complete resource grid]
        # 2. possible 'pos' [time resource: means the time slot because the resource grid is divided by time slots]
        # 3. pilots/resource block         [frequency resource]
        # 4. return value   [0/1, 0 means that the current resource block (using time resource and frequency resource
        #                   to locate) isn't assigned for a specific traffic flow]
        # Reason about why use 'Function' here:
        # 'Function' can provide the mapping functionality between scheduling configuration and resource blocks
        # Next, add constraints upon the input and output of the 'Function'
     */

    func_decl resource_grid_function = function("resource_grid_function", z3Int, z3Int, z3Int,z3Int);
    // constraint 1: return value must be 0/1

    for(int pilot = 0; pilot < numberOfPilots; pilot++){
        for(int pos = 0; pos < numberOfTimeSlotResourceGrid; pos++){
            for(int tf = 0; tf < numberOfTFs; tf++){
                // todo: to be verified. get inspiration from the z3++.h
                tmp_and.push_back(resource_grid_function(smtContext.num_val(tf, z3Int),
                                                       smtContext.num_val(pos, z3Int),
                                                       smtContext.num_val(pilot, z3Int)) >= 0 &&
                                   resource_grid_function(smtContext.num_val(tf, z3Int),
                                                          smtContext.num_val(pos, z3Int),
                                                          smtContext.num_val(pilot, z3Int)) <= 1);
            }
        }
    }
    smtOptimize.add(z3::mk_and(tmp_and));
    cout << "Adding constraints: for each resource unit, the value can only be 0/1" << endl;

    // resource grid function for control message
    func_decl resource_grid_function_control = function("resource_grid_function_control", z3Int, z3Int, z3Int, z3Int);
    // constraint 1: return value must be 0/1

    tmp_and.resize(0);
    // cout << "size of tmp: " << std::to_string(tmp.size()) << endl;
    for(int pilot = 0; pilot < numberOfPilots; pilot++){
        for(int pos = 0; pos < numberOfTimeSlotResourceGrid; pos++){
            for(int tf = 0; tf < numberOfTFs; tf++){
                // todo: to be verified. get inspiration from the z3++.h
                tmp_and.push_back(resource_grid_function_control(smtContext.num_val(tf, z3Int),
                                                       smtContext.num_val(pos, z3Int),
                                                       smtContext.num_val(pilot, z3Int)) >= 0 &&
                                resource_grid_function_control(smtContext.num_val(tf, z3Int),
                                                       smtContext.num_val(pos, z3Int),
                                                       smtContext.num_val(pilot, z3Int)) <= 1);
            }
        }
    }
    smtOptimize.add(z3::mk_and(tmp_and));
    cout << "Adding constraints: for each resource unit, the value can only be 0/1 (for control messages)" << endl;

    // constraints for data packet arrival time & deadline
    for(int i = 0; i < numberOfTFs; i++){
        for(int j = 0; j < numberOfConfigurations[i]; j++){
            for(int k = 1; k <= numberOfDataPackets[i]; k++){
                // implies constraint: part 1
                expr part1 = configurationSeries[i][j][6] == k && configurationSeries[i][j][7] == 1;
                // implies constraint: part 2
                for(int k_ = 0; k_ < k; k_++){
                 // part 2.1: for arrival time
                     // part 2.1.1: the initial time indicated by the configuration
                     expr part2_1_1 = configurationSeries[i][j][1] + k_ * configurationSeries[i][j][5];

                // part 2.2: for deadline
                    // part 2.1.1: the deadline indicated by the configuration
                    expr part2_2_1 = configurationSeries[i][j][1] +
                                     configurationSeries[i][j][2] * configurationSeries[i][j][3] +
                                     configurationSeries[i][j][4] * (configurationSeries[i][j][3] - 1) +
                                     k_ * configurationSeries[i][j][5] - 1;

                    // part 2.1.2: the initial time indicated by the data packet itself
                    // case 1: j == 0
                     if(j == 0){
                         expr part2_1 = part2_1_1 >=
                                 (trafficFlowList[i].initialOffset + k_ * trafficFlowList[i].transmissionPeriod);
                         // part 2.2.2: the deadline indicated by the data packet itself
                         // case 1: j == 0
                         expr part2_2 = part2_2_1 <
                                        (trafficFlowList[i].initialOffset +
                                         k_ * trafficFlowList[i].transmissionPeriod +
                                         trafficFlowList[i].latencyRequirement);
                         smtOptimize.add(z3::implies(part1, (part2_1 && part2_2)));

                     }else{
                     // case 2: j > 0
                        expr tmp2_1_2 = configurationSeries[i][0][6] * configurationSeries[i][0][7];
                         for(int j_ = 1; j_ < j; j_++){
                             tmp2_1_2 = tmp2_1_2 + configurationSeries[i][j_][6] * configurationSeries[i][j_][7];
                         }
                         expr part2_1 = part2_1_1 >=
                                 (trafficFlowList[i].initialOffset +
                                                (tmp2_1_2 + k_) * trafficFlowList[i].transmissionPeriod);
                         // part 2.2.2: the deadline indicated by the data packet itself
                             // case 2: j > 0
                             expr_vector tmp2_2_2(smtContext);
                         for(int j_ = 0; j_ < j; j_++){
                             tmp2_2_2.push_back(configurationSeries[i][j_][6] * configurationSeries[i][j_][7]);
                         }
                         expr part2_2 = part2_2_1 <
                                        (trafficFlowList[i].initialOffset +
                                         (sum(tmp2_2_2) + k_) * trafficFlowList[i].transmissionPeriod +
                                         trafficFlowList[i].latencyRequirement);
                         smtOptimize.add(z3::implies(part1, (part2_1 && part2_2)));
                         }
                }
            }
            smtOptimize.add(z3::implies(configurationSeries[i][j][7] == 1,
                                        configurationSeries[i][j][1] + configurationSeries[i][j][2] <= hyperPeriod));
        }
    }

    cout << "Adding constraints: arrival time and deadline" << endl;

    // control message issued before each configuration
    // step 0:let the first valid indicator of data configuration equals to 1
    tmp_and.resize(0);
    for(int i = 0; i < numberOfTFs; i++){
        tmp_and.push_back(configurationSeries[i][0][7] == 1);
    }
    smtOptimize.add(z3::mk_and(tmp_and));
    // step 1: let the valid indicator equal [control & configuration]
    // step 2: for every valid pair, set constraint
    tmp_and.resize(0);
    for(int i = 0; i< numberOfTFs; i++){
        for(int j = 1; j < numberOfConfigurations[i]; j++){
            tmp_and.push_back(controlConfigurations[i][j-1][3] == configurationSeries[i][j][7]);
            tmp_and.push_back(controlConfigurations[i][j-1][1] < configurationSeries[i][j][1]);
        }
    }
    smtOptimize.add(z3::mk_and(tmp_and));

    /*
     *  # resource grid flag function arguments:
        # 1. numbering of traffic flows [every traffic flow has a complete resource grid]
        # 2. possible 'pos' [time resource: means the time slot because the resource grid is divided by time slots]
        # 3. return value   [0/1]
        # Reason:
        # 'resource_grid_flag_function' (RGFF) is an auxiliary function of 'resource_grid_function' (RGF)
        # when we use scheduling configurations to determine whether a 'pos' would be assigned
        # it's easy to record assignment status of the pilots ['resource block allocation scheme'] in the current 'pos'
        # but for other idle 'pos', the RGF don't know how to handle the value of each location
        # so that we have to guarantee that all resource blocks in idle 'pos' equal to 0
     */
    func_decl resource_grid_flag_function = function("resource_grid_flag_function", z3Int, z3Int, z3Int);
    // constraint 1: return value 0/1
    tmp_and.resize(0);
    for(int pos = 0; pos < numberOfTimeSlotResourceGrid; pos++){
        for(int tf = 0; tf < numberOfTFs; tf++){
            tmp_and.push_back(resource_grid_flag_function(smtContext.num_val(tf, z3Int),
                                                        smtContext.num_val(pos, z3Int)) >= 0);
            tmp_and.push_back(resource_grid_flag_function(smtContext.num_val(tf, z3Int),
                                                        smtContext.num_val(pos, z3Int)) <= 1);
        }
    }
    smtOptimize.add(z3::mk_and(tmp_and));

    // constraint 2: for "pos" that hasn't been impacted, all resource units should be 0
    // build connection between "resource grid function" and "resource grid flag function"
    for(int i = 0; i < numberOfTFs; i++){
        for(int pos = 0; pos < numberOfTimeSlotResourceGrid; pos++){
            tmp_and.resize(0);
            for(int j = 0; j < numberOfPilots; j++){
                tmp_and.push_back(resource_grid_function(smtContext.num_val(i, z3Int),
                                                         smtContext.num_val(pos, z3Int),
                                                         smtContext.num_val(j, z3Int)) == 0);
            }
            smtOptimize.add(z3::implies(resource_grid_flag_function(smtContext.num_val(i, z3Int),
                                                                    smtContext.num_val(pos, z3Int)) == 0,
                                        z3::mk_and(tmp_and)));

            expr_vector tmp(smtContext);
            for(int j = 0; j < numberOfPilots; j++){
                tmp.push_back(resource_grid_function(smtContext.num_val(i, z3Int),
                                                     smtContext.num_val(pos, z3Int),
                                                     smtContext.num_val(j, z3Int)));
            }
            smtOptimize.add(z3::implies(resource_grid_flag_function(smtContext.num_val(i, z3Int),
                                                                    smtContext.num_val(pos, z3Int)) == 1,
                                        sum(tmp) >= 1));
        }
    }

    // resource grid flag function for control message
    func_decl resource_grid_flag_function_control = function("resource_grid_flag_function_control", z3Int, z3Int, z3Int);
    // constraint 1: return value 0/1
    tmp_and.resize(0);
    for(int pos = 0; pos < numberOfTimeSlotResourceGrid; pos++){
        for(int tf = 0; tf < numberOfTFs; tf++){
            tmp_and.push_back(resource_grid_flag_function_control(smtContext.num_val(tf, z3Int),
                                                        smtContext.num_val(pos, z3Int)) >= 0);
            tmp_and.push_back(resource_grid_flag_function_control(smtContext.num_val(tf, z3Int),
                                                        smtContext.num_val(pos, z3Int)) <= 1);
        }
    }
    smtOptimize.add(z3::mk_and(tmp_and));

    // constraint 2: for "pos" that hasn't been impacted, all resource units should be 0
    // build connection between "resource grid function" and "resource grid flag function"
    for(int i = 0; i < numberOfTFs; i++){
        for(int pos = 0; pos < numberOfTimeSlotResourceGrid; pos++){
            tmp_and.resize(0);
            for(int j = 0; j < numberOfPilots; j++){
                tmp_and.push_back(resource_grid_function_control(smtContext.num_val(i, z3Int),
                                                                   smtContext.num_val(pos, z3Int),
                                                                   smtContext.num_val(j, z3Int)) == 0);
            }
            smtOptimize.add(z3::implies(resource_grid_flag_function_control(smtContext.num_val(i, z3Int),
                                                                            smtContext.num_val(pos, z3Int)) == 0,
                                        z3::mk_and(tmp_and)));

            expr_vector tmp(smtContext);
            for(int j = 0; j < numberOfPilots; j++){
                tmp.push_back(resource_grid_function_control(smtContext.num_val(i, z3Int),
                                                     smtContext.num_val(pos, z3Int),
                                                     smtContext.num_val(j, z3Int)));
            }
            smtOptimize.add(z3::implies(resource_grid_flag_function_control(smtContext.num_val(i, z3Int),
                                                                    smtContext.num_val(pos, z3Int)) == 1,
                                        sum(tmp) >= 1));
        }
    }

    // the number of 'impacted' pos should equal to the needed number for each traffic flow
    for(int i = 0; i < numberOfTFs; i++){
        expr_vector tmp_pos1(smtContext);
        for(int pos = 0; pos < numberOfTimeSlotResourceGrid; pos++){
            tmp_pos1.push_back(resource_grid_flag_function(smtContext.num_val(i, z3Int),
                                                          smtContext.num_val(pos, z3Int)));
        }
        expr_vector tmp_pos2(smtContext);
        for(int j = 0; j < numberOfConfigurations[i]; j++){
            tmp_pos2.push_back(configurationSeries[i][j][2] *
                                configurationSeries[i][j][3] *
                                configurationSeries[i][j][6] *
                                configurationSeries[i][j][7]);
        }
        smtOptimize.add(sum(tmp_pos1) == sum(tmp_pos2));
    }
    // for control,
    for(int i = 0; i < numberOfTFs; i++){
        expr_vector tmp_pos1(smtContext);
        for(int pos = 0; pos < numberOfTimeSlotResourceGrid; pos++){
            tmp_pos1.push_back(resource_grid_flag_function_control(smtContext.num_val(i, z3Int),
                                                           smtContext.num_val(pos, z3Int)));
        }
        if(numberControlConfigurations[i] >= 1){
            expr_vector tmp_pos2(smtContext);
            for(int j = 0; j < numberControlConfigurations[i]; j++){
                tmp_pos2.push_back(controlConfigurations[i][j][3] *
                                   controlConfigurations[i][j][2]);
            }
            smtOptimize.add(sum(tmp_pos1) == sum(tmp_pos2));
        }else{
            smtOptimize.add(sum(tmp_pos1) == 0);
        }

    }

    // calculate the position of each resource unit as per all configurations
    // part 1: for data packets
    for(int i = 0; i < numberOfTFs; i++){
        for(int j = 0; j < numberOfConfigurations[i]; j++){
            for(int k = 1; k <= numberOfDataPackets[i]; k++){
                tmp_and.resize(0);
                for(int n = 0; n < max_RBAllocationPatterns; n++){
                    for(int m = 1; m <= maxLen; m++){
                        expr part1 =    configurationSeries[i][j][2] == m &&
                                        configurationSeries[i][j][0] == n &&
                                        configurationSeries[i][j][7] == 1 &&
                                        configurationSeries[i][j][6] == k;
                        // expr part2 = smtContext.bool_val(true);
                        expr_vector part2(smtContext);
                        for(int k_ = 0; k_ < k; k_++){
                            for(int m_ = 0; m_ < m; m_++){
                                // for resource grid function
                                // expr tmp_part1 = smtContext.bool_val(true);
                                for(int n_ = 0; n_ < numberOfPilots; n_++){
                                    part2.push_back(resource_grid_function(smtContext.num_val(i, z3Int),
                                                                         configurationSeries[i][j][1] +
                                                                         m_ +
                                                                         k_ * configurationSeries[i][j][5],
                                                                         smtContext.num_val(n_, z3Int)) ==
                                                                                 frequencyPattern[n][n_]);
                                }
                                // for resource grid flag function
                                part2.push_back(resource_grid_flag_function(smtContext.num_val(i, z3Int),
                                                                             configurationSeries[i][j][1] +
                                                                             m_ +
                                                                             k_ * configurationSeries[i][j][5]) == 1);

                            }
                        }
                        tmp_and.push_back(z3::implies(part1, z3::mk_and(part2)));

                    }
                }
                smtOptimize.add(z3::mk_and(tmp_and));
            }
        }
    }

    // for control
    for(int i = 0; i < numberOfTFs; i++){
        for(int j = 0; j < numberControlConfigurations[i]; j++){
            tmp_and.resize(0);
            for(int n = 0; n < max_RBAllocationPatterns; n++){
                expr part1 = controlConfigurations[i][j][3] == 1 && controlConfigurations[i][j][0] == n;
                expr_vector tmp_part1(smtContext);
                for(int n_ = 0; n_ < numberOfPilots; n_++){
                    tmp_part1.push_back(resource_grid_function_control(smtContext.num_val(i, z3Int),
                                                                            controlConfigurations[i][j][1] + 0,
                                                                            smtContext.num_val(n_, z3Int)) ==
                                                                                    frequencyPattern[n][n_]);
                    tmp_part1.push_back(resource_grid_flag_function_control(smtContext.num_val(i, z3Int),
                                                                         controlConfigurations[i][j][1]) == 1);

                }
                tmp_and.push_back(z3::implies(part1, z3::mk_and(tmp_part1)));
            }
            smtOptimize.add(z3::mk_and(tmp_and));
        }
    }

    // for each resource unit, it can be used only once
    for(int pos = 0; pos < numberOfTimeSlotResourceGrid; pos++){
        for(int p = 0; p < numberOfPilots; p++){
            expr_vector tmp_data(smtContext);
            expr_vector tmp_control(smtContext);
            for(int i = 0; i < numberOfTFs; i++){
                tmp_data.push_back(resource_grid_function(smtContext.num_val(i, z3Int),
                                                          smtContext.num_val(pos, z3Int),
                                                          smtContext.num_val(p, z3Int)));
                tmp_control.push_back(resource_grid_function_control(smtContext.num_val(i, z3Int),
                                                                     smtContext.num_val(pos, z3Int),
                                                                     smtContext.num_val(p, z3Int)));
            }
            smtOptimize.add((sum(tmp_data) + sum(tmp_control)) <= 1);
        }
    }
    cout << "Adding constraints: after assigning, each RB can only be used once" << endl;

    // build the optimization goal: minimize the used pilots
    expr max_frequency_pattern = smtContext.int_const("max_frequency_pattern");
    for(int i = 0; i < numberOfTFs; i++){
        for(int j = 0; j < numberOfConfigurations[i]; j++){
            smtOptimize.add(z3::implies(configurationSeries[i][j][7] == 1,
                                        max_frequency_pattern >=
                                        translation_RB_Top_Function(configurationSeries[i][j][0])));
        }
    }

    for(int i = 0; i < numberOfTFs; i++){
        for(int j = 0; j < numberControlConfigurations[i]; j++){
            smtOptimize.add(z3::implies(controlConfigurations[i][j][3] == 1,
                                        max_frequency_pattern >=
                                        translation_RB_Top_Function(controlConfigurations[i][j][0])));
        }
    }

    optimize::handle optimizationGoal = smtOptimize.minimize(max_frequency_pattern);
    cout << "Establish the optimization goal: " <<
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - systemStartTime).count() <<
    " (ms)" << endl;
    // logFile << smtOptimize << endl;
    systemStartTime = std::chrono::steady_clock::now();

    if(smtOptimize.check() == z3::sat){
        z3::model m = smtOptimize.get_model();
        // std::cout << m << "\n";
        cout << "Max pilots: " << m.eval(max_frequency_pattern).get_numeral_int() << endl;
        cout << "Got a solution: " <<
             std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - systemStartTime).count() <<
             " (ms)" << endl;

        // print the schedule
        logFile << "Max pilots: " << m.eval(max_frequency_pattern).get_numeral_int() << endl;
        logFile << "Got a solution: " <<
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - systemStartTime).count() <<
                " (ms)" << endl;

        logFile << "Resource grid flag for each traffic flow:" << endl;
        std::stringstream print_s("");
        for(int i = 0; i < numberOfTFs; i++){
            print_s << "Traffic flow " << std::to_string(i) << ":" << endl;
            for(int j = 0; j < numberOfTimeSlotResourceGrid; j++){
                print_s << std::to_string(m.eval(resource_grid_flag_function(
                        smtContext.num_val(i, z3Int),
                        smtContext.num_val(j, z3Int))).get_numeral_int()) << " ";
            }
            print_s << endl;
        }
        logFile << print_s.str();


        logFile << "Resource grid for each traffic flow: " << endl;
        for(int i = 0; i < numberOfTFs; i++){
            print_s.str("");
            print_s << "Traffic flow " << std::to_string(i) << ":" << endl;
            for(int j = 0; j < numberOfPilots; j++){
                for(int k = 0; k < numberOfTimeSlotResourceGrid; k++){
                    print_s << std::to_string(m.eval(resource_grid_function(
                            smtContext.num_val(i, z3Int),
                            smtContext.num_val(k, z3Int),
                            smtContext.num_val(j, z3Int))).get_numeral_int()) << " ";
                }
                print_s << endl;
            }
            print_s << endl;
            logFile << print_s.str();
        }
        print_s.str("");

        logFile << "Configurations series for each traffic flow: " << endl;
        for(int i = 0; i < numberOfTFs; i++){
            print_s << "Traffic flow " << std::to_string(i) << ":" << endl;
            for(int j = 0; j < numberOfConfigurations[i]; j++){
                print_s << "Allocation scheme: \t " << std::to_string(m.eval(configurationSeries[i][j][0]).get_numeral_int()) << endl
                        << "offset: \t " << std::to_string(m.eval(configurationSeries[i][j][1]).get_numeral_int()) << endl
                        << "len: \t " << std::to_string(m.eval(configurationSeries[i][j][2]).get_numeral_int()) << endl
                        << "K: \t " << std::to_string(m.eval(configurationSeries[i][j][3]).get_numeral_int()) << endl
                        << "interval: \t " << std::to_string(m.eval(configurationSeries[i][j][4]).get_numeral_int()) << endl
                        << "period: \t " << std::to_string(m.eval(configurationSeries[i][j][5]).get_numeral_int()) << endl
                        << "repeat number: \t " << std::to_string(m.eval(configurationSeries[i][j][6]).get_numeral_int()) << endl
                        << "valid indicator: \t " << std::to_string(m.eval(configurationSeries[i][j][7]).get_numeral_int()) << endl;
                print_s << endl;
            }
            logFile << print_s.str();
        }

    }
    cout << smtOptimize.assertions().size();
}


void SMT1(){
    context c;
    optimize opt(c);
    z3::params p(c);
    p.set("priority",c.str_symbol("pareto"));
    opt.set(p);
    expr x = c.int_const("x");
    opt.add(x == 2);
    opt.maximize(x);

    if (z3::sat == opt.check()) {
        z3::model m = opt.get_model();
        cout << m << endl;
        int t;
        std::cout << x << ": " <<  m.eval(x).get_numeral_int()  << "\n";
    }

//    std::cout << "eval example 1\n";
//    context c;
//    expr x = c.int_const("x");
//    expr y = c.int_const("y");
//    solver s(c);
//
//    /* assert x < y */
//    s.add(x < y);
//    /* assert x > 2 */
//    s.add(x > 2);
//
//    std::cout << s.check() << "\n";
//
//    z3::model m = s.get_model();
//    std::cout << "Model:\n" << m << "\n";
//    std::cout << "x+y = " << m.eval(x+y) << "\n";
}

/*
 * f --> the test samples to be tested, here, by default is 0: 0--> test samples for SMT solution
 */
void performSMT(int f){
    cfgInformation.clear();
    extractCfg(f);
    if(ifTest){
        SMT(testSample);
        // SMT1();
    } else{
        for (std::string &cfg: cfgInformation){
            for (double pru_ratio: pru_ratio_list){
                SMT(cfg);
            }
        }
    }

}