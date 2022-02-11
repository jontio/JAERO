#include "jserialize.h"
#include <QDebug>

bool JSerialize::fromQByteArray(const QByteArray &ba)
{
    m_error=false;
    const QMetaObject* metaObj = this->metaObject();
    QDataStream ds(ba);
#ifndef J_SERIALIZE_DROP_CLASSNAME
    QString rxclassName;
    ds>>rxclassName;
    QString className = metaObj->className();
    if(rxclassName!=className)
    {
        qDebug()<<"rx classname doesn't match expected";
        m_error=true;
        return false;
    }
#endif
    for (int i = 1; i < metaObj->propertyCount(); ++i)
    {
        const char* propertyName = metaObj->property(i).name();
        const QVariant::Type type = metaObj->property(i).type();
        if(type>=QVariant::UserType)
        {
            qDebug()<<"fromQByteArray: user type"<<type<<(int)type<<". dont know how to deal with this. please help";
            qFatal("dont know how to deal with use type. please help");
            m_error=true;
            return false;
        }
        QVariant value;
#ifdef J_SERIALIZE_USE_QBYTEARRAY_AS_FORMAT
        //exceptions to the QByteArray format
        if(J_SERIALIZE_USE_QBYTEARRAY_AS_FORMAT_EXCEPTIONS)
        {
            ds>>value;
        }
        else
        {
            QByteArray ba_tmp;
            ds>>ba_tmp;
            value=ba_tmp;
            //for some reasion some easy types fail on value.convert(type)
            //not sure why. i think it muxt be a qt issue. so far ones that
            //fail are in type QMetaType::Type but not QVariant::Type
            switch((QMetaType::Type)type)
            {
            case QMetaType::UChar:
                value=(uchar)ba_tmp[0];
                break;
            case QMetaType::Char:
                value=(char)ba_tmp[0];
                break;
            default:
                if(!value.convert(type))
                {
                    qDebug()<<"can't convert from byte array to"<<type<<". please help";
                    m_error=true;
                    return false;
                }
                break;
            }
        }
#else
        ds>>value;
#endif
        if(!value.isValid())
        {
            qDebug()<<"fail fromQByteArray: QVariant not valid";
            m_error=true;
            return false;
        }
        if(!this->setProperty(propertyName,value))
        {
            qDebug()<<"fail fromQByteArray: failed to set property"<<propertyName<<value;
            m_error=true;
            return false;
        }
    }
    return true;
}

bool JSerialize::toQByteArray(QByteArray &ba) const
{
    ba.clear();
    const QMetaObject* metaObj = this->metaObject();
    QDataStream ds(&ba,QIODevice::WriteOnly);
#ifndef J_SERIALIZE_DROP_CLASSNAME
    QString className = metaObj->className();
    ds<<className;
#endif
    for (int i = 1; i < metaObj->propertyCount(); ++i)
    {
        const char* propertyName = metaObj->property(i).name();
        const QVariant::Type type = metaObj->property(i).type();
        if(type>=QVariant::UserType)
        {
            qDebug()<<"toQByteArray: user type"<<type<<(int)type<<". dont know how to deal with this. please help";
            qFatal("dont know how to deal with use type. please help");
            return false;
        }
        const QVariant value = this->property(propertyName);
        if(!value.isValid())
        {
            qDebug()<<"fail toQByteArray: QVariant not valid";
            qFatal("fail toQByteArray: QVariant not valid");
            return false;
        }
#ifdef J_SERIALIZE_USE_QBYTEARRAY_AS_FORMAT
        //exceptions to the QByteArray format
        if(J_SERIALIZE_USE_QBYTEARRAY_AS_FORMAT_EXCEPTIONS)
        {
            ds<<value;
        }
        else
        {
            ds<<value.toByteArray();
        }
#else
        ds<<value;
#endif
    }
    return true;
}

