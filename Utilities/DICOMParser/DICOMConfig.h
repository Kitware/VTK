/*=========================================================================

  Program:   DICOMParser
  Module:    DICOMConfig.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2003 Matt Turek
  All rights reserved.
  See Copyright.txt for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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
#ifdef DICOM_NO_STD_NAMESPACE
  #define dicom_stl
#else
  #define dicom_stl std
#endif

#ifdef DICOM_ANSI_STDLIB
  #define dicom_stream std

  #include <iostream>
  #include <fstream>
  #include <iomanip>
#else
  #define dicom_stream 

  #include <iostream.h>
  #include <fstream.h>
  #include <iomanip.h>
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
