#ifndef _DELAY_EFFECT_H_
#define _DELAY_EFFECT_H_

#include "Application/Utils/fixed.h"

// Simple stereo feedback delay, used as a stage in the FX send bus chain.
// Operates in place on interleaved stereo buffers of type "fixed".

#define DELAY_MAX_MS 1200
#define DELAY_SAMPLE_RATE 44100
// +1 guards against rounding when converting ms -> samples at the max time
#define DELAY_MAX_SAMPLES (((DELAY_MAX_MS * DELAY_SAMPLE_RATE) / 1000) + 1)

class DelayEffect {
public:
	DelayEffect();
	~DelayEffect();

	void Init();
	void Process(fixed *buffer, int samplecount);

	// timeMs: 1..DELAY_MAX_MS - separate per channel so ping-pong/offset
	// delay times are possible
	void SetTimeMsL(int timeMs);
	void SetTimeMsR(int timeMs);
	// feedback: fixed 0..~0.95 (i2fp(0)..close to i2fp(1))
	void SetFeedback(fixed feedback);
	// mix: fixed 0 (dry only) .. i2fp(1) (fully wet)
	void SetMix(fixed mix);
	// width: fixed 0 (mono sum) .. i2fp(1) (normal stereo). Blends L/R via
	// a simple mid/side scale on the wet output.
	void SetWidth(fixed width);

private:
	fixed *bufferL_;
	fixed *bufferR_;
	int writePos_;
	int delaySamplesL_;
	int delaySamplesR_;
	fixed feedback_;
	fixed mix_;
	fixed width_;
};

#endif
