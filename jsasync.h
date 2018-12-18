#ifndef JSASYNC_H
#define JSASYNC_H

#include <QObject>
#include <QtConcurrent>
#include <QDebug>
#include <QJSValue>
#include <QJSEngine>
#include <QQmlEngine>

#include <memory>

#define JS_ASYNC_FN(fndecl, cb, expr) \
    Q_INVOKABLE JsAsync* fndecl { return JsAsync::create(cb, expr); }

namespace impl {

// Use virtual inheritance for type erasure. We need to store lambda
// expression and also its return value. However, QObject cannot be
// templated.
class IJsAsyncTypeErasure
{
public:
    virtual ~IJsAsyncTypeErasure() {}
    virtual void call() = 0;
    virtual void triggerCallback(QJSEngine* jseng, QJSValue& callback) = 0;
};

// TODO when c++17: so many things can be rewritten with "if constexpr"

struct JsAsyncEmpty {};

template <typename StorageType>
typename std::enable_if<!std::is_convertible<StorageType, QObject*>::value, QJSValue>::type
JsAsyncToJSValue(QJSEngine* jseng, StorageType& r)
{
    // static_assert because toScriptValue will accept a QObject* but it won't do what we want.
    static_assert(!std::is_convertible<StorageType, QObject*>::value,
                  "QObject* shouldn't have made it here.");
    return jseng->toScriptValue(r);
}

inline QJSValue JsAsyncToJSValue(QJSEngine* jseng, QObject* r)
{
    return jseng->newQObject(r);
}

template <typename StorageType, typename FnType>
struct JsAsyncHelper {
    static void call(StorageType& ret, FnType& fn)
    {
        ret = fn();
    }

    static void arg(QJSValueList& args, QJSEngine* jseng, StorageType& r)
    {
        args << JsAsyncToJSValue(jseng, r);
    }
};

// Specialization for when fn() returns void.
template <typename FnType>
struct JsAsyncHelper<JsAsyncEmpty, FnType> {
    static void call(JsAsyncEmpty&, FnType& fn)
    {
        fn();
    }

    static void arg(QJSValueList&, QJSEngine*, JsAsyncEmpty&)
    {
    }
};

// Specialization for when fn() returns a tuple.
template <typename... Args, typename FnType>
struct JsAsyncHelper<std::tuple<Args...>, FnType>
{
    using TupleType = std::tuple<Args...>;
    static void call(TupleType& ret, FnType& fn)
    {
        ret = fn();
    }

    static void arg(QJSValueList& args, QJSEngine* jseng, TupleType& r)
    {
        // TODO when c++17: use fold expression
        duurp(r, args, jseng, std::index_sequence_for<Args...>{});
    }

private:

    template <std::size_t... Is>
    static void duurp(TupleType& t, QJSValueList& args, QJSEngine* jseng, std::index_sequence<Is...>)
    {
        duurp<Is...>(t, args, jseng);
    }

    template <std::size_t I>
    static void duurp(TupleType& t, QJSValueList& args, QJSEngine* jseng)
    {
        args << JsAsyncToJSValue(jseng, std::get<I>(t));
    }

    template <std::size_t I, std::size_t... Is>
    static typename std::enable_if<(sizeof...(Is) > 0)>::type duurp(TupleType& t, QJSValueList& args, QJSEngine* jseng)
    {
        duurp<I>(t, args, jseng);
        duurp<Is...>(t, args, jseng);
    }

};

template <class FnType>
class JsAsyncTypeErasure : public IJsAsyncTypeErasure
{
    using RetType = decltype(std::declval<FnType>()());

    static constexpr bool fnReturnsVoid = std::is_void<RetType>::value;
    using StorageType =typename std::conditional<fnReturnsVoid, JsAsyncEmpty, RetType>::type;

    StorageType ret_;
    FnType fn_;

    // These shouldn't happen, but just to be sure.
    static_assert(!std::is_reference<StorageType>::value, "");
    static_assert(!std::is_reference<FnType>::value, "");

    using Helper = JsAsyncHelper<StorageType, FnType>;

public:
    JsAsyncTypeErasure(FnType&& fn) : fn_(std::move(fn))
    {
    }

    void call() override
    {
        Helper::call(ret_, fn_);
    }

    void triggerCallback(QJSEngine* jseng, QJSValue& callback) override
    {
        if (callback.isCallable()) {
            QJSValueList args;

            Helper::arg(args, jseng, ret_);

            QJSValue ret = callback.call(args);
            if (ret.isError()) {
                qCritical() << "Error returned while running callback.";
                qCritical() << ret.toString();
            }
        }
    }
};

} // namespace impl

class JsAsync : public QObject
{
    Q_OBJECT

    struct Blank {};

    QJSValue callback_;
    std::unique_ptr<impl::IJsAsyncTypeErasure> typeErasure_;

    using WatcherType = QFutureWatcher<Blank>;
    WatcherType watcher_;

    JsAsync(const QJSValue& callback,
            std::unique_ptr<impl::IJsAsyncTypeErasure>&& typeErasure)
        : callback_(callback), typeErasure_(std::move(typeErasure))
    {
        // re: QueuedConnection
        //
        // We depend on this QObject being returned to QML so that it
        // sets qjsEngine(this). I believe it's possible if we don't
        // force a QueuedConnection, if the future returns too
        // quickly, we could try to call the callback while the
        // qjsEngine hasn't been set yet.
        //
        // I believe setting QueuedConnection is probably not
        // necessary, but until I verify, put it there to be safe.
        QObject::connect(&watcher_, &WatcherType::finished, this,
                         &JsAsync::callback,
                         Qt::ConnectionType::QueuedConnection);

        watcher_.setFuture(QtConcurrent::run([this] {
            typeErasure_->call();
            return Blank();
        }));
    }

public:

    template <typename F>
    static JsAsync* create(const QJSValue& callback, F&& f)
    {
        // Template deduction can allow lvalue to be passed in.
        static_assert(std::is_rvalue_reference<decltype(f)>::value,
                      "Expecting f to be a rvalue");

        JsAsync* r = new JsAsync(
            callback,
            std::make_unique<impl::JsAsyncTypeErasure<F>>(std::move(f)));
        QQmlEngine::setObjectOwnership(r, QQmlEngine::CppOwnership);
        return r;
    }

private slots:
    void callback()
    {
        QJSEngine* jseng = qjsEngine(this);
        // If we assert here, that means we never made it to the JS
        // engine, since the act of returning to JS will set this
        // value.
        Q_ASSERT(jseng);
        typeErasure_->triggerCallback(jseng, this->callback_);
        deleteLater();
    }
};

#endif // JSASYNC_H
