/*
 * Copyright (C) 2008-2014 The QXmpp developers
 *
 * Author:
 *  Jeremy Lainé
 *
 * Source:
 *  https://github.com/qxmpp-project/qxmpp
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

#include <QTcpServer>
#include <QTcpSocket>
#include "QXmppSocks.h"
#include "util.h"

class tst_QXmppSocks : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void newConnectionSlot(QTcpSocket *socket, QString hostName, quint16 port);

    void testClient_data();
    void testClient();
    void testClientAndServer();
    void testServer_data();
    void testServer();

private:
    QTcpSocket *m_connectionSocket;
    QString m_connectionHostName;
    quint16 m_connectionPort;
};

void tst_QXmppSocks::init()
{
    m_connectionSocket = 0;
    m_connectionHostName = QString();
    m_connectionPort = 0;
}

void tst_QXmppSocks::newConnectionSlot(QTcpSocket *socket, QString hostName, quint16 port)
{
    m_connectionSocket = socket;
    m_connectionHostName = hostName;
    m_connectionPort = port;
}

void tst_QXmppSocks::testClient_data()
{
    QTest::addColumn<QByteArray>("serverHandshake");
    QTest::addColumn<bool>("serverHandshakeWorks");
    QTest::addColumn<QByteArray>("serverConnect");
    QTest::addColumn<bool>("serverConnectWorks");

    QTest::newRow("no authentication - good connect")
        << QByteArray::fromHex("0500") << true
        << QByteArray::fromHex("050000030e7777772e676f6f676c652e636f6d0050") << true;
    QTest::newRow("no authentication - bad connect")
        << QByteArray::fromHex("0500") << true
        << QByteArray::fromHex("0500") << false;
    QTest::newRow("bad authentication")
        << QByteArray::fromHex("05ff") << false
        << QByteArray() << false;
}

void tst_QXmppSocks::testClient()
{
    QFETCH(QByteArray, serverHandshake);
    QFETCH(bool, serverHandshakeWorks);
    QFETCH(QByteArray, serverConnect);
    QFETCH(bool, serverConnectWorks);

    QTcpServer server;
    QVERIFY(server.listen());
    QVERIFY(server.serverPort() != 0);

    QXmppSocksClient client("127.0.0.1", server.serverPort());

    QEventLoop loop;
    connect(&server, SIGNAL(newConnection()), &loop, SLOT(quit()));

    client.connectToHost("www.google.com", 80);
    loop.exec();

    // receive client handshake
    m_connectionSocket = server.nextPendingConnection();
    QVERIFY(m_connectionSocket);

    connect(m_connectionSocket, SIGNAL(disconnected()), &loop, SLOT(quit()));
    connect(m_connectionSocket, SIGNAL(readyRead()), &loop, SLOT(quit()));
    loop.exec();
    QCOMPARE(client.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(m_connectionSocket->state(), QAbstractSocket::ConnectedState);
    QCOMPARE(m_connectionSocket->readAll(), QByteArray::fromHex("050100"));

    // receive client connect
    m_connectionSocket->write(serverHandshake);
    loop.exec();
    if (!serverHandshakeWorks) {
        QCOMPARE(client.state(), QAbstractSocket::UnconnectedState);
        QCOMPARE(m_connectionSocket->state(), QAbstractSocket::UnconnectedState);
        return;
    }

    QCOMPARE(client.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(m_connectionSocket->state(), QAbstractSocket::ConnectedState);
    QCOMPARE(m_connectionSocket->readAll(), QByteArray::fromHex("050100030e7777772e676f6f676c652e636f6d0050"));

    // wait for client to be ready
    connect(&client, SIGNAL(ready()), &loop, SLOT(quit()));
    m_connectionSocket->write(serverConnect);
    loop.exec();
    if  (!serverConnectWorks) {
        QCOMPARE(client.state(), QAbstractSocket::UnconnectedState);
        QCOMPARE(m_connectionSocket->state(), QAbstractSocket::UnconnectedState);
        return;
    }

    QCOMPARE(client.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(m_connectionSocket->state(), QAbstractSocket::ConnectedState);

    // disconnect
    client.disconnectFromHost();
}

void tst_QXmppSocks::testClientAndServer()
{
    QXmppSocksServer server;
    QVERIFY(server.listen());
    QVERIFY(server.serverPort() != 0);
    connect(&server, SIGNAL(newConnection(QTcpSocket*,QString,quint16)),
            this, SLOT(newConnectionSlot(QTcpSocket*,QString,quint16)));

    QXmppSocksClient client("127.0.0.1", server.serverPort());

    QEventLoop loop;
    connect(&client, SIGNAL(ready()), &loop, SLOT(quit()));
  
    client.connectToHost("www.google.com", 80);
    loop.exec();

    // check client
    QCOMPARE(client.state(), QAbstractSocket::ConnectedState);

    // check server
    QVERIFY(m_connectionSocket);
    QCOMPARE(m_connectionSocket->state(), QAbstractSocket::ConnectedState);
    QCOMPARE(m_connectionHostName, QLatin1String("www.google.com"));
    QCOMPARE(m_connectionPort, quint16(80));

    // disconnect
    client.disconnectFromHost();
}

void tst_QXmppSocks::testServer_data()
{
    QTest::addColumn<QByteArray>("clientHandshake");
    QTest::addColumn<bool>("clientHandshakeWorks");

    QTest::newRow("no authentication") << QByteArray::fromHex("050100") << true;
    QTest::newRow("no authentication or GSSAPI") << QByteArray::fromHex("05020001") << true;

    QTest::newRow("bad SOCKS version") << QByteArray::fromHex("060100") << false;
    QTest::newRow("no methods") << QByteArray::fromHex("0500") << false;
    QTest::newRow("GSSAPI only") << QByteArray::fromHex("050101") << false;
}

void tst_QXmppSocks::testServer()
{
    QFETCH(QByteArray, clientHandshake);
    QFETCH(bool, clientHandshakeWorks);

    QXmppSocksServer server;
    QVERIFY(server.listen());
    QVERIFY(server.serverPort() != 0);
    connect(&server, SIGNAL(newConnection(QTcpSocket*,QString,quint16)),
            this, SLOT(newConnectionSlot(QTcpSocket*,QString,quint16)));

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, server.serverPort());
    QVERIFY2(client.waitForConnected(), qPrintable(client.errorString()));

    QEventLoop loop;
    connect(&client, SIGNAL(disconnected()), &loop, SLOT(quit()));
    connect(&client, SIGNAL(readyRead()), &loop, SLOT(quit()));

    // send client handshake
    client.write(clientHandshake);
    loop.exec();
    if (!clientHandshakeWorks) {
        QCOMPARE(client.state(), QAbstractSocket::UnconnectedState);

        QVERIFY(!m_connectionSocket);
        QVERIFY(m_connectionHostName.isNull());
        QCOMPARE(m_connectionPort, quint16(0));
        return;
    }

    QCOMPARE(client.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(client.readAll(), QByteArray::fromHex("0500"));

    // request connect to www.google.com port 80
    client.write(QByteArray::fromHex("050100030e7777772e676f6f676c652e636f6d0050"));
    loop.exec();
    QCOMPARE(client.readAll(), QByteArray::fromHex("050000030e7777772e676f6f676c652e636f6d0050"));

    QCOMPARE(client.state(), QAbstractSocket::ConnectedState);
    QVERIFY(m_connectionSocket);
    QCOMPARE(m_connectionSocket->state(), QAbstractSocket::ConnectedState);
    QCOMPARE(m_connectionHostName, QLatin1String("www.google.com"));
    QCOMPARE(m_connectionPort, quint16(80));

    // disconnect
    client.disconnectFromHost();
}

QTEST_MAIN(tst_QXmppSocks)
#include "tst_qxmppsocks.moc"