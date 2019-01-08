#ifndef vtkkissfft_fft_mangle_h
#define vtkkissfft_fft_mangle_h

/**
 * For regenerating instructions, see:
 * https://gitlab.kitware.com/third-party/repo-requests/wikis/Mangling
 */

#define kiss_fastfir vtkkissfft_fastfir
#define kiss_fastfir_alloc vtkkissfft_fastfir_alloc
#define kiss_fft vtkkissfft_fft
#define kiss_fft_alloc vtkkissfft_fft_alloc
#define kiss_fft_cleanup vtkkissfft_fft_cleanup
#define kiss_fftnd vtkkissfft_fftnd
#define kiss_fftnd_alloc vtkkissfft_fftnd_alloc
#define kiss_fftndr vtkkissfft_fftndr
#define kiss_fftndr_alloc vtkkissfft_fftndr_alloc
#define kiss_fftndri vtkkissfft_fftndri
#define kiss_fft_next_fast_size vtkkissfft_fft_next_fast_size
#define kiss_fftr vtkkissfft_fftr
#define kiss_fftr_alloc vtkkissfft_fftr_alloc
#define kiss_fftri vtkkissfft_fftri
#define kiss_fft_stride vtkkissfft_fft_stride

#endif
