#ifndef _REVERB_EFFECT_H_
#define _REVERB_EFFECT_H_

#include "Application/Utils/fixed.h"

// A simplified Freeverb-style reverb (parallel damped comb filters followed
// by series allpass diffusers), stereo, fixed point. Used as a stage in the
// FX send bus chain. Operates in place on interleaved stereo buffers.

#define REVERB_COMB_COUNT 4
#define REVERB_ALLPASS_COUNT 2

class ReverbEffect {
public:
	ReverbEffect();
	~ReverbEffect();

	void Init();
	void Process(fixed *buffer, int samplecount);

	// roomSize: 0..100, maps to comb feedback (bigger = longer decay)
	void SetRoomSize(int roomSize);
	// damping: 0..100, maps to high-frequency damping in the comb feedback path
	void SetDamping(int damping);
	// mix: fixed 0 (dry only) .. i2fp(1) (fully wet)
	void SetMix(fixed mix);
	// width: fixed 0 (mono sum) .. i2fp(1) (normal stereo)
	void SetWidth(fixed width);
	// modDepth: 0..100, adds a subtle animated/lush layer on top of the
	// static tail (short of true pitch-shifted "shimmer", but in that
	// direction - a gently chorused reverb tail)
	void SetModDepth(int modDepth);
	// modRateHz: speed of that modulation, roughly 0.05..2 Hz
	void SetModRateHz(float modRateHz);

private:
	struct Comb {
		fixed *buffer;
		int size;
		int index;
		fixed feedback;
		fixed damp1;
		fixed damp2;
		fixed filterStore;
	};
	struct Allpass {
		fixed *buffer;
		int size;
		int index;
		fixed feedback;
	};

	Comb combL_[REVERB_COMB_COUNT];
	Comb combR_[REVERB_COMB_COUNT];
	Allpass allpassL_[REVERB_ALLPASS_COUNT];
	Allpass allpassR_[REVERB_ALLPASS_COUNT];

	fixed mix_;
	fixed width_;
	float modDepthSamples_;
	float modPhase_[REVERB_COMB_COUNT];
	float modIncrement_;

	void initComb(Comb &c, int size);
	void initAllpass(Allpass &a, int size);
	fixed processComb(Comb &c, fixed input);
	fixed processAllpass(Allpass &a, fixed input);
	fixed readModulatedTap(Comb &c, float phase);
};

#endif
