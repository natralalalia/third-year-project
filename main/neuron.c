
int decoded_activation = 0;
int neuron_indexes_spikes[6] = {0};
int neuron_times_spikes[6] = {0};
int number_of_spikes = 0;
int decoded_activation_2 = 0;
int decoded_activation_1 = 0;
bool ready_to_encode = false;
int diffs[6] = {0};
int index_diffs = 0;


void neuron_do_timestep_update(timer_t time, uint timer_count) { // EXPORTED
    // the phase in this timer tick im in (not tied to neuron index)
    tdma_processing_reset_phase();

    // Prepare recording for the next timestep
    neuron_recording_setup_for_next_recording();

    // update each neuron individually
    for (index_t neuron_index = 0; neuron_index < n_neurons; neuron_index++) {
         // Get external bias from any source of intrinsic plasticity
         input_t external_bias = synapse_dynamics_get_intrinsic_bias(time, neuron_index);
         uint32_t layer = get_layer(neuron_index);
         printf("\nTime = %d Layer = %d Neuron Index = %d\n", time, layer, neuron_index);
         bool spike = neuron_impl_do_timestep_update(neuron_index, external_bias, ready_to_encode, time, decoded_activation_1, decoded_activation_2);

         if(spike) {
            printf("New spike arrived. Layer = %d neuron_index = %d time = %d\n", layer, neuron_index, time);
                if(time > 16 * (layer - 1) + 2 && time < 16 * layer + 1) {
                    printf("New spikes contributes to decoding the activation!");
                    neuron_indexes_spikes[number_of_spikes] = (int) (neuron_index + 1);
                    neuron_times_spikes[number_of_spikes] = (int) (time - 1);
                    number_of_spikes++;
                } else {
                    printf("Spike did not contribute to decoding the activation.\n");
                }
         }

         if(time % 16 == 0 && number_of_spikes != 0 && neuron_index == n_neurons - 1) {
            printf("--------------------SPIKES RECEIVED--------------------------\n");
            for(uint32_t i = 0; i < number_of_spikes; i++){
                printf("neuron_indexes_spikes[%d] = %d\n", i, neuron_indexes_spikes[i]);
                printf("neuron_times_spikes[%d] = %d\n", i, neuron_times_spikes[i]);
            }
            printf("-----------------------DECODED AS----------------------------\n");

            if(number_of_spikes == 1) {
                decoded_activation_2 = 0;
                // only received one spike; activation is between 0 and 16 or a multiple of 16
                if(neuron_indexes_spikes[0] == 1) decoded_activation_1 = 17 - neuron_times_spikes[0]; // A < 16
                else decoded_activation_1 = 16*neuron_indexes_spikes[0]; // A % 16 == 0
                printf("Decoded activation is %d as only one neuron spiked!\n", decoded_activation_1);
            } else {
                if(number_of_spikes == 2) {
                    printf("Number of spikes is 2. Something didn't work well.");
                    decoded_activation_2 = 0;
                    decoded_activation_1 = neuron_indexes_spikes[0]*(17-neuron_times_spikes[0]%16) +
                                            (17-neuron_times_spikes[1]%16)*neuron_indexes_spikes[1];
                } else {
                    if(number_of_spikes == 3) {
                        printf("Number of spikes is 3. Something didn't work well. \n");
                        decoded_activation_2 = neuron_indexes_spikes[1]*(17-neuron_times_spikes[1]%16) +
                                               (17-neuron_times_spikes[2]%16)*neuron_indexes_spikes[2];
                        if(neuron_indexes_spikes[0] == 1) decoded_activation_1 = 17 - neuron_times_spikes[0]; // A < 16
                        else decoded_activation_1 = 16*neuron_indexes_spikes[0];                         // A % 16 == 0
                    }
                    if(number_of_spikes == 4) {
                        printf("Number of spikes is 4 \n");
                        decoded_activation_1 = neuron_indexes_spikes[0]*(17-neuron_times_spikes[0]%16) +
                                                (17-neuron_times_spikes[3]%16)*neuron_indexes_spikes[3];

                        decoded_activation_2 = (17-neuron_times_spikes[1]%16)*neuron_indexes_spikes[1] +
                                                (17-neuron_times_spikes[2]%16)*neuron_indexes_spikes[2];
                    }
                }
            }

            printf("Decoded Activation 1 = %d \nDecoded Activation 2 = %d \n", decoded_activation_1, decoded_activation_2);
            printf("Need to encode the activation...\n");

            printf("Reset number of spikes, neuron_times_spikes and neuron_indexes_spikes! \n\n");
            ready_to_encode = true;
            for(uint32_t i = 0; i < number_of_spikes; i++) {
                neuron_times_spikes[i] = 0;
                neuron_indexes_spikes[i] = 0;
            }
            number_of_spikes = 0;
        } else ready_to_encode = false;

        // If the neuron has spiked
        if (spike) {
            printf("\nNeuron %d spiked! at time %d\n", neuron_index, time);
            // Do any required synapse processing
            synapse_dynamics_process_post_synaptic_event(time, neuron_index);

            if (use_key) {
                tdma_processing_send_packet(
                    (key | neuron_index), 0, NO_PAYLOAD, timer_count);
            }
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
    return decoded_activation_1;
}

uint32_t my_activation_2() {
    return decoded_activation_2;
}

void neuron_add_inputs( // EXPORTED
        index_t synapse_type_index, index_t neuron_index,
        input_t weights_this_timestep) {
    neuron_impl_add_inputs(
            synapse_type_index, neuron_index, weights_this_timestep);
}
