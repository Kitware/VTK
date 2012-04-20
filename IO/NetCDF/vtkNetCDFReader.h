// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNetCDFReader.h

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

// .NAME vtkNetCDFReader
//
// .SECTION Description
//
// A superclass for reading netCDF files.  Subclass add conventions to the
// reader.  This class just outputs data into a multi block data set with a
// vtkImageData at each block.  A block is created for each variable except that
// variables with matching dimensions will be placed in the same block.

#ifndef __vtkNetCDFReader_h
#define __vtkNetCDFReader_h

#include "vtkIONetCDFModule.h" // For export macro
#include "vtkDataObjectAlgorithm.h"

#include "vtkSmartPointer.h"    // For ivars


class vtkDataArraySelection;
class vtkDataSet;
class vtkDoubleArray;
class vtkIntArray;
class vtkStdString;
class vtkStringArray;

class VTKIONETCDF_EXPORT vtkNetCDFReader : public vtkDataObjectAlgorithm
{
public:
  vtkTypeMacro(vtkNetCDFReader, vtkDataObjectAlgorithm);
  static vtkNetCDFReader *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  virtual void SetFileName(const char *filename);
  vtkGetStringMacro(FileName);

  // Description:
  // Update the meta data from the current file.  Automatically called
  // during the RequestInformation pipeline update stage.
  int UpdateMetaData();

//   // Description:
//   // Get the data array selection tables used to configure which variables to
//   // load.
//   vtkGetObjectMacro(VariableArraySelection, vtkDataArraySelection);

  // Description:
  // Variable array selection.
  virtual int GetNumberOfVariableArrays();
  virtual const char *GetVariableArrayName(int idx);
  virtual int GetVariableArrayStatus(const char *name);
  virtual void SetVariableArrayStatus(const char *name, int status);

  // Description:
  // Convenience method to get a list of variable arrays.  The length of the
  // returned list is the same as GetNumberOfVariableArrays, and the string
  // at each index i is the same as returned from GetVariableArrayname(i).
  virtual vtkStringArray *GetAllVariableArrayNames();

  // Description:
  // Returns an array with string encodings for the dimensions used in each of
  // the variables.  The indices in the returned array correspond to those used
  // in the GetVariableArrayName method.  Two arrays with the same dimensions
  // will have the same encoded string returned by this method.
  vtkGetObjectMacro(VariableDimensions, vtkStringArray);

  // Description:
  // Loads the grid with the given dimensions.  The dimensions are encoded in a
  // string that conforms to the same format as returned by
  // GetVariableDimensions and GetAllDimensions.  This method is really a
  // convenience method for SetVariableArrayStatus.  It turns on all variables
  // that have the given dimensions and turns off all other variables.
  virtual void SetDimensions(const char *dimensions);

  // Description:
  // Returns an array with string encodings for the dimension combinations used
  // in the variables.  The result is the same as GetVariableDimensions except
  // that each entry in the array is unique (a set of dimensions is only given
  // once even if it occurs for multiple variables) and the order is
  // meaningless.
  vtkGetObjectMacro(AllDimensions, vtkStringArray);

  // Description:
  // If on, any float or double variable read that has a _FillValue attribute
  // will have that fill value replaced with a not-a-number (NaN) value.  The
  // advantage of setting these to NaN values is that, if implemented properly
  // by the system and careful math operations are used, they can implicitly be
  // ignored by calculations like finding the range of the values.  That said,
  // this option should be used with caution as VTK does not fully support NaN
  // values and therefore odd calculations may occur.  By default this is off.
  vtkGetMacro(ReplaceFillValueWithNan, int);
  vtkSetMacro(ReplaceFillValueWithNan, int);
  vtkBooleanMacro(ReplaceFillValueWithNan, int);

protected:
  vtkNetCDFReader();
  ~vtkNetCDFReader();

  char *FileName;
  vtkTimeStamp FileNameMTime;
  vtkTimeStamp MetaDataMTime;

//BTX
  // Description:
  // The dimension ids of the arrays being loaded into the data.
  vtkSmartPointer<vtkIntArray> LoadingDimensions;

  vtkSmartPointer<vtkDataArraySelection> VariableArraySelection;

  vtkSmartPointer<vtkStringArray> AllVariableArrayNames;

  // Description:
  // Placeholder for structure returned from GetVariableDimensions().
  vtkStringArray *VariableDimensions;

  // Description:
  // Placeholder for structure returned from GetAllDimensions().
  vtkStringArray *AllDimensions;
//ETX

  int ReplaceFillValueWithNan;

  int WholeExtent[6];

  virtual int RequestDataObject(vtkInformation *request,
                                vtkInformationVector **inputVector,
                                vtkInformationVector *outputVector);

  virtual int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector);

  virtual int RequestData(vtkInformation *request,
                          vtkInformationVector **inputVector,
                          vtkInformationVector *outputVector);

  // Description:
  // Callback registered with the VariableArraySelection.
  static void SelectionModifiedCallback(vtkObject *caller, unsigned long eid,
                                        void *clientdata, void *calldata);

  // Description:
  // Convenience function for getting a string that describes a set of
  // dimensions.
  vtkStdString DescribeDimensions(int ncFD, const int *dimIds, int numDims);

  // Description:
  // Reads meta data and populates ivars.  Returns 1 on success, 0 on failure.
  virtual int ReadMetaData(int ncFD);

  // Description:
  // Fills the VariableDimensions array.
  virtual int FillVariableDimensions(int ncFD);

  // Description:
  // Determines whether the given variable is a time dimension.  The default
  // implementation bases the decision on the name of the variable.  Subclasses
  // should override this function if there is a more specific way to identify
  // the time variable.  This method is always called after ReadMetaData for
  // a given file.
  virtual int IsTimeDimension(int ncFD, int dimId);

//BTX
  // Description:
  // Given a dimension already determined to be a time dimension (via a call to
  // IsTimeDimension) returns an array with time values.  The default
  // implementation just uses the time index for the time value.  Subclasses
  // should override this function if there is a convention that identifies time
  // values.  This method returns 0 on error, 1 otherwise.
  virtual vtkSmartPointer<vtkDoubleArray> GetTimeValues(int ncFD, int dimId);
//ETX

  // Description:
  // Called internally to determine whether a variable with the given set of
  // dimensions should be loaded as point data (return true) or cell data
  // (return false).  The implementation in this class always returns true.
  // Subclasses should override to load cell data for some or all variables.
  virtual bool DimensionsAreForPointData(vtkIntArray *vtkNotUsed(dimensions)) {
    return true;
  }

  // Description:
  // Retrieves the update extent for the output object.  The default
  // implementation just gets the update extent from the object as you would
  // expect.  However, if a subclass is loading an unstructured data set, this
  // gives it a chance to set the range of values to read.
  virtual void GetUpdateExtentForOutput(vtkDataSet *output, int extent[6]);

  // Description:
  // Load the variable at the given time into the given data set.  Return 1
  // on success and 0 on failure.
  virtual int LoadVariable(int ncFD, const char *varName, double time,
                           vtkDataSet *output);

private:
  vtkNetCDFReader(const vtkNetCDFReader &);     // Not implemented
  void operator=(const vtkNetCDFReader &);      // Not implemented

  int UpdateExtent[6];
};

#endif //__vtkNetCDFReader_h
