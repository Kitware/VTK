/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadricLODActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkQuadricLODActor.h"

#include "vtkObjectFactory.h"
#include "vtkMatrix4x4.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTexture.h"
#include "vtkPolyDataAlgorithm.h"
#include "vtkQuadricClustering.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkFollower.h"

vtkStandardNewMacro(vtkQuadricLODActor);

//---------------------------------------------------------------------------
// Specify the quadric clustering algorithm for decimating the geometry.
vtkCxxSetObjectMacro(vtkQuadricLODActor,LODFilter,vtkQuadricClustering);

//-------------------------------------------------------------------------
vtkQuadricLODActor::vtkQuadricLODActor()
{
  // Configure the decimation (quadric clustering) filter
  this->LODFilter = vtkQuadricClustering::New();
  this->LODFilter->UseInputPointsOn();
  this->LODFilter->CopyCellDataOn();
  this->LODFilter->UseInternalTrianglesOff();

  this->Static = 0;
  this->MaximumDisplayListSize = 25000;
  this->DeferLODConstruction = 0;
  this->CollapseDimensionRatio = 0.05;
  this->DataConfiguration = UNKNOWN;
  this->PropType = ACTOR;
  this->Camera = NULL;
  
  // Internal data members
  this->CachedInteractiveFrameRate = 0.0;

  // By default create an actor
  this->LODActor = vtkActor::New();

  // mapper for LOD actor
  this->LODMapper = vtkPolyDataMapper::New();
  this->LODMapper->ImmediateModeRenderingOff();
  
  // A internal matrix for performance
  vtkMatrix4x4 *m = vtkMatrix4x4::New();
  this->LODActor->SetUserMatrix(m);
  m->Delete();
  
}

//----------------------------------------------------------------------------
vtkQuadricLODActor::~vtkQuadricLODActor()
{
  this->LODFilter->Delete();
  
  this->LODActor->Delete();
  this->LODActor = NULL;
  this->LODMapper->Delete();
}


//----------------------------------------------------------------------------
int vtkQuadricLODActor::RenderOpaqueGeometry(vtkViewport *vp)
{
  int renderedSomething = 0; 
  vtkRenderer* ren = static_cast<vtkRenderer*>(vp);

  if ( ! this->Mapper )
    {
    return 0;
    }

  // is this actor opaque ?
  if (this->GetIsOpaque())
    {
    this->GetProperty()->Render(this, ren);

    // render the backface property
    if (this->BackfaceProperty)
      {
      this->BackfaceProperty->BackfaceRender(this, ren);
      }
    
    // render the texture 
    if (this->Texture)
      {
      this->Texture->Render(ren);
      }
    this->Render(ren,this->Mapper);

    renderedSomething = 1;
    }

  return renderedSomething;
}

//----------------------------------------------------------------------------
inline vtkIdType vtkQuadricLODActor::GetDisplayListSize(vtkPolyData *pd)
{
  vtkIdType numEntries = pd->GetVerts()->GetNumberOfConnectivityEntries();
  numEntries += pd->GetLines()->GetNumberOfConnectivityEntries();
  numEntries += pd->GetPolys()->GetNumberOfConnectivityEntries();
  numEntries += pd->GetStrips()->GetNumberOfConnectivityEntries();

  return numEntries;
}


//----------------------------------------------------------------------------
void vtkQuadricLODActor::Render(vtkRenderer *ren, vtkMapper *vtkNotUsed(m))
{
  float allowedTime, bestTime;
  vtkMatrix4x4 *matrix;
  vtkMapper *bestMapper;

  if ( !this->Mapper )
    {
    vtkErrorMacro("No mapper for actor.");
    return;
    }
  
  // determine out how much time we have to render
  allowedTime = this->AllocatedRenderTime;
  double frameRate = ren->GetRenderWindow()->GetInteractor()->GetDesiredUpdateRate();
  frameRate = (frameRate < 1.0 ? 1.0 : (frameRate > 75 ? 75.0 : frameRate));
  int interactiveRender = 0;
  // interactive renders are defined when compared with the desired update rate. Here we use
  // a generous fudge factor to insure that the LOD kicks in.
  if ( allowedTime <= (1.1/frameRate) )
    {
    interactiveRender = 1;
    }

  // Use display lists if it makes sense
  vtkIdType nCells = this->GetDisplayListSize(static_cast<vtkPolyData*>(this->Mapper->GetInput()));
  if ( nCells < this->MaximumDisplayListSize )
    {
    this->Mapper->ImmediateModeRenderingOff();
    }
  else
    {
    this->Mapper->ImmediateModeRenderingOn();
    }

  // Build LOD only if necessary
  if ( (interactiveRender || !this->DeferLODConstruction) && 
       (this->GetMTime() > this->BuildTime || 
        this->Mapper->GetMTime() > this->BuildTime ||
        this->CachedInteractiveFrameRate < 0.9*frameRate || this->CachedInteractiveFrameRate > 1.1*frameRate) )
    {
    vtkDebugMacro(">>>>>>>>>>>>>>>Building LOD");
    this->CachedInteractiveFrameRate = frameRate;

    // The mapper must be updated the first time prior to going static
    this->Mapper->Update();
    if ( this->Static )
      {
      this->Mapper->StaticOn();
      }
    else
      {
      this->Mapper->StaticOff();
      }

    // Make sure LOD mapper is consistent with mapper
    this->LODMapper->ShallowCopy(this->Mapper);
    this->LODMapper->ImmediateModeRenderingOff();
    this->LODActor->SetProperty(this->GetProperty());
    this->LODActor->SetBackfaceProperty(this->BackfaceProperty);

    // This table has been empirically defined. It specifies a quadric
    // clustering bin size go along with a desired frame rate.
    static int NumTableEntries = 7;
    static double FPSTable[] = { 0.0,  5.0, 10.0, 17.5, 25.0, 50.0, 75.0};
    static double DIMTable[] = {75.0, 60.0, 50.0, 35.0, 25.0, 20.0, 15.0};
    int dim = 15;
    for (int i=0; i < (NumTableEntries-1); i++)
      {
      if (frameRate >= FPSTable[i] && frameRate <= FPSTable[i+1] )
        {
        dim = static_cast<int>((DIMTable[i] + 
                                (frameRate-FPSTable[i])/(FPSTable[i+1]-FPSTable[i]) * (DIMTable[i+1]-DIMTable[i])));
        break;
        }
      }

    // Construct the LOD
    vtkPolyData *pd = vtkPolyData::SafeDownCast(this->Mapper->GetInput());

    // First see if there is an explicit description of the data configuration.
    if ( this->DataConfiguration == XLINE )
      {
      this->LODFilter->SetNumberOfDivisions(dim,1,1);
      }
    else if ( this->DataConfiguration == YLINE )
      {
      this->LODFilter->SetNumberOfDivisions(1,dim,1);
      }
    else if ( this->DataConfiguration == ZLINE )
      {
      this->LODFilter->SetNumberOfDivisions(1,1,dim);
      }
    else if ( this->DataConfiguration == XYPLANE )
      {
      this->LODFilter->SetNumberOfDivisions(dim,dim,1);
      }
    else if ( this->DataConfiguration == YZPLANE )
      {
      this->LODFilter->SetNumberOfDivisions(1,dim,dim);
      }
    else if ( this->DataConfiguration == XZPLANE )
      {
      this->LODFilter->SetNumberOfDivisions(dim,1,dim);
      }
    else if ( this->DataConfiguration == XYZVOLUME )
      {
      this->LODFilter->SetNumberOfDivisions(dim,dim,dim);
      }
    else //no explicit description
      {
      // If here, we analyze the data to see if we can optimize binning.  The
      // binning is optimized depending on data dimension and data aspect
      // ratio.
      double bounds[6], h[3];
      pd->GetBounds(bounds);
      h[0] = bounds[1] - bounds[0];
      h[1] = bounds[3] - bounds[2];
      h[2] = bounds[5] - bounds[4];
      double hMax = (h[0]>h[1] ? (h[0]>h[2]?h[0]:h[2]) : (h[1]>h[2]?h[1]:h[2]));
      int nDivs[3], numSmallDims=0;
      for (int i=0; i<3; i++)
        {
        if ( h[i] <= (this->CollapseDimensionRatio*hMax) )
          {
          nDivs[i] = 1;
          numSmallDims++;
          }
        else
          {
          nDivs[i] = dim;
          }
        }
      this->LODFilter->SetNumberOfDivisions(nDivs);
      }//data configuration not explicitly specified
    
    vtkDebugMacro("QC bin size: " << dim);
    this->LODFilter->AutoAdjustNumberOfDivisionsOff();
    this->LODFilter->SetInput(pd);
    this->LODFilter->Update();
    nCells = this->GetDisplayListSize(this->LODFilter->GetOutput());
    this->LODMapper->SetInputConnection(this->LODFilter->GetOutputPort());

    // Make sure the device has the same matrix. Only update when still update
    // rate is requested.
    matrix = this->LODActor->GetUserMatrix();
    this->GetMatrix(matrix);
  
    this->LODMapper->Update();
    if ( this->Static )
      {
      this->LODMapper->StaticOn();
      }
    this->BuildTime.Modified();
    }

  // Figure out which resolution to use. We want the highest resolution that
  // fits under the time allowed.  There is no order to the list, so it is
  // assumed that mappers that take longer to render are better quality.
  // Timings might become out of date, but we rely on them to be consistent
  // across renders.
  bestMapper = this->Mapper;
  bestTime = bestMapper->GetTimeToDraw();

  if ( interactiveRender )
    {//use lod
    bestMapper = this->LODMapper;
    bestTime = bestMapper->GetTimeToDraw();
    vtkDebugMacro("LOD render (best,allowed): " << bestTime << "," << allowedTime);
    }
  else
    {//use full resolution
    //Only update when still update rate is requested.
    matrix = this->LODActor->GetUserMatrix();
    this->GetMatrix(matrix);
    vtkDebugMacro("----Full render (best,allowed): " << bestTime << "," << allowedTime);
    }
    
  // render the property
  if (!this->Property)
    {
    // force creation of a property
    this->GetProperty();
    }
  this->Property->Render(this, ren);
  
  if (this->BackfaceProperty)
    {
    this->BackfaceProperty->BackfaceRender(this, ren);
    this->LODActor->SetBackfaceProperty(this->BackfaceProperty);
    }
  this->LODActor->SetProperty(this->Property);
  
  // render the texture
  if (this->Texture)
    {
    this->Texture->Render(ren);
    }
  
  // Store information on time it takes to render.
  // We might want to estimate time from the number of polygons in mapper.
  this->LODActor->Render(ren,bestMapper);
  this->EstimatedRenderTime = bestMapper->GetTimeToDraw();
}

//----------------------------------------------------------------------------
void vtkQuadricLODActor::ReleaseGraphicsResources(vtkWindow *renWin)
{
  vtkActor::ReleaseGraphicsResources(renWin);
  this->LODActor->ReleaseGraphicsResources(renWin);
  this->Mapper->ReleaseGraphicsResources(renWin);
}

//----------------------------------------------------------------------------
void vtkQuadricLODActor::ShallowCopy(vtkProp *prop)
{
  // Now do superclass
  this->vtkActor::ShallowCopy(prop);
}

//----------------------------------------------------------------------------
void vtkQuadricLODActor::SetCamera(vtkCamera *camera)
{
  vtkFollower *follower = vtkFollower::SafeDownCast(this->LODActor);
  if (follower)
    {
    follower->SetCamera(camera);
    }
}
  
//----------------------------------------------------------------------------
void vtkQuadricLODActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Defer LOD Contruction: "
     << (this->DeferLODConstruction ? "On\n" : "Off\n");

  os << indent << "Static : " << (this->Static ? "On\n" : "Off\n");

  os << indent << "Collapse Dimension Ratio: " << this->CollapseDimensionRatio << "\n";

  os << indent << "Data Configuration: ";
  if ( this->DataConfiguration == XYZVOLUME )
    {
    os << "XYZ Volume\n";
    }
  else if ( this->DataConfiguration == XLINE )
    {
    os << "X Line\n";
    }
  else if ( this->DataConfiguration == YLINE )
    {
    os << "Y Line\n";
    }
  else if ( this->DataConfiguration == ZLINE )
    {
    os << "Z Line\n";
    }
  else if ( this->DataConfiguration == XYPLANE )
    {
    os << "XY Plane\n";
    }
  else if ( this->DataConfiguration == YZPLANE )
    {
    os << "YZ Plane\n";
    }
  else if ( this->DataConfiguration == XZPLANE )
    {
    os << "XZ Plane\n";
    }
  else //if ( this->DataConfiguration == XLINE )
    {
    os << "Unknown\n";
    }
  
  os << indent << "LOD Filter: ";
  if ( this->LODFilter )
    {
    os << this->LODFilter << "\n";
    }
  else
    {
    os << "(none)\n";
    }
  
  os << indent << "Maximum Display List Size: " 
     << this->MaximumDisplayListSize << "\n";

  os << indent << "Prop Type: ";
  if ( this->PropType == FOLLOWER )
    {
    os << "Follower\n";
    }
  else 
    {
    os << "Actor\n";
    }

  os << indent << "Camera: ";
  if ( this->Camera )
    {
    os << this->Camera << "\n";
    }
  else
    {
    os << "(none)\n";
    }
}
