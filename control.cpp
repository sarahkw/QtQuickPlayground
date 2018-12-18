#include "control.h"

#include <QQmlEngine>
#include <QtDebug>

static struct Registration
{
    Registration()
    {
        qmlRegisterType<Mobject>();
    }
} registration;

Control::Control(QJSEngine* jseng, QObject *parent) : QObject(parent), jseng_(jseng)
{

}
