/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProjectedPolyDataRayBounder.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Lisa Sobierajski Avila who developed this class.

Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkProjectedPolyDataRayBounder.h"
#include "vtkRenderWindow.h"

#ifdef USE_OGLR
#include "vtkOpenGLProjectedPolyDataRayBounder.h"
#endif

// Description:
// The constructor for the class. Initialize everything to NULL.
vtkProjectedPolyDataRayBounder::vtkProjectedPolyDataRayBounder()
{
  this->ActorMatrixSource  = NULL;
  this->VolumeMatrixSource = NULL;
  this->PolyData           = NULL;
}

// Description:
// Destructor for the class. Nothing needs to be done.
vtkProjectedPolyDataRayBounder::~vtkProjectedPolyDataRayBounder()
{
}
// Description:
// New method for the class which will return the correct type of 
// ProjectPolyDataRayBounder
vtkProjectedPolyDataRayBounder *vtkProjectedPolyDataRayBounder::New()
{
  char *temp = vtkRenderWindow::GetRenderLibrary();
  
#ifdef USE_OGLR
  if (!strncmp("oglr",temp,4)) 
    return vtkOpenGLProjectedPolyDataRayBounder::New();
#endif

  vtkGenericWarningMacro( << 
    "Sorry, vtkProjectedPolyDataRayBounder is not supported for: " <<
    temp );

  return new vtkProjectedPolyDataRayBounder;
}

// Description:
// Set the matrix source to be an actor. The PolyData will be transformed
// by this actor's matrix before being projected
void vtkProjectedPolyDataRayBounder::SetMatrixSource( vtkActor *actor )
{
  this->ActorMatrixSource  = actor;
  this->VolumeMatrixSource = NULL;
}

// Description:
// Set the matrix source to be a volume. The PolyData will be transformed
// by this volume's matrix before being projected
void vtkProjectedPolyDataRayBounder::SetMatrixSource( vtkVolume *volume )
{
  this->ActorMatrixSource  = NULL;
  this->VolumeMatrixSource = volume;
}

// Description:
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
    this->ActorMatrixSource->GetMatrix( *matrix );
  else if ( this->VolumeMatrixSource )
    this->VolumeMatrixSource->GetMatrix( *matrix );

  // Call Draw() to obtain the bounds. 
  return_bounds = this->Draw( ren, matrix );

  // Delete the temporary matrix that we previously created
  delete matrix;

  return (return_bounds);
}

// Description:
// This is a bogus method that will only be called if a
// vtkProjectedPolyDataRayBounder was created with a render type other
// than OpenGL. 
void vtkProjectedPolyDataRayBounder::Build( vtkPolyData *vtkNotUsed(pdata) )
{
}

// Description:
// This is a bogus method that will only be called if a
// vtkProjectedPolyDataRayBounder was created with a render type other
// than OpenGL. 
float *vtkProjectedPolyDataRayBounder::Draw( vtkRenderer *vtkNotUsed(ren), 
					 vtkMatrix4x4 
					 *vtkNotUsed(position_matrix) )
{
  return (float *)NULL;
}

// Description:
// Print the object including the PolyData, the matrix source, and the
// build time.
void vtkProjectedPolyDataRayBounder::PrintSelf(ostream& os, vtkIndent indent)
{
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

  vtkObject::PrintSelf(os, indent);
}
