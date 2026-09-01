#ifndef _TEXT_ENTRY_DIALOG_H_
#define _TEXT_ENTRY_DIALOG_H_

#include "Application/Views/BaseClasses/ModalView.h"
#include <string>

#define TEXT_ENTRY_MAX_LENGTH 12

// A small generic on-screen-keyboard text entry dialog, for naming things
// that don't already have their own naming mechanism (e.g. Tables,
// Grooves). Reuses the same keyboard layout/navigation helpers as
// NewProjectDialog, but without that dialog's project-specific concerns
// (random name generation, checking path uniqueness, etc). Always starts
// directly in keyboard-editing mode - there's no separate "mode" to select.
class TextEntryDialog: public ModalView {
public:
	// title is shown at the top of the dialog. initialName pre-fills the
	// editable text (truncated to TEXT_ENTRY_MAX_LENGTH if longer).
	TextEntryDialog(View &view, const char *title, const char *initialName);
	virtual ~TextEntryDialog();

	virtual void DrawView();
	virtual void OnPlayerUpdate(PlayerEventType, unsigned int currentTick);
	virtual void OnFocus();
	virtual void ProcessButtonMask(unsigned short mask, bool pressed);

	// Trimmed of trailing spaces. Call after EndModal(1) (confirmed).
	std::string GetName();

private:
	void moveCursor(int direction);

	std::string title_;
	std::string initialName_;
	char name_[TEXT_ENTRY_MAX_LENGTH + 1];
	int currentChar_;
	int keyboardRow_;
	int keyboardCol_;
};

#endif
