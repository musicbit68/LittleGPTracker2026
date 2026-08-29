#ifndef _T_SEND_BUS_H_
#define _T_SEND_BUS_H_

#include "Foundation/T_Singleton.h"
#include "Services/Audio/AudioModule.h"
#include "System/System/System.h"
#include <string.h>

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

		memcpy(buffer, accumulator_, samplecount * 2 * sizeof(fixed));
		// consumed - clear the accumulator for the next audio callback
		memset(accumulator_, 0, samplecount * 2 * sizeof(fixed));

		if (enabled_) effect_.Process(buffer, samplecount);

		if (downstream_ && downstreamSend_ > 0) {
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
