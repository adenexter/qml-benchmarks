/*
 * Copyright (C) 2014 Jolla Ltd
 * Contact: Andrew den Exter <andrew.den.exter@jollamobile.com>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Nemo Mobile nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 */

#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qlocale.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>
#include <QtGui/qguiapplication.h>

template <typename T, int N> static int lengthOf(const T(&)[N]) { return N; }

template <int N> static const char *argumentValue(const char *argument, const char (&key)[N])
{
    return qstrncmp(argument, key, N - 1) == 0
            ? argument + N - 1
            : 0;
}

int main(int argc, char *argv[])
{
    static const char *defaultTypes[] = { "Item" };

    const char * const *types = defaultTypes;
    int typeCount = lengthOf(defaultTypes);

    int iterations = 100;

    QByteArray importStatements;

    for (int i = 1; i < argc; ++i) {
        if (const char *value = argumentValue(argv[i], "--iterations=")) {
            iterations = QByteArray(value).toInt();
        } else if (const char *value = argumentValue(argv[i], "--import=")) {
            importStatements += "import " + QByteArray(value) + "\n";
        } else if (i < argc) {
            types = argv + i;
            typeCount = argc - i;
            break;
        } else {
            break;
        }
    }

    QGuiApplication app(argc, argv);
    QQmlEngine engine;

    {   // One time object construction to weed out global times
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\n" + importStatements + "QtObject {}", QUrl());

        QScopedPointer<QObject> object(component.create());
    }

    for (int i = 0; i < typeCount; ++i) {
        const QByteArray qml
                = "import QtQuick 2.0\n"
                + importStatements
                + "\n"
                + types[i] + " {\n"
                + "}\n";

        QElapsedTimer timer;

        QQmlComponent component(&engine);

        timer.start();
        component.setData(qml, QUrl::fromLocalFile(QDir::currentPath() + QLatin1String("/document.qml")));
        qint64 parseTime = timer.nsecsElapsed();

        if (component.status() == QQmlComponent::Error) {
            qDebug() << "";
            qDebug() << "Error in component" << types[i];
            qDebug() << qPrintable(component.errorString());
            qDebug() << qml.constData();
            continue;
        }


        timer.start();
        QScopedPointer<QObject> object(component.create());
        qint64 oneTimeConstruction = timer.nsecsElapsed();

        qint64 accumulatedConstruction = 0;
        qint64 accumulatedTotal = 0;
        for (int j = 0; j < iterations; ++j) {
            {
                timer.restart();
                QScopedPointer<QObject> object(component.create());
                accumulatedConstruction += timer.nsecsElapsed();
            }
            accumulatedTotal += timer.nsecsElapsed();
        }

        const QLocale locale = QLocale::system();

        qDebug() << "";
        qDebug() << "Measuring construction time of" << types[i] << "over" << iterations << "iterations";
        qDebug() << "";
        qDebug() << qml.constData();
        qDebug() << "One time parse time (ns)        " << qPrintable(locale.toString(parseTime));
        qDebug() << "One time construction time (ns):" << qPrintable(locale.toString(oneTimeConstruction));
        qDebug() << "Average construction time (ns): " << qPrintable(locale.toString(accumulatedConstruction / iterations));
        qDebug() << "Average destruction time (ns):  " << qPrintable(locale.toString((accumulatedTotal - accumulatedConstruction) / iterations));
    }
    return 0;
}



