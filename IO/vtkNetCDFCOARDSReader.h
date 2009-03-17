// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNetCDFCOARDSReader.h

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

// .NAME vtkNetCDFCOARDSReader
//
// .SECTION Description
//
// Reads netCDF files that follow the COARDS convention.  Details on this
// convention can be found at
// <http://ferret.wrc.noaa.gov/noaa_coop/coop_cdf_profile.html>.
//

#ifndef __vtkNetCDFCOARDSReader_h
#define __vtkNetCDFCOARDSReader_h

#include "vtkNetCDFReader.h"

#include <vtkStdString.h> // This class has limited exposure.  No need to
#include <vtkstd/vector>  // use PIMPL to avoid compile time penalty.

class VTK_IO_EXPORT vtkNetCDFCOARDSReader : public vtkNetCDFReader
{
public:
  vtkTypeRevisionMacro(vtkNetCDFCOARDSReader, vtkNetCDFReader);
  static vtkNetCDFCOARDSReader *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // If on (the default), then 3D data with latitude/longitude dimensions
  // will be read in as curvilinear data shaped like spherical coordinates.
  // If false, then the data will always be read in Cartesian coordinates.
  vtkGetMacro(SphericalCoordinates, int);
  vtkSetMacro(SphericalCoordinates, int);
  vtkBooleanMacro(SphericalCoordinates, int);

protected:
  vtkNetCDFCOARDSReader();
  ~vtkNetCDFCOARDSReader();

  int SphericalCoordinates;

  virtual int RequestDataObject(vtkInformation *request,
                                vtkInformationVector **inputVector,
                                vtkInformationVector *outputVector);

  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

//BTX
  // Description:
  // Interprets the special conventions of COARDS.
  virtual int ReadMetaData(int ncFD);
  virtual int IsTimeDimension(int ncFD, int dimId);
  virtual vtkSmartPointer<vtkDoubleArray> GetTimeValues(int ncFD, int dimId);
//ETX

//BTX
  class vtkDimensionInfo {
  public:
    vtkDimensionInfo() { };
    vtkDimensionInfo(int ncFD, int id);
    const char *GetName() const { return this->Name.c_str(); }
    enum UnitsEnum { UNDEFINED_UNITS, TIME_UNITS, DEGREE_UNITS };
    UnitsEnum GetUnits() const { return this->Units; }
    vtkSmartPointer<vtkDoubleArray> GetCoordinates() {return this->Coordinates;}
    bool GetHasRegularSpacing() const { return this->HasRegularSpacing; }
    double GetOrigin() const { return this->Origin; }
    double GetSpacing() const { return this->Spacing; }
  protected:
    vtkStdString Name;
    int DimId;
    vtkSmartPointer<vtkDoubleArray> Coordinates;
    int LoadMetaData(int ncFD);
    UnitsEnum Units;
    bool HasRegularSpacing;
    double Origin, Spacing;
  };
  vtkstd::vector<vtkDimensionInfo> DimensionInfo;
//ETX

private:
  vtkNetCDFCOARDSReader(const vtkNetCDFCOARDSReader &); // Not implemented
  void operator=(const vtkNetCDFCOARDSReader &);        // Not implemented
};

#endif //__vtkNetCDFCOARDSReader_h

