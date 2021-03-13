#include "../fftrwrapper.h"
#include "../util/RuntimeError.h"
#include "../DSP.h"
#include "../util/stdio_utils.h"

//important for Qt include cpputest last as it mucks up new and causes compiling to fail
#include "CppUTest/TestHarness.h"

TEST_GROUP(Test_FFTrWrapper)
{
    const double doubles_equal_threshold=0.00001;

    void setup()
    {
        srand(1);
    }

    void teardown()
    {
        // This gets run after every test
    }
};

TEST(Test_FFTrWrapper, fftrwrapper_matches_unusual_kissfft_scalling_as_expected_by_old_jaero_code_by_default)
{

    //this was obtained from v1.0.4.11 of JAERO
	QVector<double> input={-0.997497,0.127171,-0.613392,0.617481,0.170019,-0.040254,-0.299417,0.791925,0.645680,0.493210,-0.651784,0.717887,0.421003,0.027070,-0.392010,-0.970031};
	QVector<cpx_type> expected_forward={cpx_type(0.047060,0.000000),cpx_type(-3.660174,-0.220869),cpx_type(-1.565029,-0.944437),cpx_type(-2.388636,-1.697451),cpx_type(2.195807,0.550066),cpx_type(-0.821067,-1.010241),cpx_type(-0.320649,-2.091933),cpx_type(0.297167,-0.537596),cpx_type(-3.481857,0.000000),cpx_type(0.000000,0.000000),cpx_type(0.000000,0.000000),cpx_type(0.000000,0.000000),cpx_type(0.000000,0.000000),cpx_type(0.000000,0.000000),cpx_type(0.000000,0.000000),cpx_type(0.000000,0.000000)};
	QVector<double> expected_forward_backwards={-15.959960,2.034730,-9.814264,9.879696,2.720298,-0.644063,-4.790674,12.670797,10.330882,7.891354,-10.428541,11.486190,6.736045,0.433119,-6.272164,-15.520493};
    int nfft=input.size();

    CHECK(input.size()==nfft);
    CHECK(expected_forward.size()==nfft);
    CHECK(expected_forward_backwards.size()==nfft);

    FFTrWrapper<double> fft=FFTrWrapper<double>(nfft);
    FFTrWrapper<double> ifft=FFTrWrapper<double>(nfft);
    QVector<cpx_type> actual_forward;
	QVector<double> actual_forward_backwards;
    actual_forward.resize(nfft);
    actual_forward_backwards.resize(nfft);
    CHECK(actual_forward.size()==nfft);
    CHECK(actual_forward_backwards.size()==nfft);
    fft.transform(input,actual_forward);
    ifft.transform(actual_forward,actual_forward_backwards);

    for(int k=0;k<nfft;k++)
    {
        DOUBLES_EQUAL(actual_forward[k].real(),expected_forward[k].real(),doubles_equal_threshold);
        DOUBLES_EQUAL(actual_forward[k].imag(),expected_forward[k].imag(),doubles_equal_threshold);
        DOUBLES_EQUAL(actual_forward_backwards[k],expected_forward_backwards[k],doubles_equal_threshold);
    }

}
