/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataInformation.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkDataInformation.h"

//----------------------------------------------------------------------------
// Construct a new vtkDataInformation 
vtkDataInformation::vtkDataInformation()
{
  this->EstimatedWholeMemorySize = 0;
  this->PipelineMTime = 0;
  this->Locality = 0.0;
  this->SeriesLength = 1;
}


//----------------------------------------------------------------------------
void vtkDataInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  //vtkObject::PrintSelf(os, indent);
  os << indent << "EstimatedWholeMemorySize: " 
     << this->EstimatedWholeMemorySize << endl;
  os << indent << "PipelineMTime: " << this->PipelineMTime << endl;
  os << indent << "Locality: " << this->Locality << endl; 
  os << indent << "SeriesLength: " << this->SeriesLength << endl; 
}




//----------------------------------------------------------------------------
int vtkDataInformation::GetClassCheck(char *className)
{
  if (strcmp(className, "vtkDataInformation") == 0)
    {
    return 1;
    }
  
  return 0;
}


  

//----------------------------------------------------------------------------
void vtkDataInformation::Copy(vtkDataInformation *info)
{
  this->SetEstimatedWholeMemorySize(info->GetEstimatedWholeMemorySize());

  // Should I copy PipelineMTime ?
  // since this is done befor ExecuteInformation and PipelineMTime has
  // already been computed, no.
  // this->SetPipelineMTime(info->GetPipelineMTime());

  // Should we compute the next locality here?
  this->SetLocality(this->GetLocality());

  this->SetSeriesLength(this->GetSeriesLength());
}


//----------------------------------------------------------------------------
void vtkDataInformation::WriteSelf(ostream& os)
{
  os << this->EstimatedWholeMemorySize << " ";
  os << this->PipelineMTime << " ";
  os << this->Locality << " ";
  os << this->SeriesLength << " ";
}

//----------------------------------------------------------------------------
void vtkDataInformation::ReadSelf(istream& is)
{
  is >> this->EstimatedWholeMemorySize ;
  is >> this->PipelineMTime ;
  is >> this->Locality ;
  is >> this->SeriesLength ;
}





