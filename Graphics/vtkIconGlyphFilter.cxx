/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIconGlyphFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


#include "vtkIconGlyphFilter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointLocator.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkIconGlyphFilter);

//-----------------------------------------------------------------------------
vtkIconGlyphFilter::vtkIconGlyphFilter()
{
  this->IconSize[0] = 1;
  this->IconSize[1] = 1;
  this->IconSheetSize[0] = 1;
  this->IconSheetSize[1] = 1;
  this->Gravity = VTK_ICON_GRAVITY_CENTER_CENTER;
  this->UseIconSize = true;

  this->SetInputArrayToProcess(0, 0, 0, 
    vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
}

//-----------------------------------------------------------------------------
vtkIconGlyphFilter::~vtkIconGlyphFilter()
{
}

//-----------------------------------------------------------------------------
void vtkIconGlyphFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "IconSize: " << this->IconSize[0] << " " << this->IconSize[1] << endl;
  os << indent << "IconSheetSize: " << this->IconSheetSize[0] << " " << this->IconSheetSize[1] << endl;
  os << indent << "Gravity: " << this->Gravity << "\n";
}

//----------------------------------------------------------------------------
void vtkIconGlyphFilter::SetUseIconSize(bool b)
{
  this->UseIconSize = b;
}

//----------------------------------------------------------------------------
bool vtkIconGlyphFilter::GetUseIconSize()
{
  return this->UseIconSize;
}

//-----------------------------------------------------------------------------
int vtkIconGlyphFilter::RequestData(vtkInformation *vtkNotUsed(request),
                                  vtkInformationVector **inputVector,
                                  vtkInformationVector *outputVector)
{
  // Get the information object.
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // Get the data objects.
  vtkPointSet *input = vtkPointSet::SafeDownCast(
                                     inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
                                    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int numPoints = input->GetNumberOfPoints();

  if (numPoints <= 0)
    {
    // nothing to do...
    return 1;
    }

  vtkIntArray* scalars = vtkIntArray::SafeDownCast(
    this->GetInputArrayToProcess(0, inputVector));
  if (!scalars)
    {
    vtkErrorMacro("Input Scalars must be specified to index into the icon sheet.");
    return 0;
    }

  double point[3], textureCoord[2];
  double sheetXDim = this->IconSheetSize[0]/this->IconSize[0];
  double sheetYDim = this->IconSheetSize[1]/this->IconSize[1];
  int iconIndex = 0;
  int j, k;

  vtkPoints * outPoints = vtkPoints::New();
  outPoints->Allocate(4 * numPoints);

  vtkCellArray * outCells = vtkCellArray::New();
  outCells->Allocate(outCells->EstimateSize(numPoints, 4));

  vtkFloatArray *outTCoords = vtkFloatArray::New();
  outTCoords->SetNumberOfComponents(2);
  outTCoords->Allocate(8*numPoints);

  double size[2] = {1.0, 1.0};
  if(this->UseIconSize)
    {
    size[0] = this->IconSize[0];
    size[1] = this->IconSize[1];
    }

  for(int i = 0; i < numPoints; i++)
    {
    iconIndex = scalars->GetValue(i);

    if(iconIndex >= 0)
      {
      this->IconConvertIndex(iconIndex, j, k);

      textureCoord[0] = j/sheetXDim;
      textureCoord[1] = k/sheetYDim;
      outTCoords->InsertTuple(i * 4, textureCoord);

      textureCoord[0] = (j + 1.0)/sheetXDim;
      textureCoord[1] = k/sheetYDim;
      outTCoords->InsertTuple(i * 4 + 1, textureCoord);

      textureCoord[0] = (j + 1.0)/sheetXDim;
      textureCoord[1] = (k + 1.0)/sheetYDim;
      outTCoords->InsertTuple(i * 4 + 2, textureCoord);

      textureCoord[0] = j/sheetXDim;
      textureCoord[1] = (k + 1.0)/sheetYDim;
      outTCoords->InsertTuple(i * 4 + 3, textureCoord);
      }

    input->GetPoint(i, point);

    switch(this->Gravity)
      {
      case VTK_ICON_GRAVITY_CENTER_CENTER:
        break;
      case VTK_ICON_GRAVITY_TOP_RIGHT:
        point[0] = point[0] + 0.5 * size[0];
        point[1] = point[1] + 0.5 * size[1];
        break;
      case VTK_ICON_GRAVITY_TOP_CENTER:
        point[1] = point[1] + 0.5 * size[1];
        break;
      case VTK_ICON_GRAVITY_TOP_LEFT:
        point[0] = point[0] - 0.5 * size[0];
        point[1] = point[1] + 0.5 * size[1];
        break;
      case VTK_ICON_GRAVITY_CENTER_RIGHT:
        point[0] = point[0] + 0.5 * size[0];
        break;
      case VTK_ICON_GRAVITY_CENTER_LEFT:
        point[0] = point[0] - 0.5 * size[0];
        break;
      case VTK_ICON_GRAVITY_BOTTOM_RIGHT:
        point[0] = point[0] + 0.5 * size[0];
        point[1] = point[1] - 0.5 * size[1];
        break;
      case VTK_ICON_GRAVITY_BOTTOM_CENTER:
        point[1] = point[1] - 0.5 * size[1];
        break;
      case VTK_ICON_GRAVITY_BOTTOM_LEFT:
        point[0] = point[0] - 0.5 * size[0];
        point[1] = point[1] - 0.5 * size[1];
        break;
      }

    outPoints->InsertNextPoint(point[0] - 0.5 * size[0], point[1] - 0.5* size[1], point[2]);
    outPoints->InsertNextPoint(point[0] + 0.5 * size[0], point[1] - 0.5 * size[1], point[2]);
    outPoints->InsertNextPoint(point[0] + 0.5 * size[0], point[1] + 0.5 * size[1], point[2]);
    outPoints->InsertNextPoint(point[0] - 0.5 * size[0], point[1] + 0.5 * size[1], point[2]);

    outCells->InsertNextCell(4);
    outCells->InsertCellPoint(i * 4);
    outCells->InsertCellPoint(i * 4 + 1);
    outCells->InsertCellPoint(i * 4 + 2);
    outCells->InsertCellPoint(i * 4 + 3);
    }

  output->SetPoints(outPoints);
  outPoints->Delete();

  outTCoords->SetName("TextureCoordinates");
  output->GetPointData()->SetTCoords(outTCoords);
  outTCoords->Delete();

  output->SetPolys(outCells);
  outCells->Delete();

  return 1;
}

