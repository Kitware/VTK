/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageData.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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
#include "vtkImageData.h"
#include "vtkVertex.h"
#include "vtkLine.h"
#include "vtkPixel.h"
#include "vtkVoxel.h"
#include "vtkObjectFactory.h"
#include "vtkExtentTranslator.h"
#include "vtkLargeInteger.h"
#include "vtkUnsignedCharArray.h"


//----------------------------------------------------------------------------
vtkImageData* vtkImageData::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageData");
  if (ret)
    {
    return (vtkImageData*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageData;
}

//----------------------------------------------------------------------------
vtkImageData::vtkImageData()
{
  int idx;
  
  this->Vertex = vtkVertex::New();
  this->Line = vtkLine::New();
  this->Pixel = vtkPixel::New();
  this->Voxel = vtkVoxel::New();
  
  // I do not like defaulting to one pixel, but it avoids
  // a lot of special case checking.
  // Since I am now allowing emtpy volumes, this should be changed.
  this->Dimensions[0] = 1;
  this->Dimensions[1] = 1;
  this->Dimensions[2] = 1;
  this->DataDescription = VTK_SINGLE_POINT;
  
  for (idx = 0; idx < 3; ++idx)
    {
    this->Extent[idx*2] = 0;
    this->Extent[idx*2+1] = 0;    
    this->Increments[idx] = 0;
    this->Origin[idx] = 0.0;
    this->Spacing[idx] = 1.0;
    }

  this->NumberOfScalarComponents = 1;

  // Making the default float for structured points.
  this->ScalarType = VTK_VOID;
  this->SetScalarType(VTK_FLOAT);

}

//----------------------------------------------------------------------------
vtkImageData::~vtkImageData()
{
  this->Vertex->Delete();
  this->Line->Delete();
  this->Pixel->Delete();
  this->Voxel->Delete();
}

//----------------------------------------------------------------------------
// Copy the geometric and topological structure of an input structured points 
// object.
void vtkImageData::CopyStructure(vtkDataSet *ds)
{
  vtkImageData *sPts=(vtkImageData *)ds;
  this->Initialize();

  for (int i=0; i<3; i++)
    {
    this->Extent[i] = sPts->Extent[i];
    this->Extent[i+3] = sPts->Extent[i+3];
    this->Dimensions[i] = sPts->Dimensions[i];
    this->Spacing[i] = sPts->Spacing[i];
    this->Origin[i] = sPts->Origin[i];
    }
  this->NumberOfScalarComponents = sPts->NumberOfScalarComponents;
  this->ScalarType = sPts->ScalarType;
  this->DataDescription = sPts->DataDescription;
  this->CopyInformation(sPts);
}

void vtkImageData::PrepareForNewData()
{
  // free everything but the scalars
  vtkScalars *scalars = this->GetPointData()->GetScalars();
  if (scalars)
    {
    scalars->Register(this);
    }
  this->Initialize();
  if (scalars)
    {
    this->GetPointData()->SetScalars(scalars);
    scalars->UnRegister(this);
    }
}


//----------------------------------------------------------------------------

// The input data object must be of type vtkImageData or a subclass!
void vtkImageData::CopyTypeSpecificInformation( vtkDataObject *data )
{
  vtkImageData *image = (vtkImageData *)data;

  // Copy the generic stuff
  this->CopyInformation( data );
  
  // Now do the specific stuff
  this->SetOrigin( image->GetOrigin() );
  this->SetSpacing( image->GetSpacing() );
  this->SetScalarType( image->GetScalarType() );
  this->SetNumberOfScalarComponents( image->GetNumberOfScalarComponents() );
}

//----------------------------------------------------------------------------

unsigned long vtkImageData::GetEstimatedMemorySize()
{
  vtkLargeInteger size; 
  int             idx;
  int             *uExt; 
  unsigned long   lsize;

  // Start with the number of scalar components
  size = (unsigned long)(this->GetNumberOfScalarComponents());

  // Multiply by the number of bytes per scalar
  switch (this->GetScalarType())
    {
    case VTK_FLOAT:
      size *= sizeof(float);
      break;
    case VTK_DOUBLE:
      size *= sizeof(double);
      break;
    case VTK_INT:
      size *= sizeof(int);
      break;
    case VTK_UNSIGNED_INT:
      size *= sizeof(unsigned int);
      break;
    case VTK_LONG:
      size *= sizeof(long);
      break;
    case VTK_UNSIGNED_LONG:
      size *= sizeof(unsigned long);
      break;
    case VTK_SHORT:
      size *= sizeof(short);
      break;
    case VTK_UNSIGNED_SHORT:
      size *= sizeof(unsigned short);
      break;
    case VTK_UNSIGNED_CHAR:
      size *= sizeof(unsigned char);
      break;
    case VTK_CHAR:
      size *= sizeof(char);
      break;
    case VTK_BIT:
      size = size / 8;
      break;
    default:
      vtkWarningMacro(<< "GetExtentMemorySize: "
        << "Cannot determine input scalar type");
    }  

  // Multiply by the number of scalars.
  uExt = this->GetUpdateExtent();
  for (idx = 0; idx < 3; ++idx)
    {
    size = size*(uExt[idx*2+1] - uExt[idx*2] + 1);
    }

  // In case the extent is set improperly, set the size to 0
  if (size < 0)
    {
    vtkWarningMacro("Oops, size should not be negative.");
    size = 0;
    }

  // Convert from double bytes to unsigned long kilobytes
  size = size >> 10;
  lsize = size.CastToUnsignedLong();
  return lsize;
}

//----------------------------------------------------------------------------

vtkCell *vtkImageData::GetCell(vtkIdType cellId)
{
  vtkCell *cell = NULL;
  int loc[3];
  vtkIdType idx, npts;
  int iMin, iMax, jMin, jMax, kMin, kMax;
  int *dims = this->GetDimensions();
  int d01 = dims[0]*dims[1];
  float x[3];
  float *origin = this->GetOrigin();
  float *spacing = this->GetSpacing();

  iMin = iMax = jMin = jMax = kMin = kMax = 0;
  
  if (dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
    {
    vtkErrorMacro("Requesting a cell from an empty image.");
    return NULL;
    }
  
  switch (this->DataDescription)
    {
    case VTK_SINGLE_POINT: // cellId can only be = 0
      cell = this->Vertex;
      break;

    case VTK_X_LINE:
      iMin = cellId;
      iMax = cellId + 1;
      cell = this->Line;
      break;

    case VTK_Y_LINE:
      jMin = cellId;
      jMax = cellId + 1;
      cell = this->Line;
      break;

    case VTK_Z_LINE:
      kMin = cellId;
      kMax = cellId + 1;
      cell = this->Line;
      break;

    case VTK_XY_PLANE:
      iMin = cellId % (dims[0]-1);
      iMax = iMin + 1;
      jMin = cellId / (dims[0]-1);
      jMax = jMin + 1;
      cell = this->Pixel;
      break;

    case VTK_YZ_PLANE:
      jMin = cellId % (dims[1]-1);
      jMax = jMin + 1;
      kMin = cellId / (dims[1]-1);
      kMax = kMin + 1;
      cell = this->Pixel;
      break;

    case VTK_XZ_PLANE:
      iMin = cellId % (dims[0]-1);
      iMax = iMin + 1;
      kMin = cellId / (dims[0]-1);
      kMax = kMin + 1;
      cell = this->Pixel;
      break;

    case VTK_XYZ_GRID:
      iMin = cellId % (dims[0] - 1);
      iMax = iMin + 1;
      jMin = (cellId / (dims[0] - 1)) % (dims[1] - 1);
      jMax = jMin + 1;
      kMin = cellId / ((dims[0] - 1) * (dims[1] - 1));
      kMax = kMin + 1;
      cell = this->Voxel;
      break;
    }

  // Extract point coordinates and point ids
  // Ids are relative to extent min.
  npts = 0;
  for (loc[2]=kMin; loc[2]<=kMax; loc[2]++)
    {
    x[2] = origin[2] + (loc[2]+this->Extent[4]) * spacing[2]; 
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      x[1] = origin[1] + (loc[1]+this->Extent[2]) * spacing[1]; 
      for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
        x[0] = origin[0] + (loc[0]+this->Extent[0]) * spacing[0]; 

        idx = loc[0] + loc[1]*dims[0] + loc[2]*d01;
        cell->PointIds->SetId(npts,idx);
        cell->Points->SetPoint(npts++,x);
        }
      }
    }

  return cell;
}

//----------------------------------------------------------------------------
void vtkImageData::GetCell(vtkIdType cellId, vtkGenericCell *cell)
{
  vtkIdType npts, idx;
  int loc[3];
  int iMin, iMax, jMin, jMax, kMin, kMax;
  int *dims = this->GetDimensions();
  int d01 = dims[0]*dims[1];
  float *origin = this->GetOrigin();
  float *spacing = this->GetSpacing();
  float x[3];

  iMin = iMax = jMin = jMax = kMin = kMax = 0;
  
  if (dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
    {
    vtkErrorMacro("Requesting a cell from an empty image.");
    return;
    }
  
  switch (this->DataDescription)
    {
    case VTK_SINGLE_POINT: // cellId can only be = 0
      cell->SetCellTypeToVertex();
      break;

    case VTK_X_LINE:
      iMin = cellId;
      iMax = cellId + 1;
      cell->SetCellTypeToLine();
      break;

    case VTK_Y_LINE:
      jMin = cellId;
      jMax = cellId + 1;
      cell->SetCellTypeToLine();
      break;

    case VTK_Z_LINE:
      kMin = cellId;
      kMax = cellId + 1;
      cell->SetCellTypeToLine();
      break;

    case VTK_XY_PLANE:
      iMin = cellId % (dims[0]-1);
      iMax = iMin + 1;
      jMin = cellId / (dims[0]-1);
      jMax = jMin + 1;
      cell->SetCellTypeToPixel();
      break;

    case VTK_YZ_PLANE:
      jMin = cellId % (dims[1]-1);
      jMax = jMin + 1;
      kMin = cellId / (dims[1]-1);
      kMax = kMin + 1;
      cell->SetCellTypeToPixel();
      break;

    case VTK_XZ_PLANE:
      iMin = cellId % (dims[0]-1);
      iMax = iMin + 1;
      kMin = cellId / (dims[0]-1);
      kMax = kMin + 1;
      cell->SetCellTypeToPixel();
      break;

    case VTK_XYZ_GRID:
      iMin = cellId % (dims[0] - 1);
      iMax = iMin + 1;
      jMin = (cellId / (dims[0] - 1)) % (dims[1] - 1);
      jMax = jMin + 1;
      kMin = cellId / ((dims[0] - 1) * (dims[1] - 1));
      kMax = kMin + 1;
      cell->SetCellTypeToVoxel();
      break;
    }

  // Extract point coordinates and point ids
  for (npts=0,loc[2]=kMin; loc[2]<=kMax; loc[2]++)
    {
    x[2] = origin[2] + (loc[2]+this->Extent[4]) * spacing[2]; 
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      x[1] = origin[1] + (loc[1]+this->Extent[2]) * spacing[1]; 
      for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
        x[0] = origin[0] + (loc[0]+this->Extent[0]) * spacing[0]; 

        idx = loc[0] + loc[1]*dims[0] + loc[2]*d01;
        cell->PointIds->SetId(npts,idx);
        cell->Points->SetPoint(npts++,x);
        }
      }
    }
}


//----------------------------------------------------------------------------
// Fast implementation of GetCellBounds().  Bounds are calculated without
// constructing a cell.
void vtkImageData::GetCellBounds(vtkIdType cellId, float bounds[6])
{
  int loc[3], iMin, iMax, jMin, jMax, kMin, kMax;
  float x[3];
  float *origin = this->GetOrigin();
  float *spacing = this->GetSpacing();
  int *dims = this->GetDimensions();

  iMin = iMax = jMin = jMax = kMin = kMax = 0;
  
  if (dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
    {
    vtkErrorMacro("Requesting cell bounds from an empty image.");
    bounds[0] = bounds[1] = bounds[2] = bounds[3] 
      = bounds[4] = bounds[5] = 0.0;
    return;
    }
  
  switch (this->DataDescription)
    {
    case VTK_SINGLE_POINT: // cellId can only be = 0
      break;

    case VTK_X_LINE:
      iMin = cellId;
      iMax = cellId + 1;
      break;

    case VTK_Y_LINE:
      jMin = cellId;
      jMax = cellId + 1;
      break;

    case VTK_Z_LINE:
      kMin = cellId;
      kMax = cellId + 1;
      break;

    case VTK_XY_PLANE:
      iMin = cellId % (dims[0]-1);
      iMax = iMin + 1;
      jMin = cellId / (dims[0]-1);
      jMax = jMin + 1;
      break;

    case VTK_YZ_PLANE:
      jMin = cellId % (dims[1]-1);
      jMax = jMin + 1;
      kMin = cellId / (dims[1]-1);
      kMax = kMin + 1;
      break;

    case VTK_XZ_PLANE:
      iMin = cellId % (dims[0]-1);
      iMax = iMin + 1;
      kMin = cellId / (dims[0]-1);
      kMax = kMin + 1;
      break;

    case VTK_XYZ_GRID:
      iMin = cellId % (dims[0] - 1);
      iMax = iMin + 1;
      jMin = (cellId / (dims[0] - 1)) % (dims[1] - 1);
      jMax = jMin + 1;
      kMin = cellId / ((dims[0] - 1) * (dims[1] - 1));
      kMax = kMin + 1;
      break;
    }


  bounds[0] = bounds[2] = bounds[4] =  VTK_LARGE_FLOAT;
  bounds[1] = bounds[3] = bounds[5] = -VTK_LARGE_FLOAT;
  
  // Extract point coordinates
  for (loc[2]=kMin; loc[2]<=kMax; loc[2]++)
    {
    x[2] = origin[2] + (loc[2]+this->Extent[4]) * spacing[2]; 
    bounds[4] = (x[2] < bounds[4] ? x[2] : bounds[4]);
    bounds[5] = (x[2] > bounds[5] ? x[2] : bounds[5]);
    }
  for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
    {
    x[1] = origin[1] + (loc[1]+this->Extent[2]) * spacing[1]; 
    bounds[2] = (x[1] < bounds[2] ? x[1] : bounds[2]);
    bounds[3] = (x[1] > bounds[3] ? x[1] : bounds[3]);
    }
  for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
    {
    x[0] = origin[0] + (loc[0]+this->Extent[0]) * spacing[0]; 
    bounds[0] = (x[0] < bounds[0] ? x[0] : bounds[0]);
    bounds[1] = (x[0] > bounds[1] ? x[0] : bounds[1]);
    }
}

//----------------------------------------------------------------------------
float *vtkImageData::GetPoint(vtkIdType ptId)
{
  static float x[3];
  int i, loc[3];
  float *origin = this->GetOrigin();
  float *spacing = this->GetSpacing();
  int *dims = this->GetDimensions();

  if (dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
    {
    vtkErrorMacro("Requesting a point from an empty image.");
    x[0] = x[1] = x[2] = 0.0;
    return x;
    }

  switch (this->DataDescription)
    {
    case VTK_SINGLE_POINT: 
      loc[0] = loc[1] = loc[2] = 0;
      break;

    case VTK_X_LINE:
      loc[1] = loc[2] = 0;
      loc[0] = ptId;
      break;

    case VTK_Y_LINE:
      loc[0] = loc[2] = 0;
      loc[1] = ptId;
      break;

    case VTK_Z_LINE:
      loc[0] = loc[1] = 0;
      loc[2] = ptId;
      break;

    case VTK_XY_PLANE:
      loc[2] = 0;
      loc[0] = ptId % dims[0];
      loc[1] = ptId / dims[0];
      break;

    case VTK_YZ_PLANE:
      loc[0] = 0;
      loc[1] = ptId % dims[1];
      loc[2] = ptId / dims[1];
      break;

    case VTK_XZ_PLANE:
      loc[1] = 0;
      loc[0] = ptId % dims[0];
      loc[2] = ptId / dims[0];
      break;

    case VTK_XYZ_GRID:
      loc[0] = ptId % dims[0];
      loc[1] = (ptId / dims[0]) % dims[1];
      loc[2] = ptId / (dims[0]*dims[1]);
      break;
    }

  for (i=0; i<3; i++)
    {
    x[i] = origin[i] + (loc[i]+this->Extent[i*2]) * spacing[i];
    }

  return x;
}

//----------------------------------------------------------------------------
vtkIdType vtkImageData::FindPoint(float x[3])
{
  int i, loc[3];
  float d;
  float *origin = this->GetOrigin();
  float *spacing = this->GetSpacing();
  int *dims = this->GetDimensions();

  //
  //  Compute the ijk location
  //
  for (i=0; i<3; i++) 
    {
    d = x[i] - origin[i];
    loc[i] = (int) ((d / spacing[i]) + 0.5);
    if ( loc[i] < this->Extent[i*2] || loc[i] > this->Extent[i*2+1] )
      {
      return -1;
      } 
    // since point id is relative to the first point actually stored
    loc[i] -= this->Extent[i*2];
    }
  //
  //  From this location get the point id
  //
  return loc[2]*dims[0]*dims[1] + loc[1]*dims[0] + loc[0];
  
}

//----------------------------------------------------------------------------
vtkIdType vtkImageData::FindCell(float x[3], vtkCell *vtkNotUsed(cell), 
                                 vtkGenericCell *vtkNotUsed(gencell),
                                 vtkIdType vtkNotUsed(cellId), 
				  float vtkNotUsed(tol2), 
				  int& subId, float pcoords[3], 
				  float *weights)
{
  return
    this->FindCell( x, (vtkCell *)NULL, 0, 0.0, subId, pcoords, weights );
}

//----------------------------------------------------------------------------
vtkIdType vtkImageData::FindCell(float x[3], vtkCell *vtkNotUsed(cell), 
                                 vtkIdType vtkNotUsed(cellId),
                                 float vtkNotUsed(tol2), 
                                 int& subId, float pcoords[3], float *weights)
{
  int loc[3];
  int *dims = this->GetDimensions();

  if ( this->ComputeStructuredCoordinates(x, loc, pcoords) == 0 )
    {
    return -1;
    }

  vtkVoxel::InterpolationFunctions(pcoords,weights);

  //
  //  From this location get the cell id
  //
  subId = 0;
  return loc[2] * (dims[0]-1)*(dims[1]-1) +
         loc[1] * (dims[0]-1) + loc[0];
}

//----------------------------------------------------------------------------
vtkCell *vtkImageData::FindAndGetCell(float x[3],
                                      vtkCell *vtkNotUsed(cell),
                                      vtkIdType vtkNotUsed(cellId),
                                      float vtkNotUsed(tol2), int& subId, 
                                      float pcoords[3], float *weights)
{
  int i, j, k, loc[3];
  vtkIdType npts, idx;
  int *dims = this->GetDimensions();
  vtkIdType d01 = dims[0]*dims[1];
  float xOut[3];
  int iMax = 0;
  int jMax = 0;
  int kMax = 0;;
  vtkCell *cell = NULL;
  float *origin = this->GetOrigin();
  float *spacing = this->GetSpacing();

  if ( this->ComputeStructuredCoordinates(x, loc, pcoords) == 0 )
    {
    return NULL;
    }

  //
  // Get the parametric coordinates and weights for interpolation
  //
  switch (this->DataDescription)
    {
    case VTK_SINGLE_POINT: // cellId can only be = 0
      vtkVertex::InterpolationFunctions(pcoords,weights);
      iMax = loc[0];
      jMax = loc[1];
      kMax = loc[2];
      cell = this->Vertex;
      break;

    case VTK_X_LINE:
      vtkLine::InterpolationFunctions(pcoords,weights);
      iMax = loc[0] + 1;
      jMax = loc[1];
      kMax = loc[2];
      cell = this->Line;
      break;

    case VTK_Y_LINE:
      vtkLine::InterpolationFunctions(pcoords,weights);
      iMax = loc[0];
      jMax = loc[1] + 1;
      kMax = loc[2];
      cell = this->Line;
      break;

    case VTK_Z_LINE:
      vtkLine::InterpolationFunctions(pcoords,weights);
      iMax = loc[0];
      jMax = loc[1];
      kMax = loc[2] + 1;
      cell = this->Line;
      break;

    case VTK_XY_PLANE:
      vtkPixel::InterpolationFunctions(pcoords,weights);
      iMax = loc[0] + 1;
      jMax = loc[1] + 1;
      kMax = loc[2];
      cell = this->Pixel;
      break;

    case VTK_YZ_PLANE:
      vtkPixel::InterpolationFunctions(pcoords,weights);
      iMax = loc[0];
      jMax = loc[1] + 1;
      kMax = loc[2] + 1;
      cell = this->Pixel;
      break;

    case VTK_XZ_PLANE:
      vtkPixel::InterpolationFunctions(pcoords,weights);
      iMax = loc[0] + 1;
      jMax = loc[1];
      kMax = loc[2] + 1;
      cell = this->Pixel;
      break;

    case VTK_XYZ_GRID:
      vtkVoxel::InterpolationFunctions(pcoords,weights);
      iMax = loc[0] + 1;
      jMax = loc[1] + 1;
      kMax = loc[2] + 1;
      cell = this->Voxel;
      break;
    }

  npts = 0;
  for (k = loc[2]; k <= kMax; k++)
    {
    xOut[2] = origin[2] + k * spacing[2]; 
    for (j = loc[1]; j <= jMax; j++)
      {
      xOut[1] = origin[1] + j * spacing[1]; 
      // make idx relative to the extent not the whole extent
      idx = loc[0]-this->Extent[0] + (j-this->Extent[2])*dims[0]
	+ (k-this->Extent[4])*d01;
      for (i = loc[0]; i <= iMax; i++, idx++)
        {
        xOut[0] = origin[0] + i * spacing[0]; 

        cell->PointIds->SetId(npts,idx);
        cell->Points->SetPoint(npts++,xOut);
        }
      }
    }
  subId = 0;

  return cell;
}

//----------------------------------------------------------------------------
int vtkImageData::GetCellType(vtkIdType vtkNotUsed(cellId))
{
  switch (this->DataDescription)
    {
    case VTK_SINGLE_POINT: 
      return VTK_VERTEX;

    case VTK_X_LINE: case VTK_Y_LINE: case VTK_Z_LINE:
      return VTK_LINE;

    case VTK_XY_PLANE: case VTK_YZ_PLANE: case VTK_XZ_PLANE:
      return VTK_PIXEL;

    case VTK_XYZ_GRID:
      return VTK_VOXEL;

    default:
      vtkErrorMacro(<<"Bad data description!");
      return VTK_EMPTY_CELL;
    }
}

//----------------------------------------------------------------------------
void vtkImageData::ComputeBounds()
{
  float *origin = this->GetOrigin();
  float *spacing = this->GetSpacing();
  
  this->Bounds[0] = origin[0] + (this->Extent[0] * spacing[0]);
  this->Bounds[2] = origin[1] + (this->Extent[2] * spacing[1]);
  this->Bounds[4] = origin[2] + (this->Extent[4] * spacing[2]);

  this->Bounds[1] = origin[0] + (this->Extent[1] * spacing[0]);
  this->Bounds[3] = origin[1] + (this->Extent[3] * spacing[1]);
  this->Bounds[5] = origin[2] + (this->Extent[5] * spacing[2]);
}

//----------------------------------------------------------------------------
// Given structured coordinates (i,j,k) for a voxel cell, compute the eight 
// gradient values for the voxel corners. The order in which the gradient
// vectors are arranged corresponds to the ordering of the voxel points. 
// Gradient vector is computed by central differences (except on edges of 
// volume where forward difference is used). The scalars s are the scalars
// from which the gradient is to be computed. This method will treat 
// only 3D structured point datasets (i.e., volumes).
void vtkImageData::GetVoxelGradient(int i, int j, int k, vtkScalars *s, 
                                    vtkVectors *g)
{
  float gv[3];
  int ii, jj, kk, idx=0;

  for ( kk=0; kk < 2; kk++)
    {
    for ( jj=0; jj < 2; jj++)
      {
      for ( ii=0; ii < 2; ii++)
        {
        this->GetPointGradient(i+ii, j+jj, k+kk, s, gv);
        g->SetVector(idx++, gv);
        }
      } 
    }
}

//----------------------------------------------------------------------------
// Given structured coordinates (i,j,k) for a point in a structured point 
// dataset, compute the gradient vector from the scalar data at that point. 
// The scalars s are the scalars from which the gradient is to be computed.
// This method will treat structured point datasets of any dimension.
void vtkImageData::GetPointGradient(int i,int j,int k, vtkScalars *s, 
                                    float g[3])
{
  int *dims=this->GetDimensions();
  float *ar=this->GetSpacing();
  vtkIdType ijsize=dims[0]*dims[1];
  float sp, sm;

  // x-direction
  if ( dims[0] == 1 )
    {
    g[0] = 0.0;
    }
  else if ( i == 0 )
    {
    sp = s->GetScalar(i+1 + j*dims[0] + k*ijsize);
    sm = s->GetScalar(i + j*dims[0] + k*ijsize);
    g[0] = (sm - sp) / ar[0];
    }
  else if ( i == (dims[0]-1) )
    {
    sp = s->GetScalar(i + j*dims[0] + k*ijsize);
    sm = s->GetScalar(i-1 + j*dims[0] + k*ijsize);
    g[0] = (sm - sp) / ar[0];
    }
  else
    {
    sp = s->GetScalar(i+1 + j*dims[0] + k*ijsize);
    sm = s->GetScalar(i-1 + j*dims[0] + k*ijsize);
    g[0] = 0.5 * (sm - sp) / ar[0];
    }

  // y-direction
  if ( dims[1] == 1 )
    {
    g[1] = 0.0;
    }
  else if ( j == 0 )
    {
    sp = s->GetScalar(i + (j+1)*dims[0] + k*ijsize);
    sm = s->GetScalar(i + j*dims[0] + k*ijsize);
    g[1] = (sm - sp) / ar[1];
    }
  else if ( j == (dims[1]-1) )
    {
    sp = s->GetScalar(i + j*dims[0] + k*ijsize);
    sm = s->GetScalar(i + (j-1)*dims[0] + k*ijsize);
    g[1] = (sm - sp) / ar[1];
    }
  else
    {
    sp = s->GetScalar(i + (j+1)*dims[0] + k*ijsize);
    sm = s->GetScalar(i + (j-1)*dims[0] + k*ijsize);
    g[1] = 0.5 * (sm - sp) / ar[1];
    }

  // z-direction
  if ( dims[2] == 1 )
    {
    g[2] = 0.0;
    }
  else if ( k == 0 )
    {
    sp = s->GetScalar(i + j*dims[0] + (k+1)*ijsize);
    sm = s->GetScalar(i + j*dims[0] + k*ijsize);
    g[2] = (sm - sp) / ar[2];
    }
  else if ( k == (dims[2]-1) )
    {
    sp = s->GetScalar(i + j*dims[0] + k*ijsize);
    sm = s->GetScalar(i + j*dims[0] + (k-1)*ijsize);
    g[2] = (sm - sp) / ar[2];
    }
  else
    {
    sp = s->GetScalar(i + j*dims[0] + (k+1)*ijsize);
    sm = s->GetScalar(i + j*dims[0] + (k-1)*ijsize);
    g[2] = 0.5 * (sm - sp) / ar[2];
    }
}

//----------------------------------------------------------------------------
// Set dimensions of structured points dataset.
void vtkImageData::SetDimensions(int i, int j, int k)
{
  this->SetExtent(0, i-1, 0, j-1, 0, k-1);
}

//----------------------------------------------------------------------------
// Set dimensions of structured points dataset.
void vtkImageData::SetDimensions(int dim[3])
{
  this->SetExtent(0, dim[0]-1, 0, dim[1]-1, 0, dim[2]-1);
}


// streaming change: ijk is in extent coordinate system.
//----------------------------------------------------------------------------
// Convenience function computes the structured coordinates for a point x[3].
// The voxel is specified by the array ijk[3], and the parametric coordinates
// in the cell are specified with pcoords[3]. The function returns a 0 if the
// point x is outside of the volume, and a 1 if inside the volume.
int vtkImageData::ComputeStructuredCoordinates(float x[3], int ijk[3], 
					       float pcoords[3])
{
  int i;
  float d, floatLoc;
  float *origin = this->GetOrigin();
  float *spacing = this->GetSpacing();
  int *dims = this->GetDimensions();
  
  //
  //  Compute the ijk location
  //
  for (i=0; i<3; i++) 
    {
    d = x[i] - origin[i];
    floatLoc = d / spacing[i];
    ijk[i] = (int) floatLoc;
    if ( ijk[i] >= this->Extent[i*2] && ijk[i] < this->Extent[i*2 + 1] )
      {
      pcoords[i] = floatLoc - (float)ijk[i];
      }

    else if ( ijk[i] < this->Extent[i*2] || ijk[i] > this->Extent[i*2+1] ) 
      {
      return 0;
      } 

    else //if ( ijk[i] == this->Extent[i*2+1] )
      {
      if (dims[i] == 1)
        {
        pcoords[i] = 0.0;
        }
      else
        {
        ijk[i] -= 1;
        pcoords[i] = 1.0;
        }
      }

    }
  return 1;
}


//----------------------------------------------------------------------------
void vtkImageData::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  int *dims = this->GetDimensions();
  
  vtkDataSet::PrintSelf(os,indent);

  os << indent << "ScalarType: " << this->ScalarType << endl;
  os << indent << "NumberOfScalarComponents: " << 
    this->NumberOfScalarComponents << endl;
  os << indent << "Spacing: (" << this->Spacing[0] << ", "
                               << this->Spacing[1] << ", "
                               << this->Spacing[2] << ")\n";
  os << indent << "Origin: (" << this->Origin[0] << ", "
                              << this->Origin[1] << ", "
                              << this->Origin[2] << ")\n";
  os << indent << "Dimensions: (" << dims[0] << ", "
                                  << dims[1] << ", "
                                  << dims[2] << ")\n";
  os << indent << "Increments: (" << this->Increments[0] << ", "
                                  << this->Increments[1] << ", "
                                  << this->Increments[2] << ")\n";
  os << indent << "Extent: (" << this->Extent[0];
  for (idx = 1; idx < 6; ++idx)
    {
    os << ", " << this->Extent[idx];
    }
  os << ")\n";
  os << indent << "WholeExtent: (" << this->WholeExtent[0];
  for (idx = 1; idx < 6; ++idx)
    {
    os << ", " << this->WholeExtent[idx];
    }
  os << ")\n";
}


//----------------------------------------------------------------------------
// Should we split up cells, or just points.  It does not matter for now.
// Extent of structured data assumes points.
void vtkImageData::SetUpdateExtent(int piece, int numPieces, int ghostLevel)
{
  int ext[6];
  
  this->UpdateInformation();
  this->GetWholeExtent(ext);
  this->ExtentTranslator->SetWholeExtent(ext);
  this->ExtentTranslator->SetPiece(piece);
  this->ExtentTranslator->SetNumberOfPieces(numPieces);
  this->ExtentTranslator->SetGhostLevel(ghostLevel);
  this->ExtentTranslator->PieceToExtent();
  this->SetUpdateExtent(this->ExtentTranslator->GetExtent());

  this->UpdatePiece = piece;
  this->UpdateNumberOfPieces = numPieces;
  this->UpdateGhostLevel = ghostLevel;
}

//----------------------------------------------------------------------------
void vtkImageData::UpdateData()
{
  this->vtkDataObject::UpdateData();

  // This stuff should really be ImageToStructuredPoints ....

  // Filters, that can't handle more data than they request, set this flag.
  if (this->RequestExactExtent)
    { // clip the data down to the UpdateExtent.
    if (this->Extent[0] == this->UpdateExtent[0]
	&& this->Extent[1] == this->UpdateExtent[1]
	&& this->Extent[2] == this->UpdateExtent[2]
	&& this->Extent[3] == this->UpdateExtent[3]
	&& this->Extent[4] == this->UpdateExtent[4]
	&& this->Extent[5] == this->UpdateExtent[5])
      {
      return;
      }
    this->Crop();
    }
  
  if (this->UpdateNumberOfPieces == 1)
    {
    // Either the piece has not been used to set the update extent,
    // or the whole image was requested.
    return;
    }

  //if (this->Extent[0] < this->UpdateExtent[0] ||
  //  this->Extent[1] > this->UpdateExtent[1] ||
  //  this->Extent[2] < this->UpdateExtent[2] ||
  //  this->Extent[3] > this->UpdateExtent[3] ||
  //  this->Extent[4] < this->UpdateExtent[4] ||
  //  this->Extent[5] > this->UpdateExtent[5])
  //  {
  //  vtkImageData *image = vtkImageData::New();
  //  image->DeepCopy(this);
  //  this->SetExtent(this->UpdateExtent);
  //  this->AllocateScalars();
  //  this->CopyAndCastFrom(image, this->UpdateExtent);
  //  image->Delete();
  //  }

  // Try to avoid generating these if the input has generated them,
  // or the image data is already up to date.
  // I guess we relly need an MTime check.
  if(this->Piece != this->UpdatePiece ||
     this->NumberOfPieces != this->UpdateNumberOfPieces ||
     this->GhostLevel != this->UpdateGhostLevel ||
     !this->PointData->GetArray("vtkGhostLevels"))
    { // Create ghost levels for cells and points.
    vtkUnsignedCharArray *levels;
    int zeroExt[6], extent[6];
    int i, j, k, di, dj, dk, dist;
    
    this->GetExtent(extent);
    // Get the extent with ghost level 0.
    this->ExtentTranslator->SetWholeExtent(this->WholeExtent);
    this->ExtentTranslator->SetPiece(this->UpdatePiece);
    this->ExtentTranslator->SetNumberOfPieces(this->UpdateNumberOfPieces);
    this->ExtentTranslator->SetGhostLevel(0);
    this->ExtentTranslator->PieceToExtent();
    this->ExtentTranslator->GetExtent(zeroExt);

    // ---- POINTS ----
    // Allocate the appropriate number levels (number of points).
    levels = vtkUnsignedCharArray::New();
    levels->Allocate((this->Extent[1]-this->Extent[0] + 1) *
		     (this->Extent[3]-this->Extent[2] + 1) *
		     (this->Extent[5]-this->Extent[4] + 1));
    
    //cerr << "max: " << extent[0] << ", " << extent[1] << ", " 
    //	 << extent[2] << ", " << extent[3] << ", " 
    //	 << extent[4] << ", " << extent[5] << endl;
    //cerr << "zero: " << zeroExt[0] << ", " << zeroExt[1] << ", " 
    //	 << zeroExt[2] << ", " << zeroExt[3] << ", "
    //	 << zeroExt[4] << ", " << zeroExt[5] << endl;
    
    // Loop through the points in this image.
    for (k = extent[4]; k <= extent[5]; ++k)
      { 
      dk = 0;
      if (k < zeroExt[4])
	{
	dk = zeroExt[4] - k;
	}
      if (k >= zeroExt[5] && k < this->WholeExtent[5])
	{ // Special case for last tile.
	dk = k - zeroExt[5] + 1;
	}
      for (j = extent[2]; j <= extent[3]; ++j)
	{ 
	dj = 0;
	if (j < zeroExt[2])
	  {
	  dj = zeroExt[2] - j;
	  }
	if (j >= zeroExt[3] && j < this->WholeExtent[3])
	  { // Special case for last tile.
	  dj = j - zeroExt[3] + 1;
	  }
	for (i = extent[0]; i <= extent[1]; ++i)
	  { 
	  di = 0;
	  if (i < zeroExt[0])
	    {
	    di = zeroExt[0] - i;
	    }
	  if (i >= zeroExt[1] && i < this->WholeExtent[1])
	    { // Special case for last tile.
	    di = i - zeroExt[1] + 1;
	    }
	  // Compute Manhatten distance.
	  dist = di;
	  if (dj > dist)
	    {
	    dist = dj;
	    }
	  if (dk > dist)
	    {
	    dist = dk;
	    }

	  //cerr << "   " << i << ", " << j << ", " << k << endl;
	  //cerr << "   " << di << ", " << dj << ", " << dk << endl;
	  //cerr << dist << endl;
	  
	  levels->InsertNextValue((unsigned char)dist);
	  }
	}
      }
    levels->SetName("vtkGhostLevels");
    this->PointData->AddArray(levels);
    levels->Delete();
    levels = NULL;
  
    // Only generate ghost call levels if zero levels are requested.
    // (Although we still need ghost points.)
    if (this->UpdateGhostLevel == 0)
      {
      return;
      }
    
    // ---- CELLS ----
    // Allocate the appropriate number levels (number of cells).
    levels = vtkUnsignedCharArray::New();
    levels->Allocate((this->Extent[1]-this->Extent[0]) *
		     (this->Extent[3]-this->Extent[2]) *
		     (this->Extent[5]-this->Extent[4]));
    
    // Loop through the cells in this image.
    // Cells may be 2d or 1d ... Treat all as 3D
    if (extent[0] == extent[1])
      {
      ++extent[1];
      ++zeroExt[1];
      }
    if (extent[2] == extent[3])
      {
      ++extent[3];
      ++zeroExt[3];
      }
    if (extent[4] == extent[5])
      {
      ++extent[5];
      ++zeroExt[5];
      }
    
    // Loop
    for (k = extent[4]; k < extent[5]; ++k)
      { // Determine the Manhatten distances to zero extent.
      dk = 0;
      if (k < zeroExt[4])
	{
	dk = zeroExt[4] - k;
	}
      if (k >= zeroExt[5])
	{
	dk = k - zeroExt[5] + 1;
	}
      for (j = extent[2]; j < extent[3]; ++j)
	{
	dj = 0;
	if (j < zeroExt[2])
	  {
	  dj = zeroExt[2] - j;
	  }
	if (j >= zeroExt[3])
	  {
	  dj = j - zeroExt[3] + 1;
	  }
	for (i = extent[0]; i < extent[1]; ++i)
	  {
	  di = 0;
	  if (i < zeroExt[0])
	    {
	    di = zeroExt[0] - i;
	    }
	  if (i >= zeroExt[1])
	    {
	    di = i - zeroExt[1] + 1;
	    }
	  // Compute Manhatten distance.
	  dist = di;
	  if (dj > dist)
	    {
	    dist = dj;
	    }
	  if (dk > dist)
	    {
	    dist = dk;
	    }

	  levels->InsertNextValue((unsigned char)dist);
	  }
	}
      }
    levels->SetName("vtkGhostLevels");
    this->CellData->AddArray(levels);
    levels->Delete();
    levels = NULL;
    }

}

//----------------------------------------------------------------------------

void vtkImageData::SetNumberOfScalarComponents(int num)
{
  this->NumberOfScalarComponents = num;
  this->ComputeIncrements();
}

int *vtkImageData::GetIncrements()
{
  // Make sure the increments are up to date. The filter bypass and update
  // mechanism make it tricky to update the increments anywhere other than here
  this->ComputeIncrements();

  return this->Increments;
}

void vtkImageData::GetIncrements(int &incX, int &incY, int &incZ)
{
  // Make sure the increments are up to date. The filter bypass and update
  // mechanism make it tricky to update the increments anywhere other than here
  this->ComputeIncrements();

  incX = this->Increments[0];
  incY = this->Increments[1];
  incZ = this->Increments[2];
}

void vtkImageData::GetIncrements(int inc[3])
{
  // Make sure the increments are up to date. The filter bypass and update
  // mechanism make it tricky to update the increments anywhere other than here
  this->ComputeIncrements();

  inc[0] = this->Increments[0];
  inc[1] = this->Increments[1];
  inc[2] = this->Increments[2];
}


//----------------------------------------------------------------------------
void vtkImageData::GetContinuousIncrements(int extent[6], int &incX,
					   int &incY, int &incZ)
{
  int e0, e1, e2, e3;
  
  incX = 0;

  e0 = extent[0];
  if (e0 < this->Extent[0])
    {
    e0 = this->Extent[0];
    }
  e1 = extent[1];
  if (e1 > this->Extent[1])
    {
    e1 = this->Extent[1];
    }
  e2 = extent[2];
  if (e2 < this->Extent[2])
    {
    e2 = this->Extent[2];
    }
  e3 = extent[3];
  if (e3 > this->Extent[3])
    {
    e3 = this->Extent[3];
    }

  // Make sure the increments are up to date
  this->ComputeIncrements();

  incY = this->Increments[1] - (e1 - e0 + 1)*this->Increments[0];
  incZ = this->Increments[2] - (e3 - e2 + 1)*this->Increments[1];
}


//----------------------------------------------------------------------------
// This method computes the increments from the MemoryOrder and the extent.
void vtkImageData::ComputeIncrements()
{
  int idx;
  int inc = this->GetNumberOfScalarComponents();

  for (idx = 0; idx < 3; ++idx)
    {
    this->Increments[idx] = inc;
    inc *= (this->Extent[idx*2+1] - this->Extent[idx*2] + 1);
    }
}




//----------------------------------------------------------------------------
float vtkImageData::GetScalarComponentAsFloat(int x, int y, int z, int comp)
{
  void *ptr;
  
  if (comp >= this->GetNumberOfScalarComponents() || comp < 0)
    {
    vtkErrorMacro("Bad component index " << comp);
    return 0.0;
    }
  
  ptr = this->GetScalarPointer(x, y, z);
  
  if (ptr == NULL)
    {
    // error message will be generated by get scalar pointer
    return 0.0;
    }
  
  switch (this->ScalarType)
    {
    case VTK_FLOAT:
      return *(((float *)ptr) + comp);
    case VTK_DOUBLE:
      return *(((double *)ptr) + comp);
    case VTK_INT:
      return (float)(*(((int *)ptr) + comp));
    case VTK_UNSIGNED_INT:
      return (float)(*(((unsigned int *)ptr) + comp));
    case VTK_LONG:
      return (float)(*(((long *)ptr) + comp));
    case VTK_UNSIGNED_LONG:
      return (float)(*(((unsigned long *)ptr) + comp));
    case VTK_SHORT:
      return (float)(*(((short *)ptr) + comp));
    case VTK_UNSIGNED_SHORT:
      return (float)(*(((unsigned short *)ptr) + comp));
    case VTK_UNSIGNED_CHAR:
      return (float)(*(((unsigned char *)ptr) + comp));
    case VTK_CHAR:
      return (float)(*(((char *)ptr) + comp));
    }

  vtkErrorMacro("Unknown Scalar type");
  return 0.0;
}


//----------------------------------------------------------------------------
// This Method returns a pointer to a location in the vtkImageData.
// Coordinates are in pixel units and are relative to the whole
// image origin.
void *vtkImageData::GetScalarPointer(int x, int y, int z)
{
  int tmp[3];
  tmp[0] = x;
  tmp[1] = y;
  tmp[2] = z;
  return this->GetScalarPointer(tmp);
}

//----------------------------------------------------------------------------
// This Method returns a pointer to a location in the vtkImageData.
// Coordinates are in pixel units and are relative to the whole
// image origin.
void *vtkImageData::GetScalarPointerForExtent(int extent[6])
{
  int tmp[3];
  tmp[0] = extent[0];
  tmp[1] = extent[2];
  tmp[2] = extent[4];
  return this->GetScalarPointer(tmp);
}

//----------------------------------------------------------------------------
// This Method returns a pointer to a location in the vtkImageData.
// Coordinates are in pixel units and are relative to the whole
// image origin.
void *vtkImageData::GetScalarPointer(int coordinates[3])
{
  vtkScalars *scalars;
  int idx;
    
  // Make sure the scalars have been allocated.
  scalars = this->PointData->GetScalars();
  if (scalars == NULL)
    {
    vtkDebugMacro("Allocating scalars in ImageData");
    this->AllocateScalars();
    scalars = this->PointData->GetScalars();
    }
  
  // error checking: since most acceses will be from pointer arithmetic.
  // this should not waste much time.
  for (idx = 0; idx < 3; ++idx)
    {
    if (coordinates[idx] < this->Extent[idx*2] ||
	coordinates[idx] > this->Extent[idx*2+1])
      {
      vtkErrorMacro(<< "GetScalarPointer: Pixel (" << coordinates[0] << ", " 
      << coordinates[1] << ", "
      << coordinates[2] << ") not in memory.\n Current extent= ("
      << this->Extent[0] << ", " << this->Extent[1] << ", "
      << this->Extent[2] << ", " << this->Extent[3] << ", "
      << this->Extent[4] << ", " << this->Extent[5] << ")");
      return NULL;
      }
    }
  
  // compute the index of the vector.
  idx = ((coordinates[0] - this->Extent[0]) * this->Increments[0]
	 + (coordinates[1] - this->Extent[2]) * this->Increments[1]
	 + (coordinates[2] - this->Extent[4]) * this->Increments[2]);
  
  return scalars->GetVoidPointer(idx);
}


//----------------------------------------------------------------------------
// This Method returns a pointer to the origin of the vtkImageData.
void *vtkImageData::GetScalarPointer()
{
  if (this->PointData->GetScalars() == NULL)
    {
    vtkDebugMacro("Allocating scalars in ImageData");
    this->AllocateScalars();
    }
  return this->PointData->GetScalars()->GetVoidPointer(0);
}

//----------------------------------------------------------------------------
int vtkImageData::GetScalarType()
{
  vtkScalars *tmp;
  int type = this->ScalarType;
  
  // if we have scalars make sure the type matches our ivar
  tmp = this->GetPointData()->GetScalars();
  if (tmp && tmp->GetDataType() != type)
    {
    // this happens when filters are being bypassed.  Don't error...
    //vtkErrorMacro("ScalarType " << tmp->GetDataType() 
    //                 << " does not match current scalars of type " << type);
    }
  
  return type;
}

//----------------------------------------------------------------------------
void vtkImageData::AllocateScalars()
{
  vtkScalars *scalars;
  
  // if the scalar type has not been set then we have a problem
  if (this->ScalarType == VTK_VOID)
    {
    vtkErrorMacro("Attempt to allocate scalars before scalar type was set!.");
    return;
    }

  // if we currently have scalars then just adjust the size
  scalars = this->PointData->GetScalars();
  if (scalars && scalars->GetDataType() == this->ScalarType) 
    {
    scalars->SetNumberOfComponents(this->GetNumberOfScalarComponents());
    scalars->SetNumberOfScalars((this->Extent[1] - this->Extent[0] + 1)*
				(this->Extent[3] - this->Extent[2] + 1)*
				(this->Extent[5] - this->Extent[4] + 1));
    // Since the execute method will be modifying the scalars
    // directly.
    scalars->Modified();
    return;
    }
  
  // allocate the new scalars
  scalars = vtkScalars::New();
  scalars->SetDataType(this->ScalarType);
  scalars->SetNumberOfComponents(this->GetNumberOfScalarComponents());
  this->PointData->SetScalars(scalars);
  scalars->Delete();
  
  // allocate enough memory
  this->PointData->GetScalars()->
    SetNumberOfScalars((this->Extent[1] - this->Extent[0] + 1)*
		       (this->Extent[3] - this->Extent[2] + 1)*
		       (this->Extent[5] - this->Extent[4] + 1));
}


//----------------------------------------------------------------------------
int vtkImageData::GetScalarSize()
{
  // allocate the new scalars
  switch (this->ScalarType)
    {
    case VTK_FLOAT:
      return sizeof(float);
    case VTK_DOUBLE:
      return sizeof(double);
    case VTK_INT:
    case VTK_UNSIGNED_INT:
      return sizeof(int);
    case VTK_LONG:
    case VTK_UNSIGNED_LONG:
      return sizeof(long);
    case VTK_SHORT:
    case VTK_UNSIGNED_SHORT:
      return 2;
    case VTK_UNSIGNED_CHAR:
      return 1;
    case VTK_CHAR:
      return 1;
    }
  
  return 1;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class IT, class OT>
static void vtkImageDataCastExecute(vtkImageData *inData, IT *inPtr,
				    vtkImageData *outData, OT *outPtr,
				    int outExt[6])
{
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  int rowLength;

  // find the region to loop over
  rowLength = (outExt[1] - outExt[0]+1)*inData->GetNumberOfScalarComponents();
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  
  // Get increments to march through data 
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      for (idxR = 0; idxR < rowLength; idxR++)
        {
        // Pixel operation
	*outPtr = (OT)(*inPtr);
        outPtr++;
        inPtr++;
        }
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}



//----------------------------------------------------------------------------
template <class T>
static void vtkImageDataCastExecute(vtkImageData *inData, T *inPtr,
				    vtkImageData *outData, int outExt[6])
{
  void *outPtr = outData->GetScalarPointerForExtent(outExt);

  switch (outData->GetScalarType())
    {
    vtkTemplateMacro5(vtkImageDataCastExecute, inData, (T *)(inPtr), 
                      outData, (VTK_TT *)(outPtr),outExt);
    default:
      vtkGenericWarningMacro("Execute: Unknown output ScalarType");
      return;
    }
}




//----------------------------------------------------------------------------
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageData::CopyAndCastFrom(vtkImageData *inData, int extent[6])
{
  void *inPtr = inData->GetScalarPointerForExtent(extent);
  
  switch (inData->ScalarType)
    {
    vtkTemplateMacro4(vtkImageDataCastExecute,inData, (VTK_TT *)(inPtr), 
                      this, extent);
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}

//----------------------------------------------------------------------------
void vtkImageData::Crop()
{
  int           nExt[6];
  int           idxY, idxZ, maxY, maxZ;
  int           inIncX, inIncY, inIncZ, rowLength;
  vtkScalars    *newScalars;
  unsigned char *inPtr, *inPtr1, *outPtr;
  
  // if the scalar type has not been set then we have a problem
  if (this->ScalarType == VTK_VOID)
    {
    vtkErrorMacro("ScalarType has not been set.");
    return;
    }

  // Take the intersection of the two extent so that 
  // we are not asking for more than the extent.
  this->GetUpdateExtent(nExt);
  if (nExt[0] < this->Extent[0]) { nExt[0] = this->Extent[0];}
  if (nExt[1] > this->Extent[1]) { nExt[1] = this->Extent[1];}
  if (nExt[2] < this->Extent[2]) { nExt[2] = this->Extent[2];}
  if (nExt[3] > this->Extent[3]) { nExt[3] = this->Extent[3];}
  if (nExt[4] < this->Extent[4]) { nExt[4] = this->Extent[4];}
  if (nExt[5] > this->Extent[5]) { nExt[5] = this->Extent[5];}

  // If the extents are the same just return.
  if (this->Extent[0] == nExt[0] && this->Extent[1] == nExt[1]
      && this->Extent[2] == nExt[2] && this->Extent[3] == nExt[3]
      && this->Extent[4] == nExt[4] && this->Extent[5] == nExt[5])
    {
    vtkDebugMacro("Extents already match.");
    return;
    }

  // Allocate new scalars.
  newScalars = vtkScalars::New();
  newScalars->SetDataType(this->ScalarType);
  newScalars->SetNumberOfComponents(this->GetNumberOfScalarComponents());
  newScalars->SetNumberOfScalars((nExt[1] - nExt[0] + 1)*
		                 (nExt[3] - nExt[2] + 1)*
		                 (nExt[5] - nExt[4] + 1));
  // Keep the array name the same.
  newScalars->GetData()->SetName(
    this->GetPointData()->GetScalars()->GetData()->GetName());
  
  inPtr = (unsigned char *) this->GetScalarPointerForExtent(nExt);
  outPtr = (unsigned char *) newScalars->GetVoidPointer(0);
  
  
  
  
  // Get increments to march through inData 
  this->GetIncrements(inIncX, inIncY, inIncZ);
  
  // find the region to loop over
  rowLength = (nExt[1] - nExt[0] + 1) * inIncX * this->GetScalarSize();
  maxY = nExt[3] - nExt[2]; 
  maxZ = nExt[5] - nExt[4];
  
  // Compensate for the fact we are using unsinged char pointers.
  inIncY *= this->GetScalarSize(); 
  inIncZ *= this->GetScalarSize();
  
  // Loop through outData pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    inPtr1 = inPtr + idxZ*inIncZ;
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      memcpy(outPtr,inPtr1,rowLength);
      inPtr1 += inIncY;
      outPtr += rowLength;
      }
    }

  this->SetExtent(nExt);
  this->PointData->SetScalars(newScalars);
  newScalars->Delete();
}



//----------------------------------------------------------------------------
double vtkImageData::GetScalarTypeMin()
{
  switch (this->ScalarType)
    {
    case VTK_DOUBLE:
      return (double)(VTK_DOUBLE_MIN);
    case VTK_FLOAT:
      return (double)(VTK_FLOAT_MIN);
    case VTK_LONG:
      return (double)(VTK_LONG);
    case VTK_UNSIGNED_LONG:
      return (double)(VTK_UNSIGNED_LONG);
    case VTK_INT:
      return (double)(VTK_INT_MIN);
    case VTK_UNSIGNED_INT:
      return (double)(VTK_UNSIGNED_INT_MIN);
    case VTK_SHORT:
      return (double)(VTK_SHORT_MIN);
    case VTK_UNSIGNED_SHORT:
      return (double)(0.0);
    case VTK_CHAR:
      return (double)(VTK_CHAR_MIN);
    case VTK_UNSIGNED_CHAR:
      return (double)(0.0);
    default:
      vtkErrorMacro("Cannot handle scalar type " << this->ScalarType);
      return 0.0;
    }
}


//----------------------------------------------------------------------------
double vtkImageData::GetScalarTypeMax()
{
  switch (this->ScalarType)
    {
    case VTK_DOUBLE:
      return (double)(VTK_DOUBLE_MAX);
    case VTK_FLOAT:
      return (double)(VTK_FLOAT_MAX);
    case VTK_LONG:
      return (double)(VTK_LONG_MAX);
    case VTK_UNSIGNED_LONG:
      return (double)(VTK_UNSIGNED_LONG_MAX);
    case VTK_INT:
      return (double)(VTK_INT_MAX);
    case VTK_UNSIGNED_INT:
      return (double)(VTK_UNSIGNED_INT_MAX);
    case VTK_SHORT:
      return (double)(VTK_SHORT_MAX);
    case VTK_UNSIGNED_SHORT:
      return (double)(VTK_UNSIGNED_SHORT_MAX);
    case VTK_CHAR:
      return (double)(VTK_CHAR_MAX);
    case VTK_UNSIGNED_CHAR:
      return (double)(VTK_UNSIGNED_CHAR_MAX);
    default:
      vtkErrorMacro("Cannot handle scalar type " << this->ScalarType);
      return 0.0;
    }
}

//----------------------------------------------------------------------------
void vtkImageData::SetExtent(int x1, int x2, int y1, int y2, int z1, int z2)
{
  int ext[6];
  ext[0] = x1;
  ext[1] = x2;
  ext[2] = y1;
  ext[3] = y2;
  ext[4] = z1;
  ext[5] = z2;
  this->SetExtent(ext);
}


//----------------------------------------------------------------------------
int *vtkImageData::GetDimensions()
{
  this->Dimensions[0] = this->Extent[1] - this->Extent[0] + 1;
  this->Dimensions[1] = this->Extent[3] - this->Extent[2] + 1;
  this->Dimensions[2] = this->Extent[5] - this->Extent[4] + 1;

  return this->Dimensions;
}

//----------------------------------------------------------------------------
void vtkImageData::GetDimensions(int *dOut)
{
  int *dims = this->GetDimensions();
  dOut[0] = dims[0];
  dOut[1] = dims[1];
  dOut[2] = dims[2];  
}

//----------------------------------------------------------------------------
void vtkImageData::SetExtent(int *extent)
{
  int description;

  description = vtkStructuredData::SetExtent(extent, this->Extent);
  if ( description < 0 ) //improperly specified
    {
    vtkErrorMacro (<< "Bad Extent, retaining previous values");
    }
  
  if (description == VTK_UNCHANGED)
    {
    return;
    }

  this->DataDescription = description;
  
  this->Modified();
  this->ComputeIncrements();
}


//----------------------------------------------------------------------------
void vtkImageData::SetAxisUpdateExtent(int idx, int min, int max)
{
  int modified = 0;
  
  if (idx > 2)
    {
    vtkWarningMacro("illegal axis!");
    return;
    }
  
  if (this->UpdateExtent[idx*2] != min)
    {
    modified = 1;
    this->UpdateExtent[idx*2] = min;
    }
  if (this->UpdateExtent[idx*2+1] != max)
    {
    modified = 1;
    this->UpdateExtent[idx*2+1] = max;
    }

  this->UpdateExtentInitialized = 1;
  if (modified)
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkImageData::GetAxisUpdateExtent(int idx, int &min, int &max)
{
  if (idx > 2)
    {
    vtkWarningMacro("illegal axis!");
    return;
    }

  min = this->UpdateExtent[idx*2];
  max = this->UpdateExtent[idx*2+1];
}


//----------------------------------------------------------------------------
unsigned long vtkImageData::GetActualMemorySize()
{
  return this->vtkDataSet::GetActualMemorySize();
}


//----------------------------------------------------------------------------
void vtkImageData::ShallowCopy(vtkDataObject *dataObject)
{
  vtkImageData *imageData = vtkImageData::SafeDownCast(dataObject);

  if ( imageData != NULL )
    {
    this->InternalImageDataCopy(imageData);
    }

  // Do superclass
  this->vtkDataSet::ShallowCopy(dataObject);
}

//----------------------------------------------------------------------------
void vtkImageData::DeepCopy(vtkDataObject *dataObject)
{
  vtkImageData *imageData = vtkImageData::SafeDownCast(dataObject);

  if ( imageData != NULL )
    {
    this->InternalImageDataCopy(imageData);
    }

  // Do superclass
  this->vtkDataSet::DeepCopy(dataObject);
}

//----------------------------------------------------------------------------
// This copies all the local variables (but not objects).
void vtkImageData::InternalImageDataCopy(vtkImageData *src)
{
  int idx;

  this->DataDescription = src->DataDescription;
  this->ScalarType = src->ScalarType;
  this->NumberOfScalarComponents = src->NumberOfScalarComponents;
  for (idx = 0; idx < 3; ++idx)
    {
    this->Dimensions[idx] = src->Dimensions[idx];
    this->Increments[idx] = src->Increments[idx];
    this->Origin[idx] = src->Origin[idx];
    this->Spacing[idx] = src->Spacing[idx];
    }
}



//----------------------------------------------------------------------------
vtkIdType vtkImageData::GetNumberOfCells() 
{
  vtkIdType nCells=1;
  int i;
  int *dims = this->GetDimensions();

  for (i=0; i<3; i++)
    {
    if (dims[i] == 0)
      {
      return 0;
      }
    if (dims[i] > 1)
      {
      nCells *= (dims[i]-1);
      }
    }

  return nCells;
}

