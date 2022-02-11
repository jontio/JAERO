#ifndef JSERIALIZE_H
#define JSERIALIZE_H

/*
 * for easy convertion of variables of classes derived from QObject to QByteArray and back.
 *
 * useful for saving and loading class variables or for transmitting over the net.
 *
 * Example usage:
 *
 * Declare say in testclass.h ...
 *
 * #ifndef TESTCLASS_H
 * #define TESTCLASS_H
 *
 * #include "jserialize.h"
 *
 * class testclass : public JSerialize
 * {
 *     Q_OBJECT
 * public:
 *     J_DECLARE_SERIALIZE
 *     J_SERIALIZE(int,val2)
 *     J_SERIALIZE(QString,num,"test")
 *     J_SERIALIZE(QString,stringProperty)
 *     J_SERIALIZE(int,val,42)
 * };
 *
 * #endif // TESTCLASS_H
 *
 * Then use in somewhere else...
 *
 * #include <QDebug>
 * #include "testclass.h"
 *
 * void test()
 * {
 *     testclass tc;
 *     tc.stringProperty="yup";
 *     QByteArray ba=tc;
 *
 *     //say transmit ba over the net
 *     //then we can create something
 *     //that looks like tc on the
 *     //remote computer
 *
 *     testclass tc2;
 *     tc2<<ba;
 *     qDebug()<<tc2.stringProperty;
 * }
 *
 */

//NB!!!!! some types don't work at all like QMap
//just use the simple ones and you should be fine.
//ones checked so far ...
//
//    J_SERIALIZE(double,_double)
//    J_SERIALIZE(float,_float)
//    J_SERIALIZE(QSize,_qsize)
//    J_SERIALIZE(int,_int)
//    J_SERIALIZE(QString,_qstring)
//    J_SERIALIZE(QStringList,_qstringlist)
//    J_SERIALIZE(uchar,_uchar)
//    J_SERIALIZE(char,_char)
//    J_SERIALIZE(quint32, _quint32)
//    J_SERIALIZE(QByteArray, _bytearray)
//    J_SERIALIZE(bool, _bool)
//

//these defined effect serialization output size
//use J_SERIALIZE_USE_QBYTEARRAY_AS_FORMAT with
//cation as it probably will have issues with
//some types.
//USE J_SERIALIZE_USE_QBYTEARRAY_AS_FORMAT WITH CATION!!!
//
//#define J_SERIALIZE_USE_QBYTEARRAY_AS_FORMAT
//#define J_SERIALIZE_DROP_CLASSNAME

//not all types can be converted to QByteArray.
//not sure what ones can but add any that don't
//to this list if you are using the
//J_SERIALIZE_USE_QBYTEARRAY_AS_FORMAT define.
//somethings like bool are shorter as a QVariant
//than a QByteArray so might as well add anything
//than makes the size shorter too as that's
//the only reasion for using the
//J_SERIALIZE_USE_QBYTEARRAY_AS_FORMAT define
//if you really want to use J_SERIALIZE_USE_QBYTEARRAY_AS_FORMAT
//then test out every type your planning to use.
//so far the only ones I've checked are in the class testclass
//int /test/jserialize_tests.h.
#define J_SERIALIZE_USE_QBYTEARRAY_AS_FORMAT_EXCEPTIONS (\
    (type==QVariant::StringList)    ||\
    (type==QVariant::Bool)          ||\
    ((QMetaType::Type)type==QMetaType::QSize)\
    )\

#include <QObject>
#include <QDataStream>
#include <QMetaProperty>
#include <QByteArray>

#define GET_SERIALIZE_MACRO(_1,_2,_3,NAME,...) NAME
#define J_SERIALIZE(...) GET_SERIALIZE_MACRO(__VA_ARGS__, J_SERIALIZE_3, J_SERIALIZE_2)(__VA_ARGS__)

#define J_SERIALIZE_3(type, val, init_val)\
    Q_PROPERTY(type val MEMBER val)\
    type val=init_val;\

#define J_SERIALIZE_2(type, val)\
    Q_PROPERTY(type val MEMBER val)\
    type val;\

class JSerialize : public QObject
{
    Q_OBJECT
public:

    bool hasSerializeError() const {return m_error;}

    explicit JSerialize(QObject *parent=nullptr) : QObject(parent){m_error=false;}
    JSerialize(const JSerialize &other) : QObject()
    {
        m_error=false;
        if(this == &other) return;
        m_error|=!(other.toQByteArray(ba));
        if(m_error)return;
        m_error|=!(this->fromQByteArray(ba));
    }
    virtual ~JSerialize(){}

    //on error toQByteArray throws an exception.
    //fromQByteArray sets or clear m_error depending
    //on if it fails or not but doesn't throw an
    //exception.
    Q_INVOKABLE bool fromQByteArray(const QByteArray &ba);
    Q_INVOKABLE bool toQByteArray(QByteArray &ba) const;

    //convinience
    const QByteArray& toQByteArray()
    {
        m_error|=!(toQByteArray(ba));
        return ba;
    }
    operator const QByteArray & ()
    {
        return toQByteArray();
    }
    JSerialize& operator =(const JSerialize& other)
    {
        m_error=false;
        if(this == &other) return *this;
        m_error|=!(other.toQByteArray(ba));
        if(m_error)return *this;
        m_error|=!(fromQByteArray(ba));
        return *this;
    }

    //QDataStream serialization
    friend QDataStream &operator<<(QDataStream &s,const JSerialize &val)
    {
        QByteArray ba;
        bool error=!(val.toQByteArray(ba));
        if(error)
        {
            s.setStatus(QDataStream::WriteFailed);
            return s;
        }
        s<<ba;
        return s;
    }
    friend QDataStream &operator>>(QDataStream &s, JSerialize &val)
    {
        QByteArray ba;
        s>>ba;
        if(s.status()!=QDataStream::Ok)
        {
            val.m_error=true;
            return s;
        }
        val.m_error=!(val.fromQByteArray(ba));
        if(val.m_error)
        {
            s.setStatus(QDataStream::ReadCorruptData);
            return s;
        }
        return s;
    }

    //QByteArray serialization
    friend JSerialize &operator<<(JSerialize &s,const QByteArray &val)
    {
        s.m_error=!(s.fromQByteArray(val));
        return s;
    }
    friend JSerialize &operator>>(JSerialize &s, QByteArray &val)
    {
        s.m_error=!(s.toQByteArray(val));
        return s;
    }

    bool operator==(const JSerialize& other)
    {
        QByteArray ba,other_ba;
        toQByteArray(ba);
        other.toQByteArray(other_ba);
        if(ba==other_ba)return true;
        return false;
    }
    inline bool operator!=(const JSerialize& other){ return !((*this) == other); }

private:
    QByteArray ba;
    bool m_error;

};

Q_DECLARE_METATYPE(JSerialize)



#endif // JSERIALIZE_H
