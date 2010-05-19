/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageData.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkLargeInteger.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPixel.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkVertex.h"
#include "vtkVoxel.h"

vtkStandardNewMacro(vtkImageData);

//----------------------------------------------------------------------------
vtkImageData::vtkImageData()
{
  int idx;

  this->Vertex = vtkVertex::New();
  this->Line = vtkLine::New();
  this->Pixel = vtkPixel::New();
  this->Voxel = vtkVoxel::New();

  this->DataDescription = VTK_EMPTY;

  for (idx = 0; idx < 3; ++idx)
    {
    this->Dimensions[idx] = 0;
    this->Increments[idx] = 0;
    this->Origin[idx] = 0.0;
    this->Spacing[idx] = 1.0;
    }

  int extent[6] = {0, -1, 0, -1, 0, -1};
  memcpy(this->Extent, extent, 6*sizeof(int));

  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_3D_EXTENT);
  this->Information->Set(vtkDataObject::DATA_EXTENT(), this->Extent, 6);
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
  vtkImageData *sPts=static_cast<vtkImageData *>(ds);
  this->Initialize();

  int i;
  for (i=0; i<3; i++)
    {
    this->Dimensions[i] = sPts->Dimensions[i];
    this->Spacing[i] = sPts->Spacing[i];
    this->Origin[i] = sPts->Origin[i];
    }
  this->SetExtent(sPts->GetExtent());

  vtkInformation* thisPInfo = this->GetPipelineInformation();
  vtkInformation* thatPInfo = ds->GetPipelineInformation();
  if(thisPInfo && thatPInfo)
    {
    // copy point data.
    if (thatPInfo->Has(POINT_DATA_VECTOR()))
      {
      thisPInfo->CopyEntry(thatPInfo, POINT_DATA_VECTOR());
      }
    // copy cell data.
    if (thatPInfo->Has(CELL_DATA_VECTOR()))
      {
      thisPInfo->CopyEntry(thatPInfo, CELL_DATA_VECTOR());
      }
    }
  this->DataDescription = sPts->DataDescription;
  this->CopyInformation(sPts);
}

//----------------------------------------------------------------------------
void vtkImageData::Initialize()
{
  this->Superclass::Initialize();
  if(this->Information)
    {
    this->SetDimensions(0,0,0);
    }
}

//----------------------------------------------------------------------------
void vtkImageData::CopyInformationToPipeline(vtkInformation* request,
                                             vtkInformation* input,
                                             vtkInformation* output,
                                             int forceCopy)
{
  // Let the superclass copy whatever it wants.
  this->Superclass::CopyInformationToPipeline(request, input, output, forceCopy);

  // Set default pipeline information during a request for information.
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    // Copy settings from the input if available.  Otherwise use our
    // current settings.

    if(input && input->Has(ORIGIN()))
      {
      output->CopyEntry(input, ORIGIN());
      }
    else if (!output->Has(ORIGIN()) || forceCopy)
      {
      // Set origin (only if it is not set).
      output->Set(ORIGIN(), this->GetOrigin(), 3);
      }

    if(input && input->Has(SPACING()))
      {
      output->CopyEntry(input, SPACING());
      }
    else if (!output->Has(SPACING()) || forceCopy)
      {
      // Set spacing (only if it is not set).
      output->Set(SPACING(), this->GetSpacing(), 3);
      }

    // copy of input to output (if input exists) occurs in vtkDataObject, so
    // only to to check need to check if the scalar info exists in the field
    // data info of the output.  If it exists, then we assume the type and
    // number of components are set; if not, set type and number of components
    // to default values
    vtkInformation *scalarInfo =
      vtkDataObject::GetActiveFieldInformation(output,
                                               FIELD_ASSOCIATION_POINTS,
                                               vtkDataSetAttributes::SCALARS);
    if (!scalarInfo || forceCopy)
      {
      vtkDataArray* scalars = this->GetPointData()->GetScalars();
      if (scalars)
        {
        vtkDataObject::SetPointDataActiveScalarInfo(
          output, scalars->GetDataType(), scalars->GetNumberOfComponents());
        }
      else
        {
        vtkDataObject::SetPointDataActiveScalarInfo(output, VTK_DOUBLE, 1);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkImageData::CopyInformationFromPipeline(vtkInformation* request)
{
  // Let the superclass copy whatever it wants.
  this->Superclass::CopyInformationFromPipeline(request);

  // Copy pipeline information to data information before the producer
  // executes.
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    this->CopyOriginAndSpacingFromPipeline();
    }
}

//----------------------------------------------------------------------------
// Graphics filters reallocate every execute.  Image filters try to reuse
// the scalars.
void vtkImageData::PrepareForNewData()
{
  // free everything but the scalars
  vtkDataArray *scalars = this->GetPointData()->GetScalars();
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
  vtkImageData *image = static_cast<vtkImageData *>(data);

  // Copy the generic stuff
  this->CopyInformation( data );

  // Now do the specific stuff
  this->SetOrigin( image->GetOrigin() );
  this->SetSpacing( image->GetSpacing() );
  this->SetScalarType( image->GetScalarType() );
  this->SetNumberOfScalarComponents(
    image->GetNumberOfScalarComponents() );
}

//----------------------------------------------------------------------------
template <class T>
unsigned long vtkImageDataGetTypeSize(T*)
{
  return sizeof(T);
}

//----------------------------------------------------------------------------

unsigned long vtkImageData::GetEstimatedMemorySize()
{
  vtkLargeInteger size;
  int             idx;
  int             *uExt;
  unsigned long   lsize;

  // Start with the number of scalar components
  size = static_cast<unsigned long>(this->GetNumberOfScalarComponents());

  // Multiply by the number of bytes per scalar
  switch (this->GetScalarType())
    {
    vtkTemplateMacro(
      size *= vtkImageDataGetTypeSize(static_cast<VTK_TT*>(0))
      );
    case VTK_BIT:
      size /= 8;
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
  double x[3];
  const double *origin = this->Origin;
  const double *spacing = this->Spacing;
  const int* extent = this->Extent;

  // Use vtkIdType to avoid overflow on large images
  vtkIdType dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;

  vtkIdType d01 = dims[0]*dims[1];

  iMin = iMax = jMin = jMax = kMin = kMax = 0;

  if (dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
    {
    vtkErrorMacro("Requesting a cell from an empty image.");
    return NULL;
    }

  switch (this->DataDescription)
    {
    case VTK_EMPTY:
      //cell = this->EmptyCell;
      return NULL;

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
    x[2] = origin[2] + (loc[2]+extent[4]) * spacing[2];
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      x[1] = origin[1] + (loc[1]+extent[2]) * spacing[1];
      for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
        x[0] = origin[0] + (loc[0]+extent[0]) * spacing[0];

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
  const double *origin = this->Origin;
  const double *spacing = this->Spacing;
  double x[3];
  const int* extent = this->Extent;

  vtkIdType dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;
  vtkIdType d01 = dims[0]*dims[1];

  iMin = iMax = jMin = jMax = kMin = kMax = 0;

  if (dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
    {
    vtkErrorMacro("Requesting a cell from an empty image.");
    cell->SetCellTypeToEmptyCell();
    return;
    }

  switch (this->DataDescription)
    {
    case VTK_EMPTY:
      cell->SetCellTypeToEmptyCell();
      return;

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
    x[2] = origin[2] + (loc[2]+extent[4]) * spacing[2];
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      x[1] = origin[1] + (loc[1]+extent[2]) * spacing[1];
      for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
        x[0] = origin[0] + (loc[0]+extent[0]) * spacing[0];

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
void vtkImageData::GetCellBounds(vtkIdType cellId, double bounds[6])
{
  int loc[3], iMin, iMax, jMin, jMax, kMin, kMax;
  double x[3];
  const double *origin = this->Origin;
  const double *spacing = this->Spacing;
  const int* extent = this->Extent;

  vtkIdType dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;

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
    case VTK_EMPTY:
      return;

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


  // carefully compute the bounds
  if (kMax >= kMin && jMax >= jMin && iMax >= iMin)
    {
    bounds[0] = bounds[2] = bounds[4] =  VTK_DOUBLE_MAX;
    bounds[1] = bounds[3] = bounds[5] =  VTK_DOUBLE_MIN;

    // Extract point coordinates
    for (loc[2]=kMin; loc[2]<=kMax; loc[2]++)
      {
      x[2] = origin[2] + (loc[2]+extent[4]) * spacing[2];
      bounds[4] = (x[2] < bounds[4] ? x[2] : bounds[4]);
      bounds[5] = (x[2] > bounds[5] ? x[2] : bounds[5]);
      }
    for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
      {
      x[1] = origin[1] + (loc[1]+extent[2]) * spacing[1];
      bounds[2] = (x[1] < bounds[2] ? x[1] : bounds[2]);
      bounds[3] = (x[1] > bounds[3] ? x[1] : bounds[3]);
      }
    for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
      {
      x[0] = origin[0] + (loc[0]+extent[0]) * spacing[0];
      bounds[0] = (x[0] < bounds[0] ? x[0] : bounds[0]);
      bounds[1] = (x[0] > bounds[1] ? x[0] : bounds[1]);
      }
    }
  else
    {
    vtkMath::UninitializeBounds(bounds);
    }
}

//----------------------------------------------------------------------------
double *vtkImageData::GetPoint(vtkIdType ptId)
{
  static double x[3];
  int i, loc[3];
  const double *origin = this->Origin;
  const double *spacing = this->Spacing;
  const int* extent = this->Extent;

  vtkIdType dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;

  x[0] = x[1] = x[2] = 0.0;
  if (dims[0] == 0 || dims[1] == 0 || dims[2] == 0)
    {
    vtkErrorMacro("Requesting a point from an empty image.");
    return x;
    }

  // "loc" holds the point x,y,z indices
  loc[0] = loc[1] = loc[2] = 0;

  switch (this->DataDescription)
    {
    case VTK_EMPTY:
      return x;

    case VTK_SINGLE_POINT:
      break;

    case VTK_X_LINE:
      loc[0] = ptId;
      break;

    case VTK_Y_LINE:
      loc[1] = ptId;
      break;

    case VTK_Z_LINE:
      loc[2] = ptId;
      break;

    case VTK_XY_PLANE:
      loc[0] = ptId % dims[0];
      loc[1] = ptId / dims[0];
      break;

    case VTK_YZ_PLANE:
      loc[1] = ptId % dims[1];
      loc[2] = ptId / dims[1];
      break;

    case VTK_XZ_PLANE:
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
    x[i] = origin[i] + (loc[i]+extent[i*2]) * spacing[i];
    }

  return x;
}

//----------------------------------------------------------------------------
vtkIdType vtkImageData::FindPoint(double x[3])
{
  int i, loc[3];
  double d;
  const double *origin = this->Origin;
  const double *spacing = this->Spacing;
  const int* extent = this->Extent;

  vtkIdType dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;

  //
  //  Compute the ijk location
  //
  for (i=0; i<3; i++)
    {
    d = x[i] - origin[i];
    loc[i] = static_cast<int>((d / spacing[i]) + 0.5);
    if ( loc[i] < extent[i*2] || loc[i] > extent[i*2+1] )
      {
      return -1;
      }
    // since point id is relative to the first point actually stored
    loc[i] -= extent[i*2];
    }
  //
  //  From this location get the point id
  //
  return loc[2]*dims[0]*dims[1] + loc[1]*dims[0] + loc[0];

}

//----------------------------------------------------------------------------
vtkIdType vtkImageData::FindCell(double x[3], vtkCell *vtkNotUsed(cell),
                                 vtkGenericCell *vtkNotUsed(gencell),
                                 vtkIdType vtkNotUsed(cellId),
                                  double vtkNotUsed(tol2),
                                  int& subId, double pcoords[3],
                                  double *weights)
{
  return
    this->FindCell( x, NULL, 0, 0.0, subId, pcoords, weights );
}

//----------------------------------------------------------------------------
vtkIdType vtkImageData::FindCell(double x[3], vtkCell *vtkNotUsed(cell),
                                 vtkIdType vtkNotUsed(cellId),
                                 double vtkNotUsed(tol2),
                                 int& subId, double pcoords[3], double *weights)
{
  int loc[3];

  if ( this->ComputeStructuredCoordinates(x, loc, pcoords) == 0 )
    {
    return -1;
    }

  this->Voxel->InterpolateFunctions(pcoords,weights);

  //
  //  From this location get the cell id
  //
  subId = 0;
  return this->ComputeCellId(loc);
}

//----------------------------------------------------------------------------
vtkCell *vtkImageData::FindAndGetCell(double x[3],
                                      vtkCell *vtkNotUsed(cell),
                                      vtkIdType vtkNotUsed(cellId),
                                      double vtkNotUsed(tol2), int& subId,
                                      double pcoords[3], double *weights)
{
  int i, j, k, ijk[3], loc[3];
  vtkIdType npts, idx;
  double xOut[3];
  int iMax = 0;
  int jMax = 0;
  int kMax = 0;;
  vtkCell *cell = NULL;
  const double *origin = this->Origin;
  const double *spacing = this->Spacing;
  const int* extent = this->Extent;

  vtkIdType dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;

  if ( this->ComputeStructuredCoordinates(x, loc, pcoords) == 0 )
    {
    return NULL;
    }

  //
  // Get the parametric coordinates and weights for interpolation
  //
  switch (this->DataDescription)
    {
    case VTK_EMPTY:
      return NULL;

    case VTK_SINGLE_POINT: // cellId can only be = 0
      iMax = loc[0];
      jMax = loc[1];
      kMax = loc[2];
      cell = this->Vertex;
      break;

    case VTK_X_LINE:
      iMax = loc[0] + 1;
      jMax = loc[1];
      kMax = loc[2];
      cell = this->Line;
      break;

    case VTK_Y_LINE:
      iMax = loc[0];
      jMax = loc[1] + 1;
      kMax = loc[2];
      cell = this->Line;
      break;

    case VTK_Z_LINE:
      iMax = loc[0];
      jMax = loc[1];
      kMax = loc[2] + 1;
      cell = this->Line;
      break;

    case VTK_XY_PLANE:
      iMax = loc[0] + 1;
      jMax = loc[1] + 1;
      kMax = loc[2];
      cell = this->Pixel;
      break;

    case VTK_YZ_PLANE:
      iMax = loc[0];
      jMax = loc[1] + 1;
      kMax = loc[2] + 1;
      cell = this->Pixel;
      break;

    case VTK_XZ_PLANE:
      iMax = loc[0] + 1;
      jMax = loc[1];
      kMax = loc[2] + 1;
      cell = this->Pixel;
      break;

    case VTK_XYZ_GRID:
      iMax = loc[0] + 1;
      jMax = loc[1] + 1;
      kMax = loc[2] + 1;
      cell = this->Voxel;
      break;
    }
  cell->InterpolateFunctions(pcoords, weights);

  npts = 0;
  for (k = loc[2]; k <= kMax; k++)
    {
    ijk[2] = k;
    xOut[2] = origin[2] + k * spacing[2];
    for (j = loc[1]; j <= jMax; j++)
      {
      ijk[1] = j;
      xOut[1] = origin[1] + j * spacing[1];
      for (i = loc[0]; i <= iMax; i++)
        {
        ijk[0] = i;
        xOut[0] = origin[0] + i * spacing[0];

        idx = this->ComputePointId(ijk);
        cell->PointIds->SetId(npts,idx);
        cell->Points->SetPoint(npts++,xOut);
        idx++;
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
    case VTK_EMPTY:
      return VTK_EMPTY_CELL;

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
  const double *origin = this->Origin;
  const double *spacing = this->Spacing;
  const int* extent = this->Extent;

  if ( extent[0] > extent[1] ||
       extent[2] > extent[3] ||
       extent[4] > extent[5] )
    {
    vtkMath::UninitializeBounds(this->Bounds);
    return;
    }

  int swapXBounds = (spacing[0] < 0);  // 1 if true, 0 if false
  int swapYBounds = (spacing[1] < 0);  // 1 if true, 0 if false
  int swapZBounds = (spacing[2] < 0);  // 1 if true, 0 if false

  this->Bounds[0] = origin[0] + (extent[0+swapXBounds] * spacing[0]);
  this->Bounds[2] = origin[1] + (extent[2+swapYBounds] * spacing[1]);
  this->Bounds[4] = origin[2] + (extent[4+swapZBounds] * spacing[2]);

  this->Bounds[1] = origin[0] + (extent[1-swapXBounds] * spacing[0]);
  this->Bounds[3] = origin[1] + (extent[3-swapYBounds] * spacing[1]);
  this->Bounds[5] = origin[2] + (extent[5-swapZBounds] * spacing[2]);
}

//----------------------------------------------------------------------------
// Given structured coordinates (i,j,k) for a voxel cell, compute the eight
// gradient values for the voxel corners. The order in which the gradient
// vectors are arranged corresponds to the ordering of the voxel points.
// Gradient vector is computed by central differences (except on edges of
// volume where forward difference is used). The scalars s are the scalars
// from which the gradient is to be computed. This method will treat
// only 3D structured point datasets (i.e., volumes).
void vtkImageData::GetVoxelGradient(int i, int j, int k, vtkDataArray *s,
                                    vtkDataArray *g)
{
  double gv[3];
  int ii, jj, kk, idx=0;

  for ( kk=0; kk < 2; kk++)
    {
    for ( jj=0; jj < 2; jj++)
      {
      for ( ii=0; ii < 2; ii++)
        {
        this->GetPointGradient(i+ii, j+jj, k+kk, s, gv);
        g->SetTuple(idx++, gv);
        }
      }
    }
}

//----------------------------------------------------------------------------
// Given structured coordinates (i,j,k) for a point in a structured point
// dataset, compute the gradient vector from the scalar data at that point.
// The scalars s are the scalars from which the gradient is to be computed.
// This method will treat structured point datasets of any dimension.
void vtkImageData::GetPointGradient(int i, int j, int k, vtkDataArray *s,
                                    double g[3])
{
  const double *ar = this->Spacing;
  double sp, sm;
  const int *extent = this->Extent;

  vtkIdType dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;

  vtkIdType ijsize=dims[0]*dims[1];

  // Adjust i,j,k to the start of the extent
  i -= extent[0];
  j -= extent[2];
  k -= extent[4];

  // Check for out-of-bounds
  if (i < 0 || i >= dims[0] || j < 0 || j >= dims[1] || k < 0 || k >= dims[2])
    {
    g[0] = g[1] = g[2] = 0.0;
    return;
    }

  // x-direction
  if ( dims[0] == 1 )
    {
    g[0] = 0.0;
    }
  else if ( i == 0 )
    {
    sp = s->GetComponent(i+1 + j*dims[0] + k*ijsize, 0);
    sm = s->GetComponent(i + j*dims[0] + k*ijsize, 0);
    g[0] = (sm - sp) / ar[0];
    }
  else if ( i == (dims[0]-1) )
    {
    sp = s->GetComponent(i + j*dims[0] + k*ijsize,0);
    sm = s->GetComponent(i-1 + j*dims[0] + k*ijsize,0);
    g[0] = (sm - sp) / ar[0];
    }
  else
    {
    sp = s->GetComponent(i+1 + j*dims[0] + k*ijsize,0);
    sm = s->GetComponent(i-1 + j*dims[0] + k*ijsize,0);
    g[0] = 0.5 * (sm - sp) / ar[0];
    }

  // y-direction
  if ( dims[1] == 1 )
    {
    g[1] = 0.0;
    }
  else if ( j == 0 )
    {
    sp = s->GetComponent(i + (j+1)*dims[0] + k*ijsize,0);
    sm = s->GetComponent(i + j*dims[0] + k*ijsize,0);
    g[1] = (sm - sp) / ar[1];
    }
  else if ( j == (dims[1]-1) )
    {
    sp = s->GetComponent(i + j*dims[0] + k*ijsize,0);
    sm = s->GetComponent(i + (j-1)*dims[0] + k*ijsize,0);
    g[1] = (sm - sp) / ar[1];
    }
  else
    {
    sp = s->GetComponent(i + (j+1)*dims[0] + k*ijsize,0);
    sm = s->GetComponent(i + (j-1)*dims[0] + k*ijsize,0);
    g[1] = 0.5 * (sm - sp) / ar[1];
    }

  // z-direction
  if ( dims[2] == 1 )
    {
    g[2] = 0.0;
    }
  else if ( k == 0 )
    {
    sp = s->GetComponent(i + j*dims[0] + (k+1)*ijsize,0);
    sm = s->GetComponent(i + j*dims[0] + k*ijsize,0);
    g[2] = (sm - sp) / ar[2];
    }
  else if ( k == (dims[2]-1) )
    {
    sp = s->GetComponent(i + j*dims[0] + k*ijsize,0);
    sm = s->GetComponent(i + j*dims[0] + (k-1)*ijsize,0);
    g[2] = (sm - sp) / ar[2];
    }
  else
    {
    sp = s->GetComponent(i + j*dims[0] + (k+1)*ijsize,0);
    sm = s->GetComponent(i + j*dims[0] + (k-1)*ijsize,0);
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
void vtkImageData::SetDimensions(const int dim[3])
{
  this->SetExtent(0, dim[0]-1, 0, dim[1]-1, 0, dim[2]-1);
}


//----------------------------------------------------------------------------
// Convenience function computes the structured coordinates for a point x[3].
// The voxel is specified by the array ijk[3], and the parametric coordinates
// in the cell are specified with pcoords[3]. The function returns a 0 if the
// point x is outside of the volume, and a 1 if inside the volume.
int vtkImageData::ComputeStructuredCoordinates(double x[3], int ijk[3],
                                               double pcoords[3])
{
  int i;
  double d, doubleLoc;
  const double *origin = this->Origin;
  const double *spacing = this->Spacing;
  const int* extent = this->Extent;

  vtkIdType dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;

  //
  //  Compute the ijk location
  //
  for (i=0; i<3; i++)
    {
    d = x[i] - origin[i];
    doubleLoc = d / spacing[i];
    // Floor for negative indexes.
    ijk[i] = static_cast<int>(floor(doubleLoc));
    if ( ijk[i] >= extent[i*2] && ijk[i] < extent[i*2 + 1] )
      {
      pcoords[i] = doubleLoc - static_cast<double>(ijk[i]);
      }

    else if ( ijk[i] < extent[i*2] || ijk[i] > extent[i*2+1] )
      {
      return 0;
      }

    else //if ( ijk[i] == extent[i*2+1] )
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
  this->Superclass::PrintSelf(os,indent);

  int idx;
  const int *dims = this->GetDimensions();
  const int* extent = this->Extent;

  os << indent << "ScalarType: " << this->GetScalarType() << endl;
  os << indent << "NumberOfScalarComponents: " <<
    this->GetNumberOfScalarComponents() << endl;
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
  os << indent << "Extent: (" << extent[0];
  for (idx = 1; idx < 6; ++idx)
    {
    os << ", " << extent[idx];
    }
  os << ")\n";
}

//----------------------------------------------------------------------------
void vtkImageData::UpdateInformation()
{
  // Use the compatibility method in the superclass to update the
  // information.
  this->Superclass::UpdateInformation();

  // Now copy the information the caller is probably expecting to get
  // from this data object instead of the pipeline information.  This
  // preserves compatibility.
  this->CopyOriginAndSpacingFromPipeline();
}

//----------------------------------------------------------------------------
void vtkImageData::SetNumberOfScalarComponents(int num)
{
  this->GetProducerPort();
  if(vtkInformation* info = this->GetPipelineInformation())
    {
    vtkDataObject::SetPointDataActiveScalarInfo(info, -1, num);
    }
  else
    {
    vtkErrorMacro("SetNumberOfScalarComponents called with no "
                  "executive producing this image data object.");
    }
  this->ComputeIncrements();
}

//----------------------------------------------------------------------------
int vtkImageData::GetNumberOfScalarComponents()
{
  this->GetProducerPort();
  if(vtkInformation* info = this->GetPipelineInformation())
    {
    vtkInformation *scalarInfo = vtkDataObject::GetActiveFieldInformation(info,
      FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
    if (scalarInfo && scalarInfo->Has(FIELD_NUMBER_OF_COMPONENTS()))
      {
      return scalarInfo->Get( FIELD_NUMBER_OF_COMPONENTS() );
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
vtkIdType *vtkImageData::GetIncrements()
{
  // Make sure the increments are up to date. The filter bypass and update
  // mechanism make it tricky to update the increments anywhere other than here
  this->ComputeIncrements();

  return this->Increments;
}

//----------------------------------------------------------------------------
void vtkImageData::GetIncrements(vtkIdType &incX, vtkIdType &incY, vtkIdType &incZ)
{
  // Make sure the increments are up to date. The filter bypass and update
  // mechanism make it tricky to update the increments anywhere other than here
  this->ComputeIncrements();

  incX = this->Increments[0];
  incY = this->Increments[1];
  incZ = this->Increments[2];
}

//----------------------------------------------------------------------------
void vtkImageData::GetIncrements(vtkIdType inc[3])
{
  // Make sure the increments are up to date. The filter bypass and update
  // mechanism make it tricky to update the increments anywhere other than here
  this->ComputeIncrements();

  inc[0] = this->Increments[0];
  inc[1] = this->Increments[1];
  inc[2] = this->Increments[2];
}


//----------------------------------------------------------------------------
void vtkImageData::GetContinuousIncrements(int extent[6], vtkIdType &incX,
                                           vtkIdType &incY, vtkIdType &incZ)
{
  int e0, e1, e2, e3;

  incX = 0;
  const int* selfExtent = this->Extent;

  e0 = extent[0];
  if (e0 < selfExtent[0])
    {
    e0 = selfExtent[0];
    }
  e1 = extent[1];
  if (e1 > selfExtent[1])
    {
    e1 = selfExtent[1];
    }
  e2 = extent[2];
  if (e2 < selfExtent[2])
    {
    e2 = selfExtent[2];
    }
  e3 = extent[3];
  if (e3 > selfExtent[3])
    {
    e3 = selfExtent[3];
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
  // make sure we have data before computing incrments to traverse it
  if (!this->GetPointData()->GetScalars())
    {
    return;
    }
  vtkIdType inc = this->GetPointData()->GetScalars()->GetNumberOfComponents();
  const int* extent = this->Extent;

  for (idx = 0; idx < 3; ++idx)
    {
    this->Increments[idx] = inc;
    inc *= (extent[idx*2+1] - extent[idx*2] + 1);
    }
}

//----------------------------------------------------------------------------
void vtkImageData::CopyOriginAndSpacingFromPipeline()
{
  // Copy origin and spacing from pipeline information to the internal
  // copies.
  vtkInformation* info = this->PipelineInformation;
  if(info->Has(SPACING()))
    {
    this->SetSpacing(info->Get(SPACING()));
    }
  if(info->Has(ORIGIN()))
    {
    this->SetOrigin(info->Get(ORIGIN()));
    }
}

//----------------------------------------------------------------------------
template <class TIn, class TOut>
void vtkImageDataConvertScalar(TIn* in, TOut* out)
{
  *out = static_cast<TOut>(*in);
}

//----------------------------------------------------------------------------
double vtkImageData::GetScalarComponentAsDouble(int x, int y, int z, int comp)
{
  // Check the component index.
  if(comp < 0 || comp >= this->GetNumberOfScalarComponents())
    {
    vtkErrorMacro("Bad component index " << comp);
    return 0.0;
    }

  // Get a pointer to the scalar tuple.
  void* ptr = this->GetScalarPointer(x, y, z);
  if(!ptr)
    {
    // An error message was already generated by GetScalarPointer.
    return 0.0;
    }
  double result = 0.0;

  // Convert the scalar type.
  switch (this->GetScalarType())
    {
    vtkTemplateMacro(vtkImageDataConvertScalar(static_cast<VTK_TT*>(ptr)+comp,
                                               &result));
    default:
      {
      vtkErrorMacro("Unknown Scalar type " << this->GetScalarType());
      }
    }

  return result;
}

//----------------------------------------------------------------------------
void vtkImageData::SetScalarComponentFromDouble(int x, int y, int z, int comp,
                                                double value)
{
  // Check the component index.
  if(comp < 0 || comp >= this->GetNumberOfScalarComponents())
    {
    vtkErrorMacro("Bad component index " << comp);
    return;
    }

  // Get a pointer to the scalar tuple.
  void* ptr = this->GetScalarPointer(x, y, z);
  if(!ptr)
    {
    // An error message was already generated by GetScalarPointer.
    return;
    }

  // Convert the scalar type.
  switch (this->GetScalarType())
    {
    vtkTemplateMacro(vtkImageDataConvertScalar(
                       &value, static_cast<VTK_TT*>(ptr)+comp));
    default:
      {
      vtkErrorMacro("Unknown Scalar type " << this->GetScalarType());
      }
    }
}

//----------------------------------------------------------------------------
float vtkImageData::GetScalarComponentAsFloat(int x, int y, int z, int comp)
{
  return this->GetScalarComponentAsDouble(x, y, z, comp);
}

//----------------------------------------------------------------------------
void vtkImageData::SetScalarComponentFromFloat(int x, int y, int z, int comp,
                                               float value)
{
  this->SetScalarComponentFromDouble(x, y, z, comp, value);
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
void *vtkImageData::GetScalarPointer(int coordinate[3])
{
  vtkDataArray *scalars = this->GetPointData()->GetScalars();

  // Make sure the array has been allocated.
  if (scalars == NULL)
    {
    vtkDebugMacro("Allocating scalars in ImageData");
    this->AllocateScalars();
    scalars = this->PointData->GetScalars();
    }

  if (scalars == NULL)
    {
    vtkErrorMacro("Could not allocate scalars.");
    return NULL;
    }

  const int* extent = this->Extent;
  // error checking: since most access will be from pointer arithmetic.
  // this should not waste much time.
  for (int idx = 0; idx < 3; ++idx)
    {
    if (coordinate[idx] < extent[idx*2] ||
        coordinate[idx] > extent[idx*2+1])
      {
      vtkErrorMacro(<< "GetScalarPointer: Pixel (" << coordinate[0] << ", "
        << coordinate[1] << ", "
        << coordinate[2] << ") not in memory.\n Current extent= ("
        << extent[0] << ", " << extent[1] << ", "
        << extent[2] << ", " << extent[3] << ", "
        << extent[4] << ", " << extent[5] << ")");
      return NULL;
      }
    }

  return this->GetArrayPointer(scalars, coordinate);
}

//----------------------------------------------------------------------------
// This method returns a pointer to the origin of the vtkImageData.
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
void vtkImageData::SetScalarType(int type)
{
  this->GetProducerPort();
  if(vtkInformation* info = this->GetPipelineInformation())
    {
    vtkDataObject::SetPointDataActiveScalarInfo(info, type, -1);
    }
  else
    {
    vtkErrorMacro("SetScalarType called with no "
                  "executive producing this image data object.");
    }
}

//----------------------------------------------------------------------------
int vtkImageData::GetScalarType()
{
  this->GetProducerPort();
  if(vtkInformation* info = this->GetPipelineInformation())
    {
    vtkInformation *scalarInfo = vtkDataObject::GetActiveFieldInformation(info,
      FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
    if (scalarInfo)
      {
      return scalarInfo->Get( FIELD_ARRAY_TYPE() );
      }
    }
  return VTK_DOUBLE;
}

//----------------------------------------------------------------------------
void vtkImageData::AllocateScalars()
{
  int newType = VTK_DOUBLE;
  int newNumComp = 1;

  // basically allocate the scalars based on the
  this->GetProducerPort();
  if(vtkInformation* info = this->GetPipelineInformation())
    {
    vtkInformation *scalarInfo = vtkDataObject::GetActiveFieldInformation(info,
      FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
    if (scalarInfo)
      {
      newType = scalarInfo->Get( FIELD_ARRAY_TYPE() );
      if ( scalarInfo->Has(FIELD_NUMBER_OF_COMPONENTS()) )
        {
        newNumComp = scalarInfo->Get( FIELD_NUMBER_OF_COMPONENTS() );
        }
      }
    }

  vtkDataArray *scalars;

  // if the scalar type has not been set then we have a problem
  if (newType == VTK_VOID)
    {
    vtkErrorMacro("Attempt to allocate scalars before scalar type was set!.");
    return;
    }

  const int* extent = this->Extent;
  // Use vtkIdType to avoid overflow on large images
  vtkIdType dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;
  vtkIdType imageSize = dims[0]*dims[1]*dims[2];

  // if we currently have scalars then just adjust the size
  scalars = this->PointData->GetScalars();
  if (scalars && scalars->GetDataType() == newType
      && scalars->GetReferenceCount() == 1)
    {
    scalars->SetNumberOfComponents(newNumComp);
    scalars->SetNumberOfTuples(imageSize);
    // Since the execute method will be modifying the scalars
    // directly.
    scalars->Modified();
    return;
    }

  // allocate the new scalars
  scalars = vtkDataArray::CreateDataArray(newType);
  scalars->SetNumberOfComponents(newNumComp);

  // allocate enough memory
  scalars->SetNumberOfTuples(imageSize);

  this->PointData->SetScalars(scalars);
  scalars->Delete();
}


//----------------------------------------------------------------------------
int vtkImageData::GetScalarSize()
{
  return vtkDataArray::GetDataTypeSize(this->GetScalarType());
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class IT, class OT>
void vtkImageDataCastExecute(vtkImageData *inData, IT *inPtr,
                             vtkImageData *outData, OT *outPtr,
                             int outExt[6])
{
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  vtkIdType inIncX, inIncY, inIncZ;
  vtkIdType outIncX, outIncY, outIncZ;
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
        *outPtr = static_cast<OT>(*inPtr);
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
void vtkImageDataCastExecute(vtkImageData *inData, T *inPtr,
                             vtkImageData *outData, int outExt[6])
{
  void *outPtr = outData->GetScalarPointerForExtent(outExt);

  if (outPtr == NULL)
    {
    vtkGenericWarningMacro("Scalars not allocated.");
    return;
    }

  switch (outData->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageDataCastExecute(inData,
                              static_cast<T *>(inPtr),
                              outData,
                              static_cast<VTK_TT *>(outPtr),
                              outExt) );
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

  if (inPtr == NULL)
    {
    vtkErrorMacro("Scalars not allocated.");
    return;
    }

  switch (inData->GetScalarType())
    {
    vtkTemplateMacro(vtkImageDataCastExecute(inData,
                                             static_cast<VTK_TT *>(inPtr),
                                             this, extent) );
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}

//----------------------------------------------------------------------------
void vtkImageData::Crop()
{
  int           nExt[6];
  int           idxX, idxY, idxZ;
  int           maxX, maxY, maxZ;
  vtkIdType     outId, inId, inIdY, inIdZ, incZ, incY;
  vtkImageData  *newImage;
  vtkIdType numPts, numCells, tmp;
  const int* extent = this->Extent;

  int updateExtent[6] = {0,-1,0,-1,0,-1};
  this->GetUpdateExtent(updateExtent);

  // If extents already match, then we need to do nothing.
  if (extent[0] == updateExtent[0]
      && extent[1] == updateExtent[1]
      && extent[2] == updateExtent[2]
      && extent[3] == updateExtent[3]
      && extent[4] == updateExtent[4]
      && extent[5] == updateExtent[5])
    {
    return;
    }

  // Take the intersection of the two extent so that
  // we are not asking for more than the extent.
  this->GetUpdateExtent(nExt);
  if (nExt[0] < extent[0]) { nExt[0] = extent[0];}
  if (nExt[1] > extent[1]) { nExt[1] = extent[1];}
  if (nExt[2] < extent[2]) { nExt[2] = extent[2];}
  if (nExt[3] > extent[3]) { nExt[3] = extent[3];}
  if (nExt[4] < extent[4]) { nExt[4] = extent[4];}
  if (nExt[5] > extent[5]) { nExt[5] = extent[5];}

  // If the extents are the same just return.
  if (extent[0] == nExt[0] && extent[1] == nExt[1]
      && extent[2] == nExt[2] && extent[3] == nExt[3]
      && extent[4] == nExt[4] && extent[5] == nExt[5])
    {
    vtkDebugMacro("Extents already match.");
    return;
    }

  // How many point/cells.
  numPts = (nExt[1]-nExt[0]+1)*(nExt[3]-nExt[2]+1)*(nExt[5]-nExt[4]+1);
  // Conditional are to handle 3d, 2d, and even 1d images.
  tmp = nExt[1] - nExt[0];
  if (tmp <= 0)
    {
    tmp = 1;
    }
  numCells = tmp;
  tmp = nExt[3] - nExt[2];
  if (tmp <= 0)
    {
    tmp = 1;
    }
  numCells *= tmp;
  tmp = nExt[5] - nExt[4];
  if (tmp <= 0)
    {
    tmp = 1;
    }
  numCells *= tmp;

  // Create a new temporary image.
  newImage = vtkImageData::New();
  newImage->SetScalarType(this->GetScalarType());
  newImage->SetNumberOfScalarComponents(this->GetNumberOfScalarComponents());
  newImage->SetExtent(nExt);
  vtkPointData *npd = newImage->GetPointData();
  vtkCellData *ncd = newImage->GetCellData();
  npd->CopyAllocate(this->PointData, numPts);
  ncd->CopyAllocate(this->CellData, numCells);

  // Loop through outData points
  incY = extent[1]-extent[0]+1;
  incZ = (extent[3]-extent[2]+1)*incY;
  outId = 0;
  inIdZ = incZ * (nExt[4]-extent[4])
          + incY * (nExt[2]-extent[2])
          + (nExt[0]-extent[0]);

  for (idxZ = nExt[4]; idxZ <= nExt[5]; idxZ++)
    {
    inIdY = inIdZ;
    for (idxY = nExt[2]; idxY <= nExt[3]; idxY++)
      {
      inId = inIdY;
      for (idxX = nExt[0]; idxX <= nExt[1]; idxX++)
        {
        npd->CopyData( this->PointData, inId, outId);
        ++inId;
        ++outId;
        }
      inIdY += incY;
      }
    inIdZ += incZ;
    }

  // Loop through outData cells
  // Have to handle the 2d and 1d cases.
  maxX = nExt[1];
  maxY = nExt[3];
  maxZ = nExt[5];
  if (maxX == nExt[0])
    {
    ++maxX;
    }
  if (maxY == nExt[2])
    {
    ++maxY;
    }
  if (maxZ == nExt[4])
    {
    ++maxZ;
    }
  incY = extent[1]-extent[0];
  incZ = (extent[3]-extent[2])*incY;
  outId = 0;
  inIdZ = incZ * (nExt[4]-extent[4])
          + incY * (nExt[2]-extent[2])
          + (nExt[0]-extent[0]);
  for (idxZ = nExt[4]; idxZ < maxZ; idxZ++)
    {
    inIdY = inIdZ;
    for (idxY = nExt[2]; idxY < maxY; idxY++)
      {
      inId = inIdY;
      for (idxX = nExt[0]; idxX < maxX; idxX++)
        {
        ncd->CopyData(this->CellData, inId, outId);
        ++inId;
        ++outId;
        }
      inIdY += incY;
      }
    inIdZ += incZ;
    }

  this->PointData->ShallowCopy(npd);
  this->CellData->ShallowCopy(ncd);
  this->SetExtent(nExt);
  newImage->Delete();
}



//----------------------------------------------------------------------------
double vtkImageData::GetScalarTypeMin()
{
  return vtkDataArray::GetDataTypeMin(this->GetScalarType());
}


//----------------------------------------------------------------------------
double vtkImageData::GetScalarTypeMax()
{
  return vtkDataArray::GetDataTypeMax(this->GetScalarType());
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
}



//----------------------------------------------------------------------------
int *vtkImageData::GetDimensions()
{
  const int* extent = this->Extent;
  this->Dimensions[0] = extent[1] - extent[0] + 1;
  this->Dimensions[1] = extent[3] - extent[2] + 1;
  this->Dimensions[2] = extent[5] - extent[4] + 1;

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
void vtkImageData::SetAxisUpdateExtent(int idx, int min, int max)
{
  int modified = 0;

  if (idx > 2)
    {
    vtkWarningMacro("illegal axis!");
    return;
    }

  int updateExtent[6] = {0,-1,0,-1,0,-1};
  this->GetUpdateExtent(updateExtent);

  if (updateExtent[idx*2] != min)
    {
    modified = 1;
    updateExtent[idx*2] = min;
    }
  if (updateExtent[idx*2+1] != max)
    {
    modified = 1;
    updateExtent[idx*2+1] = max;
    }

  this->SetUpdateExtent(updateExtent);
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

  int updateExtent[6] = {0,-1,0,-1,0,-1};
  this->GetUpdateExtent(updateExtent);
  min = updateExtent[idx*2];
  max = updateExtent[idx*2+1];
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
  this->SetScalarType(src->GetScalarType());
  this->SetNumberOfScalarComponents(src->GetNumberOfScalarComponents());
  for (idx = 0; idx < 3; ++idx)
    {
    this->Dimensions[idx] = src->Dimensions[idx];
    this->Increments[idx] = src->Increments[idx];
    this->Origin[idx] = src->Origin[idx];
    this->Spacing[idx] = src->Spacing[idx];
    }
  memcpy(this->Extent, src->GetExtent(), 6*sizeof(int));
}



//----------------------------------------------------------------------------
vtkIdType vtkImageData::GetNumberOfCells()
{
  vtkIdType nCells=1;
  int i;
  const int* extent = this->Extent;

  vtkIdType dims[3];
  dims[0] = extent[1] - extent[0] + 1;
  dims[1] = extent[3] - extent[2] + 1;
  dims[2] = extent[5] - extent[4] + 1;

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

//============================================================================
// Starting to make some more general methods that deal with any array
// (not just scalars).
//============================================================================


//----------------------------------------------------------------------------
// This Method returns a pointer to a location in the vtkImageData.
// Coordinates are in pixel units and are relative to the whole
// image origin.
void vtkImageData::GetArrayIncrements(vtkDataArray* array, vtkIdType increments[3])
{
  const int* extent = this->Extent;
  // We could store tuple increments and just
  // multiply by the number of components...
  increments[0] = array->GetNumberOfComponents();
  increments[1] = increments[0] * (extent[1]-extent[0]+1);
  increments[2] = increments[1] * (extent[3]-extent[2]+1);
}

//----------------------------------------------------------------------------
void *vtkImageData::GetArrayPointerForExtent(vtkDataArray* array,
                                             int extent[6])
{
  int tmp[3];
  tmp[0] = extent[0];
  tmp[1] = extent[2];
  tmp[2] = extent[4];
  return this->GetArrayPointer(array, tmp);
}

//----------------------------------------------------------------------------
// This Method returns a pointer to a location in the vtkImageData.
// Coordinates are in pixel units and are relative to the whole
// image origin.
void *vtkImageData::GetArrayPointer(vtkDataArray* array, int coordinate[3])
{
  vtkIdType incs[3];
  vtkIdType idx;

  if (array == NULL)
    {
    return NULL;
    }

  const int* extent = this->Extent;
  // error checking: since most acceses will be from pointer arithmetic.
  // this should not waste much time.
  for (idx = 0; idx < 3; ++idx)
    {
    if (coordinate[idx] < extent[idx*2] ||
        coordinate[idx] > extent[idx*2+1])
      {
      vtkErrorMacro(<< "GetPointer: Pixel (" << coordinate[0] << ", "
        << coordinate[1] << ", "
        << coordinate[2] << ") not in current extent: ("
        << extent[0] << ", " << extent[1] << ", "
        << extent[2] << ", " << extent[3] << ", "
        << extent[4] << ", " << extent[5] << ")");
      return NULL;
      }
    }

  // compute the index of the vector.
  this->GetArrayIncrements(array, incs);
  idx = ((coordinate[0] - extent[0]) * incs[0]
         + (coordinate[1] - extent[2]) * incs[1]
         + (coordinate[2] - extent[4]) * incs[2]);
  // I could check to see if the array has the correct number
  // of tuples for the extent, but that would be an extra multiply.
  if (idx < 0 || idx > array->GetMaxId())
    {
    vtkErrorMacro("Coordinate (" << coordinate[0] << ", " << coordinate[1]
                  << ", " << coordinate[2] << ") out side of array (max = "
                  << array->GetMaxId());
    return NULL;
    }

  return array->GetVoidPointer(idx);
}


//----------------------------------------------------------------------------
void vtkImageData::ComputeInternalExtent(int *intExt, int *tgtExt, int *bnds)
{
  int i;
  const int* extent = this->Extent;
  for (i = 0; i < 3; ++i)
    {
    intExt[i*2] = tgtExt[i*2];
    if (intExt[i*2] - bnds[i*2] < extent[i*2])
      {
      intExt[i*2] = extent[i*2] + bnds[i*2];
      }
    intExt[i*2+1] = tgtExt[i*2+1];
    if (intExt[i*2+1] + bnds[i*2+1] > extent[i*2+1])
      {
      intExt[i*2+1] = extent[i*2+1] - bnds[i*2+1];
      }
    }
}

//----------------------------------------------------------------------------
vtkImageData* vtkImageData::GetData(vtkInformation* info)
{
  return info? vtkImageData::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkImageData* vtkImageData::GetData(vtkInformationVector* v, int i)
{
  return vtkImageData::GetData(v->GetInformationObject(i));
}
