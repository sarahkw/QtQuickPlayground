#include "jsasync.h"

#include <QQmlEngine>

namespace {

static struct JsAsyncRegister
{
    JsAsyncRegister()
    {
        qmlRegisterType<JsAsync>();
    }
} jsAsyncRegistration;

}
