#ifndef kiss_fft_exports_h
#define kiss_fft_exports_h
/****************************************************************
*  Export parameters
*****************************************************************/
/*
*  KISSFFT_DLL_EXPORT :
*  Enable exporting of functions when building a Windows DLL
*  KISSFFTLIB_API :
*  Control library symbols visibility.
*/
#if defined(KISSFFT_DLL_EXPORT) && (KISSFFT_DLL_EXPORT==1)
#  define KISSFFTLIB_API __declspec(dllexport)
#elif defined(KISSFFT_DLL_IMPORT) && (KISSFFT_DLL_IMPORT==1)
#  define KISSFFTLIB_API __declspec(dllimport)
#elif defined(__GNUC__) && (__GNUC__ >= 4)
#  define KISSFFTLIB_API __attribute__ ((__visibility__ ("default")))
#else
#  define KISSFFTLIB_API
#endif

#endif // ifndef kiss_fft_exports_h
