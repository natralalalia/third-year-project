import h5py
import spynnaker8 as p
from python_models8.neuron.builds.my_full_neuron import MyFullNeuron
import numpy as np


def decode(firing_times):
    first = False
    neuron1 = 0
    neuron2 = 0
    time1 = 0
    time2 = 0
    two_spikes = False
    for (idx, val) in enumerate(firing_times):
        if val != 0 and first == False:
            neuron1 = idx + 1
            time1 = val
            first = True
        elif val != 0 and first == True:
            neuron2 = idx + 1
            time2 = val
            two_spikes = True
    if firing_times == [0]*16:
        return 0
    if not two_spikes:
        if neuron1 == 1:
            act = 17-time1
        else:
            act = 16*neuron1
    else:
        act = (17-time1)*neuron1 + (17-time2)*neuron2
    return act


hf = h5py.File('xor_weights_lr.hdf5', 'r')
print(hf.keys())

print('------------DENSE Weights------------')
for v in hf['dense']['dense']['kernel:0']:
    print(v)

print('-----------dense Bias--------')
for v in range(0, 16):
    print(hf['dense']['dense']['bias:0'][v])

print('------------DENSE_1 Weights------------')
for x in range(1, 16):
    print(hf['dense_1']['dense_1']['kernel:0'][x])

print('------------DENSE_1 Bias------------')
for x in range(0, 1):
    print(hf['dense_1']['dense_1']['bias:0'][x])

p.setup(timestep=0.1)

pops = [[], [], []]
proj = [[], []]
input_values = [20, 30]

# input layer
for val in range(0, 2):
    pops[0].append(p.Population(16, MyFullNeuron(activation=input_values[val], decode=val)))

# hidden layer
for val in range(0, 2):
    pops[1].append(p.Population(16, MyFullNeuron(decode=val + 2, activation=-1, bias=hf['dense']['dense']['bias:0'][val])))

# output layer
pops[2].append(p.Population(16, MyFullNeuron(decode=4, activation=-1)))
weights = [2, 3, 1, 1, 1, 1]
w = 0
# set projections
for i in range(0, 2):
    for j in range(0, 2):
        if weights[w] < 0:
            rt = 'inhibitory'
        else:
            rt = 'excitatory'
        proj[0].append(p.Projection(pops[0][i], pops[1][j], p.OneToOneConnector(),
                                    p.StaticSynapse(weight=weights[w], delay=0), receptor_type=rt))

        print("The connection between pop[0][{}] and pop[1][{}] -> The weight is: {}".format(i, j, weights[w]))
        w += 1
        # p.StaticSynapse(weight=hf['dense']['dense']['kernel:0'][i][j])))
        # print("The connection between pop[0][{}] and pop[1][{}] -> The weight is: {}".format(i, j,
        # hf['dense']['dense']['kernel:0'][i][j]))

for j in range(0, 2):
    proj[1].append(p.Projection(pops[1][j], pops[2][0], p.OneToOneConnector(),
                                p.StaticSynapse(weight=weights[w])))  # weights[w]
    print("The connection between pop[1][{}] and pop[2][0] -> The weight is: {}".format(j, weights[w]))
    w += 1

    # p.StaticSynapse(weight=(hf['dense_1']['dense_1']['kernel:0'][j]))))
    print("The weight is: {}".format(hf['dense_1']['dense_1']['kernel:0'][j]))

for po in pops:
    for pop in po:
        pop.record(["spikes"])

for po in pops[0]:
    po.record(['v'])

for po in pops[1]:
    po.record(['v'])

for po in pops[2]:
    po.record(['v'])

p.run(4.8)

end_spikes = [[], [], []]
for (i, po) in enumerate(pops):
    for pop in po:
        a = pop.get_data(["spikes"]).segments[0].spiketrains
        if True:
            print("Index = {}".format(i))
            voltage = pop.get_data('v').segments[0].filter(name='v')[0]
            print("Voltage = {}".format(voltage))
        end_spikes[i].append(a)

print("--------------------------------------------------------------------------------------------------")

actual_spikes = []
for (index, value) in enumerate(end_spikes):
    print("LAYER {} ".format(index))
    for i, spike in enumerate(value):
        if len(spike) > 0 and index == 2:
            actual_spikes.append((i, spike))
        print(spike)
        print('\n')
    print('------------------------------------------\n')

print("------------------------------------------------------------------FINAL RESULTS-------------------")
apz = True
decode_params = []
for value in enumerate(actual_spikes[0][1]):
    if True:  # index == 1:
        for (idx, tr) in enumerate(value):
            print("Index = {} Value = {}".format(idx, tr))
            if idx == 1 and len(tr) > 0:
                for t in tr:
                    if t > 3.2:
                        decode_params.append(int(t * 10) - 32)
                        apz = False
            if idx == 1:
                if apz:
                    decode_params.append(0)
                else:
                    apz = True


print("DECODE PARAMS = {}".format(decode_params))
SNN_result = decode(decode_params)
print("RESULT = {}".format(SNN_result))
p.end()
print("Simulation Finished!")


def sigmoid(element):
    return 1 / (1 + np.exp(-element))


input_values = [[0, 0], [0, 1], [1, 0], [1, 1]]
for v in input_values:
    computed_val = [0] * 16
    for i in [0, 1]:
        counting = 0
        for j in range(0, 16):
            counting += 1
            weight = hf['dense']['dense']['kernel:0'][i][j]
            bias = hf['dense']['dense']['bias:0'][j]
            computed_val[j] += v[i] * weight + bias

    ANN_result = 0
    for (i, val) in enumerate(computed_val):
        if val > 0:
            ANN_result += val * hf['dense_1']['dense_1']['kernel:0'][i]

    ANN_result += hf['dense_1']['dense_1']['bias:0'][0]

    print("For input value: {}".format(v))
    print("FINAL RESULT: {}".format(sigmoid(ANN_result)))
