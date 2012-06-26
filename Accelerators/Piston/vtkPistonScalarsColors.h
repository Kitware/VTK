/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPistonScalarsColors.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPistonScalarsColors - Color Mapping for piston results.
//
// .SECTION Description
// vtkPistonMapper uses this class to interface vtkScalarsToColors to GPU
// side code that implements color mapping.

#ifndef __vtkPistonScalarsColors_h
#define __vtkPistonScalarsColors_h

#include "vtkAcceleratorsPistonModule.h" // For export macro
#include <vtkObject.h>

#include <vector> // vector is required

class vtkScalarsToColors;

class VTKACCELERATORSPISTON_EXPORT vtkPistonScalarsColors : public vtkObject
{
public:
  vtkTypeMacro(vtkPistonScalarsColors, vtkObject);

  // Description:
  // Create an object with Debug turned off, modified time initialized
  // to zero, and reference counting on.
  static vtkPistonScalarsColors *New();

  // Description:
  // Methods invoked by print to print information about the object
  // including superclasses. Typically not called by the user (use
  // Print() instead) but used in the hierarchical print process to
  // combine the output of several classes.
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Set number of distinct color values
  vtkSetMacro(NumberOfValues, int);
  // Get number of distinct color values
  vtkGetMacro(NumberOfValues, int);

  // Description:
  // Set/Get the minimum/maximum scalar values for scalar mapping. Scalar
  // values less than minimum range value are clamped to minimum range value.
  // Scalar values greater than maximum range value are clamped to maximum
  // range value.
  void SetTableRange(double range[2]);
  virtual void SetTableRange(double rmin, double rmax);
  vtkGetVectorMacro(TableRange, double, 2);

  // Description:
  // Set lookup table to be used to map scalars to colors
  virtual void SetLookupTable(vtkScalarsToColors *);
  vtkGetObjectMacro(LookupTable, vtkScalarsToColors);

  // Description:
  // Compute scalars to colors as unsigned char. Size of the vector returned
  // will be NumberOfValues * numberOfChanels
  virtual std::vector<unsigned char>* ComputeScalarsColors(
    int numberOfChanels);

  // Description:
  // Compute scalars to colors as floats. Size of the vector returned
  // will be NumberOfValues * numberOfChanels
  virtual std::vector<float>* ComputeScalarsColorsf(int numberOfChanels);


protected:
  vtkPistonScalarsColors();
  ~vtkPistonScalarsColors();

  // Description:
  // Internal helper method
  void ComputeValues(float *values);

  double TableRange[2];
  int NumberOfValues;

  vtkTimeStamp ComputeColorsTime;
  std::vector<unsigned char> ScalarsColors;

  vtkTimeStamp ComputeColorsfTime;
  std::vector<float> ScalarsColorsf;

  vtkScalarsToColors *LookupTable;

private:
  vtkPistonScalarsColors(const vtkPistonScalarsColors&); // Not implemented.
  void operator=(const vtkPistonScalarsColors&);  // Not implemented.
};

#endif // __vtkPistonScalarsColors_h
