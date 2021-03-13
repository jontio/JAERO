#include "../fftwrapper.h"
#include "../util/RuntimeError.h"
#include "../DSP.h"

//important for Qt include cpputest last as it mucks up new and causes compiling to fail
#include "CppUTest/TestHarness.h"

TEST_GROUP(Test_FFTWrapper)
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

TEST(Test_FFTWrapper, fftwrapper_matches_unusual_kissfft_scalling_as_expected_by_old_jaero_code_by_default)
{

    //this was obtained from v1.0.4.11 of JAERO
	QVector<cpx_type> input={cpx_type(-0.997497,0.127171),cpx_type(-0.613392,0.617481),cpx_type(0.170019,-0.040254),cpx_type(-0.299417,0.791925),cpx_type(0.645680,0.493210),cpx_type(-0.651784,0.717887),cpx_type(0.421003,0.027070),cpx_type(-0.392010,-0.970031),cpx_type(-0.817194,-0.271096),cpx_type(-0.705374,-0.668203),cpx_type(0.977050,-0.108615),cpx_type(-0.761834,-0.990661),cpx_type(-0.982177,-0.244240),cpx_type(0.063326,0.142369),cpx_type(0.203528,0.214331),cpx_type(-0.667531,0.326090)};
	QVector<cpx_type> expected_forward={cpx_type(-4.407605,0.164434),cpx_type(2.204298,2.308064),cpx_type(-2.713014,-1.356784),cpx_type(-2.347572,1.698848),cpx_type(-2.270577,-0.201056),cpx_type(1.611736,-2.136282),cpx_type(-0.902078,1.606222),cpx_type(0.335445,-0.964384),cpx_type(3.648427,0.230720),cpx_type(-2.707027,-3.571981),cpx_type(-1.023916,-0.474082),cpx_type(1.792787,2.825653),cpx_type(-5.574999,0.226081),cpx_type(1.119577,-1.518164),cpx_type(-1.273769,-1.346937),cpx_type(-3.451670,4.544378)};
	QVector<cpx_type> expected_forward_backwards={cpx_type(-15.959960,2.034730),cpx_type(-9.814264,9.879696),cpx_type(2.720298,-0.644063),cpx_type(-4.790674,12.670797),cpx_type(10.330882,7.891354),cpx_type(-10.428541,11.486190),cpx_type(6.736045,0.433119),cpx_type(-6.272164,-15.520493),cpx_type(-13.075106,-4.337535),cpx_type(-11.285989,-10.691244),cpx_type(15.632801,-1.737846),cpx_type(-12.189337,-15.850581),cpx_type(-15.714835,-3.907834),cpx_type(1.013215,2.277902),cpx_type(3.256447,3.429304),cpx_type(-10.680502,5.217444)};
    int nfft=input.size();

    CHECK(input.size()==nfft);
    CHECK(expected_forward.size()==nfft);
    CHECK(expected_forward_backwards.size()==nfft);

    FFTWrapper<double> fft=FFTWrapper<double>(nfft,false);
    FFTWrapper<double> ifft=FFTWrapper<double>(nfft,true);
    QVector<cpx_type> actual_forward,actual_forward_backwards;
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
        DOUBLES_EQUAL(actual_forward_backwards[k].real(),expected_forward_backwards[k].real(),doubles_equal_threshold); 
        DOUBLES_EQUAL(actual_forward_backwards[k].imag(),expected_forward_backwards[k].imag(),doubles_equal_threshold);
    }

}
