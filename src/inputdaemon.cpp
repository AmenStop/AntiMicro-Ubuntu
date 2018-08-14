﻿/* antimicro Gamepad to KB+M event mapper
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

#include "inputdaemon.h"

#include "messagehandler.h"
#include "logger.h"
#include "common.h"
#include "joystick.h"
#include "joydpad.h"
#include "sdleventreader.h"
#include "antimicrosettings.h"
#include "inputdevicebitarraystatus.h"

#include <QDebug>
#include <QTime>
#include <QTimer>
#include <QEventLoop>
#include <QMapIterator>
#include <QThread>

#define USE_NEW_REFRESH

const int InputDaemon::GAMECONTROLLERTRIGGERRELEASE = 16384;


InputDaemon::InputDaemon(QMap<SDL_JoystickID, InputDevice*> *joysticks,
                         AntiMicroSettings *settings,
                         bool graphical, QObject *parent) :
    QObject(parent),
    pollResetTimer(this)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    this->joysticks = joysticks;
    this->stopped = false;
    this->graphical = graphical;
    this->settings = settings;

    eventWorker = new SDLEventReader(joysticks, settings);
    refreshJoysticks();

    sdlWorkerThread = nullptr;
    if (graphical)
    {
        sdlWorkerThread = new QThread;
        eventWorker->moveToThread(sdlWorkerThread);
    }

    if (graphical)
    {
        connect(sdlWorkerThread, &QThread::started, eventWorker, &SDLEventReader::performWork);
        connect(eventWorker, &SDLEventReader::eventRaised, this, &InputDaemon::run);

        connect(JoyButton::getMouseHelper(), &JoyButtonMouseHelper::gamepadRefreshRateUpdated,
                eventWorker, &SDLEventReader::updatePollRate);

        connect(JoyButton::getMouseHelper(), &JoyButtonMouseHelper::gamepadRefreshRateUpdated,
                this, &InputDaemon::updatePollResetRate);
        connect(JoyButton::getMouseHelper(), &JoyButtonMouseHelper::mouseRefreshRateUpdated,
                this, &InputDaemon::updatePollResetRate);

        // Timer in case SDL does not produce an axis event during a joystick
        // poll.
        pollResetTimer.setSingleShot(true);
        pollResetTimer.setInterval(
                    qMax(JoyButton::getMouseRefreshRate(),
                         JoyButton::getGamepadRefreshRate()) + 1);

        connect(&pollResetTimer, &QTimer::timeout, this,
                &InputDaemon::resetActiveButtonMouseDistances);
    }
}

InputDaemon::~InputDaemon()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    if (eventWorker != nullptr)
    {
        quit();
    }

    if (sdlWorkerThread != nullptr)
    {
        sdlWorkerThread->quit();
        sdlWorkerThread->wait();
        delete sdlWorkerThread;
        sdlWorkerThread = nullptr;
    }
}

void InputDaemon::startWorker()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    if (!sdlWorkerThread->isRunning())
    {
        sdlWorkerThread->start(QThread::HighPriority);
    }
}

void InputDaemon::run ()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    PadderCommon::inputDaemonMutex.lock();

    // SDL has found events. The timeout is not necessary.
    pollResetTimer.stop();

    if (!stopped)
    {
        JoyButton::resetActiveButtonMouseDistances();

        QQueue<SDL_Event> sdlEventQueue;

        firstInputPass(&sdlEventQueue);

        modifyUnplugEvents(&sdlEventQueue);

        secondInputPass(&sdlEventQueue);

        clearBitArrayStatusInstances();
    }

    if (stopped)
    {
        if (joysticks->size() > 0)
        {
            emit complete(joysticks->value(0));
        }
        emit complete();
        stopped = false;
    }
    else
    {
        QTimer::singleShot(0, eventWorker, SLOT(performWork()));
        pollResetTimer.start();
    }

    PadderCommon::inputDaemonMutex.unlock();
}

void InputDaemon::refreshJoysticks()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    QMapIterator<SDL_JoystickID, InputDevice*> iter(*joysticks);

    while (iter.hasNext())
    {
        InputDevice *joystick = iter.next().value();
        if (joystick != nullptr)
        {
            delete joystick;
            joystick = nullptr;
        }
    }

    joysticks->clear();
    getTrackjoysticksLocal().clear();
    trackcontrollers.clear();

    settings->getLock()->lock();
    settings->beginGroup("Mappings");

    for (int i=0; i < SDL_NumJoysticks(); i++)
    {
#ifdef USE_NEW_REFRESH
        int index = i;

        // Check if device is considered a Game Controller at the start.
        if (SDL_IsGameController(index))
        {
            SDL_GameController *controller = SDL_GameControllerOpen(index);
            if (controller != nullptr)
            {
                SDL_Joystick *sdlStick = SDL_GameControllerGetJoystick(controller);
                SDL_JoystickID tempJoystickID = SDL_JoystickInstanceID(sdlStick);

                // Check if device has already been grabbed.
                if (!joysticks->contains(tempJoystickID))
                {

                    QString temp = QString();
                    SDL_JoystickGUID tempGUID = SDL_JoystickGetGUID(sdlStick);
                    char guidString[65] = {'0'};
                    SDL_JoystickGetGUIDString(tempGUID, guidString, sizeof(guidString));
                    temp = QString(guidString);

                    bool disableGameController = settings->value(QString("%1Disable").arg(temp), false).toBool();

                    // Check if user has designated device Joystick mode.
                    if (!disableGameController)
                    {
                        GameController *damncontroller = new GameController(controller, index, settings, this);
                        connect(damncontroller, &GameController::requestWait, eventWorker, &SDLEventReader::haltServices);
                        joysticks->insert(tempJoystickID, damncontroller);
                        trackcontrollers.insert(tempJoystickID, damncontroller);

                        emit deviceAdded(damncontroller);
                    }
                    else
                    {
                        Joystick *joystick = openJoystickDevice(index);
                        if (joystick != nullptr)
                        {
                            emit deviceAdded(joystick);
                        }

                    }
                }
                else
                {
                    // Make sure to decrement reference count
                    SDL_GameControllerClose(controller);
                }
            }
        }
        else
        {
            Joystick *joystick = openJoystickDevice(index);
            if (joystick != nullptr)
            {
                emit deviceAdded(joystick);
            }
        }


#else
        SDL_Joystick *joystick = SDL_JoystickOpen(i);
        if (joystick != nullptr)
        {
            QString temp = QString();
            SDL_JoystickGUID tempGUID = SDL_JoystickGetGUID(joystick);
            char guidString[65] = {'0'};
            SDL_JoystickGetGUIDString(tempGUID, guidString, sizeof(guidString));
            temp = QString(guidString);

            bool disableGameController = settings->value(QString("%1Disable").arg(temp), false).toBool();

            if (SDL_IsGameController(i) && !disableGameController)
            {
                SDL_GameController *controller = SDL_GameControllerOpen(i);
                GameController *damncontroller = new GameController(controller, i, settings, this);
                connect(damncontroller, &GameController::requestWait, eventWorker, &SDLEventReader::haltServices);
                SDL_Joystick *sdlStick = SDL_GameControllerGetJoystick(controller);
                SDL_JoystickID joystickID = SDL_JoystickInstanceID(sdlStick);
                joysticks->insert(joystickID, damncontroller);
                trackcontrollers.insert(joystickID, damncontroller);
            }
            else
            {
                Joystick *curJoystick = new Joystick(joystick, i, settings, this);
                connect(curJoystick, &Joystick::requestWait, eventWorker, &SDLEventReader::haltServices);
                SDL_JoystickID joystickID = SDL_JoystickInstanceID(joystick);
                joysticks->insert(joystickID, curJoystick);
                trackjoysticks.insert(joystickID, curJoystick);
            }
        }
#endif
    }

    settings->endGroup();
    settings->getLock()->unlock();

    emit joysticksRefreshed(joysticks);
}

void InputDaemon::deleteJoysticks()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    QMapIterator<SDL_JoystickID, InputDevice*> iter(*joysticks);

    while (iter.hasNext())
    {
        InputDevice *joystick = iter.next().value();
        if (joystick != nullptr)
        {
            delete joystick;
            joystick = nullptr;
        }
    }

    joysticks->clear();
    getTrackjoysticksLocal().clear();
    trackcontrollers.clear();

}

void InputDaemon::stop()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    stopped = true;
    pollResetTimer.stop();
}

void InputDaemon::refresh()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    stop();

    Logger::LogInfo("Refreshing joystick list");

    QEventLoop q;
    connect(eventWorker, &SDLEventReader::sdlStarted, &q, &QEventLoop::quit);
    QMetaObject::invokeMethod(eventWorker, "refresh", Qt::BlockingQueuedConnection);

    if (eventWorker->isSDLOpen())
    {
        q.exec();
    }

    disconnect(eventWorker, &SDLEventReader::sdlStarted, &q, &QEventLoop::quit);

    pollResetTimer.stop();

    // Put in an extra delay before refreshing the joysticks
    QTimer temp;
    connect(&temp, &QTimer::timeout, &q, &QEventLoop::quit);
    temp.start(100);
    q.exec();

    refreshJoysticks();
    QTimer::singleShot(100, eventWorker, SLOT(performWork()));

    stopped = false;
}

void InputDaemon::refreshJoystick(InputDevice *joystick)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    joystick->reset();

    emit joystickRefreshed(joystick);
}

void InputDaemon::quit()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    stopped = true;
    pollResetTimer.stop();

    disconnect(eventWorker, &SDLEventReader::eventRaised, this, nullptr);

    // Wait for SDL to finish. Let worker destructor close SDL.
    // Let InputDaemon destructor close thread instance.
    if (graphical)
    {
        QMetaObject::invokeMethod(eventWorker, "stop");
        QMetaObject::invokeMethod(eventWorker, "quit");
        QMetaObject::invokeMethod(eventWorker, "deleteLater", Qt::BlockingQueuedConnection);
    }
    else
    {
        eventWorker->stop();
        eventWorker->quit();
        delete eventWorker;
    }

    eventWorker = nullptr;
}


void InputDaemon::refreshMapping(QString mapping, InputDevice *device)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    bool found = false;

    for (int i = 0; (i < SDL_NumJoysticks()) && !found; i++)
    {
        SDL_Joystick *joystick = SDL_JoystickOpen(i);
        SDL_JoystickID joystickID = SDL_JoystickInstanceID(joystick);

        if (device->getSDLJoystickID() == joystickID)
        {
            found = true;

            if (SDL_IsGameController(i))
            {
                // Mapping string updated. Perform basic refresh
                QByteArray tempbarray = mapping.toUtf8();
                SDL_GameControllerAddMapping(tempbarray.data());
            }
            else
            {
                // Previously registered as a plain joystick. Add
                // mapping and check for validity. If SDL accepts it,
                // close current device and re-open as
                // a game controller.
                SDL_GameControllerAddMapping(mapping.toUtf8().constData());

                if (SDL_IsGameController(i))
                {
                    device->closeSDLDevice();
                    getTrackjoysticksLocal().remove(joystickID);
                    joysticks->remove(joystickID);

                    SDL_GameController *controller = SDL_GameControllerOpen(i);
                    GameController *damncontroller = new GameController(controller, i, settings, this);
                    connect(damncontroller, &GameController::requestWait, eventWorker, &SDLEventReader::haltServices);
                    SDL_Joystick *sdlStick = SDL_GameControllerGetJoystick(controller);
                    joystickID = SDL_JoystickInstanceID(sdlStick);
                    joysticks->insert(joystickID, damncontroller);
                    trackcontrollers.insert(joystickID, damncontroller);
                    emit deviceUpdated(i, damncontroller);
                }
            }
        }

        // Make sure to decrement reference count
        SDL_JoystickClose(joystick);
    }
}

void InputDaemon::removeDevice(InputDevice *device)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    if (device != nullptr)
    {
        SDL_JoystickID deviceID = device->getSDLJoystickID();

        joysticks->remove(deviceID);
        getTrackjoysticksLocal().remove(deviceID);
        trackcontrollers.remove(deviceID);

        refreshIndexes();

        emit deviceRemoved(deviceID);
    }
}

void InputDaemon::refreshIndexes()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    for (int i = 0; i < SDL_NumJoysticks(); i++)
    {
        SDL_Joystick *joystick = SDL_JoystickOpen(i);
        SDL_JoystickID joystickID = SDL_JoystickInstanceID(joystick);
        // Make sure to decrement reference count
        SDL_JoystickClose(joystick);

        InputDevice *tempdevice = joysticks->value(joystickID);
        if (tempdevice != nullptr)
        {
            tempdevice->setIndex(i);
        }
    }
}

void InputDaemon::addInputDevice(int index)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

  #ifdef USE_NEW_ADD
    // Check if device is considered a Game Controller at the start.
    if (SDL_IsGameController(index))
    {
        SDL_GameController *controller = SDL_GameControllerOpen(index);
        if (controller != nullptr)
        {
            SDL_Joystick *sdlStick = SDL_GameControllerGetJoystick(controller);
            SDL_JoystickID tempJoystickID = SDL_JoystickInstanceID(sdlStick);

            // Check if device has already been grabbed.
            if (!joysticks->contains(tempJoystickID))
            {
                settings->getLock()->lock();
                settings->beginGroup("Mappings");

                QString temp = QString();
                SDL_JoystickGUID tempGUID = SDL_JoystickGetGUID(sdlStick);
                char guidString[65] = {'0'};
                SDL_JoystickGetGUIDString(tempGUID, guidString, sizeof(guidString));
                temp = QString(guidString);

                bool disableGameController = settings->value(QString("%1Disable").arg(temp), false).toBool();

                settings->endGroup();
                settings->getLock()->unlock();

                // Check if user has designated device Joystick mode.
                if (!disableGameController)
                {
                    GameController *damncontroller = new GameController(controller, index, settings, this);
                    connect(damncontroller, &GameController::requestWait, eventWorker, &SDLEventReader::haltServices);
                    joysticks->insert(tempJoystickID, damncontroller);
                    trackcontrollers.insert(tempJoystickID, damncontroller);

                    Logger::LogInfo(QString("New game controller found - #%1 [%2]")
                                    .arg(index+1)
                                    .arg(QTime::currentTime().toString("hh:mm:ss.zzz")));

                    emit deviceAdded(damncontroller);
                }
                else
                {
                    // Check if joystick is considered connected.

                    Joystick *joystick = openJoystickDevice(index);
                    if (joystick != nullptr)
                    {
                        Logger::LogInfo(QString("New joystick found - #%1 [%2]")
                                        .arg(index+1)
                                        .arg(QTime::currentTime().toString("hh:mm:ss.zzz")));

                        emit deviceAdded(joystick);
                    }

                }
            }
            else
            {
                // Make sure to decrement reference count
                SDL_GameControllerClose(controller);
            }
        }
    }
    else
    {
        Joystick *joystick = openJoystickDevice(index);
        if (joystick != nullptr)
        {
            Logger::LogInfo(QString("New joystick found - #%1 [%2]")
                            .arg(index+1)
                            .arg(QTime::currentTime().toString("hh:mm:ss.zzz")));

            emit deviceAdded(joystick);
        }
    }
#else
    SDL_Joystick *joystick = SDL_JoystickOpen(index);
    if (joystick != nullptr)
    {
        SDL_JoystickID tempJoystickID = SDL_JoystickInstanceID(joystick);

        if (!joysticks->contains(tempJoystickID))
        {
            settings->getLock()->lock();
            settings->beginGroup("Mappings");

            QString temp = QString();
            SDL_JoystickGUID tempGUID = SDL_JoystickGetGUID(joystick);
            char guidString[65] = {'0'};
            SDL_JoystickGetGUIDString(tempGUID, guidString, sizeof(guidString));
            temp = QString(guidString);

            bool disableGameController = settings->value(QString("%1Disable").arg(temp), false).toBool();

            if (SDL_IsGameController(index) && !disableGameController)
            {
                // Make sure to decrement reference count
                SDL_JoystickClose(joystick);

                SDL_GameController *controller = SDL_GameControllerOpen(index);
                if (controller != nullptr)
                {
                    SDL_Joystick *sdlStick = SDL_GameControllerGetJoystick(controller);
                    SDL_JoystickID tempJoystickID = SDL_JoystickInstanceID(sdlStick);
                    if (!joysticks->contains(tempJoystickID))
                    {
                        GameController *damncontroller = new GameController(controller, index, settings, this);
                        connect(damncontroller, &GameController::requestWait, eventWorker, &SDLEventReader::haltServices);
                        joysticks->insert(tempJoystickID, damncontroller);
                        trackcontrollers.insert(tempJoystickID, damncontroller);

                        settings->endGroup();
                        settings->getLock()->unlock();

                        emit deviceAdded(damncontroller);
                    }
                }
                else
                {
                    settings->endGroup();
                    settings->getLock()->unlock();
                }
            }
            else
            {
                Joystick *curJoystick = new Joystick(joystick, index, settings, this);
                joysticks->insert(tempJoystickID, curJoystick);
                getTrackjoysticksLocal().insert(tempJoystickID, curJoystick);

                settings->endGroup();
                settings->getLock()->unlock();

                emit deviceAdded(curJoystick);
            }
        }
        else
        {
            // Make sure to decrement reference count
            SDL_JoystickClose(joystick);
        }
    }
#endif
}

Joystick *InputDaemon::openJoystickDevice(int index)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    // Check if joystick is considered connected.
    SDL_Joystick *joystick = SDL_JoystickOpen(index);
    Joystick *curJoystick = nullptr;
    if (joystick != nullptr)
    {
        SDL_JoystickID tempJoystickID = SDL_JoystickInstanceID(joystick);

        curJoystick = new Joystick(joystick, index, settings, this);
        joysticks->insert(tempJoystickID, curJoystick);
        getTrackjoysticksLocal().insert(tempJoystickID, curJoystick);

    }

    return curJoystick;
}



InputDeviceBitArrayStatus*
InputDaemon::createOrGrabBitStatusEntry(QHash<InputDevice *, InputDeviceBitArrayStatus *> *statusHash,
                                        InputDevice *device, bool readCurrent)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    InputDeviceBitArrayStatus *bitArrayStatus = nullptr;

    if (!statusHash->contains(device))
    {
        bitArrayStatus = new InputDeviceBitArrayStatus(device, readCurrent);
        statusHash->insert(device, bitArrayStatus);
    }
    else
    {
        bitArrayStatus = statusHash->value(device);
    }

    return bitArrayStatus;
}

void InputDaemon::firstInputPass(QQueue<SDL_Event> *sdlEventQueue)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    SDL_Event event;

    while (SDL_PollEvent(&event) > 0)
    {
        switch (event.type)
        {
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
            {

                InputDevice *joy = getTrackjoysticksLocal().value(event.jbutton.which);

                if (joy != nullptr)
                {
                    SetJoystick* set = joy->getActiveSetJoystick();
                    JoyButton *button = set->getJoyButton(event.jbutton.button);

                    if (button != nullptr)
                    {
                        InputDeviceBitArrayStatus *pending = createOrGrabBitStatusEntry(&pendingEventValues, joy);
                        pending->changeButtonStatus(event.jbutton.button,
                                                  event.type == SDL_JOYBUTTONDOWN ? true : false);
                        sdlEventQueue->append(event);
                    }
                }
                else
                {
                    sdlEventQueue->append(event);
                }

                break;
            }
            case SDL_JOYAXISMOTION:
            {
                InputDevice *joy = getTrackjoysticksLocal().value(event.jaxis.which);

                if (joy != nullptr)
                {
                    SetJoystick* set = joy->getActiveSetJoystick();
                    JoyAxis *axis = set->getJoyAxis(event.jaxis.axis);

                    if (axis != nullptr)
                    {
                        InputDeviceBitArrayStatus *temp = createOrGrabBitStatusEntry(&releaseEventsGenerated, joy, false);
                        temp->changeAxesStatus(event.jaxis.axis, event.jaxis.axis == 0);

                        InputDeviceBitArrayStatus *pending = createOrGrabBitStatusEntry(&pendingEventValues, joy);
                        pending->changeAxesStatus(event.jaxis.axis, !axis->inDeadZone(event.jaxis.value));
                        sdlEventQueue->append(event);
                    }
                }
                else
                {
                    sdlEventQueue->append(event);
                }

                break;
            }
            case SDL_JOYHATMOTION:
            {
                InputDevice *joy = getTrackjoysticksLocal().value(event.jhat.which);

                if (joy != nullptr)
                {
                    SetJoystick* set = joy->getActiveSetJoystick();
                    JoyDPad *dpad = set->getJoyDPad(event.jhat.hat);

                    if (dpad != nullptr)
                    {
                        InputDeviceBitArrayStatus *pending = createOrGrabBitStatusEntry(&pendingEventValues, joy);
                        pending->changeHatStatus(event.jhat.hat, (event.jhat.value != 0) ? true : false);
                        sdlEventQueue->append(event);
                    }
                }
                else
                {
                    sdlEventQueue->append(event);
                }

                break;
            }

            case SDL_CONTROLLERAXISMOTION:
            {
                InputDevice *joy = trackcontrollers.value(event.caxis.which);
                if (joy != nullptr)
                {
                    SetJoystick* set = joy->getActiveSetJoystick();
                    JoyAxis *axis = set->getJoyAxis(event.caxis.axis);
                    if (axis != nullptr)
                    {
                        InputDeviceBitArrayStatus *temp = createOrGrabBitStatusEntry(&releaseEventsGenerated, joy, false);
                        if ((event.caxis.axis != SDL_CONTROLLER_AXIS_TRIGGERLEFT) &&
                            (event.caxis.axis != SDL_CONTROLLER_AXIS_TRIGGERRIGHT))
                        {
                            temp->changeAxesStatus(event.caxis.axis, event.caxis.value == 0);
                        }
                        else
                        {
                            temp->changeAxesStatus(event.caxis.axis, event.caxis.value == GAMECONTROLLERTRIGGERRELEASE);
                        }

                        InputDeviceBitArrayStatus *pending = createOrGrabBitStatusEntry(&pendingEventValues, joy);
                        pending->changeAxesStatus(event.caxis.axis, !axis->inDeadZone(event.caxis.value));
                        sdlEventQueue->append(event);
                    }
                }
                break;
            }

            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
            {
                InputDevice *joy = trackcontrollers.value(event.cbutton.which);
                if (joy != nullptr)
                {
                    SetJoystick* set = joy->getActiveSetJoystick();
                    JoyButton *button = set->getJoyButton(event.cbutton.button);

                    if (button != nullptr)
                    {
                        InputDeviceBitArrayStatus *pending = createOrGrabBitStatusEntry(&pendingEventValues, joy);
                        pending->changeButtonStatus(event.cbutton.button,
                                                  event.type == SDL_CONTROLLERBUTTONDOWN ? true : false);
                        sdlEventQueue->append(event);
                    }
                }

                break;
            }
            case SDL_JOYDEVICEREMOVED:
            case SDL_JOYDEVICEADDED:
            {
                sdlEventQueue->append(event);
                break;
            }
            case SDL_QUIT:
            {
                sdlEventQueue->append(event);
                break;
            }
        }
    }
}


void InputDaemon::modifyUnplugEvents(QQueue<SDL_Event> *sdlEventQueue)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    QHashIterator<InputDevice*, InputDeviceBitArrayStatus*> genIter(getReleaseEventsGeneratedLocal());
    while (genIter.hasNext())
    {
        genIter.next();
        InputDevice *device = genIter.key();
        InputDeviceBitArrayStatus *generatedTemp = genIter.value();
        QBitArray tempBitArray = generatedTemp->generateFinalBitArray();

        #ifndef QT_DEBUG_NO_OUTPUT
        qDebug() << "ARRAY: " << tempBitArray;
        #endif

        int bitArraySize = tempBitArray.size();

        #ifndef QT_DEBUG_NO_OUTPUT
        qDebug() << "ARRAY SIZE: " << bitArraySize;
        #endif

        if ((bitArraySize > 0) && (tempBitArray.count(true) == device->getNumberAxes()))
        {
            if (getPendingEventValuesLocal().contains(device))
            {
                InputDeviceBitArrayStatus *pendingTemp = getPendingEventValuesLocal().value(device);
                QBitArray pendingBitArray = pendingTemp->generateFinalBitArray();
                QBitArray unplugBitArray = createUnplugEventBitArray(device);
                int pendingBitArraySize = pendingBitArray.size();

                if ((bitArraySize == pendingBitArraySize) &&
                    (pendingBitArray == unplugBitArray))
                {
                    QQueue<SDL_Event> tempQueue;
                    while (!sdlEventQueue->isEmpty())
                    {
                        SDL_Event event = sdlEventQueue->dequeue();
                        switch (event.type)
                        {
                            case SDL_JOYBUTTONDOWN:
                            case SDL_JOYBUTTONUP:
                            {
                                tempQueue.enqueue(event);
                                break;
                            }
                            case SDL_JOYAXISMOTION:
                            {
                                if (event.jaxis.which != device->getSDLJoystickID())
                                {
                                    tempQueue.enqueue(event);
                                }
                                else
                                {
                                    InputDevice *joy = getTrackjoysticksLocal().value(event.jaxis.which);

                                    if (joy != nullptr)
                                    {
                                        JoyAxis *axis = joy->getActiveSetJoystick()->getJoyAxis(event.jaxis.axis);
                                        if (axis != nullptr)
                                        {
                                            if (axis->getThrottle() != static_cast<int>(JoyAxis::NormalThrottle))
                                            {
                                                event.jaxis.value = static_cast<short>(axis->getProperReleaseValue());
                                            }
                                        }
                                    }

                                    tempQueue.enqueue(event);
                                }

                                break;
                            }
                            case SDL_JOYHATMOTION:
                            {
                                tempQueue.enqueue(event);
                                break;
                            }
                            case SDL_CONTROLLERAXISMOTION:
                            {
                                if (event.caxis.which != device->getSDLJoystickID())
                                {
                                    tempQueue.enqueue(event);
                                }
                                else
                                {
                                    InputDevice *joy = trackcontrollers.value(event.caxis.which);
                                    if (joy != nullptr)
                                    {
                                        SetJoystick* set = joy->getActiveSetJoystick();
                                        JoyAxis *axis = set->getJoyAxis(event.caxis.axis);
                                        if (axis != nullptr)
                                        {
                                            if ((event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT) ||
                                                (event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT))
                                            {
                                                event.caxis.value = static_cast<short>(axis->getProperReleaseValue());
                                            }
                                        }
                                    }

                                    tempQueue.enqueue(event);
                                }

                                break;
                            }
                            case SDL_CONTROLLERBUTTONDOWN:
                            case SDL_CONTROLLERBUTTONUP:
                            {

                                tempQueue.enqueue(event);
                                break;
                            }
                            case SDL_JOYDEVICEREMOVED:
                            case SDL_JOYDEVICEADDED:
                            {
                                tempQueue.enqueue(event);
                                break;
                            }
                            default:
                            {
                                tempQueue.enqueue(event);
                            }
                        }
                    }

                    sdlEventQueue->swap(tempQueue);
                }
            }
        }
    }
}



QBitArray InputDaemon::createUnplugEventBitArray(InputDevice *device)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    InputDeviceBitArrayStatus tempStatus(device, false);

    for (int i = 0; i < device->getNumberRawAxes(); i++)
    {
        JoyAxis *axis = device->getActiveSetJoystick()->getJoyAxis(i);
        if ((axis != nullptr) && (axis->getThrottle() != static_cast<int>(JoyAxis::NormalThrottle)))
        {
            tempStatus.changeAxesStatus(i, true);
        }
    }

    QBitArray unplugBitArray = tempStatus.generateFinalBitArray();
    return unplugBitArray;
}


void InputDaemon::secondInputPass(QQueue<SDL_Event> *sdlEventQueue)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    QHash<SDL_JoystickID, InputDevice*> activeDevices;

    while (!sdlEventQueue->isEmpty())
    {
        SDL_Event event = sdlEventQueue->dequeue();

        switch (event.type)
        {
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
            {

                InputDevice *joy = getTrackjoysticksLocal().value(event.jbutton.which);

                if (joy != nullptr)
                {
                    SetJoystick* set = joy->getActiveSetJoystick();
                    JoyButton *button = set->getJoyButton(event.jbutton.button);

                    if (button != nullptr)
                    {
                        button->queuePendingEvent(event.type == SDL_JOYBUTTONDOWN ? true : false);

                        if (!activeDevices.contains(event.jbutton.which))
                        {
                            activeDevices.insert(event.jbutton.which, joy);
                        }
                    }
                }
                else if (trackcontrollers.contains(event.jbutton.which))
                {
                    GameController *gamepad = trackcontrollers.value(event.jbutton.which);
                    gamepad->rawButtonEvent(event.jbutton.button, event.type == SDL_JOYBUTTONDOWN ? true : false);
                }

                break;
            }

            case SDL_JOYAXISMOTION:
            {

                InputDevice *joy = getTrackjoysticksLocal().value(event.jaxis.which);

                if (joy != nullptr)
                {
                    SetJoystick* set = joy->getActiveSetJoystick();
                    JoyAxis *axis = set->getJoyAxis(event.jaxis.axis);
                    if (axis != nullptr)
                    {
                        axis->queuePendingEvent(event.jaxis.value);

                        if (!activeDevices.contains(event.jaxis.which))
                        {
                            activeDevices.insert(event.jaxis.which, joy);
                        }
                    }

                    joy->rawAxisEvent(event.jaxis.which, event.jaxis.value);
                }
                else if (trackcontrollers.contains(event.jaxis.which))
                {
                    GameController *gamepad = trackcontrollers.value(event.jaxis.which);
                    gamepad->rawAxisEvent(event.jaxis.axis, event.jaxis.value);
                }

                break;
            }

            case SDL_JOYHATMOTION:
            {
                InputDevice *joy = getTrackjoysticksLocal().value(event.jhat.which);

                if (joy != nullptr)
                {
                    SetJoystick* set = joy->getActiveSetJoystick();
                    JoyDPad *dpad = set->getJoyDPad(event.jhat.hat);
                    if (dpad != nullptr)
                    {
                        dpad->joyEvent(event.jhat.value);

                        if (!activeDevices.contains(event.jhat.which))
                        {
                            activeDevices.insert(event.jhat.which, joy);
                        }
                    }
                }
                else if (trackcontrollers.contains(event.jhat.which))
                {
                    GameController *gamepad = trackcontrollers.value(event.jaxis.which);
                    gamepad->rawDPadEvent(event.jhat.hat, event.jhat.value);
                }

                break;
            }

            case SDL_CONTROLLERAXISMOTION:
            {
                InputDevice *joy = trackcontrollers.value(event.caxis.which);
                if (joy != nullptr)
                {
                    SetJoystick* set = joy->getActiveSetJoystick();
                    JoyAxis *axis = set->getJoyAxis(event.caxis.axis);
                    if (axis != nullptr)
                    {
                        axis->queuePendingEvent(event.caxis.value);

                        if (!activeDevices.contains(event.caxis.which))
                        {
                            activeDevices.insert(event.caxis.which, joy);
                        }
                    }
                }
                break;
            }

            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
            {
                InputDevice *joy = trackcontrollers.value(event.cbutton.which);
                if (joy != nullptr)
                {
                    SetJoystick* set = joy->getActiveSetJoystick();
                    JoyButton *button = set->getJoyButton(event.cbutton.button);

                    if (button != nullptr)
                    {
                        button->queuePendingEvent(event.type == SDL_CONTROLLERBUTTONDOWN ? true : false);

                        if (!activeDevices.contains(event.cbutton.which))
                        {
                            activeDevices.insert(event.cbutton.which, joy);
                        }
                    }
                }

                break;
            }

            case SDL_JOYDEVICEREMOVED:
            {
                InputDevice *device = joysticks->value(event.jdevice.which);
                if (device != nullptr)
                {
                    Logger::LogInfo(QString("Removing joystick #%1 [%2]")
                                    .arg(device->getRealJoyNumber())
                                    .arg(QTime::currentTime().toString("hh:mm:ss.zzz")));

                    removeDevice(device);
                }

                break;
            }

            case SDL_JOYDEVICEADDED:
            {
                addInputDevice(event.jdevice.which);
                break;
            }

            case SDL_QUIT:
            {
                stopped = true;
                break;
            }

            default:
                break;
        }

        // Active possible queued events.
        QHashIterator<SDL_JoystickID, InputDevice*> activeDevIter(activeDevices);
        while (activeDevIter.hasNext())
        {
            InputDevice *tempDevice = activeDevIter.next().value();
            tempDevice->activatePossibleControlStickEvents();
            tempDevice->activatePossibleAxisEvents();
            tempDevice->activatePossibleDPadEvents();
            tempDevice->activatePossibleVDPadEvents();
            tempDevice->activatePossibleButtonEvents();
        }

        if (JoyButton::shouldInvokeMouseEvents())
        {
            // Do not wait for next event loop run. Execute immediately.
            JoyButton::invokeMouseEvents();
        }
    }
}

void InputDaemon::clearBitArrayStatusInstances()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    QHashIterator<InputDevice*, InputDeviceBitArrayStatus*> genIter(getReleaseEventsGeneratedLocal());
    while (genIter.hasNext())
    {
        InputDeviceBitArrayStatus *temp = genIter.next().value();
        if (temp != nullptr)
        {
            delete temp;
            temp = nullptr;
        }
    }

    getReleaseEventsGeneratedLocal().clear();

    QHashIterator<InputDevice*, InputDeviceBitArrayStatus*> pendIter(getPendingEventValuesLocal());
    while (pendIter.hasNext())
    {
        InputDeviceBitArrayStatus *temp = pendIter.next().value();
        if (temp != nullptr)
        {
            delete temp;
            temp = nullptr;
        }
    }

    getPendingEventValuesLocal().clear();
}

void InputDaemon::resetActiveButtonMouseDistances()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    pollResetTimer.stop();

    JoyButton::resetActiveButtonMouseDistances();
}

void InputDaemon::updatePollResetRate(int tempPollRate)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    Q_UNUSED(tempPollRate);


    bool wasActive = pollResetTimer.isActive();
    pollResetTimer.stop();

    pollResetTimer.setInterval(
                qMax(JoyButton::getMouseRefreshRate(),
                     JoyButton::getGamepadRefreshRate()) + 1);

    if (wasActive)
    {
        pollResetTimer.start();
    }
}

QHash<SDL_JoystickID, Joystick*>& InputDaemon::getTrackjoysticksLocal() {

    return trackjoysticks;
}

QHash<InputDevice*, InputDeviceBitArrayStatus*>& InputDaemon::getReleaseEventsGeneratedLocal() {

    return releaseEventsGenerated;
}

QHash<InputDevice*, InputDeviceBitArrayStatus*>& InputDaemon::getPendingEventValuesLocal() {

    return pendingEventValues;
}
