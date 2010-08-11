/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextAnalysisUtility.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

// .NAME vtkTextAnalysisUtility
//
// .SECTION Description
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.

#ifndef _vtkTextAnalysisUtility_h
#define _vtkTextAnalysisUtility_h

#include "vtkSystemIncludes.h"

class VTK_TEXT_ANALYSIS_EXPORT vtkTextAnalysisUtility
{
public:
  // Description:
  // Returns a list of newline-delimited stop-words suitable for use during
  // text feature-extraction.
  static const char* DefaultStopWords();
};

#endif // !_vtkTextAnalysisUtility_h

