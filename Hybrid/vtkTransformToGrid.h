/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformToGrid.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTransformToGrid - create a grid for a vtkGridTransform
// .SECTION Description
// vtkTransformToGrid takes any transform as input and produces a grid
// for use by a vtkGridTransform.  This can be used, for example, to 
// invert a grid transform, concatenate two grid transforms, or to
// convert a thin plate spline transform into a grid transform.
// .SECTION See Also
// vtkGridTransform vtkThinPlateSplineTransform vtkAbstractTransform

#ifndef __vtkTransformToGrid_h
#define __vtkTransformToGrid_h

#include "vtkImageSource.h"
#include "vtkAbstractTransform.h"

class VTK_HYBRID_EXPORT vtkTransformToGrid : public vtkImageSource
{
public:
  static vtkTransformToGrid *New();
  vtkTypeRevisionMacro(vtkTransformToGrid,vtkImageSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the transform which will be converted into a grid.
  vtkSetObjectMacro(Input,vtkAbstractTransform);
  vtkGetObjectMacro(Input,vtkAbstractTransform);

  // Description:
  // Get/Set the extent of the grid.
  vtkSetVector6Macro(GridExtent,int);
  vtkGetVector6Macro(GridExtent,int);

  // Description:
  // Get/Set the origin of the grid.
  vtkSetVector3Macro(GridOrigin,float);
  vtkGetVector3Macro(GridOrigin,float);

  // Description:
  // Get/Set the spacing between samples in the grid.
  vtkSetVector3Macro(GridSpacing,float);
  vtkGetVector3Macro(GridSpacing,float);

  // Description:
  // Get/Set the scalar type of the grid.  The default is
  // float.
  vtkSetMacro(GridScalarType,int);
  vtkGetMacro(GridScalarType,int);
  void SetGridScalarTypeToFloat(){this->SetGridScalarType(VTK_FLOAT);};
  void SetGridScalarTypeToShort(){this->SetGridScalarType(VTK_SHORT);};
  void SetGridScalarTypeToUnsignedShort()
    {this->SetGridScalarType(VTK_UNSIGNED_SHORT);};
  void SetGridScalarTypeToUnsignedChar()
    {this->SetGridScalarType(VTK_UNSIGNED_CHAR);};
  void SetGridScalarTypeToChar()
    {this->SetGridScalarType(VTK_CHAR);};

  // Description:
  // Get the scale and shift to convert integer grid elements into
  // real values:  dx = scale*di + shift.  If the grid is of float type,
  // then scale = 1 and shift = 0.
  float GetDisplacementScale() {
    this->UpdateShiftScale(); return this->DisplacementScale; };
  float GetDisplacementShift() {
    this->UpdateShiftScale(); return this->DisplacementShift; };

protected:
  vtkTransformToGrid();
  ~vtkTransformToGrid();

  void ExecuteInformation();

  void ExecuteData(vtkDataObject *data);

  // Description:
  // Internal method to calculate the shift and scale values which
  // will provide maximum grid precision for a particular integer type.
  void UpdateShiftScale();

  unsigned long GetMTime();

  vtkAbstractTransform *Input;

  int GridScalarType;
  int GridExtent[6];
  float GridOrigin[3];
  float GridSpacing[3];

  float DisplacementScale;
  float DisplacementShift;
  vtkTimeStamp ShiftScaleTime;
private:
  vtkTransformToGrid(const vtkTransformToGrid&);  // Not implemented.
  void operator=(const vtkTransformToGrid&);  // Not implemented.
};

#endif
