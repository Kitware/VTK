/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTCoords.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
#include "vtkTCoords.hh"
#include "vtkIdList.hh"
#include "vtkFloatTCoords.hh"

void vtkTCoords::GetTCoord(int id, float tc[3])
{
  float *tcp = this->GetTCoord(id);
  for (int i=0; i<this->Dimension; i++) tc[i] = tcp[i];
}

// Description:
// Insert texture coordinate into position indicated. Although up to three
// texture components may be specified (i.e., tc1, tc2, tc3), if the texture
// coordinates are less than 3 dimensions the extra components will be ignored.
void vtkTCoords::InsertTCoord(int id, float tc1, float tc2, float tc3)
{
  float tc[3];

  tc[0] = tc1;
  tc[1] = tc2;
  tc[2] = tc3;
  this->InsertTCoord(id,tc);
}

// Description:
// Insert texture coordinate into position indicated. Although up to three
// texture components may be specified (i.e., tc1, tc2, tc3), if the texture
// coordinates are less than 3 dimensions, the extra components will be 
// ignored.
int vtkTCoords::InsertNextTCoord(float tc1, float tc2, float tc3)
{
  float tc[3];

  tc[0] = tc1;
  tc[1] = tc2;
  tc[2] = tc3;
  return this->InsertNextTCoord(tc);
}

// Description:
// Construct object whose texture coordinates are of specified dimension.
vtkTCoords::vtkTCoords(int dim)
{
  this->Dimension = dim;
}

// Description:
// Given a list of pt ids, return an array of texture coordinates.
void vtkTCoords::GetTCoords(vtkIdList& ptId, vtkFloatTCoords& ftc)
{
  for (int i=0; i<ptId.GetNumberOfIds(); i++)
    {
    ftc.InsertTCoord(i,this->GetTCoord(ptId.GetId(i)));
    }
}

void vtkTCoords::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkRefCount::PrintSelf(os,indent);

  os << indent << "Number Of Texture Coordinates: " << this->GetNumberOfTCoords() << "\n";
  os << indent << "Texture Dimension: " << this->Dimension << "\n";
}
