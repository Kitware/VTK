/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredInformation.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkStructuredInformation.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkStructuredInformation* vtkStructuredInformation::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkStructuredInformation");
  if(ret)
    {
    return (vtkStructuredInformation*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkStructuredInformation;
}




//----------------------------------------------------------------------------
// Construct a new vtkStructuredInformation 
vtkStructuredInformation::vtkStructuredInformation()
{
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = 0;
}


//----------------------------------------------------------------------------
void vtkStructuredInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataInformation::PrintSelf(os, indent);
  int idx;
  
  os << indent << "WholeExtent: (" << this->WholeExtent[0];
  for (idx = 1; idx < 6; ++idx)
    {
    os << ", " << this->WholeExtent[idx];
    }
  os << ")\n";
}

//----------------------------------------------------------------------------
void vtkStructuredInformation::SetWholeExtent(int *ext)
{
  int idx, modified = 0;
  
  for (idx = 0; idx < 6; ++idx)
    {
    if (this->WholeExtent[idx] != ext[idx])
      {
      modified = 1;
      }
    this->WholeExtent[idx] = ext[idx];
    }
  
  if (modified)
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkStructuredInformation::GetWholeExtent(int *ext)
{
  int idx;
  
  for (idx = 0; idx < 6; ++idx)
    {
    ext[idx] = this->WholeExtent[idx];
    }
}



//----------------------------------------------------------------------------
int vtkStructuredInformation::GetClassCheck(char *className)
{
  if (strcmp(className, "vtkStructuredInformation") == 0)
    {
    return 1;
    }
  // check superclass
  if (this->vtkDataInformation::GetClassCheck(className))
    {
    return 1;
    }
  
  return 0;
}


//----------------------------------------------------------------------------
void vtkStructuredInformation::Copy(vtkDataInformation *in)
{
  this->vtkDataInformation::Copy(in);
  
  if (in->GetClassCheck("vtkStructuredInformation"))
    {
    vtkStructuredInformation *info = (vtkStructuredInformation*)(in);
    this->SetWholeExtent(info->GetWholeExtent());
    }
}

//----------------------------------------------------------------------------
void vtkStructuredInformation::WriteSelf(ostream& os)
{
  this->vtkDataInformation::WriteSelf(os);

  os << this->WholeExtent[0] << " " << this->WholeExtent[1] << " "
     << this->WholeExtent[2] << " " << this->WholeExtent[3] << " "
     << this->WholeExtent[4] << " " << this->WholeExtent[5] << " " ;
}

//----------------------------------------------------------------------------
void vtkStructuredInformation::ReadSelf(istream& is)
{
  this->vtkDataInformation::ReadSelf(is);

  is >> this->WholeExtent[0] >> this->WholeExtent[1]
     >> this->WholeExtent[2] >> this->WholeExtent[3]
     >> this->WholeExtent[4] >> this->WholeExtent[5] ;

}




  



