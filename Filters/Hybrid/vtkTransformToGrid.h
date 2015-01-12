/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformToGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

#ifndef vtkTransformToGrid_h
#define vtkTransformToGrid_h

#include "vtkFiltersHybridModule.h" // For export macro
#include "vtkAlgorithm.h"
#include "vtkImageData.h" // makes things a bit easier

class vtkAbstractTransform;

class VTKFILTERSHYBRID_EXPORT vtkTransformToGrid : public vtkAlgorithm
{
public:
  static vtkTransformToGrid *New();
  vtkTypeMacro(vtkTransformToGrid,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the transform which will be converted into a grid.
  virtual void SetInput(vtkAbstractTransform*);
  vtkGetObjectMacro(Input,vtkAbstractTransform);

  // Description:
  // Get/Set the extent of the grid.
  vtkSetVector6Macro(GridExtent,int);
  vtkGetVector6Macro(GridExtent,int);

  // Description:
  // Get/Set the origin of the grid.
  vtkSetVector3Macro(GridOrigin,double);
  vtkGetVector3Macro(GridOrigin,double);

  // Description:
  // Get/Set the spacing between samples in the grid.
  vtkSetVector3Macro(GridSpacing,double);
  vtkGetVector3Macro(GridSpacing,double);

  // Description:
  // Get/Set the scalar type of the grid.  The default is float.
  vtkSetMacro(GridScalarType,int);
  vtkGetMacro(GridScalarType,int);
  void SetGridScalarTypeToDouble(){this->SetGridScalarType(VTK_DOUBLE);};
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
  // real values:  dx = scale*di + shift.  If the grid is of double type,
  // then scale = 1 and shift = 0.
  double GetDisplacementScale() {
    this->UpdateShiftScale(); return this->DisplacementScale; };
  double GetDisplacementShift() {
    this->UpdateShiftScale(); return this->DisplacementShift; };

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkImageData* GetOutput();

  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation*,
                             vtkInformationVector**,
                             vtkInformationVector*);

protected:
  vtkTransformToGrid();
  ~vtkTransformToGrid();

  void RequestInformation (vtkInformation *,
                           vtkInformationVector **, vtkInformationVector *);

  void RequestData(vtkInformation *,
                   vtkInformationVector **, vtkInformationVector *);

  // Description:
  // Internal method to calculate the shift and scale values which
  // will provide maximum grid precision for a particular integer type.
  void UpdateShiftScale();

  unsigned long GetMTime();

  vtkAbstractTransform *Input;

  int GridScalarType;
  int GridExtent[6];
  double GridOrigin[3];
  double GridSpacing[3];

  double DisplacementScale;
  double DisplacementShift;
  vtkTimeStamp ShiftScaleTime;

  // see algorithm for more info
  virtual int FillOutputPortInformation(int port, vtkInformation* info);

private:
  vtkTransformToGrid(const vtkTransformToGrid&);  // Not implemented.
  void operator=(const vtkTransformToGrid&);  // Not implemented.
};

#endif
