
#include <QtCore/qdebug.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qlocale.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtGui/qapplication.h>

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

    QApplication app(argc, argv);
    QDeclarativeEngine engine;

    {   // One time object construction to weed out global times
        QDeclarativeComponent component(&engine);
        component.setData("import QtQuick 1.0\n" + importStatements + "QtObject {}", QUrl());
\
        QScopedPointer<QObject> object(component.create());
    }

    for (int i = 0; i < typeCount; ++i) {
        const QByteArray qml
                = "import QtQuick 1.1\n"
                + importStatements
                + "\n"
                + types[i] + " {\n"
                + "}\n";

        QDeclarativeComponent component(&engine);
        component.setData(qml, QUrl());

        if (component.status() == QDeclarativeComponent::Error) {
            qDebug() << "";
            qDebug() << "Error in component" << types[i];
            qDebug() << qPrintable(component.errorString());
            qDebug() << qml.constData();
            continue;
        }

        QElapsedTimer timer;

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
        qDebug() << "One time construction time (ns):" << qPrintable(locale.toString(oneTimeConstruction));
        qDebug() << "Average construction time (ns): " << qPrintable(locale.toString(accumulatedConstruction / iterations));
        qDebug() << "Average destruction time (ns):  " << qPrintable(locale.toString((accumulatedTotal - accumulatedConstruction) / iterations));
    }
    return 0;
}



