
#ifndef __DICOM_CONFIG_H_
#define __DICOM_CONFIG_H_

//
// CMake Hook
//
#include "DICOMCMakeConfig.h"

//
// BEGIN Toolkit (ITK,VTK, etc) specific
// 
#ifdef vtkDICOMParser_EXPORTS
  #define DICOM_EXPORT_SYMBOLS
#endif
//
// END toolkit (ITK, VTK, etc) specific
//

#ifdef DICOM_ANSI_STDLIB
  #define dicom_stream std
  #define dicom_stl std
#else
  #define dicom_stream 
  #define dicom_stl std
  #include <fstream.h>
  #include <string.h>
#endif

#ifdef DICOM_DLL
  #ifdef DICOM_EXPORT_SYMBOLS
    #define DICOM_EXPORT __declspec(dllexport)
    #define DICOM_EXPIMP_TEMPLATE 
  #else
    #define DICOM_EXPORT __declspec(dllimport)
    #define DICOM_EXPIMP_TEMPLATE extern
  #endif
#else
  #define DICOM_EXPORT 
#endif

#endif // __DICOM_CONFIG_H_
