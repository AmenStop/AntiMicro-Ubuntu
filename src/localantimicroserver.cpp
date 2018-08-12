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

#include "localantimicroserver.h"

#include "messagehandler.h"
#include "common.h"

#include <QTextStream>
#include <QLocalSocket>
#include <QLocalServer>
#include <QDebug>


LocalAntiMicroServer::LocalAntiMicroServer(QObject *parent) :
    QObject(parent)
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    localServer = new QLocalServer(this);
}

void LocalAntiMicroServer::startLocalServer()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    QLocalServer::removeServer(PadderCommon::localSocketKey);
    localServer->setMaxPendingConnections(1);
    if (!localServer->listen(PadderCommon::localSocketKey))
    {
        QTextStream errorstream(stderr);
        QString message("Could not start signal server. Profiles cannot be reloaded\n");
        message.append("from command-line");
        errorstream << trUtf8(message.toStdString().c_str()) << endl;
    }
    else
    {
        connect(localServer, &QLocalServer::newConnection, this, &LocalAntiMicroServer::handleOutsideConnection);
    }
}

void LocalAntiMicroServer::handleOutsideConnection()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    QLocalSocket *socket = localServer->nextPendingConnection();
    if (socket != nullptr)
    {
        connect(socket, &QLocalSocket::disconnected, this, &LocalAntiMicroServer::handleSocketDisconnect);
        connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
    }
}

void LocalAntiMicroServer::handleSocketDisconnect()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    emit clientdisconnect();
}

void LocalAntiMicroServer::close()
{
    qInstallMessageHandler(MessageHandler::myMessageOutput);

    localServer->close();
}

QLocalServer* LocalAntiMicroServer::getLocalServer() const {

    return localServer;
}
