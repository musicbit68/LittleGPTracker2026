#include "TextEntryDialog.h"
#include "Application/Utils/KeyboardLayout.h"
#include <string.h>

#define TEXT_ENTRY_DIALOG_WIDTH 20
#define TEXT_ENTRY_DIALOG_HEIGHT 15

TextEntryDialog::TextEntryDialog(View &view, const char *title, const char *initialName)
    : ModalView(view), title_(title), initialName_(initialName) {}

TextEntryDialog::~TextEntryDialog() {}

void TextEntryDialog::moveCursor(int direction) {
    int newPos = currentChar_ + direction;
    if (newPos >= 0 && newPos < TEXT_ENTRY_MAX_LENGTH) {
        currentChar_ = newPos;
        findCharacterInKeyboard(name_[currentChar_], keyboardRow_, keyboardCol_);
    }
}

void TextEntryDialog::OnFocus() {
    memset(name_, ' ', TEXT_ENTRY_MAX_LENGTH + 1);
    int len = initialName_.length();
    if (len > TEXT_ENTRY_MAX_LENGTH) len = TEXT_ENTRY_MAX_LENGTH;
    strncpy(name_, initialName_.c_str(), len);
    currentChar_ = 0;
    keyboardRow_ = 2;
    keyboardCol_ = 0;
    findCharacterInKeyboard(name_[currentChar_], keyboardRow_, keyboardCol_);
}

void TextEntryDialog::OnPlayerUpdate(PlayerEventType, unsigned int) {};

void TextEntryDialog::DrawView() {

    SetWindow(TEXT_ENTRY_DIALOG_WIDTH, TEXT_ENTRY_DIALOG_HEIGHT);

    GUITextProperties props;
    SetColor(CD_NORMAL);

    DrawString(2, 1, title_.c_str(), props);

    int x = (TEXT_ENTRY_DIALOG_WIDTH - TEXT_ENTRY_MAX_LENGTH) / 2;

    char buffer[2];
    buffer[1] = 0;
    for (int i = 0; i < TEXT_ENTRY_MAX_LENGTH; i++) {
        props.invert_ = (i == currentChar_);
        buffer[0] = name_[i];
        DrawString(x + i, 3, buffer, props);
    }

    SetColor(CD_NORMAL);
    for (int row = 0; row < KEYBOARD_ROWS; row++) {
        const char *rowStr = keyboardLayout[row];
        int len = strlen(rowStr);
        int startX = (TEXT_ENTRY_DIALOG_WIDTH - len) / 2;

        if (row == SPACE_ROW) {
            props.invert_ = (row == keyboardRow_ && isInSpaceSection(keyboardCol_));
            DrawString(startX, 5 + row, "[_]", props);

            props.invert_ = (row == keyboardRow_ && isInBackSection(keyboardCol_));
            DrawString(startX + 4, 5 + row, "<-", props);

            props.invert_ = (row == keyboardRow_ && isInDoneSection(keyboardCol_));
            DrawString(startX + 8, 5 + row, "OK", props);
        } else {
            for (int col = 0; col < len; col++) {
                props.invert_ = (row == keyboardRow_ && col == keyboardCol_);
                buffer[0] = rowStr[col];
                DrawString(startX + col, 5 + row, buffer, props);
            }
        }
    }

    props.invert_ = false;
    DrawString(2, 14, "A=input, B=erase", props);
    DrawString(2, 15, "L, R=move cursor", props);

    View::EnableNotification();
}

void TextEntryDialog::ProcessButtonMask(unsigned short mask, bool pressed) {

    if (!pressed) return;

    if (mask == EPBM_A) {
        char ch = getKeyAtPosition(keyboardRow_, keyboardCol_);
        if (ch == '\b') {
            if (currentChar_ > 0) {
                currentChar_--;
                name_[currentChar_] = ' ';
            }
        } else if (ch == '\r') {
            EndModal(1);
            return;
        } else if (ch != '\0') {
            name_[currentChar_] = ch;
            if (currentChar_ < TEXT_ENTRY_MAX_LENGTH - 1) {
                currentChar_++;
                findCharacterInKeyboard(name_[currentChar_], keyboardRow_, keyboardCol_);
            }
        }
        isDirty_ = true;
        return;
    } else if (mask == EPBM_B) {
        if (currentChar_ > 0) {
            currentChar_--;
            name_[currentChar_] = ' ';
            isDirty_ = true;
        }
        return;
    } else if (mask == EPBM_L) {
        moveCursor(-1);
        isDirty_ = true;
        return;
    } else if (mask == EPBM_R) {
        moveCursor(1);
        isDirty_ = true;
        return;
    } else if (mask == EPBM_UP) {
        keyboardRow_ = (keyboardRow_ - 1 + KEYBOARD_ROWS) % KEYBOARD_ROWS;
        clampKeyboardColumn(keyboardRow_, keyboardCol_);
        isDirty_ = true;
        return;
    } else if (mask == EPBM_DOWN) {
        keyboardRow_ = (keyboardRow_ + 1) % KEYBOARD_ROWS;
        clampKeyboardColumn(keyboardRow_, keyboardCol_);
        isDirty_ = true;
        return;
    } else if (mask == EPBM_LEFT) {
        cycleKeyboardColumn(keyboardRow_, -1, keyboardCol_);
        isDirty_ = true;
        return;
    } else if (mask == EPBM_RIGHT) {
        cycleKeyboardColumn(keyboardRow_, 1, keyboardCol_);
        isDirty_ = true;
        return;
    } else if (mask == EPBM_START) {
        EndModal(1);
        return;
    }
};

std::string TextEntryDialog::GetName() {
    std::string result(name_, TEXT_ENTRY_MAX_LENGTH);
    // trim trailing spaces
    size_t end = result.find_last_not_of(' ');
    if (end == std::string::npos) return "";
    return result.substr(0, end + 1);
}
