/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkStatisticsAlgorithmPrivate.h

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
// .NAME vtkDescriptiveStatistics - Private implementation for bivariate
// statistics algorithms.
//
// .SECTION Description
// The main purpose of this class is to avoid exposure of STL container
// through the APIs of the vtkStatistics classes APIs
//
// .SECTION Thanks
// Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories 
// for implementing this class.

#ifndef __vtkStatisticsAlgorithmPrivate_h
#define __vtkStatisticsAlgorithmPrivate_h

#include "vtkStdString.h"

#include <vtkstd/set> // used to iterate over internal organs

class vtkStatisticsAlgorithmPrivate
{
public:
  vtkStatisticsAlgorithmPrivate()
    {
    }
  ~vtkStatisticsAlgorithmPrivate()
    {
    }
  void SetBufferColumnStatus( const char* colName, int status )
    {
    if ( status )
      {
      this->Buffer.insert( colName );
      }
    else
      {
      this->Buffer.erase( colName );
      }
    }
  int AddBufferToRequests()
    {
    bool result = false;
    // Don't add empty selections to the list of requests.
    if ( ! this->Buffer.empty() )
      {
      result = this->Requests.insert( this->Buffer ).second;
      }
    return result ? 1 : 0;
    }
  int AddBufferEntriesToRequests()
    {
    int count = 0;
    vtkstd::set<vtkStdString>::iterator it;
    for ( it = this->Buffer.begin(); it != this->Buffer.end(); ++ it )
      {
      vtkstd::set<vtkStdString> tmp;
      tmp.insert( *it );
      if ( this->Requests.insert( tmp ).second )
        {
        ++ count;
        }
      }
    return count;
    }
  int AddBufferEntryPairsToRequests()
    {
    int count = 0;
    vtkstd::pair<vtkstd::set<vtkstd::set<vtkStdString> >::iterator,bool> result;
    vtkstd::set<vtkStdString>::iterator it;
    for ( it = this->Buffer.begin(); it != this->Buffer.end(); ++ it )
      {
      vtkstd::set<vtkStdString>::iterator it2 = it;
      for ( ++ it2; it2 != this->Buffer.end(); ++ it2 )
        {
        vtkstd::set<vtkStdString> tmp;
        tmp.insert( *it );
        tmp.insert( *it2 );
        if ( this->Requests.insert( tmp ).second )
          {
          ++ count;
          }
        }
      }
    return count;
    }
  void ResetRequests()
    {
    this->Requests.clear();
    }
  void ResetBuffer()
    {
    this->Buffer.clear();
    }
  
  vtkstd::set<vtkstd::set<vtkStdString> > Requests;
  vtkstd::set<vtkStdString> Buffer;
};

#endif // __vtkStatisticsAlgorithmPrivate_h

