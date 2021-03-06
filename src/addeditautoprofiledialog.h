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

#ifndef ADDEDITAUTOPROFILEDIALOG_H
#define ADDEDITAUTOPROFILEDIALOG_H

#include <QDialog>

#include <QtGlobal>

class AutoProfileInfo;
class AntiMicroSettings;
class InputDevice;
class CapturedWindowInfoDialog;
class UnixCaptureWindowUtility;

namespace Ui {
class AddEditAutoProfileDialog;
}

class AddEditAutoProfileDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddEditAutoProfileDialog(AutoProfileInfo *info, AntiMicroSettings *settings, QList<InputDevice*> *devices,
                                      QList<QString> &reservedGUIDS,
                                      bool edit=false, QWidget *parent = nullptr);
    ~AddEditAutoProfileDialog();

    AutoProfileInfo* getAutoProfile() const;
    QString getOriginalGUID() const;
    QString getOriginalExe() const;
    QString getOriginalWindowClass() const;
    QString getOriginalWindowName() const;

    QList<InputDevice*> *getDevices() const;
    AntiMicroSettings *getSettings() const;
    bool getEditForm() const;
    bool getDefaultInfo() const;
    QList<QString> const& getReservedGUIDs();


protected:
    virtual void accept();

signals:
    void captureFinished();

private slots:
    void openProfileBrowseDialog();
    void openApplicationBrowseDialog();
    void saveAutoProfileInformation();
    void checkForReservedGUIDs(int index);
    void checkForDefaultStatus();
    void windowPropAssignment(CapturedWindowInfoDialog *dialog);

#ifdef Q_OS_WIN
    void openWinAppProfileDialog();
    void captureWindowsApplicationPath();
#elif defined(Q_OS_UNIX)
    void showCaptureHelpWindow();
    void checkForGrabbedWindow(UnixCaptureWindowUtility* util);
#endif

    void on_setPartialCheckBox_stateChanged(int arg1);

private:
    Ui::AddEditAutoProfileDialog *ui;

    AutoProfileInfo *info;
    QList<InputDevice*> *devices;
    AntiMicroSettings *settings;
    bool editForm;
    bool defaultInfo;
    QList<QString> reservedGUIDs;
    QString originalGUID;
    QString originalExe;
    QString originalWindowClass;
    QString originalWindowName;

};

#endif // ADDEDITAUTOPROFILEDIALOG_H
