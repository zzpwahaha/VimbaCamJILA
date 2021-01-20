#ifndef I_LOG_TARGET_H_
#define I_LOG_TARGET_H_
#include <QString>
class ILogTarget
{
public:
    virtual void Log( const QString &s) = 0;
    virtual ~ILogTarget() {}
};

#endif