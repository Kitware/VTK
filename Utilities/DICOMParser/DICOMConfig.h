
#ifndef __DICOM_CONFIG_H_
#define __DICOM_CONFIG_H_

//
// Hack for now. Hook into CMake later.
//
#define DICOM_HAS_ANSI_STREAMS

#ifdef DICOM_HAS_ANSI_STREAMS
  #define dicomstd std
#else
  #define dicomstd 
#endif

#ifdef DICOM_BUILDING_DLL
  #ifdef DICOM_EXPORT
    #define DICOM_EXPORT __declspec(dllexport)
  #else
    #define DICOM_EXPORT __declspec(dllimport)
  #endif
#else
  #define DICOM_EXPORT 
#endif

#endif
