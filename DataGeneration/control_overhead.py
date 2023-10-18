# The file is for calculating the number of resource units for control overhead.
# However, this is not really taking effect since the following calculation method is applied
# in the case that the time duration is 1/2 symbols.
# In the experiments, the basic time unit is a single slot, which is much longer than several symbols.
# We generally assume that control signal consumes 1 RU.

sinr_to_control_rbs = [[-2.2, 14], [0.2, 8], [4.2, 5]]


# return resource units for control based on sinr
def getcontrolrbs(sinr):
    if sinr > sinr_to_control_rbs[-1][0]:
        return 3
    else:
        for i in range(len(sinr_to_control_rbs)):
            if sinr_to_control_rbs[i][0] >= sinr:
                return sinr_to_control_rbs[i][1]