from spynnaker.pyNN.models.neuron import AbstractPyNNNeuronModel
from spynnaker.pyNN.models.defaults import default_parameters
from python_models8.neuron.implementations.my_full_neuron_impl \
    import MyFullNeuronImpl


class MyFullNeuron(AbstractPyNNNeuronModel):
    @default_parameters({"threshold"})
    def __init__(self, threshold=0.0, v=0.0, exc_input=0.0, inh_input=0.0, activation=-1.0, decode=1.0, bias=0.0):
        AbstractPyNNNeuronModel.__init__(
            self, MyFullNeuronImpl(threshold, v, exc_input, inh_input, activation, decode, bias))
