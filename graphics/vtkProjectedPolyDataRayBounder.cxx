/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProjectedPolyDataRayBounder.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Lisa Sobierajski Avila who developed this class.

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
#include "vtkProjectedPolyDataRayBounder.h"
#include "vtkRenderWindow.h"
#include "vtkGraphicsFactory.h"



// The constructor for the class. Initialize everything to NULL.
vtkProjectedPolyDataRayBounder::vtkProjectedPolyDataRayBounder()
{
  this->ActorMatrixSource  = NULL;
  this->VolumeMatrixSource = NULL;
  this->PolyData           = NULL;
}

// Destructor for the class.
vtkProjectedPolyDataRayBounder::~vtkProjectedPolyDataRayBounder()
{
  this->SetPolyData(NULL);
  if (this->ActorMatrixSource != NULL)
    {
    this->ActorMatrixSource->UnRegister(this);
    }
  this->ActorMatrixSource = NULL;
  if (this->VolumeMatrixSource != NULL)
    {
    this->VolumeMatrixSource->UnRegister(this);
    }
  this->VolumeMatrixSource = NULL;
}

// New method for the class which will return the correct type of 
// ProjectPolyDataRayBounder
vtkProjectedPolyDataRayBounder *vtkProjectedPolyDataRayBounder::New()
{  
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = 
    vtkGraphicsFactory::CreateInstance("vtkProjectedPolyDataRayBounder");
  return (vtkProjectedPolyDataRayBounder*)ret;
}

// Set the matrix source to be an actor. The PolyData will be transformed
// by this actor's matrix before being projected
void vtkProjectedPolyDataRayBounder::SetMatrixSource( vtkActor *actor )
{
  if (this->ActorMatrixSource  != actor)
    {
    this->Modified();
    
    if (this->ActorMatrixSource != NULL)
      {
      this->ActorMatrixSource->UnRegister(this);
      }
    this->ActorMatrixSource  = actor;
    if (this->ActorMatrixSource != NULL)
      {
      this->ActorMatrixSource->Register(this);
      }
    
    if (this->VolumeMatrixSource != NULL)
      {
      this->VolumeMatrixSource->UnRegister(this);
      }
    this->VolumeMatrixSource = NULL;
    }
}

// Set the matrix source to be a volume. The PolyData will be transformed
// by this volume's matrix before being projected
void vtkProjectedPolyDataRayBounder::SetMatrixSource( vtkVolume *volume )
{
  if (this->VolumeMatrixSource  != volume)
    {
    this->Modified();
    
    if (this->VolumeMatrixSource != NULL)
      {
      this->VolumeMatrixSource->UnRegister(this);
      }
    this->VolumeMatrixSource  = volume;
    if (this->VolumeMatrixSource != NULL)
      {
      this->VolumeMatrixSource->Register(this);
      }
    
    if (this->ActorMatrixSource != NULL)
      {
      this->ActorMatrixSource->UnRegister(this);
      }
    this->ActorMatrixSource = NULL;
    }
}

// Get the ray bounds by 1) making sure the PolyData is up-to-date,
// 2) building the PolyData if necessary by calling this->Build(),
// 3) obtaining the correct matrix to transform the PolyData, and 
// 4) calling this->Draw() to actually generate the ray bounds;
float *vtkProjectedPolyDataRayBounder::GetRayBounds( vtkRenderer *ren )
{
  vtkMatrix4x4    *matrix;
  float           *return_bounds;
  

  // We must have PolyData!
  if ( !this->PolyData )
    {
    vtkErrorMacro( << "Ack! There's no input!" );
    return (float *)NULL;
    }

  // Create a new matrix to use if we don't have a matrix source
  matrix = vtkMatrix4x4::New();
  
  // Update the PolyData
  this->PolyData->Update();

  // We need to build if our PolyData is more recent than our last build,
  // or this object has been modified more recently than our last build.
  if ( this->PolyData->GetMTime() > this->BuildTime ||
       this->GetMTime() > this->BuildTime )
    {
    this->Build( this->PolyData );
    this->BuildTime.Modified();
    }

  // Get the matrix source's matrix (if there is one)
  if ( this->ActorMatrixSource )
    {
    this->ActorMatrixSource->GetMatrix( matrix );
    }
  else if ( this->VolumeMatrixSource )
    {
    this->VolumeMatrixSource->GetMatrix( matrix );
    }

  // Call Draw() to obtain the bounds. 
  return_bounds = this->Draw( ren, matrix );

  // Delete the temporary matrix that we previously created
  matrix->Delete();

  return (return_bounds);
}

// This is a bogus method that will only be called if a
// vtkProjectedPolyDataRayBounder was created with a render type other
// than OpenGL. 
void vtkProjectedPolyDataRayBounder::Build( vtkPolyData *vtkNotUsed(pdata) )
{
}

// This is a bogus method that will only be called if a
// vtkProjectedPolyDataRayBounder was created with a render type other
// than OpenGL. 
float *vtkProjectedPolyDataRayBounder::Draw( vtkRenderer *vtkNotUsed(ren), 
					 vtkMatrix4x4 
					 *vtkNotUsed(position_matrix) )
{
  return (float *)NULL;
}

// Print the object including the PolyData, the matrix source, and the
// build time.
void vtkProjectedPolyDataRayBounder::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkRayBounder::PrintSelf(os, indent);

  if ( this->PolyData )
    {
    os << indent << "PolyData: (" << this->PolyData << ")\n";
    }
  else
    {
    os << indent << "PolyData: (none)\n";
    }

  if ( this->ActorMatrixSource )
    {
    os << indent << "Matrix Source (from Actor): (" << 
      this->ActorMatrixSource << ")\n";
    }
  else if ( this->VolumeMatrixSource )
    {
    os << indent << "Matrix Source (from Volume): (" << 
      this->VolumeMatrixSource << ")\n";
    }

  os << indent << "Build Time: " <<this->BuildTime.GetMTime() << "\n";
}


unsigned long int vtkProjectedPolyDataRayBounder::GetMTime()
{
  unsigned long mTime=this-> vtkRayBounder::GetMTime();
  unsigned long time;

  if ( this->ActorMatrixSource != NULL )
    {
    time = this->ActorMatrixSource->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }
  return mTime;
}


