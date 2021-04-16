
//! neuron_impl_t struct
typedef struct neuron_impl_t {
    accum inputs[2];
    accum v;
    accum threshold;
    accum activation;
    accum decode;
    accum bias;
    accum layer;
} neuron_impl_t;

typedef struct global_neuron_params_t {
    uint32_t machine_time_step;
} global_neuron_params_t;

static const global_neuron_params_t *global_params;

//! Array of neuron states
static neuron_impl_t *neuron_array;


__attribute__((unused)) // Marked unused as only used sometimes
static bool neuron_impl_initialise(uint32_t n_neurons) {
    // Allocate DTCM for neuron array
    if (sizeof(neuron_impl_t) != 0) {
        neuron_array = spin1_malloc(n_neurons * sizeof(neuron_impl_t));
        if (neuron_array == NULL) {
            log_error("Unable to allocate neuron array - Out of DTCM");
            return false;
        }
    }

    return true;
}

__attribute__((unused)) // Marked unused as only used sometimes
static void neuron_impl_load_neuron_parameters(
        address_t address, uint32_t next, uint32_t n_neurons) {
    // Copy parameters to DTCM from SDRAM
    spin1_memcpy(neuron_array, &address[next],
            n_neurons * sizeof(neuron_impl_t));
}

__attribute__((unused)) // Marked unused as only used sometimes
static void neuron_impl_store_neuron_parameters(
        address_t address, uint32_t next, uint32_t n_neurons) {
    // Copy parameters to SDRAM from DTCM
    spin1_memcpy(&address[next], neuron_array,
            n_neurons * sizeof(neuron_impl_t));
}

int weights[100];
int index;
__attribute__((unused)) // Marked unused as only used sometimes
static void neuron_impl_add_inputs(
        index_t synapse_type_index, index_t neuron_index,
        input_t weights_this_timestep) {
    // Get the neuron itself
    neuron_impl_t *neuron = &neuron_array[neuron_index];

    if(synapse_type_index == 0) {
        if(weights_this_timestep != 0) {
            printf("Neuron Index %d in layer %d and population %d generated excitatory connection of weight %d\n",
                    neuron_index, neuron->layer, neuron->decode, weights_this_timestep);
            weights[index] = weights_this_timestep;
            index++;
        }
    } else if(synapse_type_index == 1) {
        if(weights_this_timestep != 0) {
            printf("Neuron Index %d in layer %d and population %d generated inhibitory connection of weight %d\n",
                    neuron_index, neuron->layer, neuron->decode, weights_this_timestep);
            weights[index] -= (int) weights_this_timestep;;
            index++;
        }
    }

    if(weights_this_timestep != 0) {
        neuron->inputs[synapse_type_index] += 1 ; // neutralise weights!
    }
}

__attribute__((unused)) // Marked unused as only used sometimes
static int get_activation(index_t neuron_index){
    neuron_impl_t *neuron = &neuron_array[neuron_index];
    return neuron->activation;
}

__attribute__((unused)) // Marked unused as only used sometimes
static uint32_t get_decode(index_t neuron_index){
    neuron_impl_t *neuron = &neuron_array[neuron_index];
    return neuron->decode;
}

int activation_per_population[6];
int initial_activation;
__attribute__((unused)) // Marked unused as only used sometimes
static bool neuron_impl_do_timestep_update(
        index_t neuron_index, input_t external_bias, bool two_spikes, uint32_t time, int activation_1, int activation_2) {
    // Get the neuron itself
    neuron_impl_t *neuron = &neuron_array[neuron_index];

    // Store the recorded membrane voltage
    neuron_recording_record_accum(V_RECORDING_INDEX, neuron_index, neuron->v);

//    int layer = (int) (neuron->decode) / 2;
    int layer = (int) (neuron->layer);
    printf("LAYER = %d\n", layer);
    int pop = neuron->decode;

    if(time == 0) {
        activation_per_population[pop] = neuron->activation;
    }

    initial_activation = activation_per_population[pop]; // neuron->activation;

    if(initial_activation == -1 && two_spikes == true) {
        // this means neuron needs decoding, i.e. it is in the hidden/output layers
        // and it has received enough spikes to do the decoding
        printf("Initial activation 1 = %d\nInitial activation 2 = %d \n", activation_1, activation_2);
        printf("Initial weight 1 = %d\nInitial weight 2 = %d\nBias = %d\n", weights[layer * 2], weights[layer * 2 + 1],
                                                                            (int) neuron->bias);
        int new_activation = activation_1*weights[layer * 2] + activation_2*weights[layer * 2 + 1] + neuron->bias;
        neuron->activation = new_activation;
        initial_activation = neuron->activation;
        activation_per_population[pop] = neuron->activation;
        if(initial_activation < 0) initial_activation = 0;
        if(initial_activation > 255) initial_activation = 255;
        printf("NG+ NEW activation = %d\n", initial_activation);
        printf("NG+ Need to propagate this activation as encoded spikes to the next layer!\n");
    }

    if(initial_activation != -1) {
        // encoding
        if(initial_activation <= 0) {
            neuron->v = 0k;
            neuron_recording_record_bit(SPIKE_RECORDING_BITFIELD, neuron_index);
            return false; // ReLU
        }
        if(initial_activation <= 16) {
            if(neuron_index == 0 && time == 16 * layer + 16 - initial_activation + 1) {
                neuron->v += external_bias + neuron->inputs[0] + neuron->inputs[1];
                neuron->v = 0k;
                neuron_recording_record_bit(SPIKE_RECORDING_BITFIELD, neuron_index);
                return true;
            }
        } else {
            if(initial_activation % 16 == 0) {
                if (neuron_index == initial_activation / 16 - 1 && time % 16 == 1) {
                    neuron->v += external_bias + neuron->inputs[0] + neuron->inputs[1];
                    neuron->v = 0k;
                    neuron_recording_record_bit(SPIKE_RECORDING_BITFIELD, neuron_index);
                    return true;
                }
            }
          else {
            if(neuron_index == initial_activation / 16) {
                if(time == 16 * layer + (16 - (initial_activation - (initial_activation / 16) * 16)) % 16 + 1){
                    neuron->v += external_bias + neuron->inputs[0] + neuron->inputs[1];
                    neuron->v = 0k;
                    neuron_recording_record_bit(SPIKE_RECORDING_BITFIELD, neuron_index);
                    return true;
                }
            } else {
                if(neuron_index == initial_activation / 16 - 1) {
                    if(time == 16 * layer + (16 - (16 - initial_activation - (initial_activation / 16) * 16)) % 16 + 1){
                        neuron->v += external_bias + neuron->inputs[0] + neuron->inputs[1];
                        neuron->v = 0k;
                        neuron_recording_record_bit(SPIKE_RECORDING_BITFIELD, neuron_index);
                        return true;
                    }
                }
            }
          }
        }
    }
    neuron->v += external_bias + neuron->inputs[0] + neuron->inputs[1];
    neuron->inputs[0] = 0;
    neuron->inputs[1] = 0;

    // Determine if the neuron has spiked
    if (neuron->v > neuron->threshold) {
        // Reset if spiked
        neuron->v = 0k;
        neuron_recording_record_bit(SPIKE_RECORDING_BITFIELD, neuron_index);
        return true;
    }
    return false;
}

static int contributes_to_decoding(index_t neuron_index, uint32_t time, uint32_t initial_activation) {
    // Get the neuron itself
    neuron_impl_t *neuron = &neuron_array[neuron_index];
    int k = neuron->inputs[0];
    return k;
}

static uint32_t get_layer(index_t neuron_index) {
    neuron_impl_t *neuron = &neuron_array[neuron_index];
    return neuron->layer;
}
