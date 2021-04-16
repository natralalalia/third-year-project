import h5py
import spynnaker8 as sim
from python_models8.neuron.builds.my_full_neuron import MyFullNeuron
import numpy as np
import json


def decode(firing_times):
    first = False
    neuron1 = 0
    neuron2 = 0
    time1 = 0
    time2 = 0
    two_spikes = False
    for (idx, val) in enumerate(firing_times):
        if val != 0 and not first:
            neuron1 = idx + 1
            time1 = val
            first = True
        elif val != 0 and first:
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


pre_trained_ANN = input("Pre-trained ANN parameters (json / h5py):")
input_values = [20, 30]

no_of_layers = 3
no_of_units_per_layer = [2, 2, 1]

weights = []
biases = []
with open(pre_trained_ANN, 'r') as weights_file:
    weights_data = json.load(weights_file)
    for value in weights_data["layer_1"]:
        weights.append(value)
    for value in weights_data["layer_2"]:
        weights.append(value)
    for bias in weights_data["bias_1"]:
        biases.append(bias)
    for bias in weights_data["bias_2"]:
        biases.append(bias)
    for bias in weights_data["bias_3"]:
        biases.append(bias)
sim.setup(timestep=0.1)

populations = [[] for i in range(no_of_layers)]
projections = [[] for i in range(no_of_layers - 1)]

# set up populations
pop_index = 0
for idx, layer in enumerate(no_of_units_per_layer):
    for i in range(layer):
        if idx == 0:
            act = input_values[i]
        else:
            act = -1
        populations[idx].append(sim.Population(16, MyFullNeuron(decode=pop_index, activation=act,
                                                                bias=biases[pop_index],
                                                                layer=idx)))
        pop_index += 1

weight_index = 0

# set up projections
for layer in range(no_of_layers - 1):
    for origin in range(no_of_units_per_layer[layer]):
        for destination in range(no_of_units_per_layer[layer + 1]):
            if weights[weight_index] < 0:
                rt = "inhibitory"
            else:
                rt = "excitatory"
            projections[layer].append(sim.Projection(populations[layer][origin],
                                                     populations[layer + 1][destination], sim.OneToOneConnector(),
                                                     sim.StaticSynapse(weight=weights[weight_index], delay=0),
                                                     receptor_type=rt))
            print("Connection between pop[{}][{}] and pop[{}][{}] of weight {}".format(
                layer, origin, layer + 1, destination, weights[weight_index]))
            weight_index += 1

# record spikes
for layer in populations:
    for population in layer:
        population.record(["spikes", "v"])

runtime = round(1.6 * no_of_layers, 1)
sim.run(runtime)

end_spikes = [[] for i in range(no_of_layers)]
for (i, population) in enumerate(populations):
    for unit in population:
        end_spikes[i].append(unit.get_data(["spikes"]).segments[0].spiketrains)

actual_spikes = []
for value in end_spikes[no_of_layers - 1]:
    for (i, spike) in enumerate(value):
        actual_spikes.append((i, spike))

print("------------FINAL RESULTS-------------------")
output_spike = True
decode_params = []
for value in actual_spikes:
    for (idx, tr) in enumerate(value):
        print("Index = {} Value = {}".format(idx, tr))
        if idx == 1 and len(tr) > 0:
            for t in tr:
                if t > (runtime - 1.6):
                    decode_params.append(int(t * 10) - int(runtime * 10 - 16))
                    output_spike = False
        if idx == 1:
            if output_spike:
                decode_params.append(0)
            else:
                output_spike = True

print("Firing times in the output layer = {}".format(decode_params))
SNN_result = decode(decode_params)
print("RESULT = {}".format(SNN_result))
sim.end()
print("Simulation Finished!")


# def sigmoid(element):
#     return 1 / (1 + np.exp(-element))

#
# input_values = [[0, 0], [0, 1], [1, 0], [1, 1]]
# for v in input_values:
#     computed_val = [0] * 16
#     for i in [0, 1]:
#         counting = 0
#         for j in range(0, 16):
#             counting += 1
#             weight = hf['dense']['dense']['kernel:0'][i][j]
#             bias = hf['dense']['dense']['bias:0'][j]
#             computed_val[j] += v[i] * weight + bias
#
#     ANN_result = 0
#     for (i, val) in enumerate(computed_val):
#         if val > 0:
#             ANN_result += val * hf['dense_1']['dense_1']['kernel:0'][i]
#
#     ANN_result += hf['dense_1']['dense_1']['bias:0'][0]
#
#     print("For input value: {}".format(v))
#     print("FINAL RESULT: {}".format(sigmoid(ANN_result)))
