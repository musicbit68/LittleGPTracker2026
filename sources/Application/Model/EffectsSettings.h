#ifndef _EFFECTS_SETTINGS_H_
#define _EFFECTS_SETTINGS_H_

#include "Foundation/T_Singleton.h"
#include "Foundation/Variables/VariableContainer.h"
#include "Application/Persistency/Persistent.h"

// Parameter FourCCs for the three parallel FX send buses (chorus/delay/
// reverb). These are global effect settings, not tied to any instrument or
// track - one shared chorus, one shared delay, one shared reverb, matching
// how the sends themselves work (see SIP_SNDC/SNDD/SNDR on SampleInstrument).
#define EFP_MOD_DEPTH   MAKE_FOURCC('M','D','E','P')
#define EFP_MOD_RATE    MAKE_FOURCC('M','R','A','T')
#define EFP_MOD_WIDTH   MAKE_FOURCC('M','W','I','D')
#define EFP_MOD_REVSEND MAKE_FOURCC('M','R','V','S')

#define EFP_DEL_TIMEL   MAKE_FOURCC('D','T','M','L')
#define EFP_DEL_TIMER   MAKE_FOURCC('D','T','M','R')
#define EFP_DEL_FEEDBACK MAKE_FOURCC('D','F','B','K')
#define EFP_DEL_WIDTH   MAKE_FOURCC('D','W','I','D')
#define EFP_DEL_REVSEND MAKE_FOURCC('D','R','V','S')

#define EFP_REV_ROOMSIZE MAKE_FOURCC('R','R','M','S')
#define EFP_REV_DECAY   MAKE_FOURCC('R','D','C','Y')
#define EFP_REV_MODDEPTH MAKE_FOURCC('R','M','D','P')
#define EFP_REV_MODRATE MAKE_FOURCC('R','M','R','T')
#define EFP_REV_WIDTH   MAKE_FOURCC('R','W','I','D')

class EffectsSettings: public T_Singleton<EffectsSettings>, public VariableContainer, public Persistent {
public:
	EffectsSettings();
	~EffectsSettings();

	void Clear();

	// Applies the current parameter values to the actual DSP effect
	// instances / send buses. Called once at startup and whenever a
	// parameter changes (see EffectView).
	void Apply();

	virtual void SaveContent(TiXmlNode *node);
	virtual void RestoreContent(TiXmlElement *element);

private:
	Variable *modDepth_;
	Variable *modRate_;
	Variable *modWidth_;
	Variable *modRevSend_;

	Variable *delTimeL_;
	Variable *delTimeR_;
	Variable *delFeedback_;
	Variable *delWidth_;
	Variable *delRevSend_;

	Variable *revRoomSize_;
	Variable *revDecay_;
	Variable *revModDepth_;
	Variable *revModRate_;
	Variable *revWidth_;
};

#endif
