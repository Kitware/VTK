// SPDX-FileCopyrightText: Copyright (c) 2003 Matt Turek
// SPDX-License-Identifier: BSD-4-Clause
#ifndef __DICOM_CONFIG_H_
#define __DICOM_CONFIG_H_

#include "vtkABINamespace.h"

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

#ifdef DICOM_DLL
#ifdef DICOMParser_EXPORTS
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
