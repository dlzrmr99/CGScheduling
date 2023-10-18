# This is for generating test samples of the realistic case study.

import math
from network_parameter import *
from sinrtobits import *
from control_overhead import *
import numpy as np
import os


application1 = [2, 80, 1]       # precise cooperative
application2 = [10, 1024, 6]    # machine coordination
application3 = [5, 256, 3]      #
application4 = [2, 20, 1]

applications_collection = [application1, application2, application3, application4]
numberofEachApplications = [40, 10, 50, 0]
length_timeslot = 0.25


# return a list of tested applications based on applications_collection
# characteristics of each application:
#   [offset, transmission period, payload (#. resource units), latency, # of conf.]
# rule of offset: offset + latency < transmission period
# rule of # of conf.: range [1, maximum # of data packets] or Manually defined
# everything should be normalized to the length of time slot
folder_name = ['0']
os.remove('application_info/applications_large_scale_%s.txt' % (folder_name[0]))
count = 0
file = open('application_info/applications_large_scale_%s.txt' % (folder_name[0]), 'w')
MAX_Count = 100
for _ in range(MAX_Count):
    list_of_application = []
    number_of_applications = len(numberofEachApplications)
    for application_id in range(number_of_applications):
        for iter in range(numberofEachApplications[application_id]):
            transmission_period = int(applications_collection[application_id][0] / length_timeslot)
            payload = applications_collection[application_id][1] * 8
            latency = int(applications_collection[application_id][2] / length_timeslot)
            sinr = getsinr()
            # payload_resource_unit = math.ceil((payload / getbits(sinr))) * (10 / transmission_period)
            payload_resource_unit = math.ceil((payload / getbits(sinr)))

            # number of packets == maximum number of configurations
            number_of_configurations_max = 0

            offset_max = transmission_period - latency - 1
            offset = random.randint(0, max(offset_max, 0))
            if offset_max <= 0:
                offset = 0

            control_overhead = 1

            list_of_application.append([offset,
                                        transmission_period,
                                        payload_resource_unit,
                                        latency,
                                        number_of_configurations_max,
                                        control_overhead])

    tp_list = [ele[1] for ele in list_of_application]
    hyper_period = np.lcm.reduce(tp_list)
    for i in range(len(list_of_application)):
        list_of_application[i][4] = hyper_period / list_of_application[i][1]

    ss = ''
    ss += '%d %d ' % (sum(numberofEachApplications), hyper_period)
    for i in range(len(list_of_application)):
        # offset, transmission period, payload, latency, number of packets, control overhead
        ss += '%d %d %d %d %d %d ' % (list_of_application[i][0],
                                      list_of_application[i][1],
                                      list_of_application[i][2],
                                      list_of_application[i][3],
                                      list_of_application[i][4],
                                      list_of_application[i][5])
    print(ss, file=file)





