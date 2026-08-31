#include "ReverbEffect.h"
#include "System/System/System.h"
#include <string.h>
#include <math.h>

#define REVERB_PI 3.14159265358979323846f

// A small precomputed sine table replaces runtime sinf() calls in the
// modulation layer. sinf() is comfortably the most expensive single
// operation in this file - on ARM without fast hardware trig it can cost
// far more than the actual fixed-point comb/allpass math - and it would
// otherwise run up to 8 times per sample (4 combs x 2 channels). 512
// entries is far more resolution than a slow 0.05-2Hz LFO needs; the
// quantization error is inaudible.
#define REVERB_SINE_TABLE_SIZE 512
static float sSineTable[REVERB_SINE_TABLE_SIZE];
static bool sSineTableReady = false;

static void ensureSineTable() {
	if (sSineTableReady) return;
	for (int i = 0; i < REVERB_SINE_TABLE_SIZE; i++) {
		sSineTable[i] = sinf((2.0f * REVERB_PI * (float)i) / (float)REVERB_SINE_TABLE_SIZE);
	}
	sSineTableReady = true;
}

// phase is expected in [0, 2*PI)
static inline float fastSin(float phase) {
	int idx = (int)(phase * (REVERB_SINE_TABLE_SIZE / (2.0f * REVERB_PI)));
	idx &= (REVERB_SINE_TABLE_SIZE - 1);
	return sSineTable[idx];
}

// Classic Freeverb tuning values, in samples @ 44100Hz.
// Right channel uses a small offset from left for stereo width.
static const int kCombTuningL[REVERB_COMB_COUNT] = {1116, 1188, 1277, 1356};
static const int kCombTuningR[REVERB_COMB_COUNT] = {1139, 1211, 1300, 1379};
static const int kAllpassTuningL[REVERB_ALLPASS_COUNT] = {556, 225};
static const int kAllpassTuningR[REVERB_ALLPASS_COUNT] = {579, 244};

#define REVERB_ALLPASS_FEEDBACK 0.5f

ReverbEffect::ReverbEffect() {
	ensureSineTable();
	int i;
	for (i = 0; i < REVERB_COMB_COUNT; i++) {
		combL_[i].buffer = 0;
		combR_[i].buffer = 0;
		// stagger each comb's mod phase for a smoother, less "wobbly" blend
		modPhase_[i] = (REVERB_PI * 2.0f * i) / (float)REVERB_COMB_COUNT;
	}
	for (i = 0; i < REVERB_ALLPASS_COUNT; i++) {
		allpassL_[i].buffer = 0;
		allpassR_[i].buffer = 0;
	}
	mix_ = fl2fp(0.35f);
	width_ = i2fp(1);
	modDepthSamples_ = 0.0f;
	modIncrement_ = 2.0f * REVERB_PI * 0.2f / 44100.0f; // default 0.2Hz
}

ReverbEffect::~ReverbEffect() {
	int i;
	for (i = 0; i < REVERB_COMB_COUNT; i++) {
		SAFE_FREE(combL_[i].buffer);
		SAFE_FREE(combR_[i].buffer);
	}
	for (i = 0; i < REVERB_ALLPASS_COUNT; i++) {
		SAFE_FREE(allpassL_[i].buffer);
		SAFE_FREE(allpassR_[i].buffer);
	}
}

void ReverbEffect::initComb(Comb &c, int size) {
	SAFE_FREE(c.buffer);
	c.buffer = (fixed *)SYS_MALLOC(size * sizeof(fixed));
	memset(c.buffer, 0, size * sizeof(fixed));
	c.size = size;
	c.index = 0;
	c.feedback = fl2fp(0.84f);
	c.damp1 = fl2fp(0.2f);
	c.damp2 = fl2fp(0.8f);
	c.filterStore = i2fp(0);
}

void ReverbEffect::initAllpass(Allpass &a, int size) {
	SAFE_FREE(a.buffer);
	a.buffer = (fixed *)SYS_MALLOC(size * sizeof(fixed));
	memset(a.buffer, 0, size * sizeof(fixed));
	a.size = size;
	a.index = 0;
	a.feedback = fl2fp(REVERB_ALLPASS_FEEDBACK);
}

void ReverbEffect::Init() {
	int i;
	for (i = 0; i < REVERB_COMB_COUNT; i++) {
		initComb(combL_[i], kCombTuningL[i]);
		initComb(combR_[i], kCombTuningR[i]);
	}
	for (i = 0; i < REVERB_ALLPASS_COUNT; i++) {
		initAllpass(allpassL_[i], kAllpassTuningL[i]);
		initAllpass(allpassR_[i], kAllpassTuningR[i]);
	}
}

void ReverbEffect::SetRoomSize(int roomSize) {
	if (roomSize < 0) roomSize = 0;
	if (roomSize > 100) roomSize = 100;
	float fb = 0.7f + ((float)roomSize / 100.0f) * 0.28f; // 0.70 .. 0.98
	fixed fbFp = fl2fp(fb);
	int i;
	for (i = 0; i < REVERB_COMB_COUNT; i++) {
		combL_[i].feedback = fbFp;
		combR_[i].feedback = fbFp;
	}
}

void ReverbEffect::SetDamping(int damping) {
	if (damping < 0) damping = 0;
	if (damping > 100) damping = 100;
	float d1 = ((float)damping / 100.0f) * 0.4f;
	fixed damp1Fp = fl2fp(d1);
	fixed damp2Fp = fl2fp(1.0f - d1);
	int i;
	for (i = 0; i < REVERB_COMB_COUNT; i++) {
		combL_[i].damp1 = damp1Fp;
		combL_[i].damp2 = damp2Fp;
		combR_[i].damp1 = damp1Fp;
		combR_[i].damp2 = damp2Fp;
	}
}

void ReverbEffect::SetMix(fixed mix) {
	mix_ = mix;
}

void ReverbEffect::SetWidth(fixed width) {
	width_ = width;
}

void ReverbEffect::SetModDepth(int modDepth) {
	if (modDepth < 0) modDepth = 0;
	if (modDepth > 100) modDepth = 100;
	// up to ~6 samples of wander - enough to sound lush without obvious pitch wobble
	modDepthSamples_ = ((float)modDepth / 100.0f) * 6.0f;
}

void ReverbEffect::SetModRateHz(float modRateHz) {
	if (modRateHz < 0.01f) modRateHz = 0.01f;
	modIncrement_ = 2.0f * REVERB_PI * modRateHz / 44100.0f;
}

fixed ReverbEffect::processComb(Comb &c, fixed input) {
	fixed output = c.buffer[c.index];

	// one-pole damping filter in the feedback path
	c.filterStore = fp_add(fp_mul(output, c.damp2), fp_mul(c.filterStore, c.damp1));

	c.buffer[c.index] = fp_add(input, fp_mul(c.filterStore, c.feedback));

	c.index++;
	if (c.index >= c.size) c.index = 0;

	return output;
}

fixed ReverbEffect::processAllpass(Allpass &a, fixed input) {
	fixed bufOut = a.buffer[a.index];
	fixed output = fp_sub(bufOut, input);
	a.buffer[a.index] = fp_add(input, fp_mul(bufOut, a.feedback));

	a.index++;
	if (a.index >= a.size) a.index = 0;

	return output;
}

// Reads a linearly-interpolated tap a little ahead of/behind the comb's
// current write position, for the animated "lush" modulation layer. Does
// NOT touch the comb's feedback path - purely an additional, separate tap
// blended into the output, so it can't destabilize the core reverb tail.
fixed ReverbEffect::readModulatedTap(Comb &c, float phase) {
	float offset = modDepthSamples_ * fastSin(phase);
	float readPosF = (float)c.index + offset;
	while (readPosF < 0.0f) readPosF += (float)c.size;
	while (readPosF >= (float)c.size) readPosF -= (float)c.size;

	int idx0 = (int)readPosF;
	int idx1 = idx0 + 1;
	if (idx1 >= c.size) idx1 = 0;

	float frac = readPosF - (float)idx0;
	fixed s0 = c.buffer[idx0];
	fixed s1 = c.buffer[idx1];
	fixed fracFp = fl2fp(frac);
	return fp_add(s0, fp_mul(fp_sub(s1, s0), fracFp));
}

void ReverbEffect::Process(fixed *buffer, int samplecount) {
	if (!combL_[0].buffer) return;

	fixed dryGain = fp_sub(i2fp(1), mix_);
	fixed half = fl2fp(0.5f);
	// scale feeding the combs down so the parallel sum doesn't overload
	fixed combInputGain = fl2fp(1.0f / (float)REVERB_COMB_COUNT);

	for (int n = 0; n < samplecount; n++) {
		int idx = n * 2;
		fixed inL = buffer[idx];
		fixed inR = buffer[idx + 1];

		fixed combInL = fp_mul(inL, combInputGain);
		fixed combInR = fp_mul(inR, combInputGain);

		fixed wetL = i2fp(0);
		fixed wetR = i2fp(0);
		int i;
		for (i = 0; i < REVERB_COMB_COUNT; i++) {
			// modulated tap read BEFORE processComb overwrites this comb's
			// current write index, layered on top of the standard tap
			fixed modTapL = i2fp(0);
			fixed modTapR = i2fp(0);
			if (modDepthSamples_ > 0.0f) {
				modTapL = readModulatedTap(combL_[i], modPhase_[i]);
				modTapR = readModulatedTap(combR_[i], modPhase_[i]);
			}

			wetL = fp_add(wetL, fp_add(processComb(combL_[i], combInL), modTapL));
			wetR = fp_add(wetR, fp_add(processComb(combR_[i], combInR), modTapR));
		}

		for (i = 0; i < REVERB_ALLPASS_COUNT; i++) {
			wetL = processAllpass(allpassL_[i], wetL);
			wetR = processAllpass(allpassR_[i], wetR);
		}

		if (modDepthSamples_ > 0.0f) {
			for (i = 0; i < REVERB_COMB_COUNT; i++) {
				modPhase_[i] += modIncrement_;
				if (modPhase_[i] > 2.0f * REVERB_PI) modPhase_[i] -= 2.0f * REVERB_PI;
			}
		}

		// stereo width: blend the wet signal via mid/side before final mix
		fixed mid = fp_mul(fp_add(wetL, wetR), half);
		fixed side = fp_mul(fp_sub(wetL, wetR), half);
		side = fp_mul(side, width_);
		wetL = fp_add(mid, side);
		wetR = fp_sub(mid, side);

		buffer[idx] = fp_add(fp_mul(inL, dryGain), fp_mul(wetL, mix_));
		buffer[idx + 1] = fp_add(fp_mul(inR, dryGain), fp_mul(wetR, mix_));
	}
}
