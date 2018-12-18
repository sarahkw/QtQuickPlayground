#ifndef CONTROL_H
#define CONTROL_H

#include "jsasync.h"
#include "mobject.h"

#include <QObject>
#include <QThread>
#include <QDebug>
#include <QJSValue>
#include <QJSEngine>

class Control : public QObject
{
    Q_OBJECT

    QJSEngine* jseng_;
public:
    explicit Control(QJSEngine* jseng, QObject *parent = nullptr);

    JS_ASYNC_FN(async_delay(QJSValue cb), cb, [=] { return delay(); })
    Q_INVOKABLE int delay()
    {
        QThread::sleep(4);
        return 4;
    }

    JS_ASYNC_FN(async_sleep(unsigned long seconds, QJSValue cb),
                cb,
                [=] { sleep(seconds); })
    Q_INVOKABLE void sleep(unsigned long seconds)
    {
        qInfo() << "Sleep seconds" << seconds;
        QThread::sleep(seconds);
    }

    Q_INVOKABLE void mobjectCallback(QJSValue callback)
    {
        if (callback.isCallable()) {
            QJSValueList args;
            args << jseng_->newQObject(new Mobject);
            callback.call(args);
        }
    }

    JS_ASYNC_FN(async_mobjectDirect(QJSValue cb), cb, [=] {
        auto x = mobjectDirect();
        x->moveToThread(thread());
        return x;
    })
    Q_INVOKABLE Mobject* mobjectDirect()
    {
        return new Mobject;
    }

    Q_INVOKABLE void mobjectSignal()
    {
        emit mobjectReply(jseng_->newQObject(new Mobject));
    }

    JS_ASYNC_FN(async_multiReturn(QJSValue cb), cb, [=] {
        return std::make_tuple(true, new Mobject, QJSValue(QJSValue::NullValue), QJSValue(QJSValue::UndefinedValue));
    })

signals:
    void mobjectReply(QJSValue m);

public slots:

};


#endif // CONTROL_H
