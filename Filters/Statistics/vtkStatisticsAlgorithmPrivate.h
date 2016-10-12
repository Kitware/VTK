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
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
/**
 * @class   vtkDescriptiveStatistics
 * @brief   Private implementation for bivariate
 * statistics algorithms.
 *
 *
 * The main purpose of this class is to avoid exposure of STL container
 * through the APIs of the vtkStatistics classes APIs
 *
 * @par Thanks:
 * Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories
 * for implementing this class.
*/

#ifndef vtkStatisticsAlgorithmPrivate_h
#define vtkStatisticsAlgorithmPrivate_h

#include "vtkStdString.h"

#include <set> // used to iterate over internal organs

class vtkStatisticsAlgorithmPrivate
{
public:
  vtkStatisticsAlgorithmPrivate()
  {
  }
  ~vtkStatisticsAlgorithmPrivate()
  {
  }
  // --------------------------------------------------------------------
  /**
   * Empty current set of requests
   */
  void ResetRequests()
  {
    this->Requests.clear();
  }
  // --------------------------------------------------------------------
  //@{
  /**
   * Empty current buffer
   */
  int ResetBuffer()
  {
    int rval = this->Buffer.empty() ? 0 : 1;
    this->Buffer.clear();
    return rval;
  }
  // --------------------------------------------------------------------
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
  // --------------------------------------------------------------------
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
  // --------------------------------------------------------------------
  //@}
  //@{
  /**
   * This function does not use the buffer like other column selection methods.
   */
  int AddColumnToRequests( const char* col )
  {
    if ( col && strlen( col ) )
    {
      std::set<vtkStdString> tmp;
      tmp.insert( col );
      if ( this->Requests.insert( tmp ).second )
      {
        return 1;
      }
    }
    return 0;
  }
  // --------------------------------------------------------------------
  //@}
  //@{
  /**
   * This function does not use the buffer like other column selection methods.
   */
  int AddColumnPairToRequests( const char* cola, const char* colb )
  {
    if ( cola && colb && strlen( cola ) && strlen( colb ) )
    {
      std::set<vtkStdString> tmp;
      tmp.insert( cola );
      tmp.insert( colb );
      if ( this->Requests.insert( tmp ).second )
      {
        return 1;
      }
    }
    return 0;
  }
  // --------------------------------------------------------------------
  //@}
  /**
   * Return the number of currently-defined requests
   */
  vtkIdType GetNumberOfRequests()
  {
    return static_cast<vtkIdType>( this->Requests.size() );
  }
  // --------------------------------------------------------------------
  //@{
  /**
   * Return the number of columns associated with request \a r.
   */
  vtkIdType GetNumberOfColumnsForRequest( vtkIdType r )
  {
    if ( r < 0 || r > static_cast<vtkIdType>( this->Requests.size() ) )
    {
      return 0;
    }
    std::set<std::set<vtkStdString> >::iterator it = this->Requests.begin();
    for ( vtkIdType i = 0; i < r; ++ i )
    {
      ++ it;
    }
    return it->size();
  }
  // --------------------------------------------------------------------
  //@}
  //@{
  /**
   * Provide the name of the \a c-th column of the \a r-th request in \a columnName.
   * Returns false if the request or column does not exist and true otherwise.
   */
  bool GetColumnForRequest( vtkIdType r, vtkIdType c, vtkStdString& columnName )
  {
    if ( r < 0 || r > static_cast<vtkIdType>( this->Requests.size() ) || c < 0 )
    {
      return false;
    }
    std::set<std::set<vtkStdString> >::const_iterator it = this->Requests.begin();
    for ( vtkIdType i = 0; i < r; ++ i )
    {
      ++ it;
    }
    if ( c > static_cast<vtkIdType>( it->size() ) )
    {
      return false;
    }
    std::set<vtkStdString>::const_iterator cit = it->begin();
    for ( vtkIdType j = 0; j < c; ++ j )
    {
      ++ cit;
    }
    columnName = *cit;
    return true;
  }
  //@}

  std::set<std::set<vtkStdString> > Requests;
  std::set<vtkStdString> Buffer;
};

#endif // vtkStatisticsAlgorithmPrivate_h

// VTK-HeaderTest-Exclude: vtkStatisticsAlgorithmPrivate.h
