/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSource.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkImageSource.h"
#include "vtkImageToStructuredPoints.h"

//----------------------------------------------------------------------------
vtkImageSource::vtkImageSource()
{
  this->ImageToStructuredPoints = NULL;
}


//----------------------------------------------------------------------------
vtkImageSource::~vtkImageSource()
{
  if (this->ImageToStructuredPoints)
    {
    this->ImageToStructuredPoints->Delete();
    }
}


//----------------------------------------------------------------------------
void vtkImageSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);
}
  

//----------------------------------------------------------------------------
// Description:
// This method returns an object which will generate regions.
// For non cached sources, it returns the source itself.
// The convention for connection elements in an image pipeline is
// "consumer->SetInput(source->GetOutput)".  It is primarily
// designed to allows sources with multple outputs.
vtkImageSource *vtkImageSource::GetOutput()
{
  return this;
}


//----------------------------------------------------------------------------
// Description:
// This method returns the maximum MTime of this source and all the objects
// that come before this source (that can change this sources output).
// Elegagent MTime propagation is very difficult, and is still unresolved.
// The current impelentation works, but other mechanisms are being considered.
// See the vtkImageFilter class for more information.
unsigned long vtkImageSource::GetPipelineMTime()
{
  return this->GetMTime();
}


//----------------------------------------------------------------------------
// Description:
// This method updates the whole region and returns it.
vtkImageRegion *vtkImageSource::Update()
{
  vtkImageRegion *region;
  
  region = new vtkImageRegion;
  this->UpdateImageInformation(region);
  region->SetExtent(region->GetImageExtent());
  this->UpdateRegion(region);
  
  return region;
}




//----------------------------------------------------------------------------
vtkImageToStructuredPoints *vtkImageSource::GetImageToStructuredPoints()
{
  if ( ! this->ImageToStructuredPoints)
    {
    this->ImageToStructuredPoints = new vtkImageToStructuredPoints;
    this->ImageToStructuredPoints->SetScalarInput(this);
    }
  
  return this->ImageToStructuredPoints;
}






