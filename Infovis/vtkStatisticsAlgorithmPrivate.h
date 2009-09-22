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

#include <vtksys/stl/set> // used to iterate over internal organs

class vtkStatisticsAlgorithmPrivate
{
public:
  vtkStatisticsAlgorithmPrivate()
    {
    }
  ~vtkStatisticsAlgorithmPrivate()
    {
    }
  int SetBufferColumnStatus( const char* colName, int status )
    {
    if ( status )
      {
      return this->Buffer.insert( colName ).second ? 1 : 0;
      }
    else
      {
      return this->Buffer.erase( colName ) ? 1 : 0;
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
    vtksys_stl::set<vtkStdString>::iterator it;
    for ( it = this->Buffer.begin(); it != this->Buffer.end(); ++ it )
      {
      vtksys_stl::set<vtkStdString> tmp;
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
    vtksys_stl::pair<vtksys_stl::set<vtksys_stl::set<vtkStdString> >::iterator,bool> result;
    vtksys_stl::set<vtkStdString>::iterator it;
    for ( it = this->Buffer.begin(); it != this->Buffer.end(); ++ it )
      {
      vtksys_stl::set<vtkStdString>::iterator it2 = it;
      for ( ++ it2; it2 != this->Buffer.end(); ++ it2 )
        {
        vtksys_stl::set<vtkStdString> tmp;
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
  /// This function doesn't use the buffer like other column selection methods.
  int AddColumnPairToRequests( const char* cola, const char* colb )
    {
    if ( cola && colb && strlen( cola ) && strlen( colb ) )
      {
      vtksys_stl::set<vtkStdString> tmp;
      tmp.insert( cola );
      tmp.insert( colb );
      if ( this->Requests.insert( tmp ).second )
        {
        return 1;
        }
      }
    return 0;
    }
  void ResetRequests()
    {
    this->Requests.clear();
    }
  int ResetBuffer()
    {
    int rval = this->Buffer.empty() ? 0 : 1;
    this->Buffer.clear();
    return rval;
    }
  /// Return the number of currently-defined requests
  vtkIdType GetNumberOfRequests()
    {
    return static_cast<vtkIdType>( this->Requests.size() );
    }
  /// Return the number of columns associated with request \a r.
  vtkIdType GetNumberOfColumnsForRequest( vtkIdType r )
    {
    if ( r < 0 || r > static_cast<vtkIdType>( this->Requests.size() ) )
      {
      return 0;
      }
    vtksys_stl::set<vtksys_stl::set<vtkStdString> >::iterator it = this->Requests.begin();
    for ( vtkIdType i = 0; i < r; ++ i )
      {
      ++ it;
      }
    return it->size();
    }
  /**\brief Provide the name of the \a c-th column of the \a r-th request in \a columnName.
    * Returns false if the request or column does not exist and true otherwise.
    */
  bool GetColumnForRequest( vtkIdType r, vtkIdType c, vtkStdString& columnName )
    {
    if ( r < 0 || r > static_cast<vtkIdType>( this->Requests.size() ) || c < 0 )
      {
      return false;
      }
    vtksys_stl::set<vtksys_stl::set<vtkStdString> >::const_iterator it = this->Requests.begin();
    for ( vtkIdType i = 0; i < r; ++ i )
      {
      ++ it;
      }
    if ( c > static_cast<vtkIdType>( it->size() ) )
      {
      return false;
      }
    vtksys_stl::set<vtkStdString>::const_iterator cit = it->begin();
    for ( vtkIdType j = 0; j < c; ++ j )
      {
      ++ cit;
      }
    columnName = *cit;
    return true;
    }
  
  vtksys_stl::set<vtksys_stl::set<vtkStdString> > Requests;
  vtksys_stl::set<vtkStdString> Buffer;
};

#endif // __vtkStatisticsAlgorithmPrivate_h

