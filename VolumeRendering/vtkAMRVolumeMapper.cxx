/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMRVolumeMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAMRVolumeMapper.h"

#include "vtkAMRResampleFilter.h"
#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkUniformGrid.h"

vtkStandardNewMacro( vtkAMRVolumeMapper );

// Construct a vtkAMRVolumeMapper 
//----------------------------------------------------------------------------
vtkAMRVolumeMapper::vtkAMRVolumeMapper()
{
  this->InternalMapper = vtkSmartVolumeMapper::New();
  this->Resampler = vtkAMRResampleFilter::New();
  this->Grid = NULL;
  this->NumberOfSamples[0] = 20;
  this->NumberOfSamples[1] = 20;
  this->NumberOfSamples[2] = 20;
  vtkMath::UninitializeBounds(this->Bounds);
}

//----------------------------------------------------------------------------
vtkAMRVolumeMapper::~vtkAMRVolumeMapper()
{
  this->InternalMapper->Delete();
  this->InternalMapper = NULL;
  this->Resampler->Delete();
  this->Resampler = NULL;
  if (this->Grid)
    {
    this->Grid->Delete();
    this->Grid = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkAMRVolumeMapper::SetInput(vtkDataSet *genericInput)
{
  vtkErrorMacro("Mapper expects a hierarchical dataset as input" );
  this->Resampler->SetInputConnection(0, 0);
}
//----------------------------------------------------------------------------
void vtkAMRVolumeMapper::SetInput(vtkHierarchicalBoxDataSet *hdata)
{
  if (!hdata)
    {
    vtkErrorMacro("Mapper expects a hierarchical dataset as input" );
    this->Resampler->SetInputConnection(0, 0);
    return;
    }
  this->Resampler->SetInputConnection(0, hdata->GetProducerPort());
}
//----------------------------------------------------------------------------
void vtkAMRVolumeMapper::SetInputConnection (int port, vtkAlgorithmOutput *input)
{
  this->Resampler->SetInputConnection(port, input);
  this->vtkVolumeMapper::SetInputConnection(port, input);
}
//----------------------------------------------------------------------------
double *vtkAMRVolumeMapper::GetBounds()
{
  vtkHierarchicalBoxDataSet*hdata;
  hdata = 
    vtkHierarchicalBoxDataSet::SafeDownCast
    (this->Resampler->GetInputDataObject(0,0));
  if (!hdata)
    {
    vtkMath::UninitializeBounds(this->Bounds);
    }
  else
    {
    hdata->GetBounds(this->Bounds);
    }
  return this->Bounds;
}
//----------------------------------------------------------------------------
int vtkAMRVolumeMapper::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHierarchicalBoxDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkAMRVolumeMapper::SelectScalarArray(int arrayNum)
{
  this->InternalMapper->SelectScalarArray(arrayNum);
}

//----------------------------------------------------------------------------
void vtkAMRVolumeMapper::SelectScalarArray(const char *arrayName)
{
  this->InternalMapper->SelectScalarArray(arrayName);
}

//----------------------------------------------------------------------------
const char *vtkAMRVolumeMapper::GetScalarModeAsString()
{
  return this->InternalMapper->GetScalarModeAsString();
}

//----------------------------------------------------------------------------
char *vtkAMRVolumeMapper::GetArrayName()
{
  return this->InternalMapper->GetArrayName();
}

//----------------------------------------------------------------------------
int vtkAMRVolumeMapper::GetArrayId()
{
  return this->InternalMapper->GetArrayId();
}

//----------------------------------------------------------------------------
int vtkAMRVolumeMapper::GetArrayAccessMode()
{
  return this->InternalMapper->GetArrayAccessMode();
}
//----------------------------------------------------------------------------
void vtkAMRVolumeMapper::SetScalarMode(int mode)
{
  this->vtkVolumeMapper::SetScalarMode(mode);
  // for the internal mapper we need to convert all cell based
  // modes to point based since this is what the resample filter is doing
  int newMode = mode;
  if (mode == VTK_SCALAR_MODE_USE_CELL_DATA)
    {
    newMode = VTK_SCALAR_MODE_USE_POINT_DATA;
    }
  else if (mode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA)
    {
    newMode = VTK_SCALAR_MODE_USE_POINT_FIELD_DATA;
    }

  this->InternalMapper->SetScalarMode(newMode);
}
//----------------------------------------------------------------------------
void vtkAMRVolumeMapper::SetBlendMode(int mode)
{
  this->InternalMapper->SetBlendMode(mode);
}
//----------------------------------------------------------------------------
int vtkAMRVolumeMapper::GetBlendMode()
{
  return this->InternalMapper->GetBlendMode();
}
//----------------------------------------------------------------------------
void vtkAMRVolumeMapper::SetCropping(int mode)
{
  this->InternalMapper->SetCropping(mode);
}
//----------------------------------------------------------------------------
int vtkAMRVolumeMapper::GetCropping()
{
  return this->InternalMapper->GetCropping();
}
//----------------------------------------------------------------------------
void vtkAMRVolumeMapper::SetCroppingRegionFlags(int mode)
{
  this->InternalMapper->SetCroppingRegionFlags(mode);
}
//----------------------------------------------------------------------------
int vtkAMRVolumeMapper::GetCroppingRegionFlags()
{
  return this->InternalMapper->GetCroppingRegionFlags();
}
//----------------------------------------------------------------------------
void vtkAMRVolumeMapper::SetCroppingRegionPlanes(double arg1, double arg2,
                                                 double arg3, double arg4,
                                                 double arg5, double arg6)
{
  this->InternalMapper->SetCroppingRegionPlanes(arg1, arg2, arg3,
                                                arg4, arg5, arg6);
}
//----------------------------------------------------------------------------
void vtkAMRVolumeMapper::GetCroppingRegionPlanes(double *planes)
{
  this->InternalMapper->GetCroppingRegionPlanes(planes);
}
//----------------------------------------------------------------------------
double *vtkAMRVolumeMapper::GetCroppingRegionPlanes()
{
  return this->InternalMapper->GetCroppingRegionPlanes();
}
//----------------------------------------------------------------------------
void vtkAMRVolumeMapper::SetRequestedRenderMode(int mode)
{
  this->InternalMapper->SetRequestedRenderMode(mode);
}
//----------------------------------------------------------------------------
int vtkAMRVolumeMapper::GetRequestedRenderMode()
{
  return this->InternalMapper->GetRequestedRenderMode();
}
//----------------------------------------------------------------------------
void vtkAMRVolumeMapper::SetInteractiveUpdateRate(double rate)
{
  this->InternalMapper->SetInteractiveUpdateRate(rate);
}
//----------------------------------------------------------------------------
double vtkAMRVolumeMapper::GetInteractiveUpdateRate()
{
  return this->InternalMapper->GetInteractiveUpdateRate();
}
//----------------------------------------------------------------------------
void vtkAMRVolumeMapper::SetInterpolationMode(int mode)
{
  this->InternalMapper->SetInterpolationMode(mode);
}
//----------------------------------------------------------------------------
int vtkAMRVolumeMapper::GetInterpolationMode()
{
  return this->InternalMapper->GetInterpolationMode();
}
//----------------------------------------------------------------------------
void vtkAMRVolumeMapper::ReleaseGraphicsResources(vtkWindow *window)
{
  this->InternalMapper->ReleaseGraphicsResources(window);
}
//----------------------------------------------------------------------------
void vtkAMRVolumeMapper::Render(vtkRenderer *ren, vtkVolume *vol)
{
  // If there is no grid initially we need to see if we can create one
  bool gridUpdated = false;
  if (this->Grid == NULL) 
    {
    this->UpdateGrid(ren);
    if (this->Grid == NULL)
      {
      // Could not create a grid
      return;
      }
    this->InternalMapper->SetInput(this->Grid);
    gridUpdated = true;
    }
  this->InternalMapper->Render(ren, vol);
  // Lets see if we are dealing with a still render in which 
  // case we can also update the Grid and rerender (if we haven't
  // just updated the grid)
  if (gridUpdated || (ren->GetRenderWindow()->GetDesiredUpdateRate()
                      >= this->InternalMapper->GetInteractiveUpdateRate()))
    {
    return;
    }
  this->UpdateGrid(ren);
  if (this->Grid)
    {
    this->InternalMapper->SetInput(this->Grid);
    this->InternalMapper->Render(ren, vol);
    }
}

//----------------------------------------------------------------------------
void vtkAMRVolumeMapper::UpdateGrid(vtkRenderer *ren)
{
  // First we need to create a bouding box that represents the visible region
  // of the camera in World Coordinates
  // We would just use the renderer's ViewToWorld method but that would do 8
  // matrix inverse ops when all we really need to do is one
  double mat[16];
  const double vp[8][4] = {{-1.0, -1.0, -1.0, 1.0},
                          { 1.0, -1.0, -1.0, 1.0},
                          {-1.0,  1.0, -1.0, 1.0},
                          { 1.0,  1.0, -1.0, 1.0},
                          {-1.0, -1.0,  1.0, 1.0},
                          { 1.0, -1.0,  1.0, 1.0},
                          {-1.0,  1.0,  1.0, 1.0},
                          { 1.0,  1.0,  1.0, 1.0}};
  double wp[4];
  vtkBoundingBox bbox;
  int i;
  vtkMatrix4x4 *matrix = 
    ren->GetActiveCamera()->
    GetCompositeProjectionTransformMatrix(ren->GetTiledAspectRatio(), 0, 1);
  //Need the inverse
  vtkMatrix4x4::Invert(*matrix->Element, mat);

  // Compute the bounding box
  for (i = 0; i < 8; i++)
    {
     vtkMatrix4x4::MultiplyPoint(mat,vp[i],wp);
     if (wp[3])
       {
       bbox.AddPoint(wp[0]/wp[3], wp[1]/wp[3], wp[2]/wp[3]);
       }
     else
       {
       vtkErrorMacro("UpdateGrid: Found an Ideal Point!");
       }
    }

  // Check to see if the box is valid
  if (!bbox.IsValid())
    {
    return; // There is nothing we can do
    }
  // Now set the min/max of the resample filter
  this->Resampler->SetMin(bbox.GetMinPoint());
  this->Resampler->SetMax(bbox.GetMaxPoint());
  this->Resampler->SetNumberOfSamples(this->NumberOfSamples);
  this->Resampler->Update();
  std::cerr << "New Bounds: [" << bbox.GetMinPoint()[0] 
            << ", " << bbox.GetMaxPoint()[0] << "], ["
            << bbox.GetMinPoint()[1] 
            << ", " << bbox.GetMaxPoint()[1] << "], ["
            << bbox.GetMinPoint()[2] 
            << ", " << bbox.GetMaxPoint()[2] << "\n";
  vtkMultiBlockDataSet *mb = this->Resampler->GetOutput();
  if (!mb)
    {
    return;
    }
  unsigned int nb = mb->GetNumberOfBlocks();
  if (!nb)
    {
    // No new grid was created
    return;
    }
  if (nb != 1)
    {
    vtkErrorMacro("UpdateGrid: Resampler created more than 1 Grid!");
    }
  if (this->Grid)
    {
    this->Grid->Delete();
    }
  this->Grid = vtkUniformGrid::SafeDownCast(mb->GetBlock(0));
  this->Grid->Register(0);
}
//----------------------------------------------------------------------------


// Print the vtkAMRVolumeMapper
void vtkAMRVolumeMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ScalarMode: " << this->GetScalarModeAsString() << endl;
  
  if ( this->ScalarMode == VTK_SCALAR_MODE_USE_POINT_FIELD_DATA ||
       this->ScalarMode == VTK_SCALAR_MODE_USE_CELL_FIELD_DATA )
    {
    if (this->ArrayAccessMode == VTK_GET_ARRAY_BY_ID)
      {
      os << indent << "ArrayId: " << this->ArrayId << endl;
      }
    else
      {
      os << indent << "ArrayName: " << this->ArrayName << endl;
      }
    }
}
