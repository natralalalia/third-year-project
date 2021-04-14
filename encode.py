def encode(activation):
    activation = int(activation)
    activations = [0] * 16
    if activation == 0:
        return [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    if activation <= 16:
        # neuron # 1 will fire
        return [16 - activation + 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    
    integer_part = int(activation / 16)

    if activation % 16 == 0:
        # one neuron will fire
        activations[integer_part - 1] = 1
        return activations
    firing_neurons = (integer_part, integer_part + 1)
    fractional_part = activation - integer_part * 16

    activations[firing_neurons[0] - 1] = (16 - fractional_part)
    activations[firing_neurons[1] - 1] = fractional_part
    firing_times = [(16 - x) % 16 for x in activations]
    for (i, v) in enumerate(firing_times):
        if v != 0:
            firing_times[i] += 1
    return firing_times



def alt_encode(activation):
    
    firing_times = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]

    if activation == 0:
        return firing_times
    if activation <= 16:
        return [16 - activation + 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
    if activation % 16 == 0:
        firing_times[int(activation / 16) - 1] = 1
        return firing_times

    firing_times[int(activation / 16) - 1] = (16 - (16 - activation - int(activation / 16) * 16)) % 16 + 1
    firing_times[int(activation / 16) + 1 - 1] = (16 - (activation - int(activation / 16) * 16)) % 16 + 1
    return firing_times


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
    if two_spikes == False:
        if neuron1 == 1:
            act = 17-time1
        else:
            act = 16*neuron1
    else:
        act = (17-time1)*neuron1 + (17-time2)*neuron2
    return act




correct = 0
for i in range(0, 256):
    # print("Activation {} encoded into the spiking times: {}".format(i, encode(i)))
    print(alt_encode(i))
    # print("Activation {} encoded into the spiking times: {}".format(i, alt_encode(i)))
    # print("Firing times {} decoded into activation {}".format(encode(i), decode(encode(i))))
    if(decode(encode(i)) == i):
        correct += 1
print("{} out of 256".format(correct))
# import pyNN.spiNNaker as p
# import scipy.io as sio
# from sklearn.datasets import fetch_openml
# import json

# mnist = fetch_openml('mnist_784').data[:1]

# def get_weights(layer_no):
#     with open("network_000230.json", encoding='utf-8', errors='ignore') as json_data:
#         wg = json.load(json_data, strict=False)['layers'][layer_no]
#         return wg

# def encode(activation):
#     activation = int(activation)
#     activations = [0] * 16
#     if activation == 0:
#         return [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]
#     if activation < 16:
#         # neuron # 1 will fire
#         return [activation, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]

#     integer_part = int(activation / 16)

#     if activation % 16 == 0:
#         # one neuron will fire
#         activations[integer_part] = 1
#         return activations
#     firing_neurons = (integer_part, integer_part + 1)
#     fractional_part = activation - integer_part * 16

#     activations[firing_neurons[0] - 1] = (16 - fractional_part)
#     activations[firing_neurons[1] - 1] = fractional_part
#     firing_times = [(16 - x) % 16 for x in activations]
#     return firing_times

# p.setup(timestep=0.1)
# # p.set_number_of_neurons_per_core(p.IF_curr_exp, 128)
# # Three populations corresponding to the three layers
# pops = [[], [], []]

# # Input the spikes through a spike source array
# for pixel in mnist:
#     sp = []
#     encoded_spikes = encode(mnist[pixel])
#     for val in encoded_spikes:
#         if val == 0:
#             sp.append([])
#         else:
#             sp.append([val])
#     print("activation = {} spikes = {}".format(mnist[pixel], sp))
#     pops[0].append(p.Population(16, p.SpikeSourceArray(sp)))

# for neuron in range(1, 64):
#     pops[1].append(p.Population(16, p.IF_curr_exp()))

# for neuron in range(1, 10):
#     pops[2].append(p.Population(1, p.IF_curr_exp()))

# # delays = p.RandomDistribution("normal_clipped", mu=1, sigma=1, low=1, high=2)
# # delays = p.RandomDistribution('normal', mu=0.5, sigma=0.1)

# weights = get_weights(0)
# for a in range(0, 783):
#     for b in range(0, 63):
#         p.Projection(pops[0][a], pops[1][b], p.OneToOneConnector(), 
#                      p.StaticSynapse(weight=10*weights['weightMx'][b][a]))
#                                      # delay=delays))
# weights = get_weights(1)
# for a in range(0, 63):
#     for b in range(0, 9):
#         p.Projection(pops[1][a], pops[2][b], p.OneToOneConnector(), 
#                      p.StaticSynapse(weight=100*weights['weightMx'][b][a]))
# ##                                     delay=delays))

# for po in pops:
#     for pop in po:
#         pop.record(["spikes"])

# weights

# p.run(4.8)

# end_spikes = [[], [], []]
# for (i, po) in enumerate(pops):
#     for pop in po:
#         a = pop.get_data(["spikes"]).segments[0].spiketrains
#         end_spikes[i].append(a)

# print(end_spikes)

# for (index, spike) in enumerate(end_spikes[0]):
#     for (i, s) in enumerate(spike):
#         if s.size > 0:
#             print("Initial value:")
#             print(mnist.values[0][index])
#             print("Encoded into:")
#             print(s)
#             print(i)
#         if s.size > 1:
#             print("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&")


# for (index, spike) in enumerate(end_spikes[2]):
#     for (i, s) in enumerate(spike):
#         if s.size > 0:
#             print(s)
#             print(i)
#         if s.size > 1:
#             print("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&")
# print("***")
# end_spikes[2]

# p.end()

# # import matplotlib.pyplot as plt
# # %matplotlib inline
# # fig = plt.figure
# # plt.imshow(mnist.values[0].reshape(28, 28), cmap='gray')
# # plt.show()