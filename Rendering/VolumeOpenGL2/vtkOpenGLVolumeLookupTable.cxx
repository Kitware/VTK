/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeLookupTable.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLVolumeLookupTable.h"

#include "vtkColorTransferFunction.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkTextureObject.h"

// vtkStandardNewMacro(vtkOpenGLVolumeLookupTable);

//-----------------------------------------------------------------------------
vtkOpenGLVolumeLookupTable::~vtkOpenGLVolumeLookupTable()
{
  if (this->TextureObject)
  {
    this->TextureObject->Delete();
    this->TextureObject = nullptr;
  }

  delete[] this->Table;
}

//-----------------------------------------------------------------------------
void vtkOpenGLVolumeLookupTable::Activate()
{
  if (!this->TextureObject)
  {
    return;
  }
  this->TextureObject->Activate();
}

//-----------------------------------------------------------------------------
void vtkOpenGLVolumeLookupTable::Deactivate()
{
  if (!this->TextureObject)
  {
    return;
  }
  this->TextureObject->Deactivate();
}

//-----------------------------------------------------------------------------
inline int vtkOpenGLVolumeLookupTable::GetMaximumSupportedTextureWidth(
  vtkOpenGLRenderWindow* renWin, int idealWidth)
{
  if (!this->TextureObject)
  {
    vtkErrorMacro("vtkTextureObject not initialized!");
    return -1;
  }

  // Try to match the next power of two.
  idealWidth = vtkMath::NearestPowerOfTwo(idealWidth);
  int const maxWidth = this->TextureObject->GetMaximumTextureSize(renWin);
  if (maxWidth < 0)
  {
    vtkErrorMacro("Failed to query max texture size! using default 1024.");
    return 1024;
  }

  if (maxWidth >= idealWidth)
  {
    idealWidth = vtkMath::Max(1024, idealWidth);
    return idealWidth;
  }

  vtkWarningMacro("This OpenGL implementation does not support the required "
                  "texture size of "
    << idealWidth << ", falling back to maximum allowed, " << maxWidth << "."
    << "This may cause an incorrect lookup table mapping.");

  return maxWidth;
}

//-----------------------------------------------------------------------------
int vtkOpenGLVolumeLookupTable::GetTextureUnit(void)
{
  if (!this->TextureObject)
  {
    return -1;
  }
  return this->TextureObject->GetTextureUnit();
}

//-----------------------------------------------------------------------------
void vtkOpenGLVolumeLookupTable::ReleaseGraphicsResources(vtkWindow* window)
{
  if (this->TextureObject)
  {
    this->TextureObject->ReleaseGraphicsResources(window);
    this->TextureObject->Delete();
    this->TextureObject = nullptr;
  }
}

//--------------------------------------------------------------------------
void vtkOpenGLVolumeLookupTable::ComputeIdealTextureSize(
  vtkObject* func, int& width, int& height, vtkOpenGLRenderWindow* renWin)
{
  vtkColorTransferFunction* scalarRGB = vtkColorTransferFunction::SafeDownCast(func);
  if (scalarRGB)
  {
    width = scalarRGB->EstimateMinNumberOfSamples(this->LastRange[0], this->LastRange[1]);
    height = 1;
  }
  vtkPiecewiseFunction* scalarOp = vtkPiecewiseFunction::SafeDownCast(func);
  if (scalarOp)
  {
    width = scalarOp->EstimateMinNumberOfSamples(this->LastRange[0], this->LastRange[1]);
    height = 1;
  }
  vtkImageData* transfer2D = vtkImageData::SafeDownCast(func);
  if (transfer2D)
  {
    int* dims = transfer2D->GetDimensions();
    width = dims[0];
    height = dims[1];
  }
  height = height > 1 ? this->GetMaximumSupportedTextureWidth(renWin, height) : 1;
}

//--------------------------------------------------------------------------
void vtkOpenGLVolumeLookupTable::Update(vtkObject* func, double scalarRange[2], int blendMode,
  double sampleDistance, double unitDistance, int filterValue, vtkOpenGLRenderWindow* renWin)
{
  if (!func || !renWin)
  {
    return;
  }

  if (!this->TextureObject)
  {
    this->TextureObject = vtkTextureObject::New();
  }

  this->TextureObject->SetContext(renWin);

  if (this->NeedsUpdate(func, scalarRange, blendMode, sampleDistance))
  {
    int idealW = 1024;
    int newHeight = 1;
    this->ComputeIdealTextureSize(func, idealW, newHeight, renWin);
    int const newWidth = this->GetMaximumSupportedTextureWidth(renWin, idealW);
    if (!this->Table || this->TextureWidth != newWidth || this->TextureHeight != newHeight)
    {
      this->TextureWidth = newWidth;
      this->TextureHeight = newHeight;
      this->AllocateTable();
    }

    this->InternalUpdate(func, blendMode, sampleDistance, unitDistance, filterValue);
    this->LastInterpolation = filterValue;
    this->BuildTime.Modified();
  }

  if (this->LastInterpolation != filterValue)
  {
    this->LastInterpolation = filterValue;
    this->TextureObject->SetMagnificationFilter(filterValue);
    this->TextureObject->SetMinificationFilter(filterValue);
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLVolumeLookupTable::AllocateTable()
{
  delete[] this->Table;
  this->Table = new float[this->TextureWidth * this->TextureHeight * this->NumberOfColorComponents];
}

//-----------------------------------------------------------------------------
bool vtkOpenGLVolumeLookupTable::NeedsUpdate(vtkObject* func, double scalarRange[2],
  int vtkNotUsed(blendMode), double vtkNotUsed(sampleDistance))
{
  if (!func)
  {
    return false;
  }
  if (scalarRange[0] != this->LastRange[0] || scalarRange[1] != this->LastRange[1] ||
    func->GetMTime() > this->BuildTime || this->TextureObject->GetMTime() > this->BuildTime ||
    !this->TextureObject->GetHandle())
  {
    this->LastRange[0] = scalarRange[0];
    this->LastRange[1] = scalarRange[1];
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void vtkOpenGLVolumeLookupTable::InternalUpdate(vtkObject* vtkNotUsed(func),
  int vtkNotUsed(blendMode), double vtkNotUsed(sampleDistance), double vtkNotUsed(unitDistance),
  int vtkNotUsed(filterValue))
{
}

//-----------------------------------------------------------------------------
void vtkOpenGLVolumeLookupTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "TextureObject:";
  if (this->TextureObject)
  {
    this->TextureObject->PrintSelf(os << endl, indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "Last Interpolation: " << this->LastInterpolation << endl;
  os << indent << "Last Range: (" << this->LastRange[0] << ", " << this->LastRange[1] << ")"
     << endl;
}
