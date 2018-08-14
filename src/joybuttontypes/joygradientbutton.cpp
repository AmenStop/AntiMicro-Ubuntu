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

#include "joygradientbutton.h"

#include "messagehandler.h"
#include "setjoystick.h"
#include "event.h"

#include <cmath>

#include <QDebug>

JoyGradientButton::JoyGradientButton(int index, int originset, SetJoystick *parentSet, QObject *parent) :
    JoyButton(index, originset, parentSet, parent)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);
}

/**
 * @brief Activate a turbo event on a button.
 */
void JoyGradientButton::turboEvent()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    if (getTurboMode() == NormalTurbo)
    {
        JoyButton::turboEvent();
    }
    else if ((getTurboMode() == GradientTurbo) || (getTurboMode() == PulseTurbo))
    {
        double diff = fabs(getMouseDistanceFromDeadZone() - lastDistance);

        #ifndef QT_DEBUG_NO_OUTPUT
        qDebug() << "DIFF: " << QString::number(diff);
        #endif

        bool changeState = false;

        if (!turboTimer.isActive() && !isButtonPressed)
        {
            changeState = true;
        }
        else if ((getTurboMode() == GradientTurbo) && (diff > 0) &&
                 (getMouseDistanceFromDeadZone() >= 1.0))
        {
            if (isKeyPressed)
            {
                changeState = false;
                if (!turboTimer.isActive() || (turboTimer.interval() != 5))
                {
                    turboTimer.start(5);
                }

                turboHold.restart();
                lastDistance = 1.0;
            }
            else
            {
                changeState = true;
            }
        }

        else if (turboHold.isNull() || (lastDistance == 0.0) || (turboHold.elapsed() > tempTurboInterval))
        {
            changeState = true;
        }
        else if (diff >= 0.1)
        {
            int tempInterval2 = 0;

            if (isKeyPressed)
            {
                if (getTurboMode() == GradientTurbo)
                {
                    tempInterval2 = static_cast<int>(floor((getMouseDistanceFromDeadZone() * turboInterval) + 0.5));
                }
                else
                {
                    tempInterval2 = static_cast<int>(floor((turboInterval * 0.5) + 0.5));
                }
            }
            else
            {
                if (getTurboMode() == GradientTurbo)
                {
                    tempInterval2 = static_cast<int>(floor(((1 - getMouseDistanceFromDeadZone()) * turboInterval) + 0.5));
                }
                else
                {
                    double distance = getMouseDistanceFromDeadZone();
                    if (distance > 0.0)
                    {
                        tempInterval2 = static_cast<int>(floor(((turboInterval / getMouseDistanceFromDeadZone()) * 0.5) + 0.5));
                    }
                    else
                    {
                        tempInterval2 = 0;
                    }
                }
            }

            if (turboHold.elapsed() < tempInterval2)
            {
                // Still some valid time left. Continue current action with
                // remaining time left.
                tempTurboInterval = tempInterval2 - turboHold.elapsed();
                int timerInterval = qMin(tempTurboInterval, 5);
                if (!turboTimer.isActive() || (turboTimer.interval() != timerInterval))
                {
                    turboTimer.start(timerInterval);
                }

                turboHold.restart();
                changeState = false;
                lastDistance = getMouseDistanceFromDeadZone();

                #ifndef QT_DEBUG_NO_OUTPUT
                qDebug() << "diff tmpTurbo press: " << QString::number(tempTurboInterval);
                qDebug() << "diff timer press: " << QString::number(timerInterval);
                #endif
            }
            else
            {
                changeState = true;

                #ifndef QT_DEBUG_NO_OUTPUT
                qDebug() << "YOU GOT CHANGE";
                #endif
            }
        }

        if (changeState)
        {
            if (!isKeyPressed)
            {
                if (!isButtonPressedQueue.isEmpty())
                {
                    ignoreSetQueue.clear();
                    isButtonPressedQueue.clear();

                    ignoreSetQueue.enqueue(false);
                    isButtonPressedQueue.enqueue(isButtonPressed);
                }

                createDeskEvent();

                isKeyPressed = true;
                if (turboTimer.isActive())
                {
                    if (getTurboMode() == GradientTurbo)
                    {
                        tempTurboInterval = static_cast<int>(floor((getMouseDistanceFromDeadZone() * turboInterval) + 0.5));
                    }
                    else
                    {
                        tempTurboInterval = static_cast<int>(floor((turboInterval * 0.5) + 0.5));
                    }

                    int timerInterval = qMin(tempTurboInterval, 5);

                    #ifndef QT_DEBUG_NO_OUTPUT
                    qDebug() << "tmpTurbo press: " << QString::number(tempTurboInterval);
                    qDebug() << "timer press: " << QString::number(timerInterval);
                    #endif

                    if (turboTimer.interval() != timerInterval)
                    {
                        turboTimer.start(timerInterval);
                    }

                    turboHold.restart();
                }
            }
            else
            {
                if (!isButtonPressedQueue.isEmpty())
                {
                    ignoreSetQueue.enqueue(false);
                    isButtonPressedQueue.enqueue(!isButtonPressed);
                }

                releaseDeskEvent();

                isKeyPressed = false;
                if (turboTimer.isActive())
                {
                    if (getTurboMode() == GradientTurbo)
                    {
                        tempTurboInterval = static_cast<int>(floor(((1 - getMouseDistanceFromDeadZone()) * turboInterval) + 0.5));
                    }
                    else
                    {
                        double distance = getMouseDistanceFromDeadZone();
                        if (distance > 0.0)
                        {
                            tempTurboInterval = static_cast<int>(floor(((turboInterval / getMouseDistanceFromDeadZone()) * 0.5) + 0.5));
                        }
                        else
                        {
                            tempTurboInterval = 0;
                        }
                    }

                    int timerInterval = qMin(tempTurboInterval, 5);

                    #ifndef QT_DEBUG_NO_OUTPUT
                    qDebug() << "tmpTurbo release: " << QString::number(tempTurboInterval);
                    qDebug() << "timer release: " << QString::number(timerInterval);
                    #endif

                    if (turboTimer.interval() != timerInterval)
                    {
                        turboTimer.start(timerInterval);
                    }

                    turboHold.restart();
                }

            }

            lastDistance = getMouseDistanceFromDeadZone();
        }
    }
}


void JoyGradientButton::wheelEventVertical()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    JoyButtonSlot *buttonslot = nullptr;
    bool activateEvent = false;

    int tempInterval = 0;
    double diff = fabs(getMouseDistanceFromDeadZone() - lastWheelVerticalDistance);
    int oldInterval = 0;

    if (wheelSpeedY != 0)
    {
        if (lastWheelVerticalDistance > 0.0)
        {
            oldInterval = 1000 / wheelSpeedY / lastWheelVerticalDistance;
        }
        else
        {
            oldInterval = 1000 / wheelSpeedY / 0.01;
        }
    }

    if (currentWheelVerticalEvent != nullptr)
    {
        buttonslot = currentWheelVerticalEvent;
        activateEvent = true;
    }

    if (!activateEvent)
    {
        if (!mouseWheelVerticalEventTimer.isActive())
        {
            activateEvent = true;
        }
        else if (wheelVerticalTime.elapsed() > oldInterval)
        {
            activateEvent = true;
        }
        else if ((diff >= 0.1) && (wheelSpeedY != 0))
        {
            double distance = getMouseDistanceFromDeadZone();
            if (distance > 0.0)
            {
                tempInterval = 1000 / wheelSpeedY / static_cast<int>(distance);
            }
            else
            {
                tempInterval = 0;
            }

            if (wheelVerticalTime.elapsed() < tempInterval)
            {
                // Still some valid time left. Continue current action with
                // remaining time left.
                tempInterval = tempInterval - wheelVerticalTime.elapsed();
                tempInterval = qMin(tempInterval, 5);
                if (!mouseWheelVerticalEventTimer.isActive() || (mouseWheelVerticalEventTimer.interval() != tempInterval))
                {
                    mouseWheelVerticalEventTimer.start(tempInterval);
                }
            }
            else
            {
                // Elapsed time is greater than new interval. Change state.
                activateEvent = true;
            }
        }
    }

    if ((buttonslot != nullptr) && (wheelSpeedY != 0))
    {
        bool isActive = getActiveSlots().contains(buttonslot);
        if (isActive && activateEvent)
        {
            sendevent(buttonslot, true);
            sendevent(buttonslot, false);
            mouseWheelVerticalEventQueue.enqueue(buttonslot);
            double distance = getMouseDistanceFromDeadZone();
            if (distance > 0.0)
            {
                tempInterval = 1000 / wheelSpeedY / static_cast<int>(distance);
            }
            else
            {
                tempInterval = 0;
            }

            tempInterval = qMin(tempInterval, 5);
            if (!mouseWheelVerticalEventTimer.isActive() || (mouseWheelVerticalEventTimer.interval() != tempInterval))
            {
                mouseWheelVerticalEventTimer.start(tempInterval);
            }
        }
        else if (!isActive)
        {
            mouseWheelVerticalEventTimer.stop();
        }
    }
    else if (!mouseWheelVerticalEventQueue.isEmpty() && (wheelSpeedY != 0))
    {
        QQueue<JoyButtonSlot*> tempQueue;
        while (!mouseWheelVerticalEventQueue.isEmpty())
        {
            buttonslot = mouseWheelVerticalEventQueue.dequeue();
            bool isActive = getActiveSlots().contains(buttonslot);
            if (isActive && activateEvent)
            {
                sendevent(buttonslot, true);
                sendevent(buttonslot, false);
                tempQueue.enqueue(buttonslot);
            }
            else if (isActive)
            {
                tempQueue.enqueue(buttonslot);
            }
        }

        if (!tempQueue.isEmpty())
        {
            mouseWheelVerticalEventQueue = tempQueue;
            double distance = getMouseDistanceFromDeadZone();
            if (distance > 0.0)
            {
                tempInterval = 1000 / wheelSpeedY / static_cast<int>(distance);
            }
            else
            {
                tempInterval = 0;
            }

            tempInterval = qMin(tempInterval, 5);
            if (!mouseWheelVerticalEventTimer.isActive() || (mouseWheelVerticalEventTimer.interval() != tempInterval))
            {
                mouseWheelVerticalEventTimer.start(tempInterval);
            }
        }
        else
        {
            mouseWheelVerticalEventTimer.stop();
        }
    }
    else
    {
        mouseWheelVerticalEventTimer.stop();
    }

    if (activateEvent)
    {
        wheelVerticalTime.restart();
        lastWheelVerticalDistance = getMouseDistanceFromDeadZone();
    }
}

void JoyGradientButton::wheelEventHorizontal()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    JoyButtonSlot *buttonslot = nullptr;
    bool activateEvent = false;

    int tempInterval = 0;
    double diff = fabs(getMouseDistanceFromDeadZone() - lastWheelHorizontalDistance);
    int oldInterval = 0;

    if (wheelSpeedX != 0)
    {
        if (lastWheelHorizontalDistance > 0.0)
        {
            oldInterval = 1000 / wheelSpeedX / lastWheelHorizontalDistance;
        }
        else
        {
            oldInterval = 1000 / wheelSpeedX / 0.01;
        }
    }

    if (currentWheelHorizontalEvent != nullptr)
    {
        buttonslot = currentWheelHorizontalEvent;
        activateEvent = true;
    }

    if (!activateEvent)
    {
        if (!mouseWheelHorizontalEventTimer.isActive())
        {
            activateEvent = true;
        }
        else if (wheelHorizontalTime.elapsed() > oldInterval)
        {
            activateEvent = true;
        }
        else if ((diff >= 0.1) && (wheelSpeedX != 0))
        {
            double distance = getMouseDistanceFromDeadZone();
            if (distance > 0.0)
            {
                tempInterval = 1000 / wheelSpeedX / static_cast<int>(distance);
            }
            else
            {
                tempInterval = 0;
            }

            if (wheelHorizontalTime.elapsed() < tempInterval)
            {
                // Still some valid time left. Continue current action with
                // remaining time left.
                tempInterval = tempInterval - wheelHorizontalTime.elapsed();
                tempInterval = qMin(tempInterval, 5);
                if (!mouseWheelHorizontalEventTimer.isActive() || (mouseWheelHorizontalEventTimer.interval() != tempInterval))
                {
                    mouseWheelHorizontalEventTimer.start(tempInterval);
                }
            }
            else
            {
                // Elapsed time is greater than new interval. Change state.
                activateEvent = true;
            }
        }
    }

    if ((buttonslot != nullptr) && (wheelSpeedX != 0))
    {
        bool isActive = getActiveSlots().contains(buttonslot);
        if (isActive && activateEvent)
        {
            sendevent(buttonslot, true);
            sendevent(buttonslot, false);
            mouseWheelHorizontalEventQueue.enqueue(buttonslot);
            double distance = getMouseDistanceFromDeadZone();
            if (distance > 0.0)
            {
                tempInterval = 1000 / wheelSpeedX / static_cast<int>(distance);
            }
            else
            {
                tempInterval = 0;
            }

            tempInterval = qMin(tempInterval, 5);

            if (!mouseWheelHorizontalEventTimer.isActive() || (mouseWheelVerticalEventTimer.interval() != tempInterval))
            {
                mouseWheelHorizontalEventTimer.start(tempInterval);
            }
        }
        else if (!isActive)
        {
            mouseWheelHorizontalEventTimer.stop();
        }
    }
    else if (!mouseWheelHorizontalEventQueue.isEmpty() && (wheelSpeedX != 0))
    {
        QQueue<JoyButtonSlot*> tempQueue;
        while (!mouseWheelHorizontalEventQueue.isEmpty())
        {
            buttonslot = mouseWheelHorizontalEventQueue.dequeue();
            bool isActive = getActiveSlots().contains(buttonslot);
            if (isActive)
            {
                sendevent(buttonslot, true);
                sendevent(buttonslot, false);
                tempQueue.enqueue(buttonslot);
            }
        }

        if (!tempQueue.isEmpty())
        {
            mouseWheelHorizontalEventQueue = tempQueue;
            double distance = getMouseDistanceFromDeadZone();
            if (distance > 0.0)
            {
                tempInterval = 1000 / wheelSpeedX / static_cast<int>(distance);
            }
            else
            {
                tempInterval = 0;
            }

            tempInterval = qMin(tempInterval, 5);

            if (!mouseWheelHorizontalEventTimer.isActive() || (mouseWheelVerticalEventTimer.interval() != tempInterval))
            {
                mouseWheelHorizontalEventTimer.start(tempInterval);
            }
        }
        else
        {
            mouseWheelHorizontalEventTimer.stop();
        }
    }
    else
    {
        mouseWheelHorizontalEventTimer.stop();
    }

    if (activateEvent)
    {
        wheelHorizontalTime.restart();
        lastWheelHorizontalDistance = getMouseDistanceFromDeadZone();
    }
}
