/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLookupTableWithEnabling.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLookupTableWithEnabling - A lookup table that allows for an
// optional array to be provided that specifies which scalars to "enable" and
// which to "disable".  
//
// .SECTION Description
// vtkLookupTableWithEnabling "disables" or "grays out" output colors
// based on whether the given value in EnabledArray is "0" or not. 
//
//
// .SECTION Caveats
// You must set the EnabledArray before MapScalars() is called.
// Indices of EnabledArray must map directly to those of the array passed
// to MapScalars().
//

#ifndef __vtkLookupTableWithEnabling_h
#define __vtkLookupTableWithEnabling_h

#include "vtkLookupTable.h"

class vtkDataArray;

class VTK_COMMON_EXPORT vtkLookupTableWithEnabling : public vtkLookupTable
{
public:
  static vtkLookupTableWithEnabling *New();
  
  vtkTypeMacro(vtkLookupTableWithEnabling,vtkLookupTable);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This must be set before MapScalars() is called.
  // Indices of this array must map directly to those in the scalars array
  // passed to MapScalars(). 
  // Values of 0 in the array indicate the color should be desaturatated. 
  vtkGetObjectMacro(EnabledArray,vtkDataArray);
  virtual void SetEnabledArray(vtkDataArray *enabledArray);

  // Description:
  // Map a set of scalars through the lookup table. 
  void MapScalarsThroughTable2(void *input, unsigned char *output,
                               int inputDataType, int numberOfValues,
                               int inputIncrement, int outputIncrement);

  // Description:
  // A convenience method for taking a color and desaturating it.  
  virtual void DisableColor(unsigned char r, unsigned char g, unsigned char b,
                   unsigned char *rd, unsigned char *gd, unsigned char *bd);

protected:
  vtkLookupTableWithEnabling(int sze=256, int ext=256);
  ~vtkLookupTableWithEnabling();

  vtkDataArray *EnabledArray;
  
private:
  vtkLookupTableWithEnabling(const vtkLookupTableWithEnabling&);  // Not implemented.
  void operator=(const vtkLookupTableWithEnabling&);  // Not implemented.
};


#endif



