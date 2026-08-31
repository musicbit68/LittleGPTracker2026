#include "ChorusEffect.h"
#include "System/System/System.h"
#include <math.h>
#include <string.h>

#define CHORUS_PI 3.14159265358979323846f

// Same rationale as ReverbEffect's sine table: this LFO drives sinf() on
// EVERY sample, unconditionally (chorus is always-on by default), so a
// precomputed table meaningfully cuts CPU cost with inaudible quantization
// error for a slow (0.05-5Hz) LFO.
#define CHORUS_SINE_TABLE_SIZE 512
static float sChorusSineTable[CHORUS_SINE_TABLE_SIZE];
static bool sChorusSineTableReady = false;

static void ensureChorusSineTable() {
	if (sChorusSineTableReady) return;
	for (int i = 0; i < CHORUS_SINE_TABLE_SIZE; i++) {
		sChorusSineTable[i] = sinf((2.0f * CHORUS_PI * (float)i) / (float)CHORUS_SINE_TABLE_SIZE);
	}
	sChorusSineTableReady = true;
}

// phase is expected in [0, 2*PI)
static inline float chorusFastSin(float phase) {
	int idx = (int)(phase * (CHORUS_SINE_TABLE_SIZE / (2.0f * CHORUS_PI)));
	idx &= (CHORUS_SINE_TABLE_SIZE - 1);
	return sChorusSineTable[idx];
}

ChorusEffect::ChorusEffect() {
	ensureChorusSineTable();
	bufferL_ = 0;
	bufferR_ = 0;
	writePos_ = 0;
	lfoPhaseL_ = 0.0f;
	lfoPhaseR_ = CHORUS_PI * 0.5f; // quadrature offset for stereo width
	lfoIncrement_ = 2.0f * CHORUS_PI * 0.6f / (float)CHORUS_SAMPLE_RATE; // 0.6Hz default
	depthSamples_ = ((float)CHORUS_SAMPLE_RATE * 4.0f) / 1000.0f;   // 4ms default depth
	centerSamples_ = ((float)CHORUS_SAMPLE_RATE * 18.0f) / 1000.0f; // 18ms default center (raised to give the depth range more room to swing before crossing zero)
	mix_ = fl2fp(0.5f);
	width_ = i2fp(1);
}

ChorusEffect::~ChorusEffect() {
	SAFE_FREE(bufferL_);
	SAFE_FREE(bufferR_);
}

void ChorusEffect::Init() {
	SAFE_FREE(bufferL_);
	SAFE_FREE(bufferR_);
	bufferL_ = (fixed *)SYS_MALLOC(CHORUS_BUFFER_SAMPLES * sizeof(fixed));
	bufferR_ = (fixed *)SYS_MALLOC(CHORUS_BUFFER_SAMPLES * sizeof(fixed));
	memset(bufferL_, 0, CHORUS_BUFFER_SAMPLES * sizeof(fixed));
	memset(bufferR_, 0, CHORUS_BUFFER_SAMPLES * sizeof(fixed));
	writePos_ = 0;
}

void ChorusEffect::SetRateHz(float rateHz) {
	if (rateHz < 0.01f) rateHz = 0.01f;
	lfoIncrement_ = 2.0f * CHORUS_PI * rateHz / (float)CHORUS_SAMPLE_RATE;
}

void ChorusEffect::SetDepthMs(float depthMs) {
	if (depthMs < 0.0f) depthMs = 0.0f;
	float maxDepth = (float)CHORUS_MAX_DELAY_MS * 0.5f;
	if (depthMs > maxDepth) depthMs = maxDepth;
	depthSamples_ = ((float)CHORUS_SAMPLE_RATE * depthMs) / 1000.0f;
}

void ChorusEffect::SetMix(fixed mix) {
	mix_ = mix;
}

void ChorusEffect::SetWidth(fixed width) {
	width_ = width;
}

// Linear-interpolated fractional read from a circular delay buffer
static fixed readInterpolated(fixed *buf, int writePos, float delaySamples) {
	float readPosF = (float)writePos - delaySamples;
	while (readPosF < 0.0f) readPosF += (float)CHORUS_BUFFER_SAMPLES;

	int idx0 = (int)readPosF;
	int idx1 = idx0 + 1;
	if (idx1 >= CHORUS_BUFFER_SAMPLES) idx1 = 0;
	if (idx0 >= CHORUS_BUFFER_SAMPLES) idx0 = 0;

	float frac = readPosF - (float)idx0;
	fixed s0 = buf[idx0];
	fixed s1 = buf[idx1];
	fixed fracFp = fl2fp(frac);
	return fp_add(s0, fp_mul(fp_sub(s1, s0), fracFp));
}

void ChorusEffect::Process(fixed *buffer, int samplecount) {
	if (!bufferL_ || !bufferR_) return;

	fixed dryGain = fp_sub(i2fp(1), mix_);

	for (int n = 0; n < samplecount; n++) {
		int idx = n * 2;
		fixed inL = buffer[idx];
		fixed inR = buffer[idx + 1];

		bufferL_[writePos_] = inL;
		bufferR_[writePos_] = inR;

		float delayL = centerSamples_ + depthSamples_ * chorusFastSin(lfoPhaseL_);
		float delayR = centerSamples_ + depthSamples_ * chorusFastSin(lfoPhaseR_);

		fixed wetL = readInterpolated(bufferL_, writePos_, delayL);
		fixed wetR = readInterpolated(bufferR_, writePos_, delayR);

		// stereo width: blend the two modulated taps via mid/side
		fixed half = fl2fp(0.5f);
		fixed mid = fp_mul(fp_add(wetL, wetR), half);
		fixed side = fp_mul(fp_sub(wetL, wetR), half);
		side = fp_mul(side, width_);
		wetL = fp_add(mid, side);
		wetR = fp_sub(mid, side);

		buffer[idx] = fp_add(fp_mul(inL, dryGain), fp_mul(wetL, mix_));
		buffer[idx + 1] = fp_add(fp_mul(inR, dryGain), fp_mul(wetR, mix_));

		lfoPhaseL_ += lfoIncrement_;
		if (lfoPhaseL_ > 2.0f * CHORUS_PI) lfoPhaseL_ -= 2.0f * CHORUS_PI;
		lfoPhaseR_ += lfoIncrement_;
		if (lfoPhaseR_ > 2.0f * CHORUS_PI) lfoPhaseR_ -= 2.0f * CHORUS_PI;

		writePos_++;
		if (writePos_ >= CHORUS_BUFFER_SAMPLES) writePos_ = 0;
	}
}
