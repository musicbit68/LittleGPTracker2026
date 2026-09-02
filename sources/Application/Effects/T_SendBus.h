#ifndef _T_SEND_BUS_H_
#define _T_SEND_BUS_H_

#include "Foundation/T_Singleton.h"
#include "Services/Audio/AudioModule.h"
#include "System/System/System.h"
#include <string.h>

// If a send bus has received no meaningful input for this long, its DSP
// is skipped entirely (silence is output instead) until new input arrives.
// This saves real CPU/battery on buses that aren't currently in use, and
// as a side effect puts a hard ceiling on how long any decay tail (e.g.
// reverb) can audibly persist after the last note - by the time a bus has
// been silent this long, any well-behaved effect's natural tail should
// already have decayed well past audibility anyway.
#define SEND_BUS_IDLE_TIMEOUT_SAMPLES (10 * 44100)
// Below this, a sample is treated as silence (not "no signal at all", to
// tolerate tiny rounding/dither noise without ever failing to go idle).
#define SEND_BUS_SILENCE_EPSILON 2

// Common interface so one send bus can feed a scaled copy of its processed
// output into another (e.g. chorus/delay feeding reverb), without the
// buses needing to know each other's concrete effect type.
class I_SendTarget {
public:
	virtual ~I_SendTarget() {};
	virtual void Accumulate(fixed *buffer, int samplecount, fixed sendLevel) = 0;
};

// A single-effect send bus: instruments accumulate a scaled copy of their
// own output here (via Accumulate), and once per audio callback Render()
// picks up whatever was accumulated, runs it through one effect, and hands
// it back to be summed into the master bus. Unlike a serial FX chain, each
// T_SendBus<Effect> is independent and runs in parallel with the others -
// they are all summed into master separately (see PlayerMixer::Init, which
// inserts each into its own MixBus).
//
// EffectClass just needs: Init(), Process(fixed*, int).
template <class EffectClass>
class T_SendBus: public T_Singleton<T_SendBus<EffectClass> >, public AudioModule, public I_SendTarget {
public:
	T_SendBus() {
		accumulator_ = 0;
		accumulatorSize_ = 0;
		enabled_ = true;
		downstream_ = 0;
		downstreamSend_ = i2fp(0);
		idleSamples_ = 0;
		wasIdle_ = false;
	}

	virtual ~T_SendBus() {
		SAFE_FREE(accumulator_);
	}

	void Init() {
		effect_.Init();
	}

	void SetEnabled(bool enabled) { enabled_ = enabled; }

	// Optionally feed a scaled copy of this bus's PROCESSED (post-effect)
	// output into another send bus - e.g. chorus/delay feeding reverb.
	void SetDownstream(I_SendTarget *downstream, fixed sendLevel) {
		downstream_ = downstream;
		downstreamSend_ = sendLevel;
	}

	virtual void Accumulate(fixed *buffer, int samplecount, fixed sendLevel) {
		if (sendLevel <= 0) return;

		ensureAccumulatorSize(samplecount);

		for (int i = 0; i < samplecount * 2; i++) {
			accumulator_[i] = fp_add(accumulator_[i], fp_mul(buffer[i], sendLevel));
		}
	}

	virtual bool Render(fixed *buffer, int samplecount) {
		ensureAccumulatorSize(samplecount);

		bool hasSignal = false;
		for (int i = 0; i < samplecount * 2; i++) {
			if (accumulator_[i] > SEND_BUS_SILENCE_EPSILON || accumulator_[i] < -SEND_BUS_SILENCE_EPSILON) {
				hasSignal = true;
				break;
			}
		}

		if (hasSignal) {
			idleSamples_ = 0;
		} else if (idleSamples_ <= SEND_BUS_IDLE_TIMEOUT_SAMPLES) {
			// only keep counting up to the timeout - no need to let this
			// grow unbounded while genuinely idle for a long time
			idleSamples_ += samplecount;
		}

		memcpy(buffer, accumulator_, samplecount * 2 * sizeof(fixed));
		// consumed - clear the accumulator for the next audio callback
		memset(accumulator_, 0, samplecount * 2 * sizeof(fixed));

		bool longIdle = (idleSamples_ > SEND_BUS_IDLE_TIMEOUT_SAMPLES);
		bool active = enabled_ && !longIdle;

		if (active) {
			effect_.Process(buffer, samplecount);

			// just came back from being idle - fade in over this buffer
			// instead of an instant jump, to avoid a click at the seam
			if (wasIdle_) {
				for (int n = 0; n < samplecount; n++) {
					fixed gain = fl2fp((float)n / (float)samplecount);
					buffer[n * 2] = fp_mul(buffer[n * 2], gain);
					buffer[n * 2 + 1] = fp_mul(buffer[n * 2 + 1], gain);
				}
			}
			wasIdle_ = false;
		} else {
			if (!wasIdle_ && enabled_) {
				// just became idle THIS callback - fade the last
				// processed buffer out rather than hard-cutting to
				// silence, then genuinely skip the DSP from here on
				effect_.Process(buffer, samplecount);
				for (int n = 0; n < samplecount; n++) {
					fixed gain = fl2fp(1.0f - (float)n / (float)samplecount);
					buffer[n * 2] = fp_mul(buffer[n * 2], gain);
					buffer[n * 2 + 1] = fp_mul(buffer[n * 2 + 1], gain);
				}
			} else {
				// either disabled, or already idle - genuinely skip the DSP
				memset(buffer, 0, samplecount * 2 * sizeof(fixed));
			}
			wasIdle_ = true;
		}

		if (downstream_ && downstreamSend_ > 0 && active) {
			downstream_->Accumulate(buffer, samplecount, downstreamSend_);
		}

		return true;
	}

	EffectClass *GetEffect() { return &effect_; }

private:
	fixed *accumulator_;
	int accumulatorSize_; // in samples (frames), not stereo pairs
	bool enabled_;
	EffectClass effect_;

	I_SendTarget *downstream_;
	fixed downstreamSend_;

	int idleSamples_;
	bool wasIdle_;

	void ensureAccumulatorSize(int samplecount) {
		if (accumulator_ && accumulatorSize_ >= samplecount) return;

		fixed *newBuf = (fixed *)SYS_MALLOC(samplecount * 2 * sizeof(fixed));
		memset(newBuf, 0, samplecount * 2 * sizeof(fixed));

		if (accumulator_) {
			memcpy(newBuf, accumulator_, accumulatorSize_ * 2 * sizeof(fixed));
			SYS_FREE(accumulator_);
		}

		accumulator_ = newBuf;
		accumulatorSize_ = samplecount;
	}
};

#endif
