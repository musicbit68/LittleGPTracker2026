#include "DelayEffect.h"
#include "System/System/System.h"
#include <string.h>

DelayEffect::DelayEffect() {
	bufferL_ = 0;
	bufferR_ = 0;
	writePos_ = 0;
	delaySamplesL_ = (DELAY_SAMPLE_RATE * 250) / 1000; // default 250ms
	delaySamplesR_ = (DELAY_SAMPLE_RATE * 250) / 1000;
	feedback_ = fl2fp(0.35f);
	mix_ = fl2fp(0.35f);
	width_ = i2fp(1);
}

DelayEffect::~DelayEffect() {
	SAFE_FREE(bufferL_);
	SAFE_FREE(bufferR_);
}

void DelayEffect::Init() {
	SAFE_FREE(bufferL_);
	SAFE_FREE(bufferR_);
	bufferL_ = (fixed *)SYS_MALLOC(DELAY_MAX_SAMPLES * sizeof(fixed));
	bufferR_ = (fixed *)SYS_MALLOC(DELAY_MAX_SAMPLES * sizeof(fixed));
	memset(bufferL_, 0, DELAY_MAX_SAMPLES * sizeof(fixed));
	memset(bufferR_, 0, DELAY_MAX_SAMPLES * sizeof(fixed));
	writePos_ = 0;
}

void DelayEffect::SetTimeMsL(int timeMs) {
	if (timeMs < 1) timeMs = 1;
	if (timeMs > DELAY_MAX_MS) timeMs = DELAY_MAX_MS;
	delaySamplesL_ = (DELAY_SAMPLE_RATE * timeMs) / 1000;
}

void DelayEffect::SetTimeMsR(int timeMs) {
	if (timeMs < 1) timeMs = 1;
	if (timeMs > DELAY_MAX_MS) timeMs = DELAY_MAX_MS;
	delaySamplesR_ = (DELAY_SAMPLE_RATE * timeMs) / 1000;
}

void DelayEffect::SetFeedback(fixed feedback) {
	feedback_ = feedback;
}

void DelayEffect::SetMix(fixed mix) {
	mix_ = mix;
}

void DelayEffect::SetWidth(fixed width) {
	width_ = width;
}

void DelayEffect::Process(fixed *buffer, int samplecount) {
	if (!bufferL_ || !bufferR_) return;

	fixed dryGain = fp_sub(i2fp(1), mix_);
	fixed half = fl2fp(0.5f);

	for (int n = 0; n < samplecount; n++) {
		int idx = n * 2;
		fixed inL = buffer[idx];
		fixed inR = buffer[idx + 1];

		int readPosL = writePos_ - delaySamplesL_;
		while (readPosL < 0) readPosL += DELAY_MAX_SAMPLES;
		int readPosR = writePos_ - delaySamplesR_;
		while (readPosR < 0) readPosR += DELAY_MAX_SAMPLES;

		fixed delayedL = bufferL_[readPosL];
		fixed delayedR = bufferR_[readPosR];

		// write input + feedback of delayed signal into the delay line
		bufferL_[writePos_] = fp_add(inL, fp_mul(delayedL, feedback_));
		bufferR_[writePos_] = fp_add(inR, fp_mul(delayedR, feedback_));

		// stereo width: blend delayed L/R via a mid/side scale before mixing
		fixed mid = fp_mul(fp_add(delayedL, delayedR), half);
		fixed side = fp_mul(fp_sub(delayedL, delayedR), half);
		side = fp_mul(side, width_);
		fixed wetL = fp_add(mid, side);
		fixed wetR = fp_sub(mid, side);

		buffer[idx] = fp_add(fp_mul(inL, dryGain), fp_mul(wetL, mix_));
		buffer[idx + 1] = fp_add(fp_mul(inR, dryGain), fp_mul(wetR, mix_));

		writePos_++;
		if (writePos_ >= DELAY_MAX_SAMPLES) writePos_ = 0;
	}
}
