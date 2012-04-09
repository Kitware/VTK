/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoProjection.h

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
// .NAME vtkGeoProjection - Represent a projection from a sphere to a plane
//
// .SECTION Description
// This class uses the PROJ.4 library to represent geographic coordinate
// projections.

#ifndef __vtkGeoProjection_h
#define __vtkGeoProjection_h

#include "vtkGeovisCoreModule.h" // For export macro
#include "vtkObject.h"

struct PROJconsts;
typedef PROJconsts PROJ;

class VTKGEOVISCORE_EXPORT vtkGeoProjection : public vtkObject
{
public:
  static vtkGeoProjection* New();
  virtual void PrintSelf( ostream& os, vtkIndent indent );
  vtkTypeMacro(vtkGeoProjection,vtkObject);

  // Description:
  // Returns the number of projections that this class offers.
  static int GetNumberOfProjections();

  // Description:
  // Returns the name of one of the projections supported by this class.
  // You can pass these strings to SetName(char*).
  // @param projection the index of a projection, must be in [0,GetNumberOfProjections()[.
  static const char* GetProjectionName( int projection );

  // Description:
  // Returns a description of one of the projections supported by this class.
  // @param projection the index of a projection, must be in [0,GetNumberOfProjections()[.
  static const char* GetProjectionDescription( int projection );

  // Description:
  // Set/get the short name describing the projection you wish to use.
  // This defaults to "rpoly" for no reason other than I like it.
  // To get a list of valid values, use the GetNumberOfProjections() and
  // GetProjectionName(int) static methods.
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);

  // Description:
  // Return the index of the current projection's type in the list of all projection types.
  // On error, this will return -1. On success, it returns a number in [0,GetNumberOfProjections()[.
  int GetIndex();

  // Description:
  // Get the description of a projection.
  // This will return NULL if the projection name is invalid.
  const char* GetDescription();

  // Description:
  // Set/get the longitude which corresponds to the central meridian of the projection.
  // This defaults to 0, the Greenwich Meridian.
  vtkSetMacro(CentralMeridian,double);
  vtkGetMacro(CentralMeridian,double);

  // Description:
  // Return a pointer to the PROJ.4 data structure describing this projection.
  // This may return NULL if an invalid projection name or parameter set is specified.
  // If you invoke any methods on this vtkGeoProjection object, the PROJ.4 structure
  // this method returns may be freed, so you should not use the PROJ.4 structure
  // after changing any parameters. Also, you should not modify the PROJ.4 structure
  // on your own as it will then be out of sync with the vtkGeoProjection class.
  PROJ* GetProjection();

  // Description:
  // Add an optional parameter to the projection that will be computed or
  // replace it if already present.
  void SetOptionalParameter(const char* key, const char* value);

  // Description:
  // Remove an optional parameter to the projection that will be computed
  void RemoveOptionalParameter(const char*);

  // Description:
  // Return the number of optional parameters
  int GetNumberOfOptionalParameters();

  // Description:
  // Return the number of optional parameters
  const char* GetOptionalParameterKey(int index);

  // Description:
  // Return the number of optional parameters
  const char* GetOptionalParameterValue(int index);

  // Description:
  // Clear all optional parameters
  void ClearOptionalParameters();

protected:
  vtkGeoProjection();
  virtual ~vtkGeoProjection();

  // Description:
  // Determine whether the current projection structure has any
  // changes pending and apply them if necessary.
  // Upon success, 0 is returned.
  // When an error occurs, a nonzero value is returned.
  virtual int UpdateProjection();

  char* Name;
  double CentralMeridian;
  PROJ* Projection;
  vtkTimeStamp ProjectionMTime;

private:
  vtkGeoProjection( const vtkGeoProjection& ); // Not implemented.
  void operator = ( const vtkGeoProjection& ); // Not implemented.

  class vtkInternals;
  vtkInternals* Internals;
};

#endif // __vtkGeoProjection_h
