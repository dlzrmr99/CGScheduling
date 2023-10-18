# Some basic settings for TFs used in large-scale tests are defined in this file.

import math
from network_parameter import *
from sinrtobits import *
from control_overhead import *
import numpy as np

# define characteristics of application [transmission period (ms), payload (bytes), latency (ms), length of time slot]

# Candidate transmission periods
transmissionPeriodAlternatives = [2, 3, 4, 5, 6]
# index range
tp_min = 0
tp_max = len(transmissionPeriodAlternatives) - 1
# the unit for payload is "bit"
payload_min = 320
payload_max = 1600
# define delay requirements
delay_min = 0.2
delay_max = 0.6

length_timeslot = 0.25


# return a list of tested applications based on applications_collection
# characteristics of each application:
#   [offset, transmission period, payload (#. resource units), latency, # of conf.]
# rule of offset: offset + latency < transmission period
# everything should be normalized to the length of time slot
def generating_application_info(max_applications):
    list_of_application = []
    for i in range(max_applications):
        # generate offset
        transmission_period = \
            int(transmissionPeriodAlternatives[np.random.randint(tp_min, tp_max + 1)] / length_timeslot)
        payload = np.random.randint(payload_min, payload_max + 1)
        if int(delay_max * transmission_period) == 0:
            latency = 1
        else:
            latency = max(1, int(np.random.randint(int(delay_min * transmission_period),
                                                   int(delay_max * transmission_period))))
        sinr = getsinr()
        payload_resource_unit = math.ceil((payload / getbits(sinr)))

        # number of packets == maximum number of configurations
        # calculated later when hyperperiod is known
        # this is the number of packets in a single hyperperiod
        number_of_configurations_max = 0

        offset_max = transmission_period - latency - 1
        offset = random.randint(0, max(offset_max, 0))
        if offset_max <= 0:
            offset = 0

        control_overhead = getcontrolrbs(sinr)

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

    return hyper_period, list_of_application
