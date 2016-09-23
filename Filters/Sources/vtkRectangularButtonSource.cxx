/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectangularButtonSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRectangularButtonSource.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkRectangularButtonSource);

//----------------------------------------------------------------------------
// Construct
vtkRectangularButtonSource::vtkRectangularButtonSource()
{
  this->Width = 0.5;
  this->Height = 0.5;
  this->Depth = 0.05;

  this->BoxRatio = 1.1;
  this->TextureRatio = 0.9;
  this->TextureHeightRatio = 0.95;

  this->OutputPointsPrecision = vtkAlgorithm::SINGLE_PRECISION;
}

//----------------------------------------------------------------------------
// One half of the button is made up of nine (quad) polygons.
//
static vtkIdType vtkRButtonPolys[72] = {
       0,1,5,4,     1,2,6,5,     2,3,7,6,     3,0,4,7,
       4,5,9,8,    5,6,10,9,   6,7,11,10,    7,4,8,11, 12,13,14,15,
     1,0,16,17,   2,1,17,18,   3,2,18,19,   0,3,19,16,
   17,16,20,21, 18,17,21,22, 19,18,22,23, 16,19,23,20, 25,24,27,26};

//----------------------------------------------------------------------------
// Generate the button.
//
int vtkRectangularButtonSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int i;
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<<"Generating rectangular button");

  // Check input
  if ( this->Width <= 0.0 || this->Height <= 0.0 )
  {
    vtkErrorMacro(<<"Button must have non-zero height and width");
    return 1;
  }

  // Create the button in several steps. First, create the button in
  // the x-y plane. After this, the z-depth is created. And if
  // it is a two-sided button, then a mirror reflection of the button
  // in the negative z-direction is created.
  int numPts = 16;
  int numCells = 9;
  if ( this->TwoSided )
  {
    numPts *= 2;
    numCells *= 2;
  }

  // Allocate memory for everything
  vtkPoints *newPts = vtkPoints::New();

  // Set the desired precision for the points in the output.
  if(this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(VTK_DOUBLE);
  }
  else
  {
    newPts->SetDataType(VTK_FLOAT);
  }

  newPts->SetNumberOfPoints(numPts);

  vtkFloatArray *tcoords = vtkFloatArray::New();
  tcoords->SetNumberOfComponents(2);
  tcoords->SetNumberOfTuples(numPts);

  vtkCellArray *newPolys = vtkCellArray::New();
  newPolys->Allocate(numCells);

  // Generate the points and texture coordinates-----------------------------
  //
  double shoulderX = this->Width / 2.0;
  double boxX = this->BoxRatio * shoulderX;
  double textureX = this->TextureRatio * shoulderX; //may be adjusted later

  double shoulderY = this->Height / 2.0;
  double boxY = this->BoxRatio * shoulderY;
  double textureY = this->TextureRatio * shoulderY; //may be adjusted later

  double shoulderZ = this->Depth;
  double boxZ = 0.0;
  double textureZ = this->TextureHeightRatio * this->Depth;

  // Adjust the texture region if the texture region should be fit to the image
  if ( this->TextureStyle == VTK_TEXTURE_STYLE_FIT_IMAGE )
  {
    double dX = (double)this->TextureDimensions[0];
    double dY = (double)this->TextureDimensions[1];

    double f1 = textureX/dX;
    double f2 = textureY/dY;
    if ( f1 < f2 )
    {
      textureX = f1 * dX;
      textureY = f1 * dY;
    }
    else
    {
      textureX = f2 * dX;
      textureY = f2 * dY;
    }
  }

  // The first four points are around the base
  newPts->SetPoint(0, this->Center[0]-boxX, this->Center[1]-boxY, this->Center[2]+boxZ);
  newPts->SetPoint(1, this->Center[0]+boxX, this->Center[1]-boxY, this->Center[2]+boxZ);
  newPts->SetPoint(2, this->Center[0]+boxX, this->Center[1]+boxY, this->Center[2]+boxZ);
  newPts->SetPoint(3, this->Center[0]-boxX, this->Center[1]+boxY, this->Center[2]+boxZ);

  // The next four points are around the shoulder
  newPts->SetPoint(4, this->Center[0]-shoulderX, this->Center[1]-shoulderY, this->Center[2]+shoulderZ);
  newPts->SetPoint(5, this->Center[0]+shoulderX, this->Center[1]-shoulderY, this->Center[2]+shoulderZ);
  newPts->SetPoint(6, this->Center[0]+shoulderX, this->Center[1]+shoulderY, this->Center[2]+shoulderZ);
  newPts->SetPoint(7, this->Center[0]-shoulderX, this->Center[1]+shoulderY, this->Center[2]+shoulderZ);

  // The next four points are between the shoulder and texture region
  newPts->SetPoint(8, this->Center[0]-textureX, this->Center[1]-textureY, this->Center[2]+textureZ);
  newPts->SetPoint(9, this->Center[0]+textureX, this->Center[1]-textureY, this->Center[2]+textureZ);
  newPts->SetPoint(10,this->Center[0]+textureX, this->Center[1]+textureY, this->Center[2]+textureZ);
  newPts->SetPoint(11,this->Center[0]-textureX, this->Center[1]+textureY, this->Center[2]+textureZ);

  // The last four points define the texture region
  newPts->SetPoint(12, this->Center[0]-textureX, this->Center[1]-textureY, this->Center[2]+textureZ);
  newPts->SetPoint(13, this->Center[0]+textureX, this->Center[1]-textureY, this->Center[2]+textureZ);
  newPts->SetPoint(14, this->Center[0]+textureX, this->Center[1]+textureY, this->Center[2]+textureZ);
  newPts->SetPoint(15, this->Center[0]-textureX, this->Center[1]+textureY, this->Center[2]+textureZ);

  if ( this->TwoSided )
  {
    // The next four points are around the shoulder
    newPts->SetPoint(16, this->Center[0]-shoulderX, this->Center[1]-shoulderY, this->Center[2]-shoulderZ);
    newPts->SetPoint(17, this->Center[0]+shoulderX, this->Center[1]-shoulderY, this->Center[2]-shoulderZ);
    newPts->SetPoint(18, this->Center[0]+shoulderX, this->Center[1]+shoulderY, this->Center[2]-shoulderZ);
    newPts->SetPoint(19, this->Center[0]-shoulderX, this->Center[1]+shoulderY, this->Center[2]-shoulderZ);

    // The next four points are between the shoulder and texture region
    newPts->SetPoint(20, this->Center[0]-textureX, this->Center[1]-textureY, this->Center[2]-textureZ);
    newPts->SetPoint(21, this->Center[0]+textureX, this->Center[1]-textureY, this->Center[2]-textureZ);
    newPts->SetPoint(22,this->Center[0]+textureX, this->Center[1]+textureY, this->Center[2]-textureZ);
    newPts->SetPoint(23,this->Center[0]-textureX, this->Center[1]+textureY, this->Center[2]-textureZ);

    // The last four points define the texture region
    newPts->SetPoint(24, this->Center[0]-textureX, this->Center[1]-textureY, this->Center[2]-textureZ);
    newPts->SetPoint(25, this->Center[0]+textureX, this->Center[1]-textureY, this->Center[2]-textureZ);
    newPts->SetPoint(26, this->Center[0]+textureX, this->Center[1]+textureY, this->Center[2]-textureZ);
    newPts->SetPoint(27, this->Center[0]-textureX, this->Center[1]+textureY, this->Center[2]-textureZ);
  }

  // Generate the texture coordinates-----------------------------
  //
  // The shoulder has the same value.
  for (i=0; i<12; i++)
  {
    tcoords->SetTuple(i,this->ShoulderTextureCoordinate);
  }

  // The texture region has just the four points
  tcoords->SetTuple2(12,0.0, 0.0);
  tcoords->SetTuple2(13,1.0, 0.0);
  tcoords->SetTuple2(14,1.0, 1.0);
  tcoords->SetTuple2(15,0.0, 1.0);

  if ( this->TwoSided )
  {
    for (i=16; i<24; i++)
    {
      tcoords->SetTuple(i,this->ShoulderTextureCoordinate);
    }
    // The texture region has just the four points
    tcoords->SetTuple2(24,1.0, 0.0);
    tcoords->SetTuple2(25,0.0, 0.0);
    tcoords->SetTuple2(26,0.0, 1.0);
    tcoords->SetTuple2(27,1.0, 1.0);
  }

  // Create the polygons------------------------------------------------------
  //
  vtkIdType *buttonPtr = vtkRButtonPolys;
  for (i=0; i<9; i++, buttonPtr+=4)
  {
    newPolys->InsertNextCell(4,buttonPtr);
  }

  // If two sided, create the opposite side.
  // Make sure that faces ordering is reversed.
  if ( this->TwoSided )
  {
    for (i=0; i<9; i++, buttonPtr+=4)
    {
      newPolys->InsertNextCell(4,buttonPtr);
    }
  }

  // Clean up and get out
  output->SetPoints(newPts);
  output->GetPointData()->SetTCoords(tcoords);
  output->SetPolys(newPolys);

  newPts->Delete();
  tcoords->Delete();
  newPolys->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkRectangularButtonSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Width: " << this->Width << "\n";
  os << indent << "Height: " << this->Height << "\n";
  os << indent << "Depth: " << this->Depth << "\n";

  os << indent << "BoxRatio: " << this->BoxRatio << "\n";
  os << indent << "TextureRatio: " << this->TextureRatio << "\n";
  os << indent << "TextureHeightRatio: " << this->TextureHeightRatio << "\n";
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision
     << "\n";
}
