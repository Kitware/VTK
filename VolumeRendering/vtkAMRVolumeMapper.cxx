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
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkUniformGrid.h"

#include "vtkTimerLog.h"
#include "vtkNew.h"

vtkStandardNewMacro( vtkAMRVolumeMapper );

// Construct a vtkAMRVolumeMapper 
//----------------------------------------------------------------------------
vtkAMRVolumeMapper::vtkAMRVolumeMapper()
{
  this->InternalMapper = vtkSmartVolumeMapper::New();
  this->Resampler = vtkAMRResampleFilter::New();
  this->HasMetaData = false;
  this->Resampler->SetDemandDrivenMode(0);
  this->Grid = NULL;
  this->NumberOfSamples[0] = 128;
  this->NumberOfSamples[1] = 128;
  this->NumberOfSamples[2] = 128;
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
  this->SetInputConnection(0, hdata->GetProducerPort());
}
//----------------------------------------------------------------------------
void vtkAMRVolumeMapper::SetInputConnection (int port, vtkAlgorithmOutput *input)
{
  this->Resampler->SetInputConnection(port, input);
  this->vtkVolumeMapper::SetInputConnection(port, input);
  if (this->Grid)
    {
    this->Grid->Delete();
    this->Grid = NULL;
    }
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
  if (!(this->Grid && (ren->GetRenderWindow()->GetDesiredUpdateRate()
                       >= this->InternalMapper->GetInteractiveUpdateRate())))
    {
    if (!this->HasMetaData)
      {
      // If there is no meta data then the resample filter has not been updated
      // with the proper frustrun bounds else it would have been done when
      // processing request information
      this->UpdateResampler(ren);
      }
    this->UpdateGrid();
    }
  if (this->Grid == NULL) 
    {
    // Could not create a grid
    return;
    }
  this->InternalMapper->SetInput(this->Grid);
  this->InternalMapper->Render(ren, vol);
}

//----------------------------------------------------------------------------
void vtkAMRVolumeMapper::UpdateResampler(vtkRenderer *ren)
{
  // First we need to create a bouding box that represents the visible region
  // of the camera in World Coordinates

  // In order to produce as tight of bounding box as possible we need to determine
  // the z range in view coordinates of the data and then project that part 
  // of the view volume back into world coordinates
  
  // We would just use the renderer's WorldToView and ViewToWorld methods but those
  // implementations are not efficient for example ViewToWorld would do 8
  // matrix inverse ops when all we really need to do is one

  // Make sure the bounds are up to date
  this->GetBounds();

  // Get the camera transformation
  vtkMatrix4x4 *matrix = 
    ren->GetActiveCamera()->
    GetCompositeProjectionTransformMatrix(ren->GetTiledAspectRatio(), 0, 1);

  int i, j, k;
  double pnt[4], tpnt[4];
  vtkBoundingBox bbox;
  pnt[3] = 1.0;
  for (i = 0; i < 2; i++)
    {
    pnt[0] = this->Bounds[i];
    for (j = 2; j < 4; j++)
      {
      pnt[1] = this->Bounds[j];
      for (k = 4; k < 6; k++)
        {
        pnt[2] = this->Bounds[k];
        matrix->MultiplyPoint(pnt, tpnt);
        if (tpnt[3])
          {
          bbox.AddPoint(tpnt[0]/tpnt[3], 
                        tpnt[1]/tpnt[3], tpnt[2]/tpnt[3]);
          }
        else
          {
          vtkErrorMacro("UpdateResampler: Found an Ideal Point going to VC!");
          }
        }
      }
    }

  double zRange[2];
  if (bbox.IsValid())
    {
    zRange[0] = bbox.GetMinPoint()[2];
    zRange[1] = bbox.GetMaxPoint()[2];
    // Normalize the z values to make sure they are between -1 and 1
    for (i = 0; i < 2; i++)
      {
      if (zRange[i] < -1.0)
        {
        zRange[i] = -1.0;
        }
      else if (zRange[i] > 1.0)
        {
        zRange[i] = 1.0;
        }
      }
    }
  else
    {
    // Since we could not find a valid bounding box assume that the 
    // zrange is -1 to 1
    zRange[0] = -1.0;
    zRange[1] = 1.0;
    }

  // Now that we have the z range of the data in View Coordinates lets
  // convert that part of the View Volume back into World Coordinates
  double mat[16];
  //Need the inverse
  vtkMatrix4x4::Invert(*matrix->Element, mat);

  bbox.Reset();
  // Compute the bounding box
  for (i = -1; i < 2; i+=2)
    {
    pnt[0] = i;
    for (j = -1; j < 2; j+=2)
      {
      pnt[1] = j;
      for (k = 0; k < 2; k++)
        {
        pnt[2] = zRange[k];
        vtkMatrix4x4::MultiplyPoint(mat,pnt,tpnt);
        if (tpnt[3])
          {
          bbox.AddPoint(tpnt[0]/tpnt[3], 
                        tpnt[1]/tpnt[3], tpnt[2]/tpnt[3]);
          }
        else
          {
          vtkErrorMacro("UpdateResampler: Found an Ideal Point going to WC!");
          }
        }
      }
    }

  // Check to see if the box is valid
  if (!bbox.IsValid())
    {
    return; // There is nothing we can do
    }
  // Now set the min/max of the resample filter
  this->Resampler->SetMin( const_cast< double* >(bbox.GetMinPoint()) );
  this->Resampler->SetMax( const_cast< double* >(bbox.GetMaxPoint()) );

  this->Resampler->SetNumberOfSamples(this->NumberOfSamples);
}
//----------------------------------------------------------------------------
void vtkAMRVolumeMapper::UpdateGrid()
{
  // This is for debugging
#define PRINTSTATS 0
#if PRINTSTATS
  vtkNew<vtkTimerLog> timer;
  int gridDim[3];
  double gridOrigin[3];
  timer->StartTimer();
#endif
  this->Resampler->Update();
#if PRINTSTATS
  timer->StopTimer();
  std::cerr << "Resample Time:" << timer->GetElapsedTime() << " ";
  std::cerr << "New Bounds: [" << bbox.GetMinPoint()[0] 
            << ", " << bbox.GetMaxPoint()[0] << "], ["
            << bbox.GetMinPoint()[1] 
            << ", " << bbox.GetMaxPoint()[1] << "], ["
            << bbox.GetMinPoint()[2] 
            << ", " << bbox.GetMaxPoint()[2] << "\n";
#endif
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
#if PRINTSTATS
  this->Grid->GetDimensions(gridDim);
  this->Grid->GetOrigin(gridOrigin);
  std::cerr << "Grid Dimenions: (" << gridDim[0] << ", " << gridDim[1] << ", " 
            << gridDim[2] 
            << ") Origin:(" << gridOrigin[0] << ", "<< gridOrigin[1] << ", "
            << gridOrigin[2] << ")\n";
#endif
}
//----------------------------------------------------------------------------
void vtkAMRVolumeMapper::ProcessUpdateExtentRequest(vtkRenderer *ren,
                                                    vtkInformation*info, 
                                                    vtkInformationVector **inputVector,
                                                    vtkInformationVector *outputVector)
{
  if (this->Grid && (ren->GetRenderWindow()->GetDesiredUpdateRate()
                     >= this->InternalMapper->GetInteractiveUpdateRate()))
    {
    return;
    }
  this->Resampler->RequestUpdateExtent(info, inputVector, outputVector);
}
//----------------------------------------------------------------------------
void vtkAMRVolumeMapper::ProcessInformationRequest(vtkRenderer *ren,
                                                   vtkInformation*info, 
                                                   vtkInformationVector **inputVector,
                                                   vtkInformationVector *outputVector)
{
  if (this->Grid && (ren->GetRenderWindow()->GetDesiredUpdateRate()
                     >= this->InternalMapper->GetInteractiveUpdateRate()))
    {
    return;
    }
  vtkInformation *input = inputVector[0]->GetInformationObject( 0 );
  if (!(input &&  input->Has(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA())))
    {
    this->HasMetaData = false;
    this->Resampler->SetDemandDrivenMode(0);
    return;
    }

  if (!this->HasMetaData)
    {
    this->HasMetaData = true;
    this->Resampler->SetDemandDrivenMode(1);
    }
  this->UpdateResampler(ren);
  this->Resampler->RequestInformation(info, inputVector, outputVector);
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
//----------------------------------------------------------------------------
