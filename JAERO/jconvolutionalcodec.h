#ifndef JCONVOLUTIONALCODEC_H
#define JCONVOLUTIONALCODEC_H

//new viterbi
#ifdef __cplusplus
  extern "C" {
#endif
#include "../libcorrect/include/correct.h"
#ifdef __cplusplus
  }
#endif

#include <QVector>

#include <QObject>

class JConvolutionalCodec : public QObject
{
    Q_OBJECT
public:
    explicit JConvolutionalCodec(QObject *parent = 0);
    ~JConvolutionalCodec();
    void SetCode(int inv_rate, int order, const QVector<quint16> poly, int paddinglength=24*4);
    QVector<int> &Decode_Continuous(QByteArray& soft_bits_in);//0-->-1 128-->0 255-->1
    QVector<int> &Decode_Continuous_hard(const QByteArray& soft_bits_in);//0-->1

    QVector<int> &Decode_soft(QByteArray& soft_bits_in, int size);//0-->-1 128-->0 255-->1
    QVector<int> &Decode_hard(const QByteArray& soft_bits_in, int size);//0-->1

    QVector<int> &Soft_To_Hard_Convert(const QByteArray& soft_bits_in);//0-->-1 128-->0 255-->1

    QByteArray &Hard_To_Soft_Convert(QByteArray& hard_bits_in);//0-->-1 128-->0 255-->1
    void printbits(QByteArray msg,int nbits=-1);
    int getPaddinglength(){return paddinglength;}
signals:

public slots:
private:
    correct_convolutional *convol;
    int constraint;
    int nparitybits;
    int paddinglength;
    QByteArray soft_bits_overlap_buffer_uchar;
    QVector<int> decoded_bits;//unpacked
    QByteArray decoded;//packed tempory storage
};

#endif // JCONVOLUTIONALCODEC_H
