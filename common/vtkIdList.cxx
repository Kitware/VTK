/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdList.cxx
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
#include "vtkIdList.h"
#include "vtkCell.h"


vtkIdList::vtkIdList(const int sz=512, const int ext=1000)
{
  this->Ia = new vtkIntArray(sz,ext);
}

vtkIdList::~vtkIdList()
{
  this->Ia->Delete();
}


// Description:
// Delete specified id from list. Will replace all occurences of id in list.
void vtkIdList::DeleteId(int Id)
{
  int i=0, numIds=this->GetNumberOfIds();

  // while loop is necessary to delete all occurences of Id
  while ( i < numIds )
    {
    // first find id
    for ( ; i < numIds; i++)
      if ( this->GetId(i) == Id ) 
        break;

    // if found; replace current id with last
    if ( i < numIds )
      {
      this->SetId(i,this->Ia->GetValue(this->Ia->GetMaxId()));
      this->Ia->SetNumberOfValues(--numIds);
      }
    }
}

// Description:
// Intersect this list with another vtkIdList. Updates current list according
// to result of intersection operation.
void vtkIdList::IntersectWith(vtkIdList& otherIds)
{
  int id, i, j;
  int numOriginalIds=this->GetNumberOfIds();

  for ( i=0; i < numOriginalIds; i++ )
    {
    for ( j=0; j < this->GetNumberOfIds(); j++)
      {
      id =  this->GetId(j);
      if ( ! otherIds.IsId(id) ) this->DeleteId(id);
      }
    }
}
