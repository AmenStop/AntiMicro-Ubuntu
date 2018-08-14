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

#ifndef VDPAD_H
#define VDPAD_H

#include "joydpad.h"
#include "joybutton.h"

class VDPad : public JoyDPad
{
    Q_OBJECT

public:
    explicit VDPad(int index, int originset, SetJoystick *parentSet, QObject *parent = nullptr);
    explicit VDPad(JoyButton *upButton, JoyButton *downButton, JoyButton *leftButton, JoyButton *rightButton,
                   int index, int originset, SetJoystick *parentSet, QObject *parent = nullptr);
    ~VDPad();

    void joyEvent(bool pressed, bool ignoresets=false);
    void addVButton(JoyDPadButton::JoyDPadDirections direction, JoyButton *button);
    void removeVButton(JoyDPadButton::JoyDPadDirections direction);
    void removeVButton(JoyButton *button);
    JoyButton* getVButton(JoyDPadButton::JoyDPadDirections direction);
    bool isEmpty();
    virtual QString getName(bool forceFullFormat=false, bool displayName=false);
    virtual QString getXmlName();

    void queueJoyEvent(bool ignoresets=false);
    bool hasPendingEvent();
    void clearPendingEvent();

    JoyButton *getUpButton() const;
    JoyButton *getDownButton() const;
    JoyButton *getLeftButton() const;
    JoyButton *getRightButton() const;
    bool getPendingVDPadEvent() const;

    static const QString xmlName;

public slots:
    void activatePendingEvent();

private:
    JoyButton *upButton;
    JoyButton *downButton;
    JoyButton *leftButton;
    JoyButton *rightButton;
    bool pendingVDPadEvent;

};

#endif // VDPAD_H
