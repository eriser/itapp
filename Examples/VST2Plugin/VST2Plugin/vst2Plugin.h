#include <cstdint>
#include <string>
#include <vector>

#include "audioeffectx.h"
#include "audioeffect.h"
#include "oscillator.h"


namespace ttmm { // Trommel-Tanz-Musik-Maschine

    // http://en.wikipedia.org/wiki/MIDI_Tuning_Standard
    float midi_to_hertz(std::uint8_t note_number) {
        return static_cast<float>(std::pow(2, (note_number - 69.0f) / 12) * 440.0f);
    }

    enum class MidiCodes {
        NoteOn = 0x90,
        NoteOff = 0x80
        // more but don't care now
    };

    // The SDK gives the code as int. With these, we can compare them with MidiCodes.
    bool operator==(MidiCodes const& mc, VstInt32 const& vst_code) {
        return static_cast<std::underlying_type_t<MidiCodes>>(mc) == vst_code;
    }
    bool operator==(VstInt32 const& vst_code, MidiCodes const& mc) {
        return mc == vst_code;
    }


    /**
     * Declaration of the most simple plugin-Synthesizer
     */
	class VST2Plugin : public AudioEffectX {
		using AudioSignalType = float;
        using Frequency = float;
        using Phase = float;
        using SamplingRate = unsigned;
        using Amplitude = float;
        using Note = VstInt32;
        using Velocity = VstInt32;
		using Abilities = std::vector<std::string>;

	public:
		VST2Plugin(audioMasterCallback audio_master);
		~VST2Plugin(void) = default;

		// implementation of AudioEffect[X]. See the documentation in the VST2-SDK
        virtual void setSampleRate(float sample_rate) override;

        virtual VstInt32 processEvents(VstEvents* ev) override;

		virtual void processReplacing(AudioSignalType** inputs,
                                      AudioSignalType** outputs,
                                      VstInt32 sample_frame_count) override;

		virtual VstInt32 canDo(char* ability) override;

        virtual VstInt32 getNumMidiInputChannels(void) override { return 1; }

        virtual VstInt32 getNumMidiOutputChannels(void) override { return 0; }

    private:
        Abilities abilities_; ///< queried by the host

        ttmm_oscillator::Sine sine;
        Frequency frequency_{ 880.0f };
        Phase initial_phase_{ 0.0f };
        SamplingRate sampling_rate_{ 44100 };
        Amplitude amplitude_{ 0.5 };

        Note current_note_{ 64 };
        Velocity current_velocity_{ 0 };
        bool playing_{ false };

        void note_off_event(void);
        void note_on_event(Note const& note, Velocity const& velocity);
	};
}