#ifndef _MY_FULL_NEURON_IMPL_
#define _MY_FULL_NEURON_IMPL_

// Demonstrating that a "neuron model" can be defined in a different
// way without the use of components for additional input / input / threshold

#include <neuron/implementations/neuron_impl.h>
#include <spin1_api.h>
#include <debug.h>

#define V_RECORDING_INDEX 0
#define N_RECORDED_VARS 1

#define SPIKE_RECORDING_BITFIELD 0
#define N_BITFIELD_VARS 1

#include <neuron/neuron_recording.h>

//! neuron_impl_t struct
typedef struct neuron_impl_t {
    accum inputs[2];
    accum v;
    accum threshold;
    accum activation;
    accum decode;
    accum bias;
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

__attribute__((unused)) // Marked unused as only used sometimes
static void neuron_impl_add_inputs(
        index_t synapse_type_index, index_t neuron_index,
        input_t weights_this_timestep) {
    // Get the neuron itself
    neuron_impl_t *neuron = &neuron_array[neuron_index];

    if(weights_this_timestep != 0){
        weights_this_timestep = 1;
    }

    // Do something to store the inputs for the next state update
    neuron->inputs[synapse_type_index] += weights_this_timestep;
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
int initial_activation_1;
int initial_activation_2;
uint32_t index;
int weights[6];
__attribute__((unused)) // Marked unused as only used sometimes
static bool neuron_impl_do_timestep_update(
        index_t neuron_index, input_t external_bias, bool two_spikes, uint32_t time, int activation_1, int activation_2) {
    // Get the neuron itself
    neuron_impl_t *neuron = &neuron_array[neuron_index];

    printf("NEURON INDEX  = %d \n\n\n", neuron_index);
    // Store the recorded membrane voltage
    neuron_recording_record_accum(V_RECORDING_INDEX, neuron_index, neuron->v);

//    int layer = -1;
//    if(neuron->decode == 0 || neuron->decode == 1) layer = 0;
//    if(neuron->decode == 2 || neuron->decode == 3) layer = 1;
//    if(neuron->decode == 4) layer = 2;
//    printf("layer = %d \n\n", layer);
    int layer = (int) (neuron->decode) / 2;
    int pop = neuron->decode;

    if(time == 0) {
        index=0;
        activation_per_population[pop] = neuron->activation;
    }
    int bias = 0;
    if(neuron->decode == 2) bias = 5;
    else if(neuron->decode == 3) bias = 7;
    else if(neuron->decode == 4) bias = -60;

    initial_activation = activation_per_population[pop]; // neuron->activation;
    initial_activation_2 = -1;
    initial_activation_1 = -1;
    int exc_weight = neuron->inputs[0];
    printf("exc_weight = %d\n", exc_weight);
    if(exc_weight != 0) {
        weights[index] = exc_weight;
        printf("Weights[%d]=%d", index, weights[index]);
        index++;
    }
    int inh_weight = neuron->inputs[1];
    printf("inh_weight = %d", inh_weight);
    if(inh_weight != 0) {
        weights[index] = inh_weight;
        printf("Weights[%d]=%d", index, weights[index]);
        index++;
    }

    if(neuron->inputs[0] != 0)
        neuron->inputs[0] = 1;
    if(neuron->inputs[1] != 0)
        neuron->inputs[1] = 1;
    if(external_bias != 0)
        external_bias = 1;

    if(initial_activation == -1 && two_spikes == true) {
        // this means neuron needs decoding, i.e. it is in the hidden layers
        // and it has received two spikes, so my_activation() is the initial activation
        initial_activation_1 = activation_1;
        initial_activation_2 = activation_2;

        printf("Initial activation 1 = %d \n Initial activation 2 = %d \n", initial_activation_1, initial_activation_2);
        printf("Initial weight 1 = %d \n Initial weight 2 = %d \n Bias = %d\n", weights[layer * 2], weights[layer * 2 + 1], bias);
        neuron->activation = initial_activation_1*weights[layer * 2] + initial_activation_2*weights[layer * 2 + 1] + bias;
        initial_activation = neuron->activation;
        activation_per_population[pop] = neuron->activation;

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
                neuron->v += external_bias + neuron->inputs[0] - neuron->inputs[1];
                neuron->v = 0k;
                neuron_recording_record_bit(SPIKE_RECORDING_BITFIELD, neuron_index);
                return true;
            }
        } else {
            if(initial_activation % 16 == 0) {
                if (neuron_index == initial_activation / 16 - 1 && time % 16 == 1) {
                    neuron->v += external_bias + neuron->inputs[0] - neuron->inputs[1];
                    neuron->v = 0k;
                    neuron_recording_record_bit(SPIKE_RECORDING_BITFIELD, neuron_index);
                    return true;
                }
            }
          else {
            if(neuron_index == initial_activation / 16) {
                if(time == 16 * layer + (16 - (initial_activation - (initial_activation / 16) * 16)) % 16 + 1){
                    neuron->v += external_bias + neuron->inputs[0] - neuron->inputs[1];
                    neuron->v = 0k;
                    neuron_recording_record_bit(SPIKE_RECORDING_BITFIELD, neuron_index);
                    return true;
                }
            } else {
                if(neuron_index == initial_activation / 16 - 1) {
                    if(time == 16 * layer + (16 - (16 - initial_activation - (initial_activation / 16) * 16)) % 16 + 1){
                        neuron->v += external_bias + neuron->inputs[0] - neuron->inputs[1];
                        neuron->v = 0k;
                        neuron_recording_record_bit(SPIKE_RECORDING_BITFIELD, neuron_index);
                        return true;
                    }
                }
            }
          }
        }
    }

    neuron->v += external_bias + neuron->inputs[0] - neuron->inputs[1];
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
    uint32_t layer = neuron->decode / 2;
    return layer;
}

#if LOG_LEVEL >= LOG_DEBUG
void neuron_impl_print_inputs(uint32_t n_neurons) {
    log_debug("-------------------------------------\n");
    for (index_t i = 0; i < n_neurons; i++) {
        neuron_impl_t *neuron = &neuron_array[i];
        log_debug("inputs: %k %k", neuron->inputs[0], neuron->inputs[1]);
    }
    log_debug("-------------------------------------\n");
}

void neuron_impl_print_synapse_parameters(uint32_t n_neurons) {
    // there aren't any accessible in this example
    use(n_neurons);
}

const char *neuron_impl_get_synapse_type_char(uint32_t synapse_type) {
    use(synapse_type);
    return 0;
}
#endif // LOG_LEVEL >= LOG_DEBUG


#endif // _MY_FULL_NEURON_IMPL_
