/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredExtent.cxx
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
#include "vtkUnstructuredExtent.h"
#include "vtkUnstructuredExtent.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkUnstructuredExtent* vtkUnstructuredExtent::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkUnstructuredExtent");
  if(ret)
    {
    return (vtkUnstructuredExtent*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkUnstructuredExtent;
}



// Construct a new vtkUnstructuredExtent 
vtkUnstructuredExtent::vtkUnstructuredExtent()
{
  this->Extent[0] = 0;
  this->Extent[1] = 1;
}


void vtkUnstructuredExtent::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkExtent::PrintSelf(os, indent);
  os << indent << "Piece: " << this->Extent[0] << endl;
  os << indent << "NumberOfPieces: " << this->Extent[1] << endl;
}


//----------------------------------------------------------------------------
void vtkUnstructuredExtent::Copy(vtkExtent *in)
{
  // call the supperclasses copy
  this->vtkExtent::Copy(in);
  
  // WARNING!!!
  // This logic only works because of the simple class hierachy.
  // If you subclass off this extent, ClassName cannot be used.
  if (strcmp(in->GetClassName(), "vtkUnstructuredExtent") == 0)
    {
    int idx;
    vtkUnstructuredExtent *e = (vtkUnstructuredExtent*)(in);
  
    for (idx = 0; idx < 2; ++idx)
      {
      this->Extent[idx] = e->Extent[idx];
      }
    } 
}

//----------------------------------------------------------------------------
void vtkUnstructuredExtent::WriteSelf(ostream& os)
{
  this->vtkExtent::WriteSelf(os);

  os << this->Extent[0] << " " << this->Extent[1] << " " ;
}

//----------------------------------------------------------------------------
void vtkUnstructuredExtent::ReadSelf(istream& is)
{
  this->vtkExtent::ReadSelf(is);

  is >> this->Extent[0] >> this->Extent[1] ;
}



