/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeProMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkVolumeProMapper.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"

#if defined (VTK_USE_VOLUMEPRO_1000) || defined (VTK_FORCE_COMPILE_VP1000)
#include "vtkVolumeProVP1000Mapper.h"
#endif

#include "vtkDebugLeaks.h"


#include "vtkObjectFactory.h"


//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkVolumeProMapper);
//----------------------------------------------------------------------------

// Create the mapper. No context has been created, no volume has
// been created yet.
vtkVolumeProMapper::vtkVolumeProMapper()
{
  int               i;

  this->Context = NULL;
  this->Volume = NULL;
  this->VolumeInput = NULL;
  this->VolumeBuildTime = vtkTimeStamp::New();
  this->Lights  = NULL;
  this->NumberOfLights = 0;
  this->BlendMode = VTK_BLEND_MODE_COMPOSITE;
  
  // Disable the subvolume
  for ( i = 0; i < 6; i++ )
    {
    this->SubVolume[i] = -1;
    }

  this->GradientOpacityModulation  = 0;
  this->GradientDiffuseModulation  = 0;
  this->GradientSpecularModulation = 0;

  this->Cursor            = 0;
  this->CursorType        = VTK_CURSOR_TYPE_CROSSHAIR;
  this->CursorPosition[0] = 0.0;
  this->CursorPosition[1] = 0.0;
  this->CursorPosition[2] = 0.0;
  
  this->CursorXAxisColor[0] = 1.0;
  this->CursorXAxisColor[1] = 0.0;
  this->CursorXAxisColor[2] = 0.0;
  
  this->CursorYAxisColor[0] = 0.0;
  this->CursorYAxisColor[1] = 1.0;
  this->CursorYAxisColor[2] = 0.0;
  
  this->CursorZAxisColor[0] = 0.0;
  this->CursorZAxisColor[1] = 0.0;
  this->CursorZAxisColor[2] = 1.0;
  
  this->CutPlane                   = 0;
  this->CutPlaneEquation[0]        = 1.0;
  this->CutPlaneEquation[1]        = 0.0;
  this->CutPlaneEquation[2]        = 0.0;
  this->CutPlaneEquation[3]        = 0.0;
  this->CutPlaneThickness          = 0.0;
  this->CutPlaneFallOffDistance    = 0;

  this->SuperSampling = 0;
  this->SuperSamplingFactor[0] = 1.0;
  this->SuperSamplingFactor[1] = 1.0;
  this->SuperSamplingFactor[2] = 1.0;

  this->NumberOfBoards      = 0;
  this->MajorBoardVersion   = 0;
  this->MinorBoardVersion   = 0;

  this->NoHardware        = 0;
  this->WrongVLIVersion   = 0;
  this->DisplayedMessage  = 0;

  this->Cut = NULL;
  
  this->IntermixIntersectingGeometry = 0;
  this->AutoAdjustMipmapLevels = 0;
  this->MinimumMipmapLevel = 0;
  this->MaximumMipmapLevel = 4;
  this->MipmapLevel = 0;

  this->RenderTimeTable        = NULL;
  this->RenderVolumeTable      = NULL;
  this->RenderRendererTable    = NULL;
  this->RenderTableSize        = 0;  
  this->RenderTableEntries     = 0;
  this->RenderTimer = vtkTimerLog::New();
}

// Destroy the mapper. Delete the context, volume build time, and the
// volume if necessary
vtkVolumeProMapper::~vtkVolumeProMapper()
{
  this->VolumeBuildTime->Delete();

  if ( this->RenderTableSize )
    {
    delete [] this->RenderTimeTable;
    delete [] this->RenderVolumeTable;
    delete [] this->RenderRendererTable;
    }
  this->RenderTimer->Delete();
}

// Simplified version - just assume the mapper type
vtkVolumeProMapper *vtkVolumeProMapper::New()
{ 
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkVolumeProMapper");
  if(ret)
    {
    return static_cast<vtkVolumeProMapper*>(ret);
    }
  
#if defined (VTK_USE_VOLUMEPRO_1000) || defined (VTK_FORCE_COMPILE_VP1000)
  vtkDebugLeaks::DestructClass("vtkVolumeProMapper");
  return vtkVolumeProVP1000Mapper::New();
#else
  // if not using vli, then return the stub class, which will render
  // nothing....
  return new vtkVolumeProMapper;
#endif
}

float vtkVolumeProMapper::RetrieveRenderTime( vtkRenderer *ren, 
                                              vtkVolume   *vol )
{
  int i;
  
  for ( i = 0; i < this->RenderTableEntries; i++ )
    {
    if ( this->RenderVolumeTable[i] == vol &&
         this->RenderRendererTable[i] == ren )
      {
      return this->RenderTimeTable[i];
      }
    }
  
  return 0.0;
}

void vtkVolumeProMapper::StoreRenderTime( vtkRenderer *ren, 
                                          vtkVolume   *vol, 
                                          float       time )
{
  int i;
  for ( i = 0; i < this->RenderTableEntries; i++ )
    {
    if ( this->RenderVolumeTable[i] == vol &&
         this->RenderRendererTable[i] == ren )
      {
      this->RenderTimeTable[i] = time;
      return;
      }
    }
  
  
  // Need to increase size
  if ( this->RenderTableEntries >= this->RenderTableSize )
    {
    if ( this->RenderTableSize == 0 )
      {
      this->RenderTableSize = 10;
      }
    else
      {
      this->RenderTableSize *= 2;
      }
    
    float       *oldTimePtr     = this->RenderTimeTable;
    vtkVolume   **oldVolumePtr   = this->RenderVolumeTable;
    vtkRenderer **oldRendererPtr = this->RenderRendererTable;
    
    this->RenderTimeTable     = new float [this->RenderTableSize];
    this->RenderVolumeTable   = new vtkVolume *[this->RenderTableSize];
    this->RenderRendererTable = new vtkRenderer *[this->RenderTableSize];
    
    for (i = 0; i < this->RenderTableEntries; i++ )
      {
      this->RenderTimeTable[i] = oldTimePtr[i];
      this->RenderVolumeTable[i] = oldVolumePtr[i];
      this->RenderRendererTable[i] = oldRendererPtr[i];
      }
    
    delete [] oldTimePtr;
    delete [] oldVolumePtr;
    delete [] oldRendererPtr;
    }
  
  this->RenderTimeTable[this->RenderTableEntries] = time;
  this->RenderVolumeTable[this->RenderTableEntries] = vol;
  this->RenderRendererTable[this->RenderTableEntries] = ren;
  
  this->RenderTableEntries++;
}

int vtkVolumeProMapper::StatusOK()
{
  if ( this->NoHardware )
    {
    if ( !this->DisplayedMessage )
      {
      vtkErrorMacro( << "No Hardware Found!" );
      this->DisplayedMessage = 1;
      }
    return 0;
    }

  if ( this->WrongVLIVersion )
    {
    if ( !this->DisplayedMessage )
      {
      vtkErrorMacro( << "Wrong VLI Version found!" );
      this->DisplayedMessage = 1;
      }
    return 0;
    }

  if ( this->Context == NULL )
    {
    return 0;
    }

  if ( this->LookupTable == NULL )
    {
    return 0;
    }

  if ( this->Cut == NULL )
    {
    return 0;
    }

  return 1;
}

void vtkVolumeProMapper::SetSuperSamplingFactor( double x, double y, double z )
{
  if ( x < 0.0 || x > 1.0 ||
       y < 0.0 || y > 1.0 ||
       z < 0.0 || z > 1.0  )
    {
    vtkErrorMacro( << "Invalid supersampling factor" << endl <<
      "Each component must be between 0 and 1" );
    return;
    }

  this->SuperSamplingFactor[0] = x;
  this->SuperSamplingFactor[1] = y;
  this->SuperSamplingFactor[2] = z;

  this->Modified();
}

void vtkVolumeProMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  // don't print this->SubVolume 
  os << indent << "Number Of Boards: " << this->NumberOfBoards << endl;

  os << indent << "Major Board Version: " << this->MajorBoardVersion << endl;

  os << indent << "Minor Board Version: " << this->MinorBoardVersion << endl;

  os << indent << "Hardware Available: " << 
    (this->NoHardware ? "No\n" : "Yes\n");

  os << indent << "Correct vli Version: " << 
    (this->WrongVLIVersion ? "No\n" : "Yes\n");

  os << indent << "Super Sampling: " <<  
    (this->SuperSampling ? "On\n" : "Off\n");

  os << indent << "Super Sampling Factor: " << 
    this->SuperSamplingFactor[0] << " by " << 
    this->SuperSamplingFactor[1] << " by " << 
    this->SuperSamplingFactor[2] << endl;

  os << indent << "Intermix Intersecting Geometry: "
     << (this->IntermixIntersectingGeometry ? "On\n" : "Off\n");
    
  os << indent << "Cursor: " <<  (this->Cursor ? "On\n" : "Off\n");

  os << indent << "Cursor Position: (" << 
    this->CursorPosition[0] << ", " <<
    this->CursorPosition[1] << ", " <<
    this->CursorPosition[0] << ")\n";

  os << indent << "Cursor X Axis Color: (" << 
    this->CursorXAxisColor[0] << ", " <<
    this->CursorXAxisColor[1] << ", " <<
    this->CursorXAxisColor[0] << ")\n";

  os << indent << "Cursor Y Axis Color: (" << 
    this->CursorYAxisColor[0] << ", " <<
    this->CursorYAxisColor[1] << ", " <<
    this->CursorYAxisColor[0] << ")\n";

  os << indent << "Cursor Z Axis Color: (" << 
    this->CursorZAxisColor[0] << ", " <<
    this->CursorZAxisColor[1] << ", " <<
    this->CursorZAxisColor[0] << ")\n";

  os << indent << "Cursor Type: " << this->GetCursorTypeAsString() << endl;

  os << indent << "Blend Mode: " << this->GetBlendModeAsString() << endl;

  os << indent << "Cut Plane: " << (this->CutPlane ? "On\n" : "Off\n");

  os << indent << "Cut Plane Equation: \n" << indent << "  (" <<
    this->CutPlaneEquation[0] << ")X + (" <<
    this->CutPlaneEquation[1] << ")Y + (" <<
    this->CutPlaneEquation[2] << ")Z + (" <<
    this->CutPlaneEquation[3] << ") = 0\n";

  os << indent << "Cut Plane Thickness " << this->CutPlaneThickness << endl;

  os << indent << "Cut Plane FallOff Distance " << 
    this->CutPlaneFallOffDistance << endl;

  os << indent << "Gradient Opacity Modulation: " <<
    (this->GradientOpacityModulation ? "On\n" : "Off\n");

  os << indent << "Gradient Specular Modulation: " <<
    (this->GradientSpecularModulation ? "On\n" : "Off\n");

  os << indent << "Gradient Diffuse Modulation: " <<
    (this->GradientDiffuseModulation ? "On\n" : "Off\n");

  os << indent << "AutoAdjustMipmapLevels: " <<
    (this->AutoAdjustMipmapLevels ? "On" : "Off") << endl;
  os << indent << "MipmapLevel: " << this->MipmapLevel << endl;
  os << indent << "MinimumMipmapLevel: " << this->MinimumMipmapLevel << endl;
  os << indent << "MaximumMipmapLevel: " << this->MaximumMipmapLevel << endl;
}
