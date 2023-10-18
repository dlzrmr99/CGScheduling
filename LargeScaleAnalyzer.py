import numpy as np

folder_name = ['10', '20', '30', '40', '50', '60', '70', '80']
maximum_pilots = [8, 12, 17, 19, 23, 25, 28, 30, 30, 30]
# pilots70 = [32, 34, 36, 38, 40, 42, 44, 46, 48, 50]
# pilots70 = [18, 20, 22, 24, 26, 28, 30, 32, 34, 36]
# pilots70 = [95, 100, 105, 110, 115, 120, 125, 130]
start = 0
interval = 5
pilots70 = []
for i in range(32):
    pilots70.append(start + i * interval)
print(pilots70)

# This is the same as the parameter "MAX_COUNT" in script "test_data_generation.py"
# Noted that the results with different numbers of TFs are in a single file, hence, the following parameter "interval"
# is needed as a separator.
interval = 200

# This is the folder (corresponding to different TF numbers) index
target_number_application = 7
# folder_name = ['70']
# maximum_pilots = [20]


def Heuristic_nolimitation():
    r_p = []
    r_ms = []
    r_e = []
    r_f = []
    r_u = []
    r_c = []
    r_success = []
    r_success70 = []
    r_configurationsPerPacket = []

    filename = 'Results/output1.txt'
    # filename = '../Heuristic/COmax_original/pilots_%s.txt' % (folder_name[which_folder])
    with open(filename, 'r') as f:
        lines = f.readlines()
    for folder in range(8):
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
                r_success70.append(success / len(pilots))

        r_p.append(np.mean(pilots))
        r_ms.append(np.mean(ms))
        r_e.append(np.mean(effi))
        r_u.append(np.mean(uti))
        r_success.append(success / len(pilots))
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
        s += ' %.2f' % ele
    print(s)

    s = ''
    for ele in r_ms:
        s += ' %.2f' % ele
    print(s)

    s = ''
    for ele in r_e:
        s += ' %.2f' % ele
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
    for ele in r_success70:
        s += ' %.2f' % ele
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

def Heuristic_nolimitationFCFS():
    r_p = []
    r_ms = []
    r_e = []
    r_f = []
    r_u = []
    r_c = []
    r_success = []
    r_success70 = []
    r_configurationsPerPacket = []

    filename = 'Results/output2.txt'
    # filename = '../Heuristic/COmax_original/pilots_%s.txt' % (folder_name[which_folder])
    with open(filename, 'r') as f:
        lines = f.readlines()
    for folder in range(8):
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
                r_success70.append(success / len(pilots))

        r_p.append(np.mean(pilots))
        r_ms.append(np.mean(ms))
        r_e.append(np.mean(effi))
        r_u.append(np.mean(uti))
        r_success.append(success / len(pilots))
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
        s += ' %.2f' % ele
    print(s)

    s = ''
    for ele in r_ms:
        s += ' %.2f' % ele
    print(s)

    s = ''
    for ele in r_e:
        s += ' %.2f' % ele
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
    for ele in r_success70:
        s += ' %.2f' % ele
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

def Heuristic_nolimitationN():
    r_p = []
    r_ms = []
    r_e = []
    r_f = []
    r_u = []
    r_c = []
    r_success = []
    r_success70 = []
    r_configurationsPerPacket = []

    filename = 'Results/output.txt'
    # filename = '../Heuristic/COmax_original/pilots_%s.txt' % (folder_name[which_folder])
    with open(filename, 'r') as f:
        lines = f.readlines()
    for folder in range(8):
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
                r_success70.append(success / len(pilots))

        r_p.append(np.mean(pilots))
        r_ms.append(np.mean(ms))
        r_e.append(np.mean(effi))
        r_u.append(np.mean(uti))
        r_success.append(success / len(pilots))
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
        s += ' %.2f' % ele
    print(s)

    s = ''
    for ele in r_ms:
        s += ' %.2f' % ele
    print(s)

    s = ''
    for ele in r_e:
        s += ' %.2f' % ele
    print(s)

    # s = ''
    # for ele in r_u:
    #     s += ' %.2f' % ele
    # print(s)

    # s = ''
    # for ele in r_success:
    #     s += ' %.2f' % ele
    # print(s)

    s = ''
    for ele in r_success70:
        s += ' %.2f' % ele
    print(s)

    s = ''
    # for ele in r_configurationsPerPacket:
    #     s += ' %.3f' % ele
    s += ' %.3f' % np.mean(r_configurationsPerPacket)
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