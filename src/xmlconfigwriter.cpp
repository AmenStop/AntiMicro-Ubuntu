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

#include "xmlconfigwriter.h"

#include "messagehandler.h"
#include "inputdevice.h"
#include "common.h"

#include <QDir>
#include <QFile>
#include <QXmlStreamWriter>
#include <QDebug>



XMLConfigWriter::XMLConfigWriter(QObject *parent) :
    QObject(parent)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    xml = new QXmlStreamWriter();
    xml->setAutoFormatting(true);
    configFile = nullptr;
    joystick = nullptr;
    writerError = false;
}

XMLConfigWriter::~XMLConfigWriter()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    if (configFile)
    {
        if (configFile->isOpen())
        {
            configFile->close();
        }

        delete configFile;
        configFile = nullptr;
    }

    if (xml)
    {
        delete xml;
        xml = nullptr;
    }
}

void XMLConfigWriter::write(InputDevice *joystick)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    writerError = false;

    if (!configFile->isOpen())
    {
        configFile->open(QFile::WriteOnly | QFile::Text);
        xml->setDevice(configFile);
    }
    else
    {
        writerError = true;
        writerErrorString = trUtf8("Could not write to profile at %1.").arg(configFile->fileName());
    }

    if (!writerError)
    {
        xml->writeStartDocument();
        joystick->writeConfig(xml);
        xml->writeEndDocument();
    }

    if (configFile->isOpen())
    {
        configFile->close();
    }
}


void XMLConfigWriter::setFileName(QString filename)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    QFile *temp = new QFile(filename);
    fileName = filename;
    configFile = temp;
}

bool XMLConfigWriter::hasError()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    return writerError;
}

const QString XMLConfigWriter::getErrorString()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    return writerErrorString;
}

const QXmlStreamWriter *XMLConfigWriter::getXml() {

    return xml;
}

QString const& XMLConfigWriter::getFileName() {

    return fileName;
}

const QFile *XMLConfigWriter::getConfigFile() {

    return configFile;
}

const InputDevice* XMLConfigWriter::getJoystick() {

    return joystick;
}
