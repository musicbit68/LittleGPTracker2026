#ifndef _CHORUS_EFFECT_H_
#define _CHORUS_EFFECT_H_

#include "Application/Utils/fixed.h"

// Modulated short delay (LFO-driven) stereo chorus, used as a stage in the
// FX send bus chain. Operates in place on interleaved stereo buffers.

#define CHORUS_SAMPLE_RATE 44100
#define CHORUS_MAX_DELAY_MS 45
#define CHORUS_BUFFER_SAMPLES (((CHORUS_MAX_DELAY_MS * CHORUS_SAMPLE_RATE) / 1000) + 1)

class ChorusEffect {
public:
	ChorusEffect();
	~ChorusEffect();

	void Init();
	void Process(fixed *buffer, int samplecount);

	// rateHz: LFO speed, roughly 0.05 .. 5 Hz
	void SetRateHz(float rateHz);
	// depthMs: modulation depth in milliseconds, 0 .. CHORUS_MAX_DELAY_MS/2
	void SetDepthMs(float depthMs);
	// mix: fixed 0 (dry only) .. i2fp(1) (fully wet)
	void SetMix(fixed mix);
	// width: fixed 0 (mono sum) .. i2fp(1) (normal stereo)
	void SetWidth(fixed width);

private:
	fixed *bufferL_;
	fixed *bufferR_;
	int writePos_;

	float lfoPhaseL_;
	float lfoPhaseR_;
	float lfoIncrement_;
	float depthSamples_;
	float centerSamples_;

	fixed mix_;
	fixed width_;
};

#endif
