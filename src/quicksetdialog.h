/* antimicro Gamepad to KB+M event mapper
 * Copyright (C) 2015 Travis Nickles <nickles.travis@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef QUICKSETDIALOG_H
#define QUICKSETDIALOG_H

#include "joybuttonslot.h"

#include "uihelpers/buttoneditdialoghelper.h"
#include <QDialog>


class InputDevice;
class QWidget;
class JoyButton;
class JoyControlStickButton;
class JoyDPadButton;
class JoyAxisButton;

namespace Ui {
class QuickSetDialog;
}

class QuickSetDialog : public QDialog
{
    Q_OBJECT

public:

    explicit QuickSetDialog(InputDevice *joystick, QWidget *parent = nullptr);
    QuickSetDialog(InputDevice *joystick, ButtonEditDialogHelper* helper, const char* invokeString, int code, int alias, int index, JoyButtonSlot::JoySlotInputAction mode, bool withClear, bool withTrue, QWidget *parent = nullptr);
    ~QuickSetDialog();

    JoyButton* getLastPressedButton() const;
    InputDevice *getJoystick() const;
    QDialog *getCurrentButtonDialog() const;
    const char* getInvokeString() const;
    ButtonEditDialogHelper* getHelper() const;
    JoyButtonSlot::JoySlotInputAction getMode() const;


private slots:
    void showAxisButtonDialog(JoyAxisButton* joybtn);
    void showButtonDialog(JoyButton* joybtn);
    void showStickButtonDialog(JoyControlStickButton* joyctrlstickbtn);
    void showDPadButtonDialog(JoyDPadButton* joydpadbtn);
    void restoreButtonStates();

private:
    Ui::QuickSetDialog *ui;

    InputDevice *joystick;
    QDialog *currentButtonDialog;
    ButtonEditDialogHelper* helper;
    JoyButton* lastButton;

    const char* invokeString;

    int code;
    int alias;
    int index;

    JoyButtonSlot::JoySlotInputAction mode;

    bool withClear;
    bool withTrue;

};

#endif // QUICKSETDIALOG_H
