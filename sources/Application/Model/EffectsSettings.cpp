#include "EffectsSettings.h"
#include "Application/Effects/SendBuses.h"
#include "Application/Utils/fixed.h"
#include <string.h>

EffectsSettings::EffectsSettings():Persistent("EFFECTS") {

	modDepth_ = new Variable("mod depth", EFP_MOD_DEPTH, 68, 255);
	Insert(modDepth_);
	modRate_ = new Variable("mod rate", EFP_MOD_RATE, 28, 255);
	Insert(modRate_);
	modWidth_ = new Variable("mod width", EFP_MOD_WIDTH, 255, 255);
	Insert(modWidth_);
	modRevSend_ = new Variable("mod reverb send", EFP_MOD_REVSEND, 0, 255);
	Insert(modRevSend_);

	delTimeL_ = new Variable("delay time L", EFP_DEL_TIMEL, 53, 255);
	Insert(delTimeL_);
	delTimeR_ = new Variable("delay time R", EFP_DEL_TIMER, 53, 255);
	Insert(delTimeR_);
	delFeedback_ = new Variable("delay feedback", EFP_DEL_FEEDBACK, 94, 255);
	Insert(delFeedback_);
	delWidth_ = new Variable("delay width", EFP_DEL_WIDTH, 255, 255);
	Insert(delWidth_);
	delRevSend_ = new Variable("delay reverb send", EFP_DEL_REVSEND, 0, 255);
	Insert(delRevSend_);

	revRoomSize_ = new Variable("reverb room size", EFP_REV_ROOMSIZE, 128, 255);
	Insert(revRoomSize_);
	revDecay_ = new Variable("reverb decay", EFP_REV_DECAY, 128, 255);
	Insert(revDecay_);
	revModDepth_ = new Variable("reverb mod depth", EFP_REV_MODDEPTH, 0, 255);
	Insert(revModDepth_);
	revModRate_ = new Variable("reverb mod rate", EFP_REV_MODRATE, 20, 255);
	Insert(revModRate_);
	revWidth_ = new Variable("reverb width", EFP_REV_WIDTH, 255, 255);
	Insert(revWidth_);
}

EffectsSettings::~EffectsSettings() {
}

void EffectsSettings::Clear() {
	modDepth_->SetInt(68);
	modRate_->SetInt(28);
	modWidth_->SetInt(255);
	modRevSend_->SetInt(0);

	delTimeL_->SetInt(53);
	delTimeR_->SetInt(53);
	delFeedback_->SetInt(94);
	delWidth_->SetInt(255);
	delRevSend_->SetInt(0);

	revRoomSize_->SetInt(128);
	revDecay_->SetInt(128);
	revModDepth_->SetInt(0);
	revModRate_->SetInt(20);
	revWidth_->SetInt(255);
}

void EffectsSettings::Apply() {
	ChorusEffect *chorus = ChorusBus::GetInstance()->GetEffect();
	chorus->SetDepthMs((modDepth_->GetInt() / 255.0f) * 15.0f);
	chorus->SetRateHz(0.05f + (modRate_->GetInt() / 255.0f) * 4.95f);
	chorus->SetWidth(fl2fp(modWidth_->GetInt() / 255.0f));

	DelayEffect *delay = DelayBus::GetInstance()->GetEffect();
	delay->SetTimeMsL(1 + (int)((delTimeL_->GetInt() / 255.0f) * 1199.0f));
	delay->SetTimeMsR(1 + (int)((delTimeR_->GetInt() / 255.0f) * 1199.0f));
	delay->SetFeedback(fl2fp((delFeedback_->GetInt() / 255.0f) * 0.95f));
	delay->SetWidth(fl2fp(delWidth_->GetInt() / 255.0f));

	ReverbEffect *reverb = ReverbBus::GetInstance()->GetEffect();
	reverb->SetRoomSize((int)((revRoomSize_->GetInt() / 255.0f) * 100.0f));
	reverb->SetDamping((int)((revDecay_->GetInt() / 255.0f) * 100.0f));
	reverb->SetModDepth((int)((revModDepth_->GetInt() / 255.0f) * 100.0f));
	reverb->SetModRateHz(0.05f + (revModRate_->GetInt() / 255.0f) * 1.95f);
	reverb->SetWidth(fl2fp(revWidth_->GetInt() / 255.0f));

	// cross-feed: chorus and delay can each send some of their own
	// processed output into the reverb bus, in addition to their own
	// direct output to master (M8-style "reverb send" per effect)
	ChorusBus::GetInstance()->SetDownstream(ReverbBus::GetInstance(), fl2fp(modRevSend_->GetInt() / 255.0f));
	DelayBus::GetInstance()->SetDownstream(ReverbBus::GetInstance(), fl2fp(delRevSend_->GetInt() / 255.0f));
}

void EffectsSettings::SaveContent(TiXmlNode *node) {
	IteratorPtr<Variable> it(GetIterator());
	for (it->Begin(); !it->IsDone(); it->Next()) {
		Variable &v = it->CurrentItem();
		TiXmlElement param("PARAM");
		param.SetAttribute("NAME", v.GetName());
		param.SetAttribute("VALUE", v.GetString());
		node->InsertEndChild(param);
	}
}

void EffectsSettings::RestoreContent(TiXmlElement *element) {
	TiXmlElement *current = element->FirstChildElement();
	while (current) {
		const char *name = current->Attribute("NAME");
		const char *value = current->Attribute("VALUE");
		if (name && value) {
			Variable *v = FindVariable(name);
			if (v) v->SetString(value);
		}
		current = current->NextSiblingElement();
	}
	Apply();
}
