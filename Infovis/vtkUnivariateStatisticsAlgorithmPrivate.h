/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkUnivariateStatisticsAlgorithmPrivate.h

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
// .NAME vtkDescriptiveStatistics - Private implementation for univariate
// statistics algorithms.
//
// .SECTION Description
// The main purpose of this class is to avoid exposure of STL container
// through the APIs of the vtkStatistics classes APIs
//
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories 
// for implementing this class.

#ifndef __vtkUnivariateStatisticsAlgorithmPrivate_h
#define __vtkUnivariateStatisticsAlgorithmPrivate_h

#include "vtkStdString.h"

#include <vtkstd/set> // used to iterate over internal organs

#if defined( _MSC_VER ) && _MSC_VER < 1300
// Eliminate the following VS60 warning:
// warning C4786:
// 'Std::pair<Std::_Tree<int,int,Std::set<int,Std::less<int>,Std::allocator<int> >::_Kfn,Std::less<int>,Std::allocator<int> >::const_iterator,Std::_Tree<int,int,Std::set<int,Std::less<int>,Std::allocator<int> >::_Kfn,Std::less<int>,Std::allocator<int> >::const_iterator>' :
// identifier was truncated to '255' characters in the debug informationwarning C4611: interaction between '_setjmp' and C++ object
# pragma warning ( disable : 4786 )
#endif

class vtkUnivariateStatisticsAlgorithmPrivate
{
public:
  vtkUnivariateStatisticsAlgorithmPrivate()
    {
    }
  ~vtkUnivariateStatisticsAlgorithmPrivate()
    {
    }
  
  vtkstd::set<vtkStdString> Selection;
};

#endif // __vtkUnivariateStatisticsAlgorithmPrivate_h
