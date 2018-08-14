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

#include "joycontrolstickbutton.h"

#include "messagehandler.h"
#include "joycontrolstick.h"
#include "joycontrolstickmodifierbutton.h"
#include "setjoystick.h"
#include "event.h"

#include <cmath>

#include <QStringList>
#include <QDebug>


const QString JoyControlStickButton::xmlName = "stickbutton";

JoyControlStickButton::JoyControlStickButton(JoyControlStick *stick, int index, int originset, SetJoystick *parentSet, QObject *parent) :
    JoyGradientButton(index, originset, parentSet, parent)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    this->stick = stick;
}

JoyControlStickButton::JoyControlStickButton(JoyControlStick *stick, JoyStickDirectionsType::JoyStickDirections index, int originset, SetJoystick *parentSet, QObject *parent) :
    JoyGradientButton(static_cast<int>(index), originset, parentSet, parent)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    this->stick = stick;
}

QString JoyControlStickButton::getDirectionName() const
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    QString label = QString();
    if (index == static_cast<int>(JoyControlStick::StickUp))
    {
        label.append(trUtf8("Up"));
    }
    else if (index == static_cast<int>(JoyControlStick::StickDown))
    {
        label.append(trUtf8("Down"));
    }
    else if (index == static_cast<int>(JoyControlStick::StickLeft))
    {
        label.append(trUtf8("Left"));
    }
    else if (index == static_cast<int>(JoyControlStick::StickRight))
    {
        label.append(trUtf8("Right"));
    }
    else if (index == static_cast<int>(JoyControlStick::StickLeftUp))
    {
        label.append(trUtf8("Up")).append("+").append(trUtf8("Left"));
    }
    else if (index == static_cast<int>(JoyControlStick::StickLeftDown))
    {
        label.append(trUtf8("Down")).append("+").append(trUtf8("Left"));
    }
    else if (index == static_cast<int>(JoyControlStick::StickRightUp))
    {
        label.append(trUtf8("Up")).append("+").append(trUtf8("Right"));
    }
    else if (index == static_cast<int>(JoyControlStick::StickRightDown))
    {
        label.append(trUtf8("Down")).append("+").append(trUtf8("Right"));
    }

    return label;
}

QString JoyControlStickButton::getPartialName(bool forceFullFormat, bool displayNames)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    QString temp = stick->getPartialName(forceFullFormat, displayNames);

    temp.append(": ");

    if (!buttonName.isEmpty() && displayNames)
    {
        if (forceFullFormat)
        {
            temp.append(trUtf8("Button")).append(" ");
        }

        temp.append(buttonName);
    }
    else if (!defaultButtonName.isEmpty())
    {
        if (forceFullFormat)
        {
            temp.append(trUtf8("Button")).append(" ");
        }

        temp.append(defaultButtonName);
    }
    else
    {
        temp.append(trUtf8("Button")).append(" ");
        temp.append(getDirectionName());
    }
    return temp;
}

QString JoyControlStickButton::getXmlName()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    return this->xmlName;
}

/**
 * @brief Get the distance that an element is away from its assigned
 *     dead zone
 * @return Normalized distance away from dead zone
 */
double JoyControlStickButton::getDistanceFromDeadZone()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    return stick->calculateDirectionalDistance();
}

/**
 * @brief Get the distance factor that should be used for mouse movement
 * @return Distance factor that should be used for mouse movement
 */
double JoyControlStickButton::getMouseDistanceFromDeadZone()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    return stick->calculateMouseDirectionalDistance(this);
}

void JoyControlStickButton::setChangeSetCondition(SetChangeCondition condition, bool passive)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    SetChangeCondition oldCondition = setSelectionCondition;

    if ((condition != setSelectionCondition) && !passive)
    {
        if ((condition == SetChangeWhileHeld) || (condition == SetChangeTwoWay))
        {
            // Set new condition
            emit setAssignmentChanged(index, this->stick->getIndex(), setSelection, condition);
        }
        else if ((setSelectionCondition == SetChangeWhileHeld) || (setSelectionCondition == SetChangeTwoWay))
        {
            // Remove old condition
            emit setAssignmentChanged(index, this->stick->getIndex(), setSelection, SetChangeDisabled);
        }

        setSelectionCondition = condition;
    }
    else if (passive)
    {
        setSelectionCondition = condition;
    }

    if (setSelectionCondition == SetChangeDisabled)
    {
        setChangeSetSelection(-1);
    }

    if (setSelectionCondition != oldCondition)
    {
        buildActiveZoneSummaryString();
        emit propertyUpdated();
    }
}

int JoyControlStickButton::getRealJoyNumber()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    return index;
}

JoyStickDirectionsType::JoyStickDirections JoyControlStickButton::getDirection() const
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    return static_cast<JoyStickDirectionsType::JoyStickDirections>(index);
}

/**
 * @brief Set the turbo mode that the button should use
 * @param Mode that should be used
 */
void JoyControlStickButton::setTurboMode(TurboMode mode)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    if (isPartRealAxis())
    {
        currentTurboMode = mode;
    }
}

/**
 * @brief Check if button should be considered a part of a real controller
 *     axis. Needed for some dialogs so the program won't have to resort to
 *     type checking.
 * @return Status of being part of a real controller axis
 */
bool JoyControlStickButton::isPartRealAxis()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    return true;
}

double JoyControlStickButton::getLastAccelerationDistance()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    double temp = stick->calculateLastAccelerationButtonDistance(this);
    return temp;
}

double JoyControlStickButton::getAccelerationDistance()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    double temp = stick->calculateAccelerationDistance(this);
    return temp;
}

/**
 * @brief Generate a string that represents slots that will be activated or
 *     slots that are currently active if a button is pressed
 * @return String of currently applicable slots for a button
 */
QString JoyControlStickButton::getActiveZoneSummary()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    QList<JoyButtonSlot*> tempList;

    tempList.append(getActiveZoneList());
    QString temp = buildActiveZoneSummary(tempList);
    return temp;
}

QString JoyControlStickButton::getCalculatedActiveZoneSummary()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    JoyControlStickModifierButton *tempButton = stick->getModifierButton();
    QString temp = QString();
    QStringList stringlist = QStringList();

    if ((tempButton != nullptr) && tempButton->getButtonState() &&
        tempButton->hasActiveSlots() && getButtonState())
    {
        #ifndef QT_DEBUG_NO_OUTPUT
        qDebug() << "Calculated Active Zone Summary: " << tempButton->getCalculatedActiveZoneSummary();
        #endif

        stringlist.append(tempButton->getCalculatedActiveZoneSummary());
    }

    stringlist.append(JoyButton::getCalculatedActiveZoneSummary());
    temp = stringlist.join(", ");

    #ifndef QT_DEBUG_NO_OUTPUT
    qDebug() << "Returned joined zone: " << temp;
    #endif

    return temp;
}

double JoyControlStickButton::getLastMouseDistanceFromDeadZone()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    return stick->calculateLastMouseDirectionalDistance(this);
}

double JoyControlStickButton::getCurrentSpringDeadCircle()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    double result = (springDeadCircleMultiplier * 0.01);
    if ((index == static_cast<int>(JoyControlStick::StickLeft)) || (index == static_cast<int>(JoyControlStick::StickRight)))
    {
        result = stick->getSpringDeadCircleX() * (springDeadCircleMultiplier * 0.01);
    }
    else if ((index == static_cast<int>(JoyControlStick::StickUp)) || (index == static_cast<int>(JoyControlStick::StickDown)))
    {
        result = stick->getSpringDeadCircleY() * (springDeadCircleMultiplier * 0.01);
    }
    else if ((index == static_cast<int>(JoyControlStick::StickRightUp)) || (index == static_cast<int>(JoyControlStick::StickRightDown)) ||
             (index == static_cast<int>(JoyControlStick::StickLeftDown)) || (index == static_cast<int>(JoyControlStick::StickLeftUp)))
    {
        result = 0.0;
    }

    return result;
}


JoyControlStick* JoyControlStickButton::getStick() const {

    qInstallMessageHandler(MessageHandler::myMessageOutput);

    return stick;
}
