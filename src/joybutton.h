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

#ifndef JOYBUTTON_H
#define JOYBUTTON_H


#include "joybuttonslot.h"
#include "springmousemoveinfo.h"
#include "joybuttonmousehelper.h"

#ifdef Q_OS_WIN
  #include "joykeyrepeathelper.h"
#endif

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QTime>
#include <QList>
#include <QListIterator>
#include <QHash>
#include <QQueue>
#include <QReadWriteLock>

class VDPad;
class SetJoystick;
class QXmlStreamReader;
class QXmlStreamWriter;
class QThread;

class JoyButton : public QObject
{
    Q_OBJECT

public:
    explicit JoyButton(int index, int originset, SetJoystick *parentSet, QObject *parent=0);
    ~JoyButton();

    enum SetChangeCondition {SetChangeDisabled=0, SetChangeOneWay,
                             SetChangeTwoWay, SetChangeWhileHeld};

    enum JoyMouseMovementMode {MouseCursor=0, MouseSpring};
    enum JoyMouseCurve {EnhancedPrecisionCurve=0, LinearCurve, QuadraticCurve,
                        CubicCurve, QuadraticExtremeCurve, PowerCurve,
                        EasingQuadraticCurve, EasingCubicCurve};
    enum JoyExtraAccelerationCurve {LinearAccelCurve, EaseOutSineCurve,
                                    EaseOutQuadAccelCurve, EaseOutCubicAccelCurve};
    enum TurboMode {NormalTurbo=0, GradientTurbo, PulseTurbo};

    void joyEvent(bool pressed, bool ignoresets=false);
    void queuePendingEvent(bool pressed, bool ignoresets=false);
    void activatePendingEvent();
    void setJoyNumber(int index);
    void clearPendingEvent();
    void setCustomName(QString name);
    void copyExtraAccelerationState(JoyButton *srcButton);
    void setUpdateInitAccel(bool state);
    void removeVDPad();
    void setIgnoreEventState(bool ignore);
    void setMouseMode(JoyMouseMovementMode mousemode);
    void setMouseCurve(JoyMouseCurve selectedCurve);
    void setWhileHeldStatus(bool status);
    void setCycleResetStatus(bool enabled);
    void copyAssignments(JoyButton *destButton);
    void resetAccelerationDistances();
    void setExtraAccelerationStatus(bool status);
    void setExtraAccelerationMultiplier(double value);
    void setCycleResetTime(int interval); // .., unsigned
    void setMinAccelThreshold(double value);
    void setExtraAccelerationCurve(JoyExtraAccelerationCurve curve);
    void setSpringDeadCircleMultiplier(int value);
    void setAccelExtraDuration(double value);
    void setStartAccelMultiplier(double value);
    void setMaxAccelThreshold(double value);
    void setChangeSetSelection(int index, bool updateActiveString=true);

    bool hasPendingEvent();
    bool getToggleState();
    bool isUsingTurbo();
    bool getButtonState();
    bool containsSequence();
    bool containsDistanceSlots();
    bool containsReleaseSlots();
    bool getIgnoreEventState();
    bool getWhileHeldStatus();
    bool hasActiveSlots();
    bool isCycleResetActive();
    bool isRelativeSpring();
    bool isPartVDPad();
    bool isExtraAccelerationEnabled();

    double getMinAccelThreshold();
    double getMaxAccelThreshold();
    double getStartAccelMultiplier();
    double getAccelExtraDuration();
    double getExtraAccelerationMultiplier();
    double getSensitivity();
    double getEasingDuration();

    int getJoyNumber();
    int getTurboInterval();
    int getMouseSpeedX();
    int getMouseSpeedY();
    int getWheelSpeedX();
    int getWheelSpeedY();
    int getSetSelection();
    int getOriginSet();
    int getSpringWidth();
    int getSpringHeight();
    int getCycleResetTime(); // unsigned
    int getSpringDeadCircleMultiplier();

    QString getCustomName();
    QString getActionName();
    QString getButtonName();

    QList<JoyButtonSlot*>* getAssignedSlots();
    QList<JoyButtonSlot*> const& getActiveSlots();

    virtual bool isPartRealAxis();
    virtual bool isModifierButton();
    virtual bool isDefault();

    virtual int getRealJoyNumber() const;

    virtual double getDistanceFromDeadZone();
    virtual double getMouseDistanceFromDeadZone();
    virtual double getLastMouseDistanceFromDeadZone();
    virtual double getAccelerationDistance();
    virtual double getLastAccelerationDistance();

    virtual void initializeDistanceValues();
    virtual void setTurboMode(TurboMode mode);
    virtual void setDefaultButtonName(QString tempname);
    virtual void copyLastMouseDistanceFromDeadZone(JoyButton *srcButton); // Don't use direct assignment but copying from a current button.
    virtual void copyLastAccelerationDistance(JoyButton *srcButton);
    virtual void setVDPad(VDPad *vdpad);
    virtual void readConfig(QXmlStreamReader *xml);
    virtual void writeConfig(QXmlStreamWriter *xml);
    virtual void setChangeSetCondition(SetChangeCondition condition,
                                       bool passive=false, bool updateActiveString=true);

    virtual QString getPartialName(bool forceFullFormat=false, bool displayNames=false) const;
    virtual QString getSlotsSummary();
    virtual QString getSlotsString();
    virtual QString getActiveZoneSummary();
    virtual QString getCalculatedActiveZoneSummary();
    virtual QString getName(bool forceFullFormat=false, bool displayNames=false);
    virtual QString getXmlName();
    virtual QString getDefaultButtonName();

    virtual QList<JoyButtonSlot*> getActiveZoneList();


    SetChangeCondition getChangeSetCondition();

    VDPad* getVDPad();

    JoyMouseMovementMode getMouseMode();

    JoyMouseCurve getMouseCurve();

    SetJoystick* getParentSet();

    TurboMode getTurboMode();


    static int calculateFinalMouseSpeed(JoyMouseCurve curve, int value);
    static int getMouseHistorySize();
    static int getSpringModeScreen();
    static int getMouseRefreshRate();
    static int getGamepadRefreshRate();

    static double getWeightModifier();
    static double cursorRemainderX;
    static double cursorRemainderY;

    static bool hasCursorEvents();
    static bool hasSpringEvents();
    static bool shouldInvokeMouseEvents();

    static void setWeightModifier(double modifier);
    static void moveMouseCursor(int &movedX, int &movedY, int &movedElapsed);
    static void moveSpringMouse(int &movedX, int &movedY, bool &hasMoved);
    static void setMouseHistorySize(int size);
    static void setMouseRefreshRate(int refresh);
    static void setSpringModeScreen(int screen);
    static void resetActiveButtonMouseDistances();
    static void setGamepadRefreshRate(int refresh);
    static void restartLastMouseTime();
    static void setStaticMouseThread(QThread *thread);
    static void indirectStaticMouseThread(QThread *thread);
    static void invokeMouseEvents();

    static JoyButtonMouseHelper* getMouseHelper();
    static QList<JoyButton*>* getPendingMouseButtons();
    static QList<double> mouseHistoryX;
    static QList<double> mouseHistoryY;

    JoyExtraAccelerationCurve getExtraAccelerationCurve();

    static const QString xmlName;

    // Define default values for many properties.
    static const int ENABLEDTURBODEFAULT;
    static const double DEFAULTMOUSESPEEDMOD;
    static const int DEFAULTKEYREPEATDELAY; // unsigned
    static const int DEFAULTKEYREPEATRATE; // unsigned
    static const JoyMouseCurve DEFAULTMOUSECURVE;
    static const bool DEFAULTTOGGLE;
    static const int DEFAULTTURBOINTERVAL;
    static const bool DEFAULTUSETURBO;
    static const int DEFAULTMOUSESPEEDX;
    static const int DEFAULTMOUSESPEEDY;
    static const int DEFAULTSETSELECTION;
    static const SetChangeCondition DEFAULTSETCONDITION;
    static const JoyMouseMovementMode DEFAULTMOUSEMODE;
    static const int DEFAULTSPRINGWIDTH;
    static const int DEFAULTSPRINGHEIGHT;
    static const double DEFAULTSENSITIVITY;
    static const int DEFAULTWHEELX;
    static const int DEFAULTWHEELY;
    static const bool DEFAULTCYCLERESETACTIVE;
    static const int DEFAULTCYCLERESET;
    static const bool DEFAULTRELATIVESPRING;
    static const TurboMode DEFAULTTURBOMODE;
    static const double DEFAULTEASINGDURATION;
    static const double MINIMUMEASINGDURATION;
    static const double MAXIMUMEASINGDURATION;

    static const int DEFAULTMOUSEHISTORYSIZE;
    static const double DEFAULTWEIGHTMODIFIER;

    static const int MAXIMUMMOUSEHISTORYSIZE;
    static const double MAXIMUMWEIGHTMODIFIER;

    static const int MAXIMUMMOUSEREFRESHRATE;
    static const int DEFAULTIDLEMOUSEREFRESHRATE;
    static int IDLEMOUSEREFRESHRATE;

    static const int MINCYCLERESETTIME; // unsigned
    static const int MAXCYCLERESETTIME; // unsigned

    static const double DEFAULTEXTRACCELVALUE;
    static const double DEFAULTMINACCELTHRESHOLD;
    static const double DEFAULTMAXACCELTHRESHOLD;
    static const double DEFAULTSTARTACCELMULTIPLIER;
    static const double DEFAULTACCELEASINGDURATION;
    static const JoyExtraAccelerationCurve DEFAULTEXTRAACCELCURVE;

    static const int DEFAULTSPRINGRELEASERADIUS;


protected:
    int getPreferredKeyPressTime(); // unsigned

    double getTotalSlotDistance(JoyButtonSlot *slot);

    bool distanceEvent();
    bool checkForDelaySequence();
    bool insertAssignedSlot(JoyButtonSlot *newSlot, bool updateActiveString=true);

    void clearAssignedSlots(bool signalEmit=true);
    void releaseSlotEvent();
    void findReleaseEventEnd();
    void findReleaseEventIterEnd(QListIterator<JoyButtonSlot*> *tempiter);
    void findHoldEventEnd();
    void checkForPressedSetChange();
    void checkTurboCondition(JoyButtonSlot *slot);
    void vdpadPassEvent(bool pressed, bool ignoresets=false);
    void localBuildActiveZoneSummaryString();

    static bool hasFutureSpringEvents();

    virtual double getCurrentSpringDeadCircle();

    virtual bool readButtonConfig(QXmlStreamReader *xml);

    TurboMode currentTurboMode;

    QString buildActiveZoneSummary(QList<JoyButtonSlot*> &tempList);


    typedef struct _mouseCursorInfo
    {
        JoyButtonSlot *slot;
        double code;
    } mouseCursorInfo;

    static double mouseSpeedModifier;
    static QList<JoyButtonSlot*> mouseSpeedModList;
    static QList<mouseCursorInfo> cursorXSpeeds;
    static QList<mouseCursorInfo> cursorYSpeeds;
    static QList<PadderCommon::springModeInfo> springXSpeeds;
    static QList<PadderCommon::springModeInfo> springYSpeeds;
    static QList<JoyButton*> pendingMouseButtons;

    static QHash<int, int> activeKeys; // QHash<unsigned int, int
    static QHash<int, int> activeMouseButtons; // QHash<unsigned int, int

#ifdef Q_OS_WIN
    static JoyKeyRepeatHelper repeatHelper;
#endif

    static JoyButtonSlot *lastActiveKey;
    static JoyButtonMouseHelper mouseHelper;
    static double weightModifier;
    static int mouseHistorySize;
    static int mouseRefreshRate;
    static int springModeScreen;
    static int gamepadRefreshRate;

    int index; // Used to denote the SDL index of the actual joypad button
    int turboInterval;
    int wheelSpeedX;
    int wheelSpeedY;
    int setSelection;
    int tempTurboInterval;
    int springDeadCircleMultiplier;

    bool isButtonPressed; // Used to denote whether the actual joypad button is pressed
    bool isKeyPressed; // Used to denote whether the virtual key is pressed

    double lastDistance;
    double lastWheelVerticalDistance;
    double lastWheelHorizontalDistance;

    QTimer turboTimer;
    QTimer mouseWheelVerticalEventTimer;
    QTimer mouseWheelHorizontalEventTimer;

    QTime wheelVerticalTime;
    QTime wheelHorizontalTime;
    QTime turboHold;

    SetJoystick *parentSet; // Pointer to set that button is assigned to.
    SetChangeCondition setSelectionCondition;
    JoyButtonSlot *currentWheelVerticalEvent;
    JoyButtonSlot *currentWheelHorizontalEvent;

    QQueue<bool> ignoreSetQueue;
    QQueue<bool> isButtonPressedQueue;
    QQueue<JoyButtonSlot*> mouseWheelVerticalEventQueue;
    QQueue<JoyButtonSlot*> mouseWheelHorizontalEventQueue;

    QString buttonName; // User specified button name
    QString defaultButtonName; // Name used by the system

signals:
    void clicked (int index);
    void released (int index);
    void keyChanged(int keycode);
    void mouseChanged(int mousecode);
    void setChangeActivated(int index);
    void setAssignmentChanged(int current_button, int associated_set, int mode);
    void finishedPause();
    void turboChanged(bool state);
    void toggleChanged(bool state);
    void turboIntervalChanged(int interval);
    void slotsChanged();
    void actionNameChanged();
    void buttonNameChanged();
    void propertyUpdated();
    void activeZoneChanged();

public slots:
    void setTurboInterval (int interval);
    void setToggle (bool toggle);
    void setUseTurbo(bool useTurbo);
    void setMouseSpeedX(int speed);
    void setMouseSpeedY(int speed);
    void setWheelSpeedX(int speed);
    void setWheelSpeedY(int speed);
    void setSpringWidth(int value);
    void setSpringHeight(int value);
    void setSensitivity(double value);
    void setSpringRelativeStatus(bool value);
    void setActionName(QString tempName);
    void setButtonName(QString tempName);
    void setEasingDuration(double value);
    void establishPropertyUpdatedConnections();
    void disconnectPropertyUpdatedConnections();
    void removeAssignedSlot(int index);

    virtual void reset();
    virtual void reset(int index);
    virtual void resetProperties();
    virtual void clearSlotsEventReset(bool clearSignalEmit=true);
    virtual void eventReset();
    virtual void mouseEvent();

    static void establishMouseTimerConnections();

    bool setAssignedSlot(int code, int alias, int index,
                         JoyButtonSlot::JoySlotInputAction mode=JoyButtonSlot::JoyKeyboard); // .., .., unsigned, .., ..

    bool setAssignedSlot(int code,
                         JoyButtonSlot::JoySlotInputAction mode=JoyButtonSlot::JoyKeyboard);

    bool setAssignedSlot(int code, int alias,
                         JoyButtonSlot::JoySlotInputAction mode=JoyButtonSlot::JoyKeyboard); // .., .., unsigned, ..

    bool setAssignedSlot(JoyButtonSlot *otherSlot, int index);

    bool insertAssignedSlot(int code, int alias, int index,
                            JoyButtonSlot::JoySlotInputAction mode=JoyButtonSlot::JoyKeyboard); // .., .., unsigned, .., ..


protected slots:
    virtual void turboEvent();
    virtual void wheelEventVertical();
    virtual void wheelEventHorizontal();

    void createDeskEvent();
    void releaseDeskEvent(bool skipsetchange=false);
    void buildActiveZoneSummaryString();

private slots:
    void releaseActiveSlots();
    void activateSlots();
    void waitForDeskEvent();
    void waitForReleaseDeskEvent();
    void holdEvent();
    void delayEvent();
    void pauseWaitEvent();
    void checkForSetChange();
    void keyPressEvent();
    void slotSetChange();

private:
    QList<JoyButtonSlot*>& getAssignmentsLocal();
    QList<JoyButtonSlot*>& getActiveSlotsLocal();

    bool toggle;
    bool quitEvent;
    bool isDown;
    bool toggleActiveState;
    bool useTurbo;
    bool lastUnlessInList;
    bool ignoresets;
    bool ignoreEvents;
    bool whileHeldStatus;
    bool updateLastMouseDistance; // Should lastMouseDistance be updated. Set after mouse event.
    bool updateStartingMouseDistance; // Should startingMouseDistance be updated. Set after acceleration has finally been applied.
    bool relativeSpring;
    bool pendingPress;
    bool pendingEvent;
    bool pendingIgnoreSets;
    bool extraAccelerationEnabled;
    bool cycleResetActive;
    bool updateInitAccelValues;

    int mouseSpeedX;
    int mouseSpeedY;
    int originset;
    int springWidth;
    int springHeight;
    int currentRawValue;
    int cycleResetInterval; // unsigned

    double sensitivity;
    double lastMouseDistance; // Keep track of the previous mouse distance from the previous gamepad poll.
    double lastAccelerationDistance; // Keep track of the previous full distance from the previous gamepad poll
    double currentAccelMulti; // Multiplier and time used for acceleration easing.
    double accelDuration;
    double oldAccelMulti;
    double accelTravel; // Track travel when accel started
    double updateOldAccelMulti;
    double currentMouseDistance; // Keep track of the current mouse distance after a poll. Used to update lastMouseDistance later.
    double currentAccelerationDistance; // Keep track of the current mouse distance after a poll. Used to update lastMouseDistance later.
    double startingAccelerationDistance; // Take into account when mouse acceleration started
    double minMouseDistanceAccelThreshold;
    double maxMouseDistanceAccelThreshold;
    double startAccelMultiplier;
    double easingDuration;
    double extraAccelerationMultiplier;

    QTimer pauseTimer;
    QTimer holdTimer;
    QTimer pauseWaitTimer;
    QTimer createDeskTimer;
    QTimer releaseDeskTimer;
    QTimer setChangeTimer;
    QTimer keyPressTimer;
    QTimer delayTimer;
    QTimer slotSetChangeTimer;
    QTimer activeZoneTimer;
    static QTimer staticMouseEventTimer;

    QString customName;
    QString actionName;
    QString activeZoneString;

    QList<JoyButtonSlot*> assignments;
    QList<JoyButtonSlot*> activeSlots;
    QListIterator<JoyButtonSlot*> *slotiter;
    QQueue<JoyButtonSlot*> mouseEventQueue;
    JoyButtonSlot *currentPause;
    JoyButtonSlot *currentHold;
    JoyButtonSlot *currentCycle;
    JoyButtonSlot *previousCycle;
    JoyButtonSlot *currentDistance;
    JoyButtonSlot *currentMouseEvent;
    JoyButtonSlot *currentRelease;
    JoyButtonSlot *currentKeyPress;
    JoyButtonSlot *currentDelay;
    JoyButtonSlot *currentSetChangeSlot;

    QTime buttonHold;
    QTime pauseHold;
    QTime inpauseHold;
    QTime buttonHeldRelease;
    QTime keyPressHold;
    QTime buttonDelay;
    QTime accelExtraDurationTime;
    QTime cycleResetHold;
    static QTime testOldMouseTime;

    VDPad *vdpad;
    JoyMouseMovementMode mouseMode;
    JoyMouseCurve mouseCurve;
    JoyExtraAccelerationCurve extraAccelCurve;

    QReadWriteLock activeZoneLock;
    QReadWriteLock assignmentsLock;
    QReadWriteLock activeZoneStringLock;

};


#endif // JOYBUTTON_H
