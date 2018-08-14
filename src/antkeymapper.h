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

#ifndef ANTKEYMAPPER_H
#define ANTKEYMAPPER_H

#include <QObject>

#ifdef Q_OS_WIN
  #include "qtwinkeymapper.h"

  #ifdef WITH_VMULTI
    #include "qtvmultikeymapper.h"
  #endif
#elif defined(Q_OS_UNIX)

  #if defined(WITH_XTEST)
    #include "qtx11keymapper.h"
  #endif

  #if defined(WITH_UINPUT)
    #include "qtuinputkeymapper.h"
  #endif
#endif

class AntKeyMapper : public QObject
{
    Q_OBJECT

public:
    static AntKeyMapper* getInstance(QString handler = "");
    void deleteInstance();

    int returnVirtualKey(int qkey); // unsigned, unsigned
    int returnQtKey(int key, int scancode=0); // unsigned, unsigned, unsigned
    bool isModifierKey(int qkey); // .., unsigned
    QtKeyMapperBase* getNativeKeyMapper() const;
    QtKeyMapperBase* getKeyMapper() const;
    bool hasNativeKeyMapper();

protected:
    static AntKeyMapper *_instance;

private:
    explicit AntKeyMapper(QString handler = "", QObject *parent = nullptr);

    QtKeyMapperBase *internalMapper;
    QtKeyMapperBase *nativeKeyMapper;

#ifdef Q_OS_WIN
    QtWinKeyMapper winMapper;

  #ifdef WITH_VMULTI
    QtVMultiKeyMapper vmultiMapper;
  #endif

#elif defined(Q_OS_UNIX)
  #if defined(WITH_XTEST)
    QtX11KeyMapper x11Mapper;
  #endif

  #if defined(WITH_UINPUT)
    QtUInputKeyMapper uinputMapper;
  #endif

#endif

};

#endif // ANTKEYMAPPER_H
