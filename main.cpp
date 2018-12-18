#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QLoggingCategory>

#include <QThread>

#include "control.h"

int main(int argc, char *argv[])
{
    //QLoggingCategory::setFilterRules("*.debug=true");

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    Control* control = new Control(&engine);

    engine.rootContext()->setContextProperty("control", control);
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
