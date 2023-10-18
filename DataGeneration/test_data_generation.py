# This is for generating test samples of large-scale systems.

from application_parameter_large_scale import *
import os

# define the maximum loop for testing
MAX_COUNT = 200


folder_name = ['10', '20', '30', '40', '50', '60', '70', '80']

for whichfolder in range(len(folder_name)):
    os.remove('application_info/applications_large_scale_%s.txt' % (folder_name[whichfolder]))
    count = 0
    file = open('application_info/applications_large_scale_%s.txt' % (folder_name[whichfolder]), 'w')
    # format of each record
    # [number of applications, (offset, transmission period, payload, latency, number of packets, control overhead), (), ()]
    max_applications = int(folder_name[whichfolder])
    while count < MAX_COUNT:
        hp, list_applications = generating_application_info(max_applications)
        ss = ''
        ss += '%d %d ' % (max_applications, hp)
        for i in range(len(list_applications)):
            # offset, transmission period, payload, latency, number of packets, control overhead
            ss += '%d %d %d %d %d %d ' % (list_applications[i][0],
                                          list_applications[i][1],
                                          list_applications[i][2],
                                          list_applications[i][3],
                                          list_applications[i][4],
                                          list_applications[i][5])
        print(ss, file=file)

        count += 1