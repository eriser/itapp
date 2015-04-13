#include "vst2Plugin.h"

#include <algorithm>

static float pulse[4096];
static float scale = 4096 / 44100.0f; 

ttmm::VST2Plugin::VST2Plugin(audioMasterCallback audio_master) : AudioEffectX(audio_master,
                                                                              0, // no programs
                                                                              0) { // no paramater
    abilities_.push_back("receiveVstMidiEvent");
    abilities_.push_back("receiveVstEvents");
    setNumInputs(0);
    setNumOutputs(2);
    sine.set_sampling_rate(44100); // default rate, actual not known yet
}

// Instantiate the plugin and return a pointer to the base-baseclass
AudioEffect* createEffectInstance(audioMasterCallback audio_master) {
    return new ttmm::VST2Plugin(audio_master);
}

void ttmm::VST2Plugin::setSampleRate(float sampling_rate) {
    sine.set_sampling_rate(static_cast<unsigned>(sampling_rate));
    AudioEffectX::setSampleRate(sampling_rate);
}

VstInt32 ttmm::VST2Plugin::processEvents(VstEvents* events) {
    // see aeffectx.h for the declaration of VstEvents
    // see http://www.somascape.org/midi/tech/spec.html for the contents of midi messages
    // Ignoring everything but Note-On/Off signals here

    static const auto steinberg_midi_flag = kVstMidiType;

    for (auto i = 0; i < events->numEvents; ++i) {
        auto const* event = reinterpret_cast<VstMidiEvent*>(events->events[i]);
        if (event->type != steinberg_midi_flag) {
            continue;
        }

        auto const payload = event->midiData;
        auto const status = payload[0] & 0xf0; // status sits in the upper four bytes

        if (status == MidiCodes::NoteOff || status == MidiCodes::NoteOn) {
            auto note = payload[1] & 0x7f; // actual data in the lower 7 bits
            auto velocity = payload[2] & 0x7f; // same

            if ((!velocity) && (note == current_note_)) {
                note_off_event();
            }
            else {
                note_on_event(note, velocity);
            }
        }
    }

    return 1;
}

void ttmm::VST2Plugin::note_off_event(void) {
    playing_ = false;
}

void ttmm::VST2Plugin::note_on_event(Note const& note, Velocity const& velocity) {
    current_note_ = note;
    current_velocity_ = velocity;
    sine.set_frequency(midi_to_hertz(note));
    playing_ = true;
}

void ttmm::VST2Plugin::processReplacing(AudioSignalType** inputs, AudioSignalType** outputs,
                                        VstInt32 sample_frame_count) {
    if (!playing_) {
        std::memset(outputs[0], 0, sizeof(AudioSignalType) * sample_frame_count);
        std::memset(outputs[1], 0, sizeof(AudioSignalType) * sample_frame_count);
    }

    else {
        // process data from inputs and put it into outputs
        sine.fill_stereo_buffer(outputs, sample_frame_count);
    }
}

VstInt32 ttmm::VST2Plugin::canDo(char* ability) {
    auto as_string = std::string{ ability };
    if (std::find(std::begin(abilities_), std::end(abilities_), as_string) != abilities_.end()) {
        return 1;
    }

    return -1; // can't do. 0 is maybe.
}