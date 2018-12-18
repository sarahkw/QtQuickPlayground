#ifndef MOBJECT_H
#define MOBJECT_H

#include <QObject>
#include <QDebug>

class Mobject : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE QString muhName()
    {
        return "Heyo";
    }

    virtual ~Mobject()
    {
        qInfo() << "~Mobject";
    }
};

#endif // MOBJECT_H
