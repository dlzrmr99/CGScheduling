# Scripts to process the collected data from the simulations.
# It's better to develop the one you preferred according to your needs.

import numpy as np

maximum_pilots = [5]
# the following parameter are used to calculate the schedulable ratio
start = 5
interval = 4
pilots70 = []
for i in range(1):
    pilots70.append(start + i * interval)
print(pilots70)

interval = 200
target_number_application = 0
# folder_name = ['70']
# maximum_pilots = [20]

# Co1
def Heuristic_nolimitation():
    r_p = []        # number of pilots
    r_ms = []       # execution time
    r_e = []        # resource efficiency
    r_u = []        # resource utilization
    r_success = []  # success ratio
    r_configurationsPerPacket = []

    filename = 'Results/output1.txt'
    # filename = '../Heuristic/COmax_original/pilots_%s.txt' % (folder_name[which_folder])
    with open(filename, 'r') as f:
        lines = f.readlines()
    for folder in range(1):
        pilots = []
        ms = []
        effi = []
        uti = []
        packets = []
        configurations = []
        success = 0
        line = lines[folder*interval: (folder+1)*interval]
        for l in line:
            l = l.strip('\n').split(' ')
            if int(l[0]) <= maximum_pilots[folder]:     # only schedulable TF is valid
                success += 1
                pilots.append(int(l[0]))
                ms.append(int(l[1]))
                effi.append(float(l[2]))
                uti.append(float(l[3]))
                packets.append(int(l[4]))
                configurations.append(int(l[5]))

        if folder == target_number_application:
            line = lines[folder*interval: (folder+1)*interval]
            for pilotsAlternatives in range(len(pilots70)):
                success = 0
                for l in line:
                    l = l.strip('\n').split(' ')
                    if int(l[0]) <= pilots70[pilotsAlternatives]:
                        success += 1
                r_success.append(success / interval)

        r_p.append(np.mean(pilots))
        r_ms.append(np.mean(ms))
        r_e.append(np.mean(effi))
        r_u.append(np.mean(uti))
        r_configurationsPerPacket.append(np.sum(configurations) / np.sum(packets))
    #
    # for which_folder in range(len(folder_name)):
    #     filename = 'Results/heatmap_%s_b.txt' % (folder_name[which_folder])
    #     with open(filename, 'r') as f:
    #         lines = f.readlines()
    #
    #     ratio_data = []
    #     ratio_data_mean = []
    #     for line in lines:
    #         l = line.strip('\n').split(' ')
    #         # print(l)
    #         tmp_ratio_data = []
    #         for l_ in range(1, len(l)-1):
    #             # print(l[l_])
    #             tmp_ratio_data.append(float(l[l_]))
    #         ratio_data.append(tmp_ratio_data)
    #     for i in range(len(ratio_data[0])):
    #         tmp = []
    #         for j in range(len(ratio_data)):
    #             tmp.append(ratio_data[j][i])
    #         ratio_data_mean.append(np.mean(tmp))
    #     s = ''
    #     for ele in ratio_data_mean:
    #         s += ' %.4f' % ele
    #     print(s)
    #
    # print('-----------------')
    # for which_folder in range(len(folder_name)):
    #     filename = 'Results/heatmap_%s.txt' % (folder_name[which_folder])
    #     with open(filename, 'r') as f:
    #         lines = f.readlines()
    #
    #     ratio_data = []
    #     ratio_data_mean = []
    #     for line in lines:
    #         l = line.strip('\n').split(' ')
    #         # print(l)
    #         tmp_ratio_data = []
    #         for l_ in range(1, len(l) - 1):
    #             # print(l[l_])
    #             tmp_ratio_data.append(float(l[l_]))
    #         ratio_data.append(tmp_ratio_data)
    #     for i in range(len(ratio_data[0])):
    #         tmp = []
    #         for j in range(len(ratio_data)):
    #             tmp.append(ratio_data[j][i])
    #         ratio_data_mean.append(np.mean(tmp))
    #     s = ''
    #     for ele in ratio_data_mean:
    #         s += ' %.4f' % ele
    #     print(s)

    # print('Average pilots: %f' % (np.mean(pilots)))
    # print('Max pilots: %f' % max(pilots))
    # print('Average efficiency: %f' % (np.mean(efficiency)))
    # print('Average ratio between actial conf. and allowed conf.: %f' % np.mean(ratio_actual_allowed))
    # print('Real number of conf.: %f' % np.mean(real_conf))

    s = ''
    for ele in r_p:
        s += ' %.3f' % ele
    print(s)

    s = ''
    for ele in r_ms:
        s += ' %.2f' % ele
    print(s)

    s = ''
    for ele in r_e:
        s += ' %.3f' % ele
    print(s)

    s = ''
    for ele in r_u:
        s += ' %.2f' % ele
    print(s)

    # s = ''
    # for ele in r_success:
    #     s += ' %.2f' % ele
    # print(s)

    s = ''
    for ele in r_success:
        s += ' %.3f' % ele
    print(s)

    s = ''
    for ele in r_configurationsPerPacket:
        s += ' %.3f' % ele
    print(s)

    # s = ''
    # for ele in r_c:
    #     s += ' %.2f' % ele
    # print(s)

    # s = ''
    # for ele in r_f:
    #     s += ' %.2f' % ele
    # print(s)

# First come, first served
def Heuristic_nolimitationFCFS():
    r_p = []        # number of pilots
    r_ms = []       # execution time
    r_e = []        # resource efficiency
    r_u = []        # resource utilization
    r_success = []  # success ratio
    r_configurationsPerPacket = []

    filename = 'Results/output2.txt'
    # filename = '../Heuristic/COmax_original/pilots_%s.txt' % (folder_name[which_folder])
    with open(filename, 'r') as f:
        lines = f.readlines()
    for folder in range(1):
        pilots = []
        ms = []
        effi = []
        uti = []
        packets = []
        configurations = []
        success = 0
        line = lines[folder*interval: (folder+1)*interval]
        for l in line:
            l = l.strip('\n').split(' ')
            if int(l[0]) <= maximum_pilots[folder]:     # only schedulable TF is valid
                success += 1
                pilots.append(int(l[0]))
                ms.append(int(l[1]))
                effi.append(float(l[2]))
                uti.append(float(l[3]))
                packets.append(int(l[4]))
                configurations.append(int(l[5]))

        if folder == target_number_application:
            line = lines[folder*interval: (folder+1)*interval]
            for pilotsAlternatives in range(len(pilots70)):
                success = 0
                for l in line:
                    l = l.strip('\n').split(' ')
                    if int(l[0]) <= pilots70[pilotsAlternatives]:
                        success += 1
                r_success.append(success / interval)

        r_p.append(np.mean(pilots))
        r_ms.append(np.mean(ms))
        r_e.append(np.mean(effi))
        r_u.append(np.mean(uti))
        r_configurationsPerPacket.append(np.sum(configurations) / np.sum(packets))
    #
    # for which_folder in range(len(folder_name)):
    #     filename = 'Results/heatmap_%s_b.txt' % (folder_name[which_folder])
    #     with open(filename, 'r') as f:
    #         lines = f.readlines()
    #
    #     ratio_data = []
    #     ratio_data_mean = []
    #     for line in lines:
    #         l = line.strip('\n').split(' ')
    #         # print(l)
    #         tmp_ratio_data = []
    #         for l_ in range(1, len(l)-1):
    #             # print(l[l_])
    #             tmp_ratio_data.append(float(l[l_]))
    #         ratio_data.append(tmp_ratio_data)
    #     for i in range(len(ratio_data[0])):
    #         tmp = []
    #         for j in range(len(ratio_data)):
    #             tmp.append(ratio_data[j][i])
    #         ratio_data_mean.append(np.mean(tmp))
    #     s = ''
    #     for ele in ratio_data_mean:
    #         s += ' %.4f' % ele
    #     print(s)
    #
    # print('-----------------')
    # for which_folder in range(len(folder_name)):
    #     filename = 'Results/heatmap_%s.txt' % (folder_name[which_folder])
    #     with open(filename, 'r') as f:
    #         lines = f.readlines()
    #
    #     ratio_data = []
    #     ratio_data_mean = []
    #     for line in lines:
    #         l = line.strip('\n').split(' ')
    #         # print(l)
    #         tmp_ratio_data = []
    #         for l_ in range(1, len(l) - 1):
    #             # print(l[l_])
    #             tmp_ratio_data.append(float(l[l_]))
    #         ratio_data.append(tmp_ratio_data)
    #     for i in range(len(ratio_data[0])):
    #         tmp = []
    #         for j in range(len(ratio_data)):
    #             tmp.append(ratio_data[j][i])
    #         ratio_data_mean.append(np.mean(tmp))
    #     s = ''
    #     for ele in ratio_data_mean:
    #         s += ' %.4f' % ele
    #     print(s)

    # print('Average pilots: %f' % (np.mean(pilots)))
    # print('Max pilots: %f' % max(pilots))
    # print('Average efficiency: %f' % (np.mean(efficiency)))
    # print('Average ratio between actial conf. and allowed conf.: %f' % np.mean(ratio_actual_allowed))
    # print('Real number of conf.: %f' % np.mean(real_conf))

    s = ''
    for ele in r_p:
        s += ' %.3f' % ele
    print(s)

    s = ''
    for ele in r_ms:
        s += ' %.2f' % ele
    print(s)

    s = ''
    for ele in r_e:
        s += ' %.3f' % ele
    print(s)

    s = ''
    for ele in r_u:
        s += ' %.2f' % ele
    print(s)

    # s = ''
    # for ele in r_success:
    #     s += ' %.2f' % ele
    # print(s)

    s = ''
    for ele in r_success:
        s += ' %.3f' % ele
    print(s)

    s = ''
    for ele in r_configurationsPerPacket:
        s += ' %.3f' % ele
    print(s)

    # s = ''
    # for ele in r_c:
    #     s += ' %.2f' % ele
    # print(s)

    # s = ''
    # for ele in r_f:
    #     s += ' %.2f' % ele
    # print(s)

# CoU
def Heuristic_nolimitationN():
    r_p = []        # number of pilots
    r_ms = []       # execution time
    r_e = []        # resource efficiency
    r_u = []        # resource utilization
    r_success = []  # success ratio
    r_configurationsPerPacket = []

    filename = 'Results/output.txt'
    # filename = '../Heuristic/COmax_original/pilots_%s.txt' % (folder_name[which_folder])
    with open(filename, 'r') as f:
        lines = f.readlines()
    for folder in range(1):
        pilots = []
        ms = []
        effi = []
        uti = []
        packets = []
        configurations = []
        success = 0
        line = lines[folder*interval: (folder+1)*interval]
        for l in line:
            l = l.strip('\n').split(' ')
            if int(l[0]) <= maximum_pilots[folder]:     # only schedulable TF is valid
                success += 1
                pilots.append(int(l[0]))
                ms.append(int(l[1]))
                effi.append(float(l[2]))
                uti.append(float(l[3]))
                packets.append(int(l[4]))
                configurations.append(int(l[5]))

        if folder == target_number_application:
            line = lines[folder*interval: (folder+1)*interval]
            for pilotsAlternatives in range(len(pilots70)):
                success = 0
                for l in line:
                    l = l.strip('\n').split(' ')
                    if int(l[0]) <= pilots70[pilotsAlternatives]:
                        success += 1
                r_success.append(success / interval)

        r_p.append(np.mean(pilots))
        r_ms.append(np.mean(ms))
        r_e.append(np.mean(effi))
        r_u.append(np.mean(uti))
        r_configurationsPerPacket.append(np.sum(configurations) / np.sum(packets))
    #
    # for which_folder in range(len(folder_name)):
    #     filename = 'Results/heatmap_%s_b.txt' % (folder_name[which_folder])
    #     with open(filename, 'r') as f:
    #         lines = f.readlines()
    #
    #     ratio_data = []
    #     ratio_data_mean = []
    #     for line in lines:
    #         l = line.strip('\n').split(' ')
    #         # print(l)
    #         tmp_ratio_data = []
    #         for l_ in range(1, len(l)-1):
    #             # print(l[l_])
    #             tmp_ratio_data.append(float(l[l_]))
    #         ratio_data.append(tmp_ratio_data)
    #     for i in range(len(ratio_data[0])):
    #         tmp = []
    #         for j in range(len(ratio_data)):
    #             tmp.append(ratio_data[j][i])
    #         ratio_data_mean.append(np.mean(tmp))
    #     s = ''
    #     for ele in ratio_data_mean:
    #         s += ' %.4f' % ele
    #     print(s)
    #
    # print('-----------------')
    # for which_folder in range(len(folder_name)):
    #     filename = 'Results/heatmap_%s.txt' % (folder_name[which_folder])
    #     with open(filename, 'r') as f:
    #         lines = f.readlines()
    #
    #     ratio_data = []
    #     ratio_data_mean = []
    #     for line in lines:
    #         l = line.strip('\n').split(' ')
    #         # print(l)
    #         tmp_ratio_data = []
    #         for l_ in range(1, len(l) - 1):
    #             # print(l[l_])
    #             tmp_ratio_data.append(float(l[l_]))
    #         ratio_data.append(tmp_ratio_data)
    #     for i in range(len(ratio_data[0])):
    #         tmp = []
    #         for j in range(len(ratio_data)):
    #             tmp.append(ratio_data[j][i])
    #         ratio_data_mean.append(np.mean(tmp))
    #     s = ''
    #     for ele in ratio_data_mean:
    #         s += ' %.4f' % ele
    #     print(s)

    # print('Average pilots: %f' % (np.mean(pilots)))
    # print('Max pilots: %f' % max(pilots))
    # print('Average efficiency: %f' % (np.mean(efficiency)))
    # print('Average ratio between actial conf. and allowed conf.: %f' % np.mean(ratio_actual_allowed))
    # print('Real number of conf.: %f' % np.mean(real_conf))

    s = ''
    for ele in r_p:
        s += ' %.3f' % ele
    print(s)

    s = ''
    for ele in r_ms:
        s += ' %.2f' % ele
    print(s)

    s = ''
    for ele in r_e:
        s += ' %.3f' % ele
    print(s)

    s = ''
    for ele in r_u:
        s += ' %.2f' % ele
    print(s)

    # s = ''
    # for ele in r_success:
    #     s += ' %.2f' % ele
    # print(s)

    s = ''
    for ele in r_success:
        s += ' %.3f' % ele
    print(s)

    s = ''
    for ele in r_configurationsPerPacket:
        s += ' %.3f' % ele
    print(s)

    # s = ''
    # for ele in r_c:
    #     s += ' %.2f' % ele
    # print(s)

    # s = ''
    # for ele in r_f:
    #     s += ' %.2f' % ele
    # print(s)



print('FCFS:')
Heuristic_nolimitationFCFS()
print()
print('Co1:')
Heuristic_nolimitation()
print()
print('CoN:')
Heuristic_nolimitationN()