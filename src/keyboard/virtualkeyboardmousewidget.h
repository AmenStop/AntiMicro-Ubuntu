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

#ifndef VIRTUALKEYBOARDMOUSEWIDGET_H
#define VIRTUALKEYBOARDMOUSEWIDGET_H

#include <QObject>
#include <QTabWidget>
#include <QHash>

class JoyButton;
class JoyButtonSlot;
class VirtualKeyPushButton;

class QVBoxLayout;
class QPushButton;
class QResizeEvent;
class QWidget;
class QMenu;
class InputDevice;
class QuickSetDialog;
class ButtonEditDialogHelper;

class VirtualKeyboardMouseWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit VirtualKeyboardMouseWidget(InputDevice *joystick, ButtonEditDialogHelper* helper, QuickSetDialog* quickSetDialog = nullptr, QWidget *parent = nullptr);
    explicit VirtualKeyboardMouseWidget(QWidget *parent = nullptr);
    bool isKeyboardTabVisible();
    bool is_numlock_activated();
    bool isLaptop();

    InputDevice *getJoystick() const;
    ButtonEditDialogHelper* getHelper() const;
    QWidget *getKeyboardTab() const;
    QWidget *getMouseTab() const;
    QPushButton *getNoneButton() const;
    QPushButton *getMouseSettingsPushButton() const;
    QMenu *getOtherKeysMenu() const;
    QuickSetDialog* getCurrentQuickDialog() const;

    static QHash<QString, QString> topRowKeys;

protected:
    void setupVirtualKeyboardLayout();
    QVBoxLayout* setupMainKeyboardLayout();
    QVBoxLayout* setupAuxKeyboardLayout();
    QVBoxLayout* setupKeyboardNumPadLayout();

    void setupMouseControlLayout();
    VirtualKeyPushButton* createNewKey(QString xcodestring);
    QPushButton* createNoneKey();
    void populateTopRowKeys();
    QPushButton* createOtherKeysMenu();

    virtual void resizeEvent(QResizeEvent *event);


signals:
    void selectionFinished();
    void selectionCleared();
    void selectionMade(int keycode, int alias); // (.., unsigned)
    void selectionMade(JoyButtonSlot *slot);
    void buttonDialogClosed();

public slots:
    void establishVirtualKeyboardSingleSignalConnections();
    void establishVirtualMouseSignalConnections();
    void establishVirtualKeyboardAdvancedSignalConnections();
    void establishVirtualMouseAdvancedSignalConnections();

private slots:
    void processSingleKeyboardSelection(int keycode, int alias); // (.., unsigned)
    void processAdvancedKeyboardSelection(int keycode, int alias); // (.., unsigned)
    void processSingleMouseSelection(JoyButtonSlot *tempslot);
    void processAdvancedMouseSelection(JoyButtonSlot *tempslot);
    void clearButtonSlots();
    void clearButtonSlotsFinish();
    void openMouseSettingsDialog();
    void enableMouseSettingButton();
    void setButtonFontSizes();
    void otherKeysActionSingle(QAction* action, bool triggered);
    void otherKeysActionAdvanced(QAction* action, bool triggered);
    void nullifyDialogPointer();

private:
    bool isLaptopDevice;
    InputDevice *joystick;
    ButtonEditDialogHelper* helper;
    QWidget *keyboardTab;
    QWidget *mouseTab;
    QPushButton *noneButton;
    QPushButton *mouseSettingsPushButton;
    QMenu *otherKeysMenu;
    QuickSetDialog* currentQuickDialog;


};

#endif // VIRTUALKEYBOARDMOUSEWIDGET_H
