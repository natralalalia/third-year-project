/*
 * Copyright (c) 2017-2019 The University of Manchester
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*! \file
 * \brief implementation of the neuron.h interface.
 */

#include "neuron.h"
#include "neuron_recording.h"
#include "implementations/neuron_impl.h"
#include "plasticity/synapse_dynamics.h"
#include <debug.h>
#include <tdma_processing.h>

//! The key to be used for this core (will be ORed with neuron ID)
static key_t key;

//! A checker that says if this model should be transmitting. If set to false
//! by the data region, then this model should not have a key.
static bool use_key;

//! The number of neurons on the core
static uint32_t n_neurons;

//! The recording flags
static uint32_t recording_flags = 0;

//! parameters that reside in the neuron_parameter_data_region
struct neuron_parameters {
    uint32_t has_key;
    uint32_t transmission_key;
    uint32_t n_neurons_to_simulate;
    uint32_t n_synapse_types;
    uint32_t incoming_spike_buffer_size;
};

//! Offset of start of global parameters, in words.
#define START_OF_GLOBAL_PARAMETERS \
    ((sizeof(struct neuron_parameters) + \
      sizeof(struct tdma_parameters)) / sizeof(uint32_t))

//! \brief does the memory copy for the neuron parameters
//! \param[in] address: the address where the neuron parameters are stored
//!     in SDRAM
//! \return bool which is true if the mem copy's worked, false otherwise
static bool neuron_load_neuron_parameters(address_t address) {
    log_debug("loading parameters");
    // call the neuron implementation functions to do the work
    neuron_impl_load_neuron_parameters(
        address, START_OF_GLOBAL_PARAMETERS, n_neurons);
    return true;
}

bool neuron_resume(address_t address) { // EXPORTED
    if (!neuron_recording_reset(n_neurons)){
        log_error("failed to reload the neuron recording parameters");
        return false;
    }

    log_debug("neuron_reloading_neuron_parameters: starting");
    return neuron_load_neuron_parameters(address);
}

bool neuron_initialise(
        address_t address, address_t recording_address, // EXPORTED
        uint32_t *n_neurons_value, uint32_t *n_synapse_types_value,
        uint32_t *incoming_spike_buffer_size, uint32_t *n_rec_regions_used) {
    log_debug("neuron_initialise: starting");

    // init the TDMA
    void *data_addr = address;
    tdma_processing_initialise(&data_addr);

    // cast left over SDRAM into neuron struct.
    struct neuron_parameters *params = data_addr;

    // Check if there is a key to use
    use_key = params->has_key;

    // Read the spike key to use
    key = params->transmission_key;

    // output if this model is expecting to transmit
    if (!use_key) {
        log_debug("\tThis model is not expecting to transmit as it has no key");
    } else {
        log_debug("\tThis model is expected to transmit with key = %08x", key);
    }

    // Read the neuron details
    n_neurons = params->n_neurons_to_simulate;
    *n_neurons_value = n_neurons;
    *n_synapse_types_value = params->n_synapse_types;

    // Read the size of the incoming spike buffer to use
    *incoming_spike_buffer_size = params->incoming_spike_buffer_size;

    log_debug("\t n_neurons = %u, spike buffer size = %u", n_neurons,
            *incoming_spike_buffer_size);

    // Call the neuron implementation initialise function to setup DTCM etc.
    if (!neuron_impl_initialise(n_neurons)) {
        return false;
    }

    // load the data into the allocated DTCM spaces.
    if (!neuron_load_neuron_parameters(address)) {
        return false;
    }

    // setup recording region
    if (!neuron_recording_initialise(
            recording_address, &recording_flags, n_neurons, n_rec_regions_used)) {
        return false;
    }

    return true;
}

void neuron_pause(address_t address) { // EXPORTED
    /* Finalise any recordings that are in progress, writing back the final
     * amounts of samples recorded to SDRAM */
    if (recording_flags > 0) {
        log_debug("updating recording regions");
        neuron_recording_finalise();
    }

    // call neuron implementation function to do the work
    neuron_impl_store_neuron_parameters(
            address, START_OF_GLOBAL_PARAMETERS, n_neurons);
}

int act;
int official_activation;
int neuron_indexes_spikes[6] = {0};
int neuron_times_spikes[6] = {0};
int number_of_spikes = 0;
int official_activation_2;
int official_activation_1;
bool two_spikes = false;
int diffs[6] = {0};
int index_diffs = 0;


void neuron_do_timestep_update(timer_t time, uint timer_count) { // EXPORTED
    // the phase in this timer tick im in (not tied to neuron index)
    tdma_processing_reset_phase();

    // Prepare recording for the next timestep
    neuron_recording_setup_for_next_recording();

    if(time == 0) {
        official_activation = 0;
        official_activation_1 = 0;
        official_activation_2 = 0;
    }

    // update each neuron individually
    for (index_t neuron_index = 0; neuron_index < n_neurons; neuron_index++) {
         // Get external bias from any source of intrinsic plasticity
         input_t external_bias = synapse_dynamics_get_intrinsic_bias(time, neuron_index);
         printf("NG+++Neuron.c External Bias = %.6f n_neurons = %d \n\n\n", external_bias, n_neurons);
         uint32_t layer = get_layer(neuron_index);
         printf("Layer = %d\n", layer);
         printf("Time = %d", time);
         int diff = contributes_to_decoding(neuron_index, time, initial_activation);
         bool spike = neuron_impl_do_timestep_update(neuron_index, external_bias, two_spikes, time, official_activation_1, official_activation_2);

         if(spike) {
            printf("New spike arrived. Layer = %d neuron_index = %d time = %d\n", layer, neuron_index, time);
            printf("NUMBER OF SPIKES = %d \n", number_of_spikes);
                if(time > 16 * (layer - 1) + 1 && time < 16 * layer + 1) {
//
//                    printf("DIFF = %d which should be subtracted from neuron_times_spikes[%d], right???\n", diff, number_of_spikes);
//                    diffs[index_diffs] = diff;
//                    index_diffs++;
//
//                    for(int i = 0; i < index_diffs; i++) {
//                        printf("diff[%d] = %d; ", i, diffs[i]);
//                    }
//                    printf('\n');

                    printf("New spikes contributes. Layer = %d neuron_index = %d time = %d\n", layer, neuron_index, time);
                    neuron_indexes_spikes[number_of_spikes] = (int) (neuron_index + 1);
                    neuron_times_spikes[number_of_spikes] = (int) (time - 1);
                    number_of_spikes++;
//                    printf("NUMBER OF SPIKES = %d \n", number_of_spikes);
//                    for(uint32_t i = 0; i < 7; i++){
//                        printf("neuron_indexes_spikes[%d] = %d\n", i, neuron_indexes_spikes[i]);
//                        printf("neuron_times_spikes[%d] = %d\n", i, neuron_times_spikes[i]);
//                    }
                } else {
                    printf("Spike did not contribute.\n");
                }
         }
         if(layer == 2 && time <= 16) two_spikes = false;

         if(time % 16 == 0 && number_of_spikes != 0 && neuron_index == n_neurons - 1) { // for middle and output layers
            printf("--------------------SPIKES RECEIVED--------------------------\n");
            for(uint32_t i = 0; i < number_of_spikes; i++){
                printf("neuron_indexes_spikes[%d] = %d\n", i, neuron_indexes_spikes[i]);
                printf("neuron_times_spikes[%d] = %d\n", i, neuron_times_spikes[i]);
            }
            printf("-----------------------DECODED AS------------------------------\n");

            if(number_of_spikes == 1) {
                official_activation_2 = 0;
                // only received one spike; activation is between 0 and 16 or a multiple of 16
                if(neuron_indexes_spikes[0] == 1) official_activation_1 = 17 - neuron_times_spikes[0]; // A < 16
                else official_activation_1 = 16*neuron_indexes_spikes[0]; // A % 16 == 0
                printf("Official activation is %d as only one neuron spiked!\n", official_activation_1);
            } else {
                if(number_of_spikes == 2) {
                    printf("Number of spikes is 2. Something didn't work well. Maybe each of the two activations from the previous layer was a multiple of 16, or less than 16 \n");
                    official_activation_2 = 0;
                    official_activation_1 = neuron_indexes_spikes[0]*(17-neuron_times_spikes[0]%16) +
                                            (17-neuron_times_spikes[1]%16)*neuron_indexes_spikes[1];
                } else {
                    if(number_of_spikes == 3) {
                        printf("Number of spikes is 3. Something didn't work well. \n");
                    }
                    if(number_of_spikes == 4) {
                        printf("Number of spikes is 4 \n");
                        official_activation_1 = neuron_indexes_spikes[0]*(17-neuron_times_spikes[0]%16) +
                                                (17-neuron_times_spikes[3]%16)*neuron_indexes_spikes[3];

                        official_activation_2 = (17-neuron_times_spikes[1]%16)*neuron_indexes_spikes[1] +
                                                (17-neuron_times_spikes[2]%16)*neuron_indexes_spikes[2];
                    }
                }
            }

            printf("Official Activation 1 = %d \n Official Activation 2 = %d \n", official_activation_1, official_activation_2);
            printf("Need to decode the activation...\n");

            printf("Reset number of spikes, neuron_times_spikes and neuron_indexes_spikes! \n\n");
            two_spikes = true;
            for(uint32_t i = 0; i < number_of_spikes; i++) {
                neuron_times_spikes[i] = 0;
                neuron_indexes_spikes[i] = 0;
            }
            number_of_spikes = 0;
        } else two_spikes = false;

        // If the neuron has spiked
        if (spike) {
            printf("\nNeuron %d spiked! at time %d\n", neuron_index, time);
            // Do any required synapse processing
            synapse_dynamics_process_post_synaptic_event(time, neuron_index);

            if (use_key) {
                tdma_processing_send_packet(
                    (key | neuron_index), 0, NO_PAYLOAD, timer_count);
            }
        } else {
            printf("the neuron %d has been determined to not spike",
                      neuron_index);
         }
    }
    log_debug("time left of the timer after tdma is %d", tc[T1_COUNT]);

    // Disable interrupts to avoid possible concurrent access
    uint cpsr = spin1_int_disable();

    printf("Recorded time = %u", time);
    // Record the recorded variables
    neuron_recording_record(time);

    // Re-enable interrupts
    spin1_mode_restore(cpsr);
}

uint32_t my_activation_1() {
    return official_activation_1;
}

uint32_t my_activation_2() {
    return official_activation_2;
}

void neuron_add_inputs( // EXPORTED
        index_t synapse_type_index, index_t neuron_index,
        input_t weights_this_timestep) {
    neuron_impl_add_inputs(
            synapse_type_index, neuron_index, weights_this_timestep);
}

#if LOG_LEVEL >= LOG_DEBUG
void neuron_print_inputs(void) { // EXPORTED
    neuron_impl_print_inputs(n_neurons);
}

void neuron_print_synapse_parameters(void) { // EXPORTED
    neuron_impl_print_synapse_parameters(n_neurons);
}

const char *neuron_get_synapse_type_char(uint32_t synapse_type) { // EXPORTED
    return neuron_impl_get_synapse_type_char(synapse_type);
}
#endif // LOG_LEVEL >= LOG_DEBUG
