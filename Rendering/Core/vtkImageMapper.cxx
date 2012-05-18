/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkImageMapper.h"

#include "vtkActor2D.h"
#include "vtkExecutive.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkViewport.h"
#include "vtkWindow.h"

//----------------------------------------------------------------------------
// Return NULL if no override is supplied.
vtkAbstractObjectFactoryNewMacro(vtkImageMapper)

//----------------------------------------------------------------------------

#define VTK_RINT(x) ((x > 0.0) ? (int)(x + 0.5) : (int)(x - 0.5))

vtkImageMapper::vtkImageMapper()
{
  vtkDebugMacro(<< "vtkImageMapper::vtkImageMapper" );

  this->ColorWindow = 2000;
  this->ColorLevel  = 1000;

  this->DisplayExtent[0] = this->DisplayExtent[1] = 0;
  this->DisplayExtent[2] = this->DisplayExtent[3] = 0;
  this->DisplayExtent[4] = this->DisplayExtent[5] = 0;
  this->ZSlice = 0;

  this->RenderToRectangle = 0;
  this->UseCustomExtents  = 0;
  this->CustomDisplayExtents[0] = this->CustomDisplayExtents[1] = 0;
  this->CustomDisplayExtents[2] = this->CustomDisplayExtents[3] = 0;

}

vtkImageMapper::~vtkImageMapper()
{
}

//----------------------------------------------------------------------------
void vtkImageMapper::SetInputData(vtkImageData *input)
{
  this->SetInputDataInternal(0, input);
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageMapper::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    return 0;
    }
  return vtkImageData::SafeDownCast(
    this->GetExecutive()->GetInputData(0, 0));
}

unsigned long int vtkImageMapper::GetMTime()
{
  unsigned long mTime=this->vtkMapper2D::GetMTime();
  return mTime;
}

void vtkImageMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Color Window: " << this->ColorWindow << "\n";
  os << indent << "Color Level: " << this->ColorLevel << "\n";
  os << indent << "ZSlice: " << this->ZSlice << "\n";
  os << indent << "RenderToRectangle: " << this->RenderToRectangle << "\n";
  os << indent << "UseCustomExtents: " << this->UseCustomExtents << "\n";
  os << indent << "CustomDisplayExtents: " <<
    this->CustomDisplayExtents[0] << " " <<
    this->CustomDisplayExtents[1] << " " <<
    this->CustomDisplayExtents[2] << " " <<
    this->CustomDisplayExtents[3] << "\n";
  //
}

double vtkImageMapper::GetColorShift()
{
  return this->ColorWindow /2.0 - this->ColorLevel;
}

double vtkImageMapper::GetColorScale()
{
  return 255.0 / this->ColorWindow;
}

void vtkImageMapper::RenderStart(vtkViewport* viewport, vtkActor2D* actor)
{
  vtkDebugMacro(<< "vtkImageMapper::RenderOverlay");

  vtkImageData *data;

  if (!viewport)
    {
    vtkErrorMacro (<< "vtkImageMapper::Render - Null viewport argument");
    return;
    }

  if (!actor)
    {
    vtkErrorMacro (<<"vtkImageMapper::Render - Null actor argument");
    return;
    }


  if (!this->GetInputAlgorithm())
    {
    vtkDebugMacro(<< "vtkImageMapper::Render - Please Set the input.");
    return;
    }

  this->GetInputAlgorithm()->UpdateInformation();

  vtkInformation* inInfo = this->GetInputInformation();

  if (!this->UseCustomExtents)
    {
    // start with the wholeExtent
    int wholeExtent[6];
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
    inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->DisplayExtent);
    // Set The z values to the zslice
    this->DisplayExtent[4] = this->ZSlice;
    this->DisplayExtent[5] = this->ZSlice;

    // scale currently not handled
    //double *scale = actor->GetScale();

    // get the position
    int *pos =
      actor->GetActualPositionCoordinate()->GetComputedViewportValue(viewport);

    // Get the viewport coordinates
    double vCoords[4];
    vCoords[0] = 0.0;
    vCoords[1] = 0.0;
    vCoords[2] = 1.0;
    vCoords[3] = 1.0;
    viewport->NormalizedViewportToViewport(vCoords[0],vCoords[1]);
    viewport->NormalizedViewportToViewport(vCoords[2],vCoords[3]);
    int *vSize = viewport->GetSize();

    // the basic formula is that the draw pos equals
    // the pos + extentPos + clippedAmount
    // The concrete subclass will get the pos in display
    // coordinates so we need to provide the extentPos plus
    // clippedAmount in the PositionAdjustment variable

    // Now clip to imager extents
    if (pos[0] + wholeExtent[0] < 0)
      {
      this->DisplayExtent[0] = -pos[0];
      }
    if ((pos[0]+wholeExtent[1]) > vSize[0])
      {
      this->DisplayExtent[1] = vSize[0] - pos[0];
      }
    if (pos[1] + wholeExtent[2] < 0)
      {
      this->DisplayExtent[2] = -pos[1];
      }
    if ((pos[1]+wholeExtent[3]) > vSize[1])
      {
      this->DisplayExtent[3] = vSize[1] - pos[1];
      }

    // check for the condition where no pixels are visible.
    if (this->DisplayExtent[0] > wholeExtent[1] ||
      this->DisplayExtent[1] < wholeExtent[0] ||
      this->DisplayExtent[2] > wholeExtent[3] ||
      this->DisplayExtent[3] < wholeExtent[2] ||
      this->DisplayExtent[4] > wholeExtent[5] ||
      this->DisplayExtent[5] < wholeExtent[4])
      {
      return;
      }

    vtkStreamingDemandDrivenPipeline::SetUpdateExtent(
      this->GetInputInformation(), this->DisplayExtent);

    // set the position adjustment
    this->PositionAdjustment[0] = this->DisplayExtent[0];
    this->PositionAdjustment[1] = this->DisplayExtent[2];
    }
  else // UseCustomExtents
    {
    this->DisplayExtent[0] = this->CustomDisplayExtents[0];
    this->DisplayExtent[1] = this->CustomDisplayExtents[1];
    this->DisplayExtent[2] = this->CustomDisplayExtents[2];
    this->DisplayExtent[3] = this->CustomDisplayExtents[3];
    this->DisplayExtent[4] = this->ZSlice;
    this->DisplayExtent[5] = this->ZSlice;
    //
    vtkStreamingDemandDrivenPipeline::SetUpdateExtentToWholeExtent(
      this->GetInputInformation());
    // clear the position adjustment
    this->PositionAdjustment[0] = 0;
    this->PositionAdjustment[1] = 0;
    }

  // Get the region from the input
  this->GetInputAlgorithm()->Update();
  data = this->GetInput();
  if ( !data)
    {
    vtkErrorMacro(<< "Render: Could not get data from input.");
    return;
    }

  this->RenderData(viewport, data, actor);
}

//----------------------------------------------------------------------------
int vtkImageMapper::GetWholeZMin()
{
  int *extent;

  if ( ! this->GetInput())
    {
    return 0;
    }
  this->GetInputAlgorithm()->UpdateInformation();
  extent = this->GetInputInformation()->Get(
    vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  return extent[4];
}

//----------------------------------------------------------------------------
int vtkImageMapper::GetWholeZMax()
{
  int *extent;

  if ( ! this->GetInput())
    {
    return 0;
    }
  this->GetInputAlgorithm()->UpdateInformation();
  extent = this->GetInputInformation()->Get(
    vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  return extent[5];
}

//----------------------------------------------------------------------------
int vtkImageMapper::FillInputPortInformation(
  int vtkNotUsed( port ), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}
