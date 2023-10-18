# This is to generate input sequences to the algorithms (i.e., SMT, Co1 and CoU).
# The input file: application_info/applications_large_scale_x.txt
# The output file: conf_x.cfg

import os

# RB_max
nopilotsH = 300
# Different numbers of TFs
folder_name = ['10', '20', '30', '40', '50', '60', '70', '80', '0']


def generateCommandsHNoLimitation(whichfolder):
    cfgFile = open("conf_%s.cfg" % folder_name[whichfolder], 'w')
    with open('application_info/applications_large_scale_%d.txt' % int(folder_name[whichfolder]), 'r') as f:
        lines = f.readlines()
        for current_count in range(len(lines)):
            # parse the information
            l = lines[current_count].strip('\n').strip(' ').split(' ')
            # meta info
            nrofTF = int(l[0])
            hp = int(l[1])

            # Commands for Heuristic Approach CoU
            heu_command = '%d %d %d %d %d %d' % \
                          (current_count, 0, nrofTF, nopilotsH, hp, whichfolder)

            for tf in range(nrofTF):
                offset = int(l[tf * 6 + 2])
                transmission_period = int(l[tf * 6 + 3])
                payload = int(l[tf * 6 + 4])
                latency = int(l[tf * 6 + 5])
                confs = int(l[tf * 6 + 6])

                # co = int(float(l[tf * 6 + 7]))
                co = 1
                heu_command += ' %d %d %d %d %d %d' % (offset, transmission_period, payload, latency, confs, co)
            print(heu_command, file=cfgFile)
    cfgFile.close()


# del file
for i in range(len(folder_name)):
    # os.remove("conf_%s.cfg" % folder_name[i])
    generateCommandsHNoLimitation(i)
