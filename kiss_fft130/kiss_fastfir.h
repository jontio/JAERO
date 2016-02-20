#ifndef KISS_FASTFIR_H
#define KISS_FASTFIR_H
//moved header stuff from kiss_fastfir.c to here. crazy why it wasnt here in the first place
//jonti 2015
//

#include "_kiss_fft_guts.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 Some definitions that allow real or complex filtering
*/
#ifdef REAL_FASTFIR
#define MIN_FFT_LEN 2048
#include "kiss_fftr.h"
typedef kiss_fft_scalar kffsamp_t;
typedef kiss_fftr_cfg kfcfg_t;
#define FFT_ALLOC kiss_fftr_alloc
#define FFTFWD kiss_fftr
#define FFTINV kiss_fftri
#else
#define MIN_FFT_LEN 1024
typedef kiss_fft_cpx kffsamp_t;
typedef kiss_fft_cfg kfcfg_t;
#define FFT_ALLOC kiss_fft_alloc
#define FFTFWD kiss_fft
#define FFTINV kiss_fft
#endif

typedef struct kiss_fastfir_state *kiss_fastfir_cfg;



kiss_fastfir_cfg kiss_fastfir_alloc(const kffsamp_t * imp_resp,size_t n_imp_resp,
        size_t * nfft,void * mem,size_t*lenmem);

/* see do_file_filter for usage */
size_t kiss_fastfir( kiss_fastfir_cfg cfg, kffsamp_t * inbuf, kffsamp_t * outbuf, size_t n, size_t *offset);


struct kiss_fastfir_state{
    size_t nfft;
    size_t ngood;
    kfcfg_t fftcfg;
    kfcfg_t ifftcfg;
    kiss_fft_cpx * fir_freq_resp;
    kiss_fft_cpx * freqbuf;
    size_t n_freq_bins;
    kffsamp_t * tmpbuf;
};


#ifdef __cplusplus
}
#endif
#endif  // KISS_FASTFIR_H
