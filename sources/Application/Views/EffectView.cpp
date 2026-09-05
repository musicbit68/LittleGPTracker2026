#include "EffectView.h"
#include "BaseClasses/UIStaticField.h"
#include "BaseClasses/UIIntVarField.h"
#include "Application/Model/EffectsSettings.h"
#include "Application/Player/Player.h"
#include "Application/AppWindow.h"

EffectView::EffectView(GUIWindow &w, ViewData *data):FieldView(w, data) {

	EffectsSettings *fx = EffectsSettings::GetInstance();
	GUIPoint position = GetAnchor();
	Variable *v;
	UIIntVarField *f1;
	UIStaticField *sf;

	// Single column - FieldView's up/down navigation only compares vertical
	// position, so a real multi-column layout can't be navigated correctly
	// with up/down; everything lives in one list, top to bottom.

	sf = new UIStaticField(position, "MODFX (chorus)");
	T_SimpleList<UIField>::Insert(sf);

	position._y += 1;
	v = fx->FindVariable(EFP_MOD_DEPTH);
	f1 = new UIIntVarField(position, *v, "mod depth:    %2.2X", 0, 0xFF, 1, 0x10);
	T_SimpleList<UIField>::Insert(f1);
	f1->SetFocus();

	position._y += 1;
	v = fx->FindVariable(EFP_MOD_RATE);
	f1 = new UIIntVarField(position, *v, "mod rate:     %2.2X", 0, 0xFF, 1, 0x10);
	T_SimpleList<UIField>::Insert(f1);

	position._y += 1;
	v = fx->FindVariable(EFP_MOD_WIDTH);
	f1 = new UIIntVarField(position, *v, "mod width:    %2.2X", 0, 0xFF, 1, 0x10);
	T_SimpleList<UIField>::Insert(f1);

	position._y += 1;
	v = fx->FindVariable(EFP_MOD_REVSEND);
	f1 = new UIIntVarField(position, *v, "mod->rvb:     %2.2X", 0, 0xFF, 1, 0x10);
	T_SimpleList<UIField>::Insert(f1);

	position._y += 2;
	sf = new UIStaticField(position, "DELAY");
	T_SimpleList<UIField>::Insert(sf);

	position._y += 1;
	v = fx->FindVariable(EFP_DEL_TIMEL);
	f1 = new UIIntVarField(position, *v, "dly time L:   %2.2X", 0, 0xFF, 1, 0x10);
	T_SimpleList<UIField>::Insert(f1);

	position._y += 1;
	v = fx->FindVariable(EFP_DEL_TIMER);
	f1 = new UIIntVarField(position, *v, "dly time R:   %2.2X", 0, 0xFF, 1, 0x10);
	T_SimpleList<UIField>::Insert(f1);

	position._y += 1;
	v = fx->FindVariable(EFP_DEL_FEEDBACK);
	f1 = new UIIntVarField(position, *v, "dly feedback: %2.2X", 0, 0xFF, 1, 0x10);
	T_SimpleList<UIField>::Insert(f1);

	position._y += 1;
	v = fx->FindVariable(EFP_DEL_WIDTH);
	f1 = new UIIntVarField(position, *v, "dly width:    %2.2X", 0, 0xFF, 1, 0x10);
	T_SimpleList<UIField>::Insert(f1);

	position._y += 1;
	v = fx->FindVariable(EFP_DEL_REVSEND);
	f1 = new UIIntVarField(position, *v, "dly->rvb:     %2.2X", 0, 0xFF, 1, 0x10);
	T_SimpleList<UIField>::Insert(f1);

	position._y += 2;
	sf = new UIStaticField(position, "REVERB");
	T_SimpleList<UIField>::Insert(sf);

	position._y += 1;
	v = fx->FindVariable(EFP_REV_ROOMSIZE);
	f1 = new UIIntVarField(position, *v, "room size:    %2.2X", 0, 0xFF, 1, 0x10);
	T_SimpleList<UIField>::Insert(f1);

	position._y += 1;
	v = fx->FindVariable(EFP_REV_DECAY);
	f1 = new UIIntVarField(position, *v, "decay:        %2.2X", 0, 0xFF, 1, 0x10);
	T_SimpleList<UIField>::Insert(f1);

	position._y += 1;
	v = fx->FindVariable(EFP_REV_MODDEPTH);
	f1 = new UIIntVarField(position, *v, "mod depth:    %2.2X", 0, 0xFF, 1, 0x10);
	T_SimpleList<UIField>::Insert(f1);

	position._y += 1;
	v = fx->FindVariable(EFP_REV_MODRATE);
	f1 = new UIIntVarField(position, *v, "mod rate:     %2.2X", 0, 0xFF, 1, 0x10);
	T_SimpleList<UIField>::Insert(f1);

	position._y += 1;
	v = fx->FindVariable(EFP_REV_WIDTH);
	f1 = new UIIntVarField(position, *v, "width:        %2.2X", 0, 0xFF, 1, 0x10);
	T_SimpleList<UIField>::Insert(f1);
}

EffectView::~EffectView() {
}

void EffectView::ProcessButtonMask(unsigned short mask, bool pressed) {

	if (!pressed) return;

	isDirty_ = false;

	FieldView::ProcessButtonMask(mask);

	if (mask & EPBM_R) {
		if (mask & EPBM_UP) {
			// R + UP = back to the Mixer page
			ViewType vt = VT_MIXER;
			ViewEvent ve(VET_SWITCH_VIEW, &vt);
			SetChanged();
			NotifyObservers(&ve);
		} else if (mask & EPBM_START) {
			onStop();
		}
	} else if (mask & EPBM_START) {
		onStart();
	}
}

void EffectView::onStart() {
	Player *player = Player::GetInstance();
	unsigned char from = viewData_->songX_;
	unsigned char to = from;
	player->OnStartButton(PM_SONG, from, false, to);
}

void EffectView::onStop() {
	Player *player = Player::GetInstance();
	unsigned char from = viewData_->songX_;
	unsigned char to = from;
	player->OnStartButton(PM_SONG, from, true, to);
}

void EffectView::DrawView() {

	Clear();
	View::EnableNotification();

	GUITextProperties props;
	GUIPoint pos = GetTitlePosition();

	SetColor(CD_NORMAL);
	DrawString(pos._x, pos._y, "Effect Settings", props);

	// Push any edited values straight to the DSP - cheap, so just do it
	// unconditionally every draw rather than trying to track exactly which
	// field changed.
	EffectsSettings::GetInstance()->Apply();

	FieldView::Redraw();
	drawMap();
}

void EffectView::OnFocus() {
}
