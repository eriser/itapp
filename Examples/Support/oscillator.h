#include <cmath>
#include <array>

namespace ttmm_oscillator {

    class Oscillator {
    public:
        using audioSignalType = float;
        virtual void fill_stereo_buffer(audioSignalType** buffer, size_t samples) = 0;
        virtual void set_sampling_rate(unsigned rate) = 0;
        virtual void set_frequency(float frequency) = 0;
        virtual void reset(void) = 0;
    };

    // Abstract base class for implementation of an interpolating table lookup oscillator.
    // The only method that concrete classes have to implment is gen_table which has to
    // generate the lookup table used by the Oscillator.
    template<size_t TableSize, typename AudioSignalType>
    class InterpolatingTableLookupOscillator : public Oscillator {
    public:
        static const size_t tableSize = TableSize;
        using audioSignalType = AudioSignalType;
        using table = std::array<audioSignalType, tableSize>;

        virtual ~InterpolatingTableLookupOscillator(void) = default;

        virtual void set_sampling_rate(unsigned sr) override final {
            sampling_rate_ = sr;
            reset();
        }

        virtual void reset(void) override final {
            gen_table(table_);
        }

        virtual void set_frequency(float frequency) override final {
            increment_ = tableSize * frequency / static_cast<float>(sampling_rate_);
            phase_ = 0;
        }

        /**
         * Fill the stereo buffer with samples retrieved from the pre-calculated table.
         * The actual index into the table is interpolated between two consecutive values,
         * since the 'real' index is based on our current frequency and not an integer.
         * The interpolated value is calculated in the inner helper function interpol.
         */
        virtual void fill_stereo_buffer(audioSignalType** buffer, size_t samples) override final {

            // Two helper functions. These will be inlined in the loop below, so no overhead
            auto interpol = [this](float const& virtual_index) {
                auto const lower = static_cast<int>(virtual_index);
                auto const upper = static_cast<int>(virtual_index + 1) % tableSize;
                auto const weight = virtual_index - lower;

                return weight > 0.5
                    ? (1 - weight) * table_[lower] + weight * table_[upper]
                    : weight * table_[lower] + (1 - weight) * table_[upper];
            };

            // modulo for a floating point number.
            auto modulo_add_increment = [this](float const& phase) {
                return (phase + increment_) >= tableSize
                    ? (phase + increment_ - tableSize)
                    : (phase + increment_);
            };

            for (size_t i = 0; i < samples; i++) {
                // get the interpolated value between two table indices
                buffer[0][i] = buffer[1][i] = interpol(phase_);

                // increment the pointer to the table, modulo TableSize
                phase_ = modulo_add_increment(phase_);
            }
        }

    protected:
        virtual void gen_table(table& table_) = 0;
        float frequency_;

    private:
        table table_;      ///< the waveform
        float phase_{ 0 }; ///< position in the waveform
        //float frequency_{ 440.0f }; ///< Current frequency of this oscillator
        unsigned sampling_rate_{ 44100 }; ///< How many samples per second?
        float increment_{ 1.0f }; ///< Stepsize when retrieving values from the table
    };

    //
    // Implementations of the interpolating table lookup-oscillator
    // 

    using Base = InterpolatingTableLookupOscillator<4096, float>;


    // A simple Sine oscillator
    class Sine : public Base {
    protected:

        // Generate one period of a sine wave with min/max-amplitude -1/1
        virtual void gen_table(Base::table& table) override {
            const auto two_pi = 6.28318530718f;
            auto const scale = two_pi / table.size();

            for (size_t i = 0; i < table.size(); i++) {
                table[i] = 2.0f * std::sin(i * static_cast<Base::audioSignalType>(scale)) - 1.0f;
            }
        }
    };


    // A simple Sawtooth oscillator
    class Saw : public Base {
    protected:
        
        // Generate one period of a sawtooth - increasing values from -1 to 1
        virtual void gen_table(Base::table& table) override {
            auto const factor = 2.0 / static_cast<double>(table.size());

            for (size_t i = 0; i < table.size(); i++) {
                table[i] = static_cast<Base::audioSignalType>(-1.0 + (i * factor));
            }
        }
    };

    // A simple Square Wave oscillator
    class Square : public Base {
    private:
        const float ratio = 0.5f;
    protected:

        // Generate one period of a square wave with set ratio
        virtual void gen_table(Base::table& table) override {
            auto const jump = table.size() * ratio;
            for (size_t i = 0; i < table.size(); i++) {
                table[i] = (i < jump ? -1.0f : 1.0f);
            }
        }
    };
}
 