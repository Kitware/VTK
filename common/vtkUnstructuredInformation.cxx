/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredInformation.cxx
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
#include "vtkUnstructuredInformation.h"

//----------------------------------------------------------------------------
// Construct a new vtkUnstructuredInformation 
vtkUnstructuredInformation::vtkUnstructuredInformation()
{
  this->MaximumNumberOfPieces = 1;
}


//----------------------------------------------------------------------------
void vtkUnstructuredInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataInformation::PrintSelf(os, indent);

  os << indent << "MaximumNumberOfPieces: " 
     << this->MaximumNumberOfPieces << endl;
}


//----------------------------------------------------------------------------
int vtkUnstructuredInformation::GetClassCheck(char *className)
{
  if (strcmp(className, "vtkUnstructuredInformation") == 0)
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
void vtkUnstructuredInformation::Copy(vtkDataInformation *in)
{
  this->vtkDataInformation::Copy(in);
  
  if (in->GetClassCheck("vtkUnstructuredInformation"))
    {
    vtkUnstructuredInformation *info = (vtkUnstructuredInformation*)(in);
    this->SetMaximumNumberOfPieces(info->GetMaximumNumberOfPieces());
    }
}




  



