#ifndef _EFFECT_VIEW_H_
#define _EFFECT_VIEW_H_

#include "BaseClasses/FieldView.h"
#include "ViewData.h"

class EffectView: public FieldView {
public:
	EffectView(GUIWindow &w, ViewData *data);
	virtual ~EffectView();

	virtual void ProcessButtonMask(unsigned short mask, bool pressed);
	virtual void DrawView();
	virtual void OnPlayerUpdate(PlayerEventType, unsigned int) {};
	virtual void OnFocus();
};

#endif
