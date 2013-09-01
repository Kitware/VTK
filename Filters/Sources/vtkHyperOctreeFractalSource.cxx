/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperOctreeFractalSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperOctreeFractalSource.h"

#include "vtkHyperOctree.h"
#include "vtkHyperOctreeCursor.h"
#include "vtkObjectFactory.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include <cassert>
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkFloatArray.h"
#include "vtkGarbageCollector.h"

vtkStandardNewMacro(vtkHyperOctreeFractalSource);

//----------------------------------------------------------------------------
vtkHyperOctreeFractalSource::vtkHyperOctreeFractalSource()
{
  this->SetNumberOfInputPorts(0);

  this->SizeCX[0] = 2.5;
  this->SizeCX[1] = 2.5;
  this->SizeCX[2] = 2.0;
  this->SizeCX[3] = 1.5;

  this->OriginCX[0] = -1.75;
  this->OriginCX[1] = -1.25;
  this->OriginCX[2] = 0.0;
  this->OriginCX[3] = 0.0;

  this->ProjectionAxes[0] = 0;
  this->ProjectionAxes[1] = 1;
  this->ProjectionAxes[2] = 2;

  this->Dimension = 3;

  this->MaximumLevel=5;
  this->MinimumLevel=3;

  this->MaximumNumberOfIterations = 100;

  this->SpanThreshold = 2.0;
}

//----------------------------------------------------------------------------
vtkHyperOctreeFractalSource::~vtkHyperOctreeFractalSource()
{
}

//----------------------------------------------------------------------------
void vtkHyperOctreeFractalSource::SetProjectionAxes(int x, int y, int z)
{
  if (this->ProjectionAxes[0] != x ||
      this->ProjectionAxes[1] != y ||
      this->ProjectionAxes[2] != z )
    {
    this->ProjectionAxes[0] = x;
    this->ProjectionAxes[1] = y;
    this->ProjectionAxes[2] = z;

    this->Modified();
    }
}

//----------------------------------------------------------------------------
// Description:
// Return the maximum number of levels of the hyperoctree.
// \post positive_result: result>=1
int vtkHyperOctreeFractalSource::GetMaximumLevel()
{
  assert("post: positive_result" && this->MaximumLevel>=1);
  return this->MaximumLevel;
}

//----------------------------------------------------------------------------
// Description:
// Set the maximum number of levels of the hyperoctree. If
// GetMinLevels()>=levels, GetMinLevels() is changed to levels-1.
// \pre positive_levels: levels>=1
// \post is_set: this->GetLevels()==levels
// \post min_is_valid: this->GetMinLevels()<this->GetLevels()
void vtkHyperOctreeFractalSource::SetMaximumLevel(int levels)
{
  if (levels < 1)
    {
    levels = 1;
    }

  if (this->MaximumLevel == levels)
    {
    return;
    }

  this->Modified();
  this->MaximumLevel=levels;
  if(this->MinimumLevel>levels)
    {
    this->MinimumLevel=levels;
    }

  assert("post: is_set" && this->GetMaximumLevel()==levels);
  assert("post: min_is_valid" && this->GetMinimumLevel()<=this->GetMaximumLevel());
}


//----------------------------------------------------------------------------
// Description:
// Return the minimal number of levels of systematic subdivision.
// \post positive_result: result>=0
int vtkHyperOctreeFractalSource::GetMinimumLevel()
{
  assert("post: positive_result" && this->MinimumLevel>=0);
  return this->MinimumLevel;
}

//----------------------------------------------------------------------------
// Description:
// Set the minimal number of levels of systematic subdivision.
// \pre positive_minLevels: minLevels>=0 && minLevels<this->GetLevels()
// \post is_set: this->GetMinLevels()==minLevels
void vtkHyperOctreeFractalSource::SetMinimumLevel(int minLevels)
{
  if (minLevels < 1)
    {
    minLevels = 1;
    }

  if (this->MinimumLevel == minLevels)
    {
    return;
    }

  this->Modified();
  this->MinimumLevel = minLevels;
  assert("post: is_set" && this->GetMinimumLevel()==minLevels);
}

//----------------------------------------------------------------------------
int vtkHyperOctreeFractalSource::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector ** vtkNotUsed( inputVector ),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // We cannot give the exact number of levels of the hyperoctree
  // because it is not generated yet and this process is random-based.
  // Just send an upper limit.
  // Used by the vtkHyperOctreeToUniformGrid to send some
  // whole extent in RequestInformation().
  outInfo->Set(vtkHyperOctree::LEVELS(),this->MaximumLevel);
  outInfo->Set(vtkHyperOctree::DIMENSION(),this->Dimension);
  int ii;
  for (ii = 0; ii < 3; ++ii)
    {
    this->Size[ii] = this->SizeCX[this->ProjectionAxes[ii]];
    this->Origin[ii] = this->OriginCX[this->ProjectionAxes[ii]];
    }
  if (this->Dimension == 2)
    {
    this->Size[2] = 0.0;
    }
  outInfo->Set(vtkHyperOctree::SIZES(),this->Size,3);
  outInfo->Set(vtkDataObject::ORIGIN(),this->Origin,3);

  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperOctreeFractalSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkHyperOctree *output = vtkHyperOctree::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  output->SetDimension(this->Dimension);
  int ii;
  for (ii = 0; ii < 3; ++ii)
    {
    this->Size[ii] = this->SizeCX[this->ProjectionAxes[ii]];
    this->Origin[ii] = this->OriginCX[this->ProjectionAxes[ii]];
    }
  output->SetSize(this->Size);
  output->SetOrigin(this->Origin);

  vtkFloatArray *scalars=vtkFloatArray::New();
  scalars->SetNumberOfComponents(1);

  vtkIdType fact=(1<<(this->MaximumLevel-1));
  vtkIdType maxNumberOfCells=fact*fact*fact;

  scalars->Allocate(maxNumberOfCells/fact);
  scalars->SetName("FractalIterations");
  output->GetLeafData()->SetScalars(scalars);
  scalars->UnRegister(this);

  vtkHyperOctreeCursor *cursor=output->NewCellCursor();
  cursor->ToRoot();

  // Eight corner values.
  double sample[3];
  float cornerVals[8];
  int numCorners = 1 << this->Dimension;
  for (ii = 0; ii < numCorners; ++ii)
    {
    sample[0] = this->Origin[0];
    sample[1] = this->Origin[1];
    sample[2] = this->Origin[2];
    if (ii&1)
      {
      sample[0] += this->Size[0];
      }
    if (ii&2)
      {
      sample[1] += this->Size[1];
      }
    if (ii&4)
      {
      sample[2] += this->Size[2];
      }
    cornerVals[ii] = this->EvaluateWorldPoint(sample);
    }

  this->Subdivide(cursor,1,output, this->Origin,this->Size, cornerVals);

  cursor->UnRegister(this);

  scalars->Squeeze();
  assert("post: valid_levels" && output->GetNumberOfLevels()<=this->GetMaximumLevel());
  assert("post: dataset_and_data_size_match" && output->CheckAttributes()==0);

  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperOctreeFractalSource::Subdivide(vtkHyperOctreeCursor *cursor,
                                            int level, vtkHyperOctree *output,
                                            double* origin, double* size,
                                            float* cornerVals)
{
  // Determine whether to subdivide.
  int subdivide = 1;
  int ii;
  float min, max;
  int numCorners = 1 << this->Dimension;
  min = VTK_FLOAT_MAX;
  max = 0.0;
  for (ii = 0; ii < numCorners; ++ii)
    {
    if (cornerVals[ii] < min)
      {
      min = cornerVals[ii];
      }
    if (cornerVals[ii] > max)
      {
      max = cornerVals[ii];
      }
    }
  if (max-min <= this->SpanThreshold)
    {
    subdivide = 0;
    }

  // Check for hard coded minimum and maximum level restrictions.
  if (level < this->MinimumLevel)
    {
    subdivide = 1;
    }
  if (level >= this->MaximumLevel)
    {
    subdivide = 0;
    }

  if (subdivide)
    {
    output->SubdivideLeaf(cursor);
    double newOrigin[3];
    double newSize[3];
    float newCornerVals[8];
    int ix, iy, iz;
    newSize[0] = size[0] * 0.5;
    newSize[1] = size[1] * 0.5;
    newSize[2] = size[2] * 0.5;

    // Make a tempoaray 3x3x3 array of fractal values.
    float values[27];
    memset(values,0,27*sizeof(float));
    values[0]  = cornerVals[0]; // 000
    values[2]  = cornerVals[1]; // 002
    values[6]  = cornerVals[2]; // 020
    values[8]  = cornerVals[3]; // 022
    if (this->Dimension == 3)
      { // We could just fill these in with junk for 2d.
      values[18] = cornerVals[4]; // 200
      values[20] = cornerVals[5]; // 202
      values[24] = cornerVals[6]; // 220
      values[26] = cornerVals[7]; // 222
      }
    // Find values for samples that have not been initialized.
    // edges, faces, and center.
    int valueIdx;
    int zNum = 3;
    if (this->Dimension == 2)
      { // 1d not allowed for now.
      zNum = 1;
      }
    for (iz = 0; iz < zNum; ++iz)
      {
      for (iy = 0; iy < 3; ++iy)
        {
        for (ix = 0; ix < 3; ++ix)
          {
          valueIdx = ix + 3*iy + 9*iz;
          if (values[valueIdx] == 0.0)
            {
            double sample[3];
            sample[0] = origin[0] + newSize[0]*static_cast<double>(ix);
            sample[1] = origin[1] + newSize[1]*static_cast<double>(iy);
            sample[2] = origin[2] + newSize[2]*static_cast<double>(iz);
            values[valueIdx] = this->EvaluateWorldPoint(sample);
            }
          }
        }
      }
    // Now traverse to children.
    for (ii = 0; ii < numCorners; ++ii)
      {
      // extract the 8 2x2 corners from the grid of 27 3x3x3.
      int xStart = ii & 1;
      int yStart = (ii>>1) & 1;
      int zStart = (ii>>2) & 1;
      int zEnd = 2;
      if (this->Dimension == 2)
        {
        zEnd = 1;
        }
      for (iz = 0; iz < zEnd; ++iz)
        {
        for (iy = 0; iy < 2; ++iy)
          {
          for (ix = 0; ix < 2; ++ix)
            {
            newCornerVals[ix + iy*2 + iz*4]
              = values[(xStart+ix)+(yStart+iy)*3 + (zStart+iz)*9];
            }
          }
        }
      newOrigin[0] = origin[0] + static_cast<double>(xStart)*newSize[0];
      newOrigin[1] = origin[1] + static_cast<double>(yStart)*newSize[1];
      newOrigin[2] = origin[2] + static_cast<double>(zStart)*newSize[2];

      cursor->ToChild(ii);
      this->Subdivide(cursor,level+1,output,
                      newOrigin,newSize,newCornerVals);
      cursor->ToParent();
      }
    }
  else
    {
    double center[3];
    center[0] = origin[0] + 0.5*size[0];
    center[1] = origin[1] + 0.5*size[1];
    center[2] = origin[2];
    if (this->Dimension == 3)
      {
      center[2] += 0.5*size[2];
      }
    float val = this->EvaluateWorldPoint(center);
    // Weight cell values for smoother iso surface.
    float fVal = 0.0;
    if (this->Dimension == 3)
      {
      fVal = (static_cast<float>(val) * 4
              + static_cast<float>(cornerVals[0])
              + static_cast<float>(cornerVals[1])
              + static_cast<float>(cornerVals[2])
              + static_cast<float>(cornerVals[3])
              + static_cast<float>(cornerVals[4])
              + static_cast<float>(cornerVals[5])
              + static_cast<float>(cornerVals[6])
              + static_cast<float>(cornerVals[7])) / 12.0;
      }
    else if (this->Dimension == 2)
      {
      fVal = (static_cast<float>(val) * 2
              + static_cast<float>(cornerVals[0])
              + static_cast<float>(cornerVals[1])
              + static_cast<float>(cornerVals[2])
              + static_cast<float>(cornerVals[3])) / 6.0;
      }
    vtkIdType id=cursor->GetLeafId();
    output->GetLeafData()->GetScalars()->InsertTuple1(id,fVal);
    }
}


//-----------------------------------------------------------------------------
float vtkHyperOctreeFractalSource::EvaluateWorldPoint(double p[3])
{
  double p4[4];

  p4[0] = this->OriginCX[0];
  p4[1] = this->OriginCX[1];
  p4[2] = this->OriginCX[2];
  p4[3] = this->OriginCX[3];

  p4[this->ProjectionAxes[0]] = p[0];
  p4[this->ProjectionAxes[1]] = p[1];
  p4[this->ProjectionAxes[2]] = p[2];

  return this->EvaluateSet(p4);
}

//----------------------------------------------------------------------------
float vtkHyperOctreeFractalSource::EvaluateSet(double p[4])
{
  unsigned short count = 0;
  double v0, v1;
  double cReal, cImag, zReal, zImag;
  double zReal2, zImag2;

  cReal = p[0];
  cImag = p[1];
  zReal = p[2];
  zImag = p[3];

  zReal2 = zReal * zReal;
  zImag2 = zImag * zImag;
  v0 = 0.0;
  v1 = (zReal2 + zImag2);
  while ( v1 < 4.0 && count < this->MaximumNumberOfIterations)
    {
    zImag = 2.0 * zReal * zImag + cImag;
    zReal = zReal2 - zImag2 + cReal;
    zReal2 = zReal * zReal;
    zImag2 = zImag * zImag;
    ++count;
    v0 = v1;
    v1 = (zReal2 + zImag2);
    }

  if (count == this->MaximumNumberOfIterations)
    {
    return static_cast<float>(count);
    }

  return static_cast<float>(count) + static_cast<float>((4.0 - v0)/(v1 - v0));
}

//-----------------------------------------------------------------------------
void vtkHyperOctreeFractalSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os<<indent<<"MaximumLevel: "<<this->MaximumLevel<<endl;
  os<<indent<<"MinimumLevel: "<<this->MinimumLevel<<endl;

  os<<indent<<"SpanThreshold: "<<this->SpanThreshold<<endl;
  os<<indent<<"Dimension: "<<this->Dimension<<endl;

  os << indent << "OriginC: (" << this->OriginCX[0] << ", "
     << this->OriginCX[1] << ")\n";
  os << indent << "OriginX: (" << this->OriginCX[2] << ", "
     << this->OriginCX[3] << ")\n";

  double *size = this->GetSizeCX();
  os << indent << "SizeC: (" << size[0] << ", " << size[1] << ")\n";
  os << indent << "SizeX: (" << size[2] << ", " << size[3] << ")\n";

  os << "MaximumNumberOfIterations: " << this->MaximumNumberOfIterations << endl;

  os << indent << "ProjectionAxes: (" << this->ProjectionAxes[0] << ", "
     << this->ProjectionAxes[1] << this->ProjectionAxes[2] << ")\n";
}
