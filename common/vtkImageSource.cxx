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
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkImageSource* vtkImageSource::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageSource");
  if(ret)
    {
    return (vtkImageSource*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageSource;
}




//----------------------------------------------------------------------------
vtkImageSource::vtkImageSource()
{
  this->vtkSource::SetNthOutput(0,vtkImageData::New());
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  this->Outputs[0]->ReleaseData();
  this->Outputs[0]->Delete();
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkImageSource::SetOutput(vtkImageData *output)
{
  this->vtkSource::SetNthOutput(0, output);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkImageData *vtkImageSource::GetOutput()
{
  if (this->NumberOfOutputs < 1)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Outputs[0]);
}


//----------------------------------------------------------------------------
void vtkImageSource::PropagateUpdateExtent(vtkDataObject *out)
{
  vtkImageData *output = (vtkImageData*)out;
  
  // ----------------------------------------------
  // For legacy compatability
  this->LegacyHack = 1;
  this->InterceptCacheUpdate();
  if (this->LegacyHack)
    {
    vtkErrorMacro( << "Change your method InterceptCacheUpdate " 
                   << "to the name EnlargeOutputUpdateExtents.");
    return;
    }

  this->vtkSource::PropagateUpdateExtent(output);
}


//----------------------------------------------------------------------------
// Convert to Imaging API
void vtkImageSource::Execute()
{
  vtkImageData *output = this->GetOutput();

  // If we have multiple Outputs, they need to be allocate
  // in a subclass.  We cannot be sure all outputs are images.
  output->SetExtent(output->GetUpdateExtent());
  output->AllocateScalars();

  this->Execute(this->GetOutput());
}

//----------------------------------------------------------------------------
// This function can be defined in a subclass to generate the data
// for a region.
void vtkImageSource::Execute(vtkImageData *)
{
  vtkErrorMacro(<< "Execute(): Method not defined.");
}

