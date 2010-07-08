/*
 * Copyright (C) 2008-2009 Manjeet Dahiya
 *
 * Author:
 *	Manjeet Dahiya
 *
 * Source:
 *	http://code.google.com/p/qxmpp
 *
 * This file is a part of QXmpp library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 */


#ifndef QXMPPIQ_H
#define QXMPPIQ_H

#include "QXmppStanza.h"

// forward declarations of QXmlStream* classes will not work on Mac, we need to
// include the whole header.
// See http://lists.trolltech.com/qt-interest/2008-07/thread00798-0.html
// for an explanation.
#include <QXmlStreamWriter>

class QXmppIq : public QXmppStanza
{
public:
    enum Type
    {
        Error = 0,
        Get,
        Set,
        Result
    };

    QXmppIq(QXmppIq::Type type = QXmppIq::Get);
    ~QXmppIq();

    QXmppIq::Type type() const;
    void setType(QXmppIq::Type);

    void parse(const QDomElement &element);
    void toXml(QXmlStreamWriter *writer) const;

    // deprecated accessors, use the form without "get" instead
    QXmppIq::Type Q_DECL_DEPRECATED getType() const;

protected:
    virtual void parseElementFromChild(const QDomElement &element);
    virtual void toXmlElementFromChild(QXmlStreamWriter *writer) const;

private:
    QString getTypeStr() const;
    void setTypeFromStr(const QString& str);

    Type m_type;
};

#endif // QXMPPIQ_H
