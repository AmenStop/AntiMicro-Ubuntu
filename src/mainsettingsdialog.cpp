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

#include "mainsettingsdialog.h"
#include "ui_mainsettingsdialog.h"

#include "messagehandler.h"
#include "addeditautoprofiledialog.h"
#include "editalldefaultautoprofiledialog.h"
#include "common.h"
#include "autoprofileinfo.h"
#include "inputdevice.h"
#include "antimicrosettings.h"

#ifdef Q_OS_WIN
  #include "eventhandlerfactory.h"
  #include "winextras.h"
#elif defined(Q_OS_UNIX)
  #include "x11extras.h"
#endif

#include <QTableWidgetItem>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QLocale>
#include <QTranslator>
#include <QListIterator>
#include <QStringList>
#include <QTableWidgetItem>
#include <QMapIterator>
#include <QVariant>
#include <QMessageBox>
#include <QDesktopWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QList>
#include <QListWidget>
#include <QWidget>

#ifdef Q_OS_WIN
  #include <QSysInfo>
#endif

#ifdef Q_OS_UNIX

  #include <QApplication>
  #include "eventhandlerfactory.h"

#endif



static const QString RUNATSTARTUPREGKEY(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run");
static const QString RUNATSTARTUPLOCATION(
        QString("%0\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\antimicro.lnk")
        .arg(QString::fromUtf8(qgetenv("AppData"))));

MainSettingsDialog::MainSettingsDialog(AntiMicroSettings *settings,
                                       QList<InputDevice *> *devices,
                                       QWidget *parent) :
    QDialog(parent, Qt::Dialog),
    ui(new Ui::MainSettingsDialog)
{
    ui->setupUi(this);

    qInstallMessageHandler(MessageHandler::myMessageOutput);
    setAttribute(Qt::WA_DeleteOnClose);

    ui->profileOpenDirPushButton->setIcon(QIcon::fromTheme("document-open-folder",
                                                           QIcon(":/icons/16x16/actions/document-open-folder.png")));

    ui->logFilePushButton->setIcon(QIcon::fromTheme("document-open-folder",
						    QIcon(":/icons/16x16/actions/document-open-folder.png")));
    
    this->settings = settings;
    this->allDefaultProfile = nullptr;
    this->connectedDevices = devices;

    fillControllerMappingsTable();

    settings->getLock()->lock();

    QString defaultProfileDir = settings->value("DefaultProfileDir", "").toString();
    int numberRecentProfiles = settings->value("NumberRecentProfiles", 5).toInt();
    bool closeToTray = settings->value("CloseToTray", false).toBool();

    if (!defaultProfileDir.isEmpty() && QDir(defaultProfileDir).exists())
    {
        ui->profileDefaultDirLineEdit->setText(defaultProfileDir);
    }
    else
    {
        ui->profileDefaultDirLineEdit->setText(PadderCommon::preferredProfileDir(settings));
    }

    ui->numberRecentProfileSpinBox->setValue(numberRecentProfiles);

    if (closeToTray)
    {
        ui->closeToTrayCheckBox->setChecked(true);
    }

    changePresetLanguage();

#ifdef Q_OS_WIN
    ui->autoProfileTableWidget->hideColumn(3);
#endif

    ui->autoProfileTableWidget->hideColumn(7);

#ifdef Q_OS_UNIX
    #if defined(USE_SDL_2) && defined(WITH_X11)
    if (QApplication::platformName() == QStringLiteral("xcb"))
    {
    populateAutoProfiles();
    fillAllAutoProfilesTable();
    fillGUIDComboBox();
    }
    else
    {
        delete ui->categoriesListWidget->item(2);
        ui->stackedWidget->removeWidget(ui->page_2);
    }
    #elif defined(USE_SDL_2) && !defined(WITH_X11)
    delete ui->categoriesListWidget->item(2);
    ui->stackedWidget->removeWidget(ui->page_2);

    #elif !defined(USE_SDL_2)
    delete ui->categoriesListWidget->item(2);
    delete ui->categoriesListWidget->item(1);
    ui->stackedWidget->removeWidget(ui->controllerMappingsPage);
    ui->stackedWidget->removeWidget(ui->page_2);
    #endif
#elif defined(Q_OS_WIN)
    populateAutoProfiles();
    fillAllAutoProfilesTable();
    fillGUIDComboBox();
#endif


    QString autoProfileActive = settings->value("AutoProfiles/AutoProfilesActive", "").toString();
    if (autoProfileActive == "1")
    {
        ui->activeCheckBox->setChecked(true);
        ui->autoProfileTableWidget->setEnabled(true);
        ui->autoProfileAddPushButton->setEnabled(true);
    }

#ifdef Q_OS_WIN
    BaseEventHandler *handler = EventHandlerFactory::getInstance()->handler();

    if (QSysInfo::windowsVersion() >= QSysInfo::WV_VISTA)
    {
        // Handle Windows Vista and later
        QFile tempFile(RUNATSTARTUPLOCATION);
        if (tempFile.exists())
        {
            ui->launchAtWinStartupCheckBox->setChecked(true);
        }
    }
    else
    {
        // Handle Windows XP
        QSettings autoRunReg(RUNATSTARTUPREGKEY, QSettings::NativeFormat);
        QString autoRunEntry = autoRunReg.value("antimicro", "").toString();
        if (!autoRunEntry.isEmpty())
        {
            ui->launchAtWinStartupCheckBox->setChecked(true);
        }
    }

    if ((handler != nullptr) && (handler->getIdentifier() == "sendinput"))
    {
        bool keyRepeatEnabled = settings->value("KeyRepeat/KeyRepeatEnabled", true).toBool();
        if (keyRepeatEnabled)
        {
            ui->keyRepeatEnableCheckBox->setChecked(true);
            ui->keyDelayHorizontalSlider->setEnabled(true);
            ui->keyDelaySpinBox->setEnabled(true);
            ui->keyRateHorizontalSlider->setEnabled(true);
            ui->keyRateSpinBox->setEnabled(true);
        }

        int keyRepeatDelay = settings->value("KeyRepeat/KeyRepeatDelay", InputDevice::DEFAULTKEYREPEATDELAY).toInt();
        int keyRepeatRate = settings->value("KeyRepeat/KeyRepeatRate", InputDevice::DEFAULTKEYREPEATRATE).toInt();

        ui->keyDelayHorizontalSlider->setValue(keyRepeatDelay);
        ui->keyDelaySpinBox->setValue(keyRepeatDelay);

        ui->keyRateHorizontalSlider->setValue(1000/keyRepeatRate);
        ui->keyRateSpinBox->setValue(1000/keyRepeatRate);
    }
    else
    {
        ui->keyRepeatGroupBox->setVisible(false);
    }

#elif defined(Q_OS_UNIX)
    ui->launchAtWinStartupCheckBox->setVisible(false);
    ui->keyRepeatGroupBox->setVisible(false);

#endif

    bool useSingleProfileList = settings->value("TrayProfileList", false).toBool();
    if (useSingleProfileList)
    {
        ui->traySingleProfileListCheckBox->setChecked(true);
    }

    bool minimizeToTaskBar = settings->value("MinimizeToTaskbar", false).toBool();
    if (minimizeToTaskBar)
    {
        ui->minimizeTaskbarCheckBox->setChecked(true);
    }

    bool hideEmpty = settings->value("HideEmptyButtons", false).toBool();
    if (hideEmpty)
    {
        ui->hideEmptyCheckBox->setChecked(true);
    }

    bool autoOpenLastProfile = settings->value("AutoOpenLastProfile", true).toBool();
    if (autoOpenLastProfile)
    {
        ui->autoLoadPreviousCheckBox->setChecked(true);
    }
    else
    {
        ui->autoLoadPreviousCheckBox->setChecked(false);
    }

    bool launchInTray = settings->value("LaunchInTray", false).toBool();
    if (launchInTray)
    {
        ui->launchInTrayCheckBox->setChecked(true);
    }

#ifdef Q_OS_WIN
    bool associateProfiles = settings->value("AssociateProfiles", true).toBool();
    if (associateProfiles)
    {
        ui->associateProfilesCheckBox->setChecked(true);
    }
    else
    {
        ui->associateProfilesCheckBox->setChecked(false);
    }
#elif defined(Q_OS_UNIX)
    ui->associateProfilesCheckBox->setVisible(false);
#endif

#ifdef Q_OS_WIN
    bool disableEnhancedMouse = settings->value("Mouse/DisableWinEnhancedPointer", false).toBool();
    if (disableEnhancedMouse)
    {
        ui->disableWindowsEnhancedPointCheckBox->setChecked(true);
    }
#elif defined(Q_OS_UNIX)
    ui->disableWindowsEnhancedPointCheckBox->setVisible(false);
#endif

    bool smoothingEnabled = settings->value("Mouse/Smoothing", false).toBool();
    if (smoothingEnabled)
    {
        ui->smoothingEnableCheckBox->setChecked(true);
        ui->historySizeSpinBox->setEnabled(true);
        ui->weightModifierDoubleSpinBox->setEnabled(true);
    }

    int historySize = settings->value("Mouse/HistorySize", 0).toInt();
    if ((historySize > 0) && (historySize <= JoyButton::MAXIMUMMOUSEHISTORYSIZE))
    {
        ui->historySizeSpinBox->setValue(historySize);
    }

    double weightModifier = settings->value("Mouse/WeightModifier", 0).toDouble();
    if ((weightModifier > 0.0) && (weightModifier <= JoyButton::MAXIMUMWEIGHTMODIFIER))
    {
        ui->weightModifierDoubleSpinBox->setValue(weightModifier);
    }

    for (int i = 1; i <= JoyButton::MAXIMUMMOUSEREFRESHRATE; i++)
    {
        ui->mouseRefreshRateComboBox->addItem(QString("%1 ms").arg(i), i);
    }

    int refreshIndex = ui->mouseRefreshRateComboBox->findData(JoyButton::getMouseRefreshRate());
    if (refreshIndex >= 0)
    {
        ui->mouseRefreshRateComboBox->setCurrentIndex(refreshIndex);
    }

#ifdef Q_OS_WIN
    QString tempTooltip = ui->mouseRefreshRateComboBox->toolTip();
    tempTooltip.append("\n\n");
    tempTooltip.append(trUtf8("Also, Windows users who want to use a low value should also check the\n"
                          "\"Disable Enhance Pointer Precision\" checkbox if you haven't disabled\n"
                          "the option in Windows."));
    ui->mouseRefreshRateComboBox->setToolTip(tempTooltip);
#endif

    fillSpringScreenPresets();

    for (int i=1; i <= 16; i++)
    {
        ui->gamepadPollRateComboBox->addItem(QString("%1 ms").arg(i), QVariant(i));
    }

    int gamepadPollIndex = ui->gamepadPollRateComboBox->findData(JoyButton::getGamepadRefreshRate());
    if (gamepadPollIndex >= 0)
    {
        ui->gamepadPollRateComboBox->setCurrentIndex(gamepadPollIndex);
    }

#ifdef Q_OS_UNIX
    if (QApplication::platformName() == QStringLiteral("xcb"))
    {
        refreshExtraMouseInfo();
    }
    else
    {
        ui->extraInfoFrame->hide();
    }
#else
    ui->extraInfoFrame->hide();
#endif

    // Begin Advanced Tab
    QString curLogFile = settings->value("LogFile", "").toString();
    int logLevel = settings->value("LogLevel", Logger::LOG_NONE).toInt();

    if( !curLogFile.isEmpty() ) {
      ui->logFilePathEdit->setText(curLogFile);
    }

    ui->logLevelComboBox->setCurrentIndex( logLevel );
    // End Advanced Tab

    settings->getLock()->unlock();

    connect(ui->categoriesListWidget, &QListWidget::currentRowChanged, ui->stackedWidget, &QStackedWidget::setCurrentIndex);
    connect(ui->controllerMappingsTableWidget, &QTableWidget::itemChanged, this, &MainSettingsDialog::mappingsTableItemChanged);
    connect(ui->mappingDeletePushButton, &QPushButton::clicked, this, &MainSettingsDialog::deleteMappingRow);
    connect(ui->mappngInsertPushButton, &QPushButton::clicked, this, &MainSettingsDialog::insertMappingRow);
    connect(this, &MainSettingsDialog::accepted, this, &MainSettingsDialog::saveNewSettings);
    connect(ui->profileOpenDirPushButton, &QPushButton::clicked, this, &MainSettingsDialog::selectDefaultProfileDir);
    connect(ui->activeCheckBox, &QCheckBox::toggled, ui->autoProfileTableWidget, &QTableWidget::setEnabled);
    connect(ui->activeCheckBox, &QCheckBox::toggled, this, &MainSettingsDialog::autoProfileButtonsActiveState);
    connect(ui->devicesComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &MainSettingsDialog::changeDeviceForProfileTable);
    connect(ui->autoProfileTableWidget, &QTableWidget::itemChanged, this, &MainSettingsDialog::processAutoProfileActiveClick);
    connect(ui->autoProfileAddPushButton, &QPushButton::clicked, this, &MainSettingsDialog::openAddAutoProfileDialog);
    connect(ui->autoProfileDeletePushButton, &QPushButton::clicked, this, &MainSettingsDialog::openDeleteAutoProfileConfirmDialog);
    connect(ui->autoProfileEditPushButton, &QPushButton::clicked, this, &MainSettingsDialog::openEditAutoProfileDialog);
    connect(ui->autoProfileTableWidget, &QTableWidget::itemSelectionChanged, this, &MainSettingsDialog::changeAutoProfileButtonsState);

    connect(ui->keyRepeatEnableCheckBox, &QCheckBox::clicked, this, &MainSettingsDialog::changeKeyRepeatWidgetsStatus);
    connect(ui->keyDelayHorizontalSlider, &QSlider::valueChanged, ui->keyDelaySpinBox, &QSpinBox::setValue);
    connect(ui->keyDelaySpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui->keyDelayHorizontalSlider, &QSlider::setValue);
    connect(ui->keyRateHorizontalSlider, &QSlider::valueChanged, ui->keyRateSpinBox, &QSpinBox::setValue);
    connect(ui->keyRateSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), ui->keyRateHorizontalSlider, &QSlider::setValue);

    connect(ui->smoothingEnableCheckBox, &QCheckBox::toggled, this, &MainSettingsDialog::checkSmoothingWidgetStatus);
    connect(ui->resetAccelPushButton, &QPushButton::clicked, this, &MainSettingsDialog::resetMouseAcceleration);

    // Advanced Tab
    connect(ui->logFilePushButton, &QPushButton::clicked, this, &MainSettingsDialog::selectLogFile);
}

MainSettingsDialog::~MainSettingsDialog()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    delete ui;
    if (connectedDevices != nullptr)
    {
        delete connectedDevices;
        connectedDevices = nullptr;
    }
}

void MainSettingsDialog::fillControllerMappingsTable()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    ui->controllerMappingsTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QHash<QString, QList<QVariant> > tempHash;

    settings->getLock()->lock();
    settings->beginGroup("Mappings");

    QStringList mappings = settings->allKeys();
    QStringListIterator iter(mappings);
    while (iter.hasNext())
    {
        QString tempkey = iter.next();
        QString tempGUID = QString();

        if (tempkey.contains("Disable"))
        {
            bool disableGameController = settings->value(tempkey, false).toBool();
            tempGUID = tempkey.remove("Disable");
            insertTempControllerMapping(tempHash, tempGUID);
            if (tempHash.contains(tempGUID))
            {
                QList<QVariant> templist = tempHash.value(tempGUID);
                templist.replace(2, QVariant(disableGameController));
                tempHash.insert(tempGUID, templist); // Overwrite original list
            }
        }
        else
        {
            QString mappingString = settings->value(tempkey, QString()).toString();
            if (!mappingString.isEmpty())
            {
                tempGUID = tempkey;
                insertTempControllerMapping(tempHash, tempGUID);
                if (tempHash.contains(tempGUID))
                {
                    QList<QVariant> templist = tempHash.value(tempGUID);
                    templist.replace(1, mappingString);
                    tempHash.insert(tempGUID, templist); // Overwrite original list
                }
            }
        }
    }

    settings->endGroup();
    settings->getLock()->unlock();

    QHashIterator<QString, QList<QVariant> > iter2(tempHash);
    int i = 0;
    while (iter2.hasNext())
    {
        ui->controllerMappingsTableWidget->insertRow(i);

        QList<QVariant> templist = iter2.next().value();
        QTableWidgetItem* item = new QTableWidgetItem(templist.at(0).toString());
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setData(Qt::UserRole, iter2.key());
        item->setToolTip(templist.at(0).toString());
        ui->controllerMappingsTableWidget->setItem(i, 0, item);

        item = new QTableWidgetItem(templist.at(1).toString());
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setData(Qt::UserRole, iter2.key());

        ui->controllerMappingsTableWidget->setItem(i, 1, item);

        bool disableController = templist.at(2).toBool();
        item = new QTableWidgetItem();
        item->setCheckState(disableController ? Qt::Checked : Qt::Unchecked);
        item->setData(Qt::UserRole, iter2.key());
        ui->controllerMappingsTableWidget->setItem(i, 2, item);

        i++;
    }
}

void MainSettingsDialog::insertTempControllerMapping(QHash<QString, QList<QVariant> > &hash, QString newGUID)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    if (!newGUID.isEmpty() && !hash.contains(newGUID))
    {
        QList<QVariant> templist;
        templist.append(QVariant(newGUID));
        templist.append(QVariant(""));
        templist.append(QVariant(false));

        hash.insert(newGUID, templist);
    }
}

void MainSettingsDialog::mappingsTableItemChanged(QTableWidgetItem *item)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    int column = item->column();
    int row = item->row();

    if ((column == 0) && !item->text().isEmpty())
    {
        QTableWidgetItem *disableitem = ui->controllerMappingsTableWidget->item(row, column);
        if (disableitem != nullptr)
        {
            disableitem->setData(Qt::UserRole, item->text());
        }

        item->setData(Qt::UserRole, item->text());
    }
}

void MainSettingsDialog::insertMappingRow()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    int insertRowIndex = ui->controllerMappingsTableWidget->rowCount();
    ui->controllerMappingsTableWidget->insertRow(insertRowIndex);

    QTableWidgetItem* item = new QTableWidgetItem();

    ui->controllerMappingsTableWidget->setItem(insertRowIndex, 0, item);

    item = new QTableWidgetItem();
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
;
    ui->controllerMappingsTableWidget->setItem(insertRowIndex, 1, item);

    item = new QTableWidgetItem();
    item->setCheckState(Qt::Unchecked);
    ui->controllerMappingsTableWidget->setItem(insertRowIndex, 2, item);
}

void MainSettingsDialog::deleteMappingRow()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    int row = ui->controllerMappingsTableWidget->currentRow();

    if (row >= 0)
    {
        ui->controllerMappingsTableWidget->removeRow(row);
    }
}

void MainSettingsDialog::syncMappingSettings()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    settings->getLock()->lock();

    settings->beginGroup("Mappings");
    settings->remove("");

    for (int i=0; i < ui->controllerMappingsTableWidget->rowCount(); i++)
    {
        QTableWidgetItem *itemGUID = ui->controllerMappingsTableWidget->item(i, 0);
        QTableWidgetItem *itemMapping = ui->controllerMappingsTableWidget->item(i, 1);
        QTableWidgetItem *itemDisable = ui->controllerMappingsTableWidget->item(i, 2);

        if ((itemGUID != nullptr) && !itemGUID->text().isEmpty() && (itemDisable != nullptr))
        {
            QString disableController = (itemDisable->checkState() == Qt::Checked) ? "1" : "0";
            if ((itemMapping != nullptr) && !itemMapping->text().isEmpty())
            {
                settings->setValue(itemGUID->text(), itemMapping->text());
            }

            settings->setValue(QString("%1Disable").arg(itemGUID->text()), disableController);
        }
    }

    settings->endGroup();
    settings->getLock()->unlock();
}

void MainSettingsDialog::saveNewSettings()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

#if defined(USE_SDL_2)
    syncMappingSettings();
#endif

    settings->getLock()->lock();
    QString oldProfileDir = settings->value("DefaultProfileDir", "").toString();
    QString possibleProfileDir = ui->profileDefaultDirLineEdit->text();
    bool closeToTray = ui->closeToTrayCheckBox->isChecked();

    if (oldProfileDir != possibleProfileDir)
    {
        if (QFileInfo(possibleProfileDir).exists())
        {
            settings->setValue("DefaultProfileDir", possibleProfileDir);
        }
        else if (possibleProfileDir.isEmpty())
        {
            settings->remove("DefaultProfileDir");
        }
    }

    int numRecentProfiles = ui->numberRecentProfileSpinBox->value();
    settings->setValue("NumberRecentProfiles", numRecentProfiles);

    if (closeToTray)
    {
        settings->setValue("CloseToTray", closeToTray ? "1" : "0");
    }
    else
    {
        settings->remove("CloseToTray");
    }
    settings->getLock()->unlock();

    checkLocaleChange();
#ifdef Q_OS_UNIX
    #if defined(USE_SDL_2) && defined(WITH_X11)

    if (QApplication::platformName() == QStringLiteral("xcb"))
    {

    saveAutoProfileSettings();

    }

    #endif
#elif defined(Q_OS_WIN)
    saveAutoProfileSettings();
#endif

    settings->getLock()->lock();
#ifdef Q_OS_WIN
    if (QSysInfo::windowsVersion() >= QSysInfo::WV_VISTA)
    {
        // Handle Windows Vista and later
        QFile tempFile(RUNATSTARTUPLOCATION);

        if (ui->launchAtWinStartupCheckBox->isChecked() && !tempFile.exists())
        {
            if (tempFile.open(QFile::WriteOnly))
            {
                QFile currentAppLocation(qApp->applicationFilePath());
                currentAppLocation.link(QFileInfo(tempFile).absoluteFilePath());
            }
        }
        else if (tempFile.exists() && QFileInfo(tempFile).isWritable())
        {
            tempFile.remove();
        }
    }
    else
    {
        // Handle Windows XP
        QSettings autoRunReg(RUNATSTARTUPREGKEY, QSettings::NativeFormat);
        QString autoRunEntry = autoRunReg.value("antimicro", "").toString();

        if (ui->launchAtWinStartupCheckBox->isChecked())
        {
            QString nativeFilePath = QDir::toNativeSeparators(qApp->applicationFilePath());
            autoRunReg.setValue("antimicro", nativeFilePath);
        }
        else if (!autoRunEntry.isEmpty())
        {
            autoRunReg.remove("antimicro");
        }
    }

    BaseEventHandler *handler = EventHandlerFactory::getInstance()->handler();

    if ((handler != nullptr) && (handler->getIdentifier() == "sendinput"))
    {
        settings->setValue("KeyRepeat/KeyRepeatEnabled", ui->keyRepeatEnableCheckBox->isChecked() ? "1" : "0");
        settings->setValue("KeyRepeat/KeyRepeatDelay", ui->keyDelaySpinBox->value());
        settings->setValue("KeyRepeat/KeyRepeatRate", 1000/ui->keyRateSpinBox->value());
    }

#endif

    if (ui->traySingleProfileListCheckBox->isChecked())
    {
        settings->setValue("TrayProfileList", "1");
    }
    else
    {
        settings->setValue("TrayProfileList", "0");
    }

    bool minimizeToTaskbar = ui->minimizeTaskbarCheckBox->isChecked();
    settings->setValue("MinimizeToTaskbar", minimizeToTaskbar ? "1" : "0");

    bool hideEmpty = ui->hideEmptyCheckBox->isChecked();
    settings->setValue("HideEmptyButtons", hideEmpty ? "1" : "0");

    bool autoOpenLastProfile = ui->autoLoadPreviousCheckBox->isChecked();
    settings->setValue("AutoOpenLastProfile", autoOpenLastProfile ? "1" : "0");

    bool launchInTray = ui->launchInTrayCheckBox->isChecked();
    settings->setValue("LaunchInTray", launchInTray ? "1" : "0");

#ifdef Q_OS_WIN
    bool associateProfiles = ui->associateProfilesCheckBox->isChecked();
    settings->setValue("AssociateProfiles", associateProfiles ? "1" : "0");

    bool associationExists = WinExtras::containsFileAssociationinRegistry();
    if (associateProfiles && !associationExists)
    {
        WinExtras::writeFileAssocationToRegistry();
    }
    else if (!associateProfiles && associationExists)
    {
        WinExtras::removeFileAssociationFromRegistry();
    }

    bool disableEnhancePoint = ui->disableWindowsEnhancedPointCheckBox->isChecked();
    bool oldEnhancedValue = settings->value("Mouse/DisableWinEnhancedPointer", false).toBool();
    bool usingEnhancedPointer = WinExtras::isUsingEnhancedPointerPrecision();
    settings->setValue("Mouse/DisableWinEnhancedPointer", disableEnhancePoint ? "1" : "0");

    if (disableEnhancePoint != oldEnhancedValue)
    {
        if (usingEnhancedPointer && disableEnhancePoint)
        {
            WinExtras::disablePointerPrecision();
        }
        else if (!usingEnhancedPointer && !disableEnhancePoint)
        {
            WinExtras::enablePointerPrecision();
        }
    }

#endif

    PadderCommon::lockInputDevices();

    if (connectedDevices->size() > 0)
    {
        InputDevice *tempDevice = connectedDevices->at(0);
        QMetaObject::invokeMethod(tempDevice, "haltServices", Qt::BlockingQueuedConnection);
    }


    bool smoothingEnabled = ui->smoothingEnableCheckBox->isChecked();
    int historySize = ui->historySizeSpinBox->value();
    double weightModifier = ui->weightModifierDoubleSpinBox->value();

    settings->setValue("Mouse/Smoothing", smoothingEnabled ? "1" : "0");
    if (smoothingEnabled)
    {
        if (historySize > 0)
        {
            JoyButton::setMouseHistorySize(historySize);
        }

        if (weightModifier)
        {
            JoyButton::setWeightModifier(weightModifier);
        }
    }
    else
    {
        JoyButton::setMouseHistorySize(1);
        JoyButton::setWeightModifier(0.0);
    }

    if (historySize > 0)
    {
        settings->setValue("Mouse/HistorySize", historySize);
    }

    if (weightModifier > 0.0)
    {
        settings->setValue("Mouse/WeightModifier", weightModifier);
    }

    int refreshIndex = ui->mouseRefreshRateComboBox->currentIndex();
    int mouseRefreshRate = ui->mouseRefreshRateComboBox->itemData(refreshIndex).toInt();
    if (mouseRefreshRate != JoyButton::getMouseRefreshRate())
    {
        settings->setValue("Mouse/RefreshRate", mouseRefreshRate);
        JoyButton::setMouseRefreshRate(mouseRefreshRate);
    }

    int springIndex = ui->springScreenComboBox->currentIndex();
    int springScreen = ui->springScreenComboBox->itemData(springIndex).toInt();
    JoyButton::setSpringModeScreen(springScreen);
    settings->setValue("Mouse/SpringScreen", QString::number(springScreen));

    int pollIndex = ui->gamepadPollRateComboBox->currentIndex();
    int gamepadPollRate = ui->gamepadPollRateComboBox->itemData(pollIndex).toUInt();
    if (gamepadPollRate != JoyButton::getGamepadRefreshRate())
    {
        JoyButton::setGamepadRefreshRate(gamepadPollRate);
        settings->setValue("GamepadPollRate", QString::number(gamepadPollRate));
    }

    // Advanced Tab
    settings->setValue("LogFile", ui->logFilePathEdit->text());
    int logLevel = ui->logLevelComboBox->currentIndex();
    if( logLevel < 0 ) {
      logLevel = 0;
    }
    if( Logger::LOG_MAX < logLevel ) {
      logLevel = Logger::LOG_MAX;
    }
    settings->setValue("LogLevel", logLevel);
    // End Advanced Tab

    PadderCommon::unlockInputDevices();

    settings->sync();
    settings->getLock()->unlock();
}

void MainSettingsDialog::selectDefaultProfileDir()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    QString lookupDir = PadderCommon::preferredProfileDir(settings);
    QString directory = QFileDialog::getExistingDirectory(this, trUtf8("Select Default Profile Directory"), lookupDir);
    if (!directory.isEmpty() && QFileInfo(directory).exists())
    {
        ui->profileDefaultDirLineEdit->setText(directory);
    }
}

void MainSettingsDialog::checkLocaleChange()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    settings->getLock()->lock();
    int row = ui->localeListWidget->currentRow();
    if (row == 0)
    {
        if (settings->contains("Language"))
        {
            settings->remove("Language");
        }

        settings->getLock()->unlock();
        emit changeLanguage(QLocale::system().name());
    }
    else
    {
        QString newLocale = "en";
        if (row == 1)
        {
            newLocale = "br";
        }
        else if (row == 2)
        {
            newLocale = "en";
        }
        else if (row == 3)
        {
            newLocale = "fr";
        }
        else if (row == 4)
        {
            newLocale = "de";
        }
        else if (row == 5)
        {
            newLocale = "it";
        }
        else if (row == 6)
        {
            newLocale = "ja";
        }
        else if (row == 7)
        {
            newLocale = "ru";
        }
        else if (row == 8)
        {
            newLocale = "sr";
        }
        else if (row == 9)
        {
            newLocale = "zh_CN";
        }
        else if (row == 10)
        {
            newLocale = "es";
        }
        else if (row == 11)
        {
            newLocale = "uk";
        }
        else if (row == 12)
        {
            newLocale = "pl";
        }

        settings->setValue("Language", newLocale);

        settings->getLock()->unlock();
        emit changeLanguage(newLocale);
    }
}

void MainSettingsDialog::populateAutoProfiles()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    exeAutoProfiles.clear();
    defaultAutoProfiles.clear();

    settings->beginGroup("DefaultAutoProfiles");
    QStringList registeredGUIDs = settings->value("GUIDs", QStringList()).toStringList();

    settings->endGroup();

    QString allProfile = settings->value(QString("DefaultAutoProfileAll/Profile"), "").toString();
    QString allActive = settings->value(QString("DefaultAutoProfileAll/Active"), "0").toString();
    QString partialTitle = settings->value(QString("DefaultAutoProfileAll/PartialTitle"), "").toString();

    bool defaultActive = allActive == "1" ? true : false;
    bool partialTitleBool = partialTitle == "1" ? true : false;
    allDefaultProfile = new AutoProfileInfo("all", allProfile, defaultActive, partialTitleBool, this);
    allDefaultProfile->setDefaultState(true);

    QStringListIterator iter(registeredGUIDs);
    while (iter.hasNext())
    {
        QString tempkey = iter.next();
        QString guid = tempkey;
        //QString guid = QString(tempkey).replace("GUID", "");

        QString profile = settings->value(QString("DefaultAutoProfile-%1/Profile").arg(guid), "").toString();
        QString active = settings->value(QString("DefaultAutoProfile-%1/Active").arg(guid), "0").toString();
        QString partialTitle = settings->value(QString("DefaultAutoProfile-%1/PartialTitle").arg(guid), "0").toString();
        bool partialTitleBool = partialTitle == "1" ? true : false;
        QString deviceName = settings->value(QString("DefaultAutoProfile-%1/DeviceName").arg(guid), "").toString();

        if (!guid.isEmpty() && !profile.isEmpty() && !deviceName.isEmpty())
        {
            bool profileActive = active == "1" ? true : false;
            if (!defaultAutoProfiles.contains(guid) && guid != "all")
            {
                AutoProfileInfo *info = new AutoProfileInfo(guid, profile, profileActive, partialTitleBool, this);
                info->setDefaultState(true);
                info->setDeviceName(deviceName);
                defaultAutoProfiles.insert(guid, info);
                defaultList.append(info);
                QList<AutoProfileInfo*> templist;
                templist.append(info);
                deviceAutoProfiles.insert(guid, templist);
            }
        }
    }

    settings->beginGroup("AutoProfiles");
    bool quitSearch = false;

    for (int i = 1; !quitSearch; i++)
    {
        QString exe = settings->value(QString("AutoProfile%1Exe").arg(i), "").toString();
        QString windowName = settings->value(QString("AutoProfile%1WindowName").arg(i), "").toString();
#ifdef Q_OS_UNIX
        QString windowClass = settings->value(QString("AutoProfile%1WindowClass").arg(i), "").toString();
#else
        QString windowClass = QString();
#endif

        QString guid = settings->value(QString("AutoProfile%1GUID").arg(i), "").toString();
        QString profile = settings->value(QString("AutoProfile%1Profile").arg(i), "").toString();
        QString active = settings->value(QString("AutoProfile%1Active").arg(i), 0).toString();
        QString partialTitle = settings->value(QString("AutoProfile%1PartialTitle").arg(i), 0).toString();
        bool partialTitleBool = partialTitle == "1" ? true : false;
        QString deviceName = settings->value(QString("AutoProfile%1DeviceName").arg(i), "").toString();

        // Check if all required elements exist. If not, assume that the end of the
        // list has been reached.
        if ((!exe.isEmpty() || !windowClass.isEmpty() || !windowName.isEmpty()) &&
            !guid.isEmpty())
        {
            bool profileActive = active == "1" ? true : false;
            AutoProfileInfo *info = new AutoProfileInfo(guid, profile, exe, profileActive, partialTitleBool, this);
            if (!deviceName.isEmpty())
            {
                info->setDeviceName(deviceName);
            }

            info->setWindowName(windowName);

#ifdef Q_OS_UNIX
            info->setWindowClass(windowClass);
#endif

            profileList.append(info);
            QList<AutoProfileInfo*> templist;
            if (guid != "all")
            {
                if (deviceAutoProfiles.contains(guid))
                {
                    templist = deviceAutoProfiles.value(guid);
                }
                templist.append(info);
                deviceAutoProfiles.insert(guid, templist);
            }
        }
        else
        {
            quitSearch = true;
        }
    }

    settings->endGroup();
}

void MainSettingsDialog::fillAutoProfilesTable(QString guid)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    for (int i = ui->autoProfileTableWidget->rowCount() - 1; i >= 0; i--)
    {
        ui->autoProfileTableWidget->removeRow(i);
    }

    ui->autoProfileTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    if (defaultAutoProfiles.contains(guid) ||
        deviceAutoProfiles.contains(guid))
    {
        int i = 0;

        AutoProfileInfo *defaultForGUID = nullptr;
        if (defaultAutoProfiles.contains(guid))
        {
            AutoProfileInfo *info = defaultAutoProfiles.value(guid);
            defaultForGUID = info;
            ui->autoProfileTableWidget->insertRow(i);

            QTableWidgetItem *item = new QTableWidgetItem();
            item->setCheckState(info->isActive() ? Qt::Checked : Qt::Unchecked);

            ui->autoProfileTableWidget->setItem(i, 0, item);

            QString deviceName = info->getDeviceName();
            QString guidDisplay = info->getGUID();

            if (!deviceName.isEmpty())
            {
                guidDisplay = QString("%1 ").arg(info->getDeviceName());
                guidDisplay.append(QString("(%1)").arg(info->getGUID()));
            }

            item = new QTableWidgetItem(guidDisplay);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            item->setData(Qt::UserRole, info->getGUID());
            item->setToolTip(info->getGUID());
            ui->autoProfileTableWidget->setItem(i, 1, item);

            QFileInfo profilePath(info->getProfileLocation());
            item = new QTableWidgetItem(profilePath.fileName());
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            item->setData(Qt::UserRole, info->getProfileLocation());
            item->setToolTip(info->getProfileLocation());
            ui->autoProfileTableWidget->setItem(i, 2, item);

            item = new QTableWidgetItem(info->getWindowClass());
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            item->setData(Qt::UserRole, info->getWindowClass());
            item->setToolTip(info->getWindowClass());
            ui->autoProfileTableWidget->setItem(i, 3, item);

            item = new QTableWidgetItem(info->getWindowName());
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            item->setData(Qt::UserRole, info->getWindowName());
            item->setToolTip(info->getWindowName());
            ui->autoProfileTableWidget->setItem(i, 4, item);

            QFileInfo exeInfo(info->getExe());
            item = new QTableWidgetItem(exeInfo.fileName());
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            item->setData(Qt::UserRole, info->getExe());
            item->setToolTip(info->getExe());
            ui->autoProfileTableWidget->setItem(i, 5, item);

            item = new QTableWidgetItem("Default");
            item->setData(Qt::UserRole, "default");
            ui->autoProfileTableWidget->setItem(i, 6, item);

            item = new QTableWidgetItem("Instance");
            item->setData(Qt::UserRole, QVariant::fromValue<AutoProfileInfo*>(info));
            ui->autoProfileTableWidget->setItem(i, 7, item);

            i++;
        }

        QListIterator<AutoProfileInfo*> iter(deviceAutoProfiles.value(guid));
        while (iter.hasNext())
        {
            AutoProfileInfo *info = iter.next();
            if (!defaultForGUID || info != defaultForGUID)
            {
                ui->autoProfileTableWidget->insertRow(i);

                QTableWidgetItem *item = new QTableWidgetItem();
                item->setCheckState(info->isActive() ? Qt::Checked : Qt::Unchecked);
                ui->autoProfileTableWidget->setItem(i, 0, item);

                QString deviceName = info->getDeviceName();
                QString guidDisplay = info->getGUID();

                if (!deviceName.isEmpty())
                {
                    guidDisplay = QString("%1 ").arg(info->getDeviceName());
                    guidDisplay.append(QString("(%1)").arg(info->getGUID()));
                }

                item = new QTableWidgetItem(guidDisplay);
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                item->setData(Qt::UserRole, info->getGUID());
                item->setToolTip(info->getGUID());
                ui->autoProfileTableWidget->setItem(i, 1, item);

                QFileInfo profilePath(info->getProfileLocation());
                item = new QTableWidgetItem(profilePath.fileName());
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                item->setData(Qt::UserRole, info->getProfileLocation());
                item->setToolTip(info->getProfileLocation());
                ui->autoProfileTableWidget->setItem(i, 2, item);

                item = new QTableWidgetItem(info->getWindowClass());
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                item->setData(Qt::UserRole, info->getWindowClass());
                item->setToolTip(info->getWindowClass());
                ui->autoProfileTableWidget->setItem(i, 3, item);

                item = new QTableWidgetItem(info->getWindowName());
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                item->setData(Qt::UserRole, info->getWindowName());
                item->setToolTip(info->getWindowName());
                ui->autoProfileTableWidget->setItem(i, 4, item);

                QFileInfo exeInfo(info->getExe());
                item = new QTableWidgetItem(exeInfo.fileName());
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                item->setData(Qt::UserRole, info->getExe());
                item->setToolTip(info->getExe());
                ui->autoProfileTableWidget->setItem(i, 5, item);

                item = new QTableWidgetItem("Instance");
                item->setData(Qt::UserRole, QVariant::fromValue<AutoProfileInfo*>(info));
                ui->autoProfileTableWidget->setItem(i, 7, item);

                i++;
            }
        }
    }
}

void MainSettingsDialog::clearAutoProfileData()
{
qInstallMessageHandler(MessageHandler::myMessageOutput);
}

void MainSettingsDialog::fillGUIDComboBox()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    ui->devicesComboBox->clear();
    ui->devicesComboBox->addItem(trUtf8("All"), QVariant("all"));
    QList<QString> guids = deviceAutoProfiles.keys();
    QListIterator<QString> iter(guids);
    while (iter.hasNext())
    {
        QString guid = iter.next();
        QList<AutoProfileInfo*> temp = deviceAutoProfiles.value(guid);
        if (temp.count() > 0)
        {
            QString deviceName = temp.first()->getDeviceName();
            if (!deviceName.isEmpty())
            {
                ui->devicesComboBox->addItem(deviceName, QVariant(guid));
            }
            else
            {
                ui->devicesComboBox->addItem(guid, QVariant(guid));
            }
        }
        else
        {
            ui->devicesComboBox->addItem(guid, QVariant(guid));
        }
    }
}

void MainSettingsDialog::changeDeviceForProfileTable(int index)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    disconnect(ui->autoProfileTableWidget, &QTableWidget::itemChanged, this, &MainSettingsDialog::processAutoProfileActiveClick);

    if (index == 0)
    {
        fillAllAutoProfilesTable();
    }
    else
    {
        QString guid = ui->devicesComboBox->itemData(index).toString();
        fillAutoProfilesTable(guid);
    }

    connect(ui->autoProfileTableWidget, &QTableWidget::itemChanged, this, &MainSettingsDialog::processAutoProfileActiveClick);
}

void MainSettingsDialog::saveAutoProfileSettings()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    settings->getLock()->lock();
    settings->beginGroup("DefaultAutoProfiles");
    QStringList defaultkeys = settings->allKeys();
    settings->endGroup();

    QStringListIterator iterDefaults(defaultkeys);
    while (iterDefaults.hasNext())
    {
        QString tempkey = iterDefaults.next();
        QString guid = QString(tempkey).replace("GUID", "");
        QString testkey = QString("DefaultAutoProfile-%1").arg(guid);
        settings->beginGroup(testkey);
        settings->remove("");
        settings->endGroup();
    }

    settings->beginGroup("DefaultAutoProfiles");
    settings->remove("");
    settings->endGroup();

    settings->beginGroup("DefaultAutoProfileAll");
    settings->remove("");
    settings->endGroup();

    settings->beginGroup("AutoProfiles");
    settings->remove("");
    settings->endGroup();

    if (allDefaultProfile)
    {
        QString profile = allDefaultProfile->getProfileLocation();
        QString defaultActive = allDefaultProfile->isActive() ? "1" : "0";
        settings->setValue(QString("DefaultAutoProfileAll/Profile"), profile);
        settings->setValue(QString("DefaultAutoProfileAll/Active"), defaultActive);
    }

    QMapIterator<QString, AutoProfileInfo*> iter(defaultAutoProfiles);
    QStringList registeredGUIDs = QStringList();
    while (iter.hasNext())
    {
        iter.next();
        QString guid = iter.key();
        registeredGUIDs.append(guid);
        AutoProfileInfo *info = iter.value();
        QString profileActive = info->isActive() ? "1" : "0";
        QString deviceName = info->getDeviceName();
        //settings->setValue(QString("DefaultAutoProfiles/GUID%1").arg(guid), guid);
        settings->setValue(QString("DefaultAutoProfile-%1/Profile").arg(guid), info->getProfileLocation());
        settings->setValue(QString("DefaultAutoProfile-%1/Active").arg(guid), profileActive);
        settings->setValue(QString("DefaultAutoProfile-%1/DeviceName").arg(guid), deviceName);
    }

    if (!registeredGUIDs.isEmpty())
    {
        settings->setValue("DefaultAutoProfiles/GUIDs", registeredGUIDs);
    }

    settings->beginGroup("AutoProfiles");
    QString autoActive = ui->activeCheckBox->isChecked() ? "1" : "0";
    settings->setValue("AutoProfilesActive", autoActive);

    QListIterator<AutoProfileInfo*> iterProfiles(profileList);
    int i = 1;
    while (iterProfiles.hasNext())
    {
        AutoProfileInfo *info = iterProfiles.next();
        QString defaultActive = info->isActive() ? "1" : "0";
        QString partialTitle = info->isPartialState() ? "1" : "0";
        if (!info->getExe().isEmpty())
        {
            settings->setValue(QString("AutoProfile%1Exe").arg(i), info->getExe());
        }

        if (!info->getWindowClass().isEmpty())
        {
            settings->setValue(QString("AutoProfile%1WindowClass").arg(i), info->getWindowClass());
        }

        if (!info->getWindowName().isEmpty())
        {
            settings->setValue(QString("AutoProfile%1WindowName").arg(i), info->getWindowName());
        }

        settings->setValue(QString("AutoProfile%1GUID").arg(i), info->getGUID());
        settings->setValue(QString("AutoProfile%1Profile").arg(i), info->getProfileLocation());
        settings->setValue(QString("AutoProfile%1Active").arg(i), defaultActive);
        settings->setValue(QString("AutoProfile%1PartialTitle").arg(i), partialTitle);
        settings->setValue(QString("AutoProfile%1DeviceName").arg(i), info->getDeviceName());
        i++;
    }
    settings->endGroup();
    settings->getLock()->unlock();
}

void MainSettingsDialog::fillAllAutoProfilesTable()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    for (int i = ui->autoProfileTableWidget->rowCount()-1; i >= 0; i--)
    {
        ui->autoProfileTableWidget->removeRow(i);
    }

    ui->autoProfileTableWidget->horizontalHeader()->setVisible(true);
    ui->autoProfileTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    ui->autoProfileTableWidget->hideColumn(7);

    int i = 0;

    AutoProfileInfo *info = allDefaultProfile;

    ui->autoProfileTableWidget->insertRow(i);
    QTableWidgetItem *item = new QTableWidgetItem();
    item->setCheckState(info->isActive() ? Qt::Checked : Qt::Unchecked);
    ui->autoProfileTableWidget->setItem(i, 0, item);

    QString deviceName = info->getDeviceName();
    QString guidDisplay = info->getGUID();
    if (!deviceName.isEmpty())
    {
        guidDisplay = QString("%1 ").arg(info->getDeviceName());
        guidDisplay.append(QString("(%1)").arg(info->getGUID()));
    }
    item = new QTableWidgetItem(guidDisplay);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setData(Qt::UserRole, info->getGUID());
    item->setToolTip(info->getGUID());
    ui->autoProfileTableWidget->setItem(i, 1, item);

    QFileInfo profilePath(info->getProfileLocation());
    item = new QTableWidgetItem(profilePath.fileName());
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setData(Qt::UserRole, info->getProfileLocation());
    item->setToolTip(info->getProfileLocation());
    ui->autoProfileTableWidget->setItem(i, 2, item);

    QFileInfo exeInfo(info->getExe());
    item = new QTableWidgetItem(exeInfo.fileName());
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setData(Qt::UserRole, info->getExe());
    item->setToolTip(info->getExe());
    ui->autoProfileTableWidget->setItem(i, 5, item);

    item = new QTableWidgetItem("Default");
    item->setData(Qt::UserRole, "default");
    ui->autoProfileTableWidget->setItem(i, 6, item);

    item = new QTableWidgetItem("Instance");
    item->setData(Qt::UserRole, QVariant::fromValue<AutoProfileInfo*>(info));
    ui->autoProfileTableWidget->setItem(i, 7, item);

    i++;

    QListIterator<AutoProfileInfo*> iterDefaults(defaultList);
    while (iterDefaults.hasNext())
    {
        AutoProfileInfo *info = iterDefaults.next();
        ui->autoProfileTableWidget->insertRow(i);

        QTableWidgetItem *item = new QTableWidgetItem();
        item->setCheckState(info->isActive() ? Qt::Checked : Qt::Unchecked);
        ui->autoProfileTableWidget->setItem(i, 0, item);

        QString deviceName = info->getDeviceName();
        QString guidDisplay = info->getGUID();
        if (!deviceName.isEmpty())
        {
            guidDisplay = QString("%1 ").arg(info->getDeviceName());
            guidDisplay.append(QString("(%1)").arg(info->getGUID()));
        }

        item = new QTableWidgetItem(guidDisplay);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setData(Qt::UserRole, info->getGUID());
        item->setToolTip(info->getGUID());
        ui->autoProfileTableWidget->setItem(i, 1, item);

        QFileInfo profilePath(info->getProfileLocation());
        item = new QTableWidgetItem(profilePath.fileName());
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setData(Qt::UserRole, info->getProfileLocation());
        item->setToolTip(info->getProfileLocation());
        ui->autoProfileTableWidget->setItem(i, 2, item);

        item = new QTableWidgetItem("Default");
        item->setData(Qt::UserRole, "default");
        ui->autoProfileTableWidget->setItem(i, 6, item);

        item = new QTableWidgetItem("Instance");
        item->setData(Qt::UserRole, QVariant::fromValue<AutoProfileInfo*>(info));
        ui->autoProfileTableWidget->setItem(i, 7, item);

        i++;
    }

    QListIterator<AutoProfileInfo*> iter(profileList);
    while (iter.hasNext())
    {
        AutoProfileInfo *info = iter.next();
        ui->autoProfileTableWidget->insertRow(i);

        QTableWidgetItem *item = new QTableWidgetItem();
        item->setCheckState(info->isActive() ? Qt::Checked : Qt::Unchecked);
        ui->autoProfileTableWidget->setItem(i, 0, item);

        QString deviceName = info->getDeviceName();
        QString guidDisplay = info->getGUID();
        if (!deviceName.isEmpty())
        {
            guidDisplay = QString("%1 ").arg(info->getDeviceName());
            guidDisplay.append(QString("(%1)").arg(info->getGUID()));
        }
        item = new QTableWidgetItem(guidDisplay);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setData(Qt::UserRole, info->getGUID());
        item->setToolTip(info->getGUID());
        ui->autoProfileTableWidget->setItem(i, 1, item);

        QFileInfo profilePath(info->getProfileLocation());
        item = new QTableWidgetItem(profilePath.fileName());
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setData(Qt::UserRole, info->getProfileLocation());
        item->setToolTip(info->getProfileLocation());
        ui->autoProfileTableWidget->setItem(i, 2, item);

        item = new QTableWidgetItem(info->getWindowClass());
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setData(Qt::UserRole, info->getWindowClass());
        item->setToolTip(info->getWindowClass());
        ui->autoProfileTableWidget->setItem(i, 3, item);

        item = new QTableWidgetItem(info->getWindowName());
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setData(Qt::UserRole, info->getWindowName());
        item->setToolTip(info->getWindowName());
        ui->autoProfileTableWidget->setItem(i, 4, item);

        QFileInfo exeInfo(info->getExe());
        item = new QTableWidgetItem(exeInfo.fileName());
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        item->setData(Qt::UserRole, info->getExe());
        item->setToolTip(info->getExe());
        ui->autoProfileTableWidget->setItem(i, 5, item);

        item = new QTableWidgetItem();
        item->setData(Qt::UserRole, "");
        ui->autoProfileTableWidget->setItem(i, 6, item);

        item = new QTableWidgetItem("Instance");
        item->setData(Qt::UserRole, QVariant::fromValue<AutoProfileInfo*>(info));
        ui->autoProfileTableWidget->setItem(i, 7, item);

        i++;
    }
}

void MainSettingsDialog::processAutoProfileActiveClick(QTableWidgetItem *item)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    if (item && (item->column() == 0))
    {
        QTableWidgetItem *infoitem = ui->autoProfileTableWidget->item(item->row(), 7);
        AutoProfileInfo *info = infoitem->data(Qt::UserRole).value<AutoProfileInfo*>();
        Qt::CheckState active = item->checkState();
        if (active == Qt::Unchecked)
        {
            info->setActive(false);
        }
        else if (active == Qt::Checked)
        {
            info->setActive(true);
        }
    }
}

void MainSettingsDialog::openAddAutoProfileDialog()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    QList<QString> reservedGUIDs = defaultAutoProfiles.keys();
    AutoProfileInfo *info = new AutoProfileInfo(this);
    AddEditAutoProfileDialog *dialog = new AddEditAutoProfileDialog(info, settings, connectedDevices, reservedGUIDs, false, this);
    connect(dialog, &AddEditAutoProfileDialog::accepted, this, [this, dialog] {
        addNewAutoProfile(dialog);
    });

    connect(dialog, &AddEditAutoProfileDialog::rejected, info, &AutoProfileInfo::deleteLater);
    dialog->show();
}

void MainSettingsDialog::openEditAutoProfileDialog()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    int selectedRow = ui->autoProfileTableWidget->currentRow();
    if (selectedRow >= 0)
    {
        QTableWidgetItem *item = ui->autoProfileTableWidget->item(selectedRow, 7);

        AutoProfileInfo *info = item->data(Qt::UserRole).value<AutoProfileInfo*>();
        if (info != allDefaultProfile)
        {
            QList<QString> reservedGUIDs = defaultAutoProfiles.keys();
            if (info->getGUID() != "all")
            {
                AutoProfileInfo *temp = defaultAutoProfiles.value(info->getGUID());
                if (info == temp)
                {
                    reservedGUIDs.removeAll(info->getGUID());
                }
            }
            AddEditAutoProfileDialog *dialog = new AddEditAutoProfileDialog(info, settings, connectedDevices, reservedGUIDs, true, this);
            connect(dialog, &AddEditAutoProfileDialog::accepted, this, [this, dialog] {
                transferEditsToCurrentTableRow(dialog);
            });

            dialog->show();
        }
        else
        {
            EditAllDefaultAutoProfileDialog *dialog = new EditAllDefaultAutoProfileDialog(info, settings, this);
            dialog->show();
            connect(dialog, &EditAllDefaultAutoProfileDialog::accepted, this, [this, dialog] {
                transferAllProfileEditToCurrentTableRow(dialog);
            });
        }
    }
}

void MainSettingsDialog::openDeleteAutoProfileConfirmDialog()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    QMessageBox msgBox;
    msgBox.setText(trUtf8("Are you sure you want to delete the profile?"));
    msgBox.setStandardButtons(QMessageBox::Discard | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();
    if (ret == QMessageBox::Discard)
    {
        int selectedRow = ui->autoProfileTableWidget->currentRow();
        if (selectedRow >= 0)
        {
            QTableWidgetItem *item = ui->autoProfileTableWidget->item(selectedRow, 7);
            AutoProfileInfo *info = item->data(Qt::UserRole).value<AutoProfileInfo*>();
            if (info->isCurrentDefault())
            {
                if (info->getGUID() == "all")
                {
                    delete allDefaultProfile;
                    allDefaultProfile = nullptr;
                }
                else if (defaultAutoProfiles.contains(info->getGUID()))
                {
                    defaultAutoProfiles.remove(info->getGUID());
                    defaultList.removeAll(info);
                    delete info;
                    info = nullptr;
                }
            }
            else
            {
                if (deviceAutoProfiles.contains(info->getGUID()))
                {
                    QList<AutoProfileInfo*> temp = deviceAutoProfiles.value(info->getGUID());
                    temp.removeAll(info);
                    deviceAutoProfiles.insert(info->getGUID(), temp);
                }

                profileList.removeAll(info);

                delete info;
                info = nullptr;
            }
        }
        ui->autoProfileTableWidget->removeRow(selectedRow);
    }
}

void MainSettingsDialog::changeAutoProfileButtonsState()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    int selectedRow = ui->autoProfileTableWidget->currentRow();

    if (selectedRow >= 0)
    {
        QTableWidgetItem *item = ui->autoProfileTableWidget->item(selectedRow, 7);

        AutoProfileInfo *info = item->data(Qt::UserRole).value<AutoProfileInfo*>();

        if (info == allDefaultProfile)
        {
            ui->autoProfileAddPushButton->setEnabled(true);
            ui->autoProfileEditPushButton->setEnabled(true);
            ui->autoProfileDeletePushButton->setEnabled(false);
        }
        else
        {
            ui->autoProfileAddPushButton->setEnabled(true);
            ui->autoProfileEditPushButton->setEnabled(true);
            ui->autoProfileDeletePushButton->setEnabled(true);
        }
    }
    else
    {
        ui->autoProfileAddPushButton->setEnabled(true);
        ui->autoProfileDeletePushButton->setEnabled(false);
        ui->autoProfileEditPushButton->setEnabled(false);
    }
}

void MainSettingsDialog::transferAllProfileEditToCurrentTableRow(EditAllDefaultAutoProfileDialog* dialog)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    AutoProfileInfo *info = dialog->getAutoProfile();
    allDefaultProfile = info;
    changeDeviceForProfileTable(0);
}

void MainSettingsDialog::transferEditsToCurrentTableRow(AddEditAutoProfileDialog *dialog)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    AutoProfileInfo *info = dialog->getAutoProfile();

    // Delete pointers to object that might be misplaced
    // due to an association change.
    QString oldGUID = dialog->getOriginalGUID();

    if (oldGUID != info->getGUID())
    {
        if (defaultAutoProfiles.value(oldGUID) == info)
        {
            defaultAutoProfiles.remove(oldGUID);
        }

        if (info->isCurrentDefault())
        {
            defaultAutoProfiles.insert(info->getGUID(), info);
        }
    }

    if ((oldGUID != info->getGUID()) && deviceAutoProfiles.contains(oldGUID))
    {
        QList<AutoProfileInfo*> temp = deviceAutoProfiles.value(oldGUID);
        temp.removeAll(info);
        if (temp.count() > 0)
        {
            deviceAutoProfiles.insert(oldGUID, temp);
        }
        else
        {
            deviceAutoProfiles.remove(oldGUID);
        }

        if (deviceAutoProfiles.contains(info->getGUID()))
        {
            QList<AutoProfileInfo*> temp2 = deviceAutoProfiles.value(oldGUID);
            if (!temp2.contains(info))
            {
                temp2.append(info);
                deviceAutoProfiles.insert(info->getGUID(), temp2);
            }
        }
        else if (info->getGUID().toLower() != "all")
        {
            QList<AutoProfileInfo*> temp2;
            temp2.append(info);
            deviceAutoProfiles.insert(info->getGUID(), temp2);
        }
    }
    else if ((oldGUID != info->getGUID()) && (info->getGUID().toLower() != "all"))
    {
        QList<AutoProfileInfo*> temp;
        temp.append(info);
        deviceAutoProfiles.insert(info->getGUID(), temp);
    }

    if (!info->isCurrentDefault())
    {
        defaultList.removeAll(info);

        if (!profileList.contains(info))
        {
            profileList.append(info);
        }
    }
    else
    {
        profileList.removeAll(info);

        if (!defaultList.contains(info))
        {
            defaultList.append(info);
        }
    }

    if (deviceAutoProfiles.contains(info->getGUID()))
    {
        QList<AutoProfileInfo*> temp2 = deviceAutoProfiles.value(info->getGUID());
        if (!temp2.contains(info))
        {
            temp2.append(info);
            deviceAutoProfiles.insert(info->getGUID(), temp2);
        }
    }
    else
    {
        QList<AutoProfileInfo*> temp2;
        temp2.append(info);
        deviceAutoProfiles.insert(info->getGUID(), temp2);
    }

    fillGUIDComboBox();
    int currentIndex = ui->devicesComboBox->currentIndex();
    changeDeviceForProfileTable(currentIndex);
}

void MainSettingsDialog::addNewAutoProfile(AddEditAutoProfileDialog *dialog)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    AutoProfileInfo *info = dialog->getAutoProfile();

    bool found = false;
    if (info->isCurrentDefault())
    {
        if (defaultAutoProfiles.contains(info->getGUID()))
        {
            found = true;
        }
    }

    if (!found)
    {
        if (info->isCurrentDefault())
        {
            if (!info->getGUID().isEmpty() && !info->getExe().isEmpty())
            {
                defaultAutoProfiles.insert(info->getGUID(), info);
                defaultList.append(info);
            }
        }
        else
        {
            if (!info->getGUID().isEmpty() &&
                !info->getExe().isEmpty())
            {
                profileList.append(info);

                if (info->getGUID() != "all")
                {
                    QList<AutoProfileInfo*> tempDevProfileList;
                    if (deviceAutoProfiles.contains(info->getGUID()))
                    {
                        tempDevProfileList = deviceAutoProfiles.value(info->getGUID());
                    }

                    tempDevProfileList.append(info);
                    deviceAutoProfiles.insert(info->getGUID(), tempDevProfileList);
                }
            }
        }

        fillGUIDComboBox();
        changeDeviceForProfileTable(ui->devicesComboBox->currentIndex());
    }
}

void MainSettingsDialog::autoProfileButtonsActiveState(bool enabled)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    if (enabled)
    {
        changeAutoProfileButtonsState();
    }
    else
    {
        ui->autoProfileAddPushButton->setEnabled(false);
        ui->autoProfileEditPushButton->setEnabled(false);
        ui->autoProfileDeletePushButton->setEnabled(false);
    }
}

void MainSettingsDialog::changeKeyRepeatWidgetsStatus(bool enabled)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    ui->keyDelayHorizontalSlider->setEnabled(enabled);
    ui->keyDelaySpinBox->setEnabled(enabled);
    ui->keyRateHorizontalSlider->setEnabled(enabled);
    ui->keyRateSpinBox->setEnabled(enabled);
}

void MainSettingsDialog::checkSmoothingWidgetStatus(bool enabled)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    if (enabled)
    {
        ui->historySizeSpinBox->setEnabled(true);
        ui->weightModifierDoubleSpinBox->setEnabled(true);
    }
    else
    {
        ui->historySizeSpinBox->setEnabled(false);
        ui->weightModifierDoubleSpinBox->setEnabled(false);
    }
}


void MainSettingsDialog::changePresetLanguage()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    if (settings->contains("Language"))
    {
        QString targetLang = settings->value("Language").toString();
        if (targetLang == "br")
        {
            ui->localeListWidget->setCurrentRow(1);
        }
        else if (targetLang == "en")
        {
            ui->localeListWidget->setCurrentRow(2);
        }
        else if (targetLang == "fr")
        {
            ui->localeListWidget->setCurrentRow(3);
        }
        else if (targetLang == "de")
        {
            ui->localeListWidget->setCurrentRow(4);
        }
        else if (targetLang == "it")
        {
            ui->localeListWidget->setCurrentRow(5);
        }
        else if (targetLang == "ja")
        {
            ui->localeListWidget->setCurrentRow(6);
        }
        else if (targetLang == "ru")
        {
            ui->localeListWidget->setCurrentRow(7);
        }
        else if (targetLang == "sr")
        {
            ui->localeListWidget->setCurrentRow(8);
        }
        else if (targetLang == "zh_CN")
        {
            ui->localeListWidget->setCurrentRow(9);
        }
        else if (targetLang == "es")
        {
            ui->localeListWidget->setCurrentRow(10);
        }
        else if (targetLang == "uk")
        {
            ui->localeListWidget->setCurrentRow(11);
        }
        else if (targetLang == "pl")
        {
            ui->localeListWidget->setCurrentRow(12);
        }
        else
        {
            ui->localeListWidget->setCurrentRow(0);
        }
    }
    else
    {
        ui->localeListWidget->setCurrentRow(0);
    }
}

void MainSettingsDialog::fillSpringScreenPresets()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    ui->springScreenComboBox->clear();
    ui->springScreenComboBox->addItem(trUtf8("Default"),
                                      QVariant(AntiMicroSettings::defaultSpringScreen));

    QDesktopWidget deskWid;
    for (int i=0; i < deskWid.screenCount(); i++)
    {
        ui->springScreenComboBox->addItem(QString(":%1").arg(i), QVariant(i));
    }

    int screenIndex = ui->springScreenComboBox->findData(JoyButton::getSpringModeScreen());
    if (screenIndex > -1)
    {
        ui->springScreenComboBox->setCurrentIndex(screenIndex);
    }
}

void MainSettingsDialog::refreshExtraMouseInfo()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

#if defined(Q_OS_UNIX) && defined(WITH_X11)
    QString handler = EventHandlerFactory::getInstance()->handler()->getIdentifier();
    if (QApplication::platformName() == QStringLiteral("xcb"))
    {
        struct X11Extras::ptrInformation temp;
        if (handler == "uinput")
        {
            temp = X11Extras::getInstance()->getPointInformation();
        }
        else if (handler == "xtest")
        {
            temp = X11Extras::getInstance()->getPointInformation(X11Extras::xtestMouseDeviceName);
        }

        if (temp.id >= 0)
        {
            ui->accelNumLabel->setText(QString::number(temp.accelNum));
            ui->accelDenomLabel->setText(QString::number(temp.accelDenom));
            ui->accelThresLabel->setText(QString::number(temp.threshold));
        }
    }

#endif
}

void MainSettingsDialog::resetMouseAcceleration()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

#if defined(Q_OS_UNIX) && defined(WITH_X11)
    if (QApplication::platformName() == QStringLiteral("xcb"))
    {
    X11Extras::getInstance()->x11ResetMouseAccelerationChange();
    refreshExtraMouseInfo();
    }
#endif
}

void MainSettingsDialog::selectLogFile() {

    qInstallMessageHandler(MessageHandler::myMessageOutput);

    QString oldLogFile = settings->value("LogFile", "").toString();
    QString newLogFile =
      QFileDialog::getSaveFileName(this, trUtf8("Save Log File As"), oldLogFile,
				   trUtf8("Log Files (*.log)"));
    if( !newLogFile.isEmpty() ) {
      ui->logFilePathEdit->setText(newLogFile);
    }
}

AntiMicroSettings *MainSettingsDialog::getSettings() const {

    return settings;
}

QMap<QString, AutoProfileInfo*>* MainSettingsDialog::getDefaultAutoProfiles() {

    return &defaultAutoProfiles;
}

QMap<QString, QList<AutoProfileInfo*> >* MainSettingsDialog::getDeviceAutoProfiles() {

    return &deviceAutoProfiles;
}

QMap<QString, QList<AutoProfileInfo*> >* MainSettingsDialog::getExeAutoProfiles() {

    return &exeAutoProfiles;
}

QList<AutoProfileInfo*>* MainSettingsDialog::getDefaultList() {

    return &defaultList;
}

QList<AutoProfileInfo*>* MainSettingsDialog::getProfileList() {

    return &profileList;
}

AutoProfileInfo* MainSettingsDialog::getAllDefaultProfile() const {

    return allDefaultProfile;
}

QList<InputDevice*>* MainSettingsDialog::getConnectedDevices() const {

    return connectedDevices;
}

void MainSettingsDialog::on_resetBtn_clicked()
{
    QMessageBox msgBox;
    msgBox.setText(trUtf8("Do you really want to reset setting?"));
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);

    switch(msgBox.exec())
    {
        case QMessageBox::Ok:
            resetGeneralSett();
            resetAutoProfSett();
            resetMouseSett();
            resetAdvancedSett();
            ui->localeListWidget->setCurrentRow(0);

        break;

        case QMessageBox::Cancel:

        break;

        default:

        break;
    }

    QMessageBox msgBox2;
    msgBox2.setText(trUtf8("Would you like to reset mappings too?"));
    msgBox2.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);

    switch(msgBox2.exec())
    {
        case QMessageBox::Ok:

            ui->controllerMappingsTableWidget->setRowCount(0);

        break;

        case QMessageBox::Cancel:

        break;

        default:

        break;
    }


}


void MainSettingsDialog::resetGeneralSett()
{
    ui->profileDefaultDirLineEdit->setText(PadderCommon::preferredProfileDir(settings));
    ui->numberRecentProfileSpinBox->setValue(5);
    int gamepadPollIndex = ui->gamepadPollRateComboBox->findData(JoyButton::getGamepadRefreshRate());

    if (gamepadPollIndex >= 0)
    {
        ui->gamepadPollRateComboBox->setCurrentIndex(gamepadPollIndex);
    }

    ui->closeToTrayCheckBox->setChecked(false);
    ui->launchAtWinStartupCheckBox->setChecked(false);
    ui->traySingleProfileListCheckBox->setChecked(false);
    ui->minimizeTaskbarCheckBox->setChecked(false);
    ui->hideEmptyCheckBox->setChecked(false);
    ui->autoLoadPreviousCheckBox->setChecked(true);
    ui->launchInTrayCheckBox->setChecked(false);
    ui->associateProfilesCheckBox->setChecked(true);
    ui->keyRepeatEnableCheckBox->setChecked(false);

    ui->keyDelayHorizontalSlider->setValue(660);
    ui->keyRateHorizontalSlider->setValue(25);
}


void MainSettingsDialog::resetAutoProfSett()
{

    disconnect(ui->autoProfileTableWidget, &QTableWidget::itemChanged, this, &MainSettingsDialog::processAutoProfileActiveClick);

    ui->activeCheckBox->setChecked(false);
    ui->devicesComboBox->setCurrentIndex(0);

    ui->autoProfileTableWidget->setRowCount(0);

    AutoProfileInfo *info = allDefaultProfile;

    ui->autoProfileTableWidget->insertRow(0);
    QTableWidgetItem *item = new QTableWidgetItem();
    item->setCheckState(info->isActive() ? Qt::Checked : Qt::Unchecked);
    ui->autoProfileTableWidget->setItem(0, 0, item);

    QString deviceName = info->getDeviceName();
    QString guidDisplay = info->getGUID();

    if (!deviceName.isEmpty())
    {
        guidDisplay = QString("%1 ").arg(info->getDeviceName());
        guidDisplay.append(QString("(%1)").arg(info->getGUID()));
    }

    item = new QTableWidgetItem(guidDisplay);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setData(Qt::UserRole, info->getGUID());
    item->setToolTip(info->getGUID());
    ui->autoProfileTableWidget->setItem(0, 1, item);

    item = new QTableWidgetItem("Default");
    item->setData(Qt::UserRole, "default");
    ui->autoProfileTableWidget->setItem(0, 6, item);

    item = new QTableWidgetItem("Instance");
    item->setData(Qt::UserRole, QVariant::fromValue<AutoProfileInfo*>(info));
    ui->autoProfileTableWidget->setItem(0, 7, item);

    if (ui->autoProfileTableWidget->rowCount() == 1) profileList.clear();

    connect(ui->autoProfileTableWidget, &QTableWidget::itemChanged, this, &MainSettingsDialog::processAutoProfileActiveClick);
}


void MainSettingsDialog::resetMouseSett()
{
    ui->disableWindowsEnhancedPointCheckBox->setChecked(false);
    ui->smoothingEnableCheckBox->setChecked(false);
    ui->smoothingEnableCheckBox->setEnabled(true);
    ui->historySizeSpinBox->setValue(10);
    ui->historySizeSpinBox->setEnabled(false);
    ui->weightModifierDoubleSpinBox->setValue(0.20);
    ui->weightModifierDoubleSpinBox->setEnabled(false);

    int refreshIndex = ui->mouseRefreshRateComboBox->findData(JoyButton::getMouseRefreshRate());
    if (refreshIndex >= 0)
    {
        ui->mouseRefreshRateComboBox->setCurrentIndex(refreshIndex);
    }

    int screenIndex = ui->springScreenComboBox->findData(JoyButton::getSpringModeScreen());
    if (screenIndex > -1)
    {
        ui->springScreenComboBox->setCurrentIndex(screenIndex);
    }
}


void MainSettingsDialog::resetAdvancedSett()
{
    ui->logFilePathEdit->setText("");
    ui->logLevelComboBox->setCurrentIndex(0);
}
