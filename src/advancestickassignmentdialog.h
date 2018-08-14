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

#ifndef ADVANCESTICKASSIGNMENTDIALOG_H
#define ADVANCESTICKASSIGNMENTDIALOG_H

#include <QDialog>

class Joystick;
class JoyAxisButton;
class JoyButton;


namespace Ui {
class AdvanceStickAssignmentDialog;
}

class AdvanceStickAssignmentDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit AdvanceStickAssignmentDialog(Joystick *joystick, QWidget *parent = nullptr);
    ~AdvanceStickAssignmentDialog();

    Joystick *getJoystick() const;
       

signals:
    void stickConfigurationChanged();
    void vdpadConfigurationChanged();

private slots:
    void refreshStickConfiguration();
    void refreshVDPadConfiguration();

    void checkForAxisAssignmentStickOne(QWidget* comboBox);
    void checkForAxisAssignmentStickTwo(QWidget* comboBox);

    void changeStateStickOneWidgets(bool enabled);
    void changeStateStickTwoWidgets(bool enabled);
    void changeStateVDPadWidgets(bool enabled);

    void populateDPadComboBoxes();

    void changeVDPadUpButton(int index);
    void changeVDPadDownButton(int index);
    void changeVDPadLeftButton(int index);
    void changeVDPadRightButton(int index);

    void disableVDPadComboBoxes();
    void enableVDPadComboBoxes();

    void openQuickAssignDialogStick1();
    void openQuickAssignDialogStick2();

    void openAssignVDPadUp();
    void openAssignVDPadDown();
    void openAssignVDPadLeft();
    void openAssignVDPadRight();

    void quickAssignVDPadUp(JoyAxisButton* joyaxisbtn);
    void quickAssignVDPadUpBtn(JoyButton* joybtn);
    void quickAssignVDPadDown(JoyAxisButton* axbtn);
    void quickAssignVDPadDownJbtn(JoyButton* axbtn);
    void quickAssignVDPadLeft(JoyAxisButton* joyaxisbtn);
    void quickAssignVDPadLeftJbtn(JoyButton* joybtn);
    void quickAssignVDPadRight(JoyAxisButton* joyaxisbtn);
    void quickAssignVDPadRightJbtn(JoyButton* joybtn);

    void reenableButtonEvents();

private:
    Ui::AdvanceStickAssignmentDialog *ui;

    Joystick *joystick;

};

#endif // ADVANCESTICKASSIGNMENTDIALOG_H
