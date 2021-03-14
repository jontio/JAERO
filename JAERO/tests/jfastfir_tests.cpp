#include "../fftrwrapper.h"
#include "../util/RuntimeError.h"
#include "../DSP.h"
#include "../util/stdio_utils.h"
#include "../util/file_utils.h"
#include "jfastfir_data.h"

//important for Qt include cpputest last as it mucks up new and causes compiling to fail
#include "CppUTest/TestHarness.h"

TEST_GROUP(Test_JFastFir)
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

TEST(Test_JFastFir, check_matches_old_jaero_code)
{


    JFastFir fir_pre;
    RootRaisedCosine rrc_pre_imp;
    rrc_pre_imp.design(0.6,2048,Fs,fb/2);
    fir_pre.SetKernel(rrc_pre_imp.Points,4096);

    QVector<cpx_type> actual_output(input);
    fir_pre.update(actual_output);

    CHECK(input.size()==expected_output.size());
    CHECK(actual_output.size()==expected_output.size());


    File_Utils::Matlab::print(QString(MATLAB_PATH)+"actual_input_include.m","actual_input",input);
    File_Utils::Matlab::print(QString(MATLAB_PATH)+"actual_output_include.m","actual_output",actual_output);


    //the first 4096 samples need not match
    for(int k=4096;k<actual_output.size();k++)
    {
        DOUBLES_EQUAL(actual_output[k].real(),expected_output[k].real(),doubles_equal_threshold);
        DOUBLES_EQUAL(actual_output[k].imag(),expected_output[k].imag(),doubles_equal_threshold);
    }

}
