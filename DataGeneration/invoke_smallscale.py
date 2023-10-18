# The same functionality as "invoke.py". This is specialized for small-scale systems.

nopilotsH = 50

# generate commands for the small scale test
def generateCommandsSmallScale():
    cfgFile = open("conf_0.cfg", 'w')
    with open('application_info/applications_small_scale_.txt', 'r') as f:
        lines = f.readlines()
        for current_count in range(len(lines)):
            # parse the information
            l = lines[current_count].strip('\n').strip(' ').split(' ')
            # meta info
            nrofTF = int(l[0])
            hp = int(l[1])

            # Commands for Heuristic Approach CoU
            heu_command = '%d %d %d %d %d %d' % \
                          (current_count, 0, nrofTF, nopilotsH, hp, 0)

            for tf in range(nrofTF):
                offset = int(l[tf * 6 + 2])
                transmission_period = int(l[tf * 6 + 3])
                payload = int(l[tf * 6 + 4])
                latency = int(l[tf * 6 + 5])
                confs = int(l[tf * 6 + 6])

                co = int(float(l[tf * 6 + 7]))
                # co = 1
                heu_command += ' %d %d %d %d %d %d' % (offset, transmission_period, payload, latency, confs, co)
            print(heu_command, file=cfgFile)
    cfgFile.close()


generateCommandsSmallScale()
