/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransformToGrid.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

class VTK_EXPORT vtkTransformToGrid : public vtkImageSource
{
public:
  static vtkTransformToGrid *New();
  vtkTypeMacro(vtkTransformToGrid,vtkImageSource);
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
  vtkTransformToGrid(const vtkTransformToGrid&);
  void operator=(const vtkTransformToGrid&);

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
};

#endif
