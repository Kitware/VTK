/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdList.cxx
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
#include "vtkIdList.h"
#include "vtkObjectFactory.h"

vtkIdList* vtkIdList::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkIdList");
  if(ret)
    {
    return (vtkIdList*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkIdList;
}

vtkIdList::vtkIdList()
{
  this->NumberOfIds = 0;
  this->Size = 0;
  this->Ids = NULL;
}

vtkIdList::~vtkIdList()
{
  if ( this->Ids != NULL )
    {
    delete [] this->Ids;
    }
}

void vtkIdList::Initialize()
{
  if ( this->Ids != NULL )
    {
    delete [] this->Ids;
    this->Ids = NULL;
    }
  this->NumberOfIds = 0;
  this->Size = 0;
}

int vtkIdList::Allocate(const int sz, const int vtkNotUsed(strategy))
{
  if ( sz > this->Size)
    {
    this->Initialize();
    this->Size = ( sz > 0 ? sz : 1);
    if ( (this->Ids = new int[this->Size]) == NULL )
      {
      return 0;
      }
    }
  this->NumberOfIds = 0;
  return 1;
}

void vtkIdList::SetNumberOfIds(const int number)
{
  this->Allocate(number,0);
  this->NumberOfIds = number;
}

void vtkIdList::InsertId(const int i, const int id)
{
  if ( i >= this->Size )
    {
    this->Resize(i+1);
    }
  this->Ids[i] = id;
  if ( i >= this->NumberOfIds )
    {
    this->NumberOfIds = i + 1;
    }
}

int vtkIdList::InsertUniqueId(const int id)
{
  for (int i=0; i < this->NumberOfIds; i++)
    {
    if ( id == this->Ids[i] )
      {
      return i;
      }
    }
  
  return this->InsertNextId(id);
}

int *vtkIdList::WritePointer(const int i, const int number)
{
  int newSize=i+number;
  if ( newSize > this->Size )
    {
    this->Resize(newSize);
    }
  if ( newSize > this->NumberOfIds )
    {
    this->NumberOfIds = newSize;
    }
  return this->Ids + i;
}

void vtkIdList::DeleteId(int id)
{
  int i=0;

  // while loop is necessary to delete all occurences of id
  while ( i < this->NumberOfIds )
    {
    for ( ; i < this->NumberOfIds; i++)
      {
      if ( this->Ids[i] == id )
        {
        break;
        }
      }

    // if found; replace current id with last
    if ( i < this->NumberOfIds )
      {
      this->SetId(i,this->Ids[this->NumberOfIds-1]);
      this->NumberOfIds--;
      }
    }
}

void vtkIdList::DeepCopy(vtkIdList *ids)
{
  this->Initialize();
  this->NumberOfIds = ids->NumberOfIds;
  this->Size = ids->Size;
  this->Ids = new int [ids->Size];
  for (int i=0; i < ids->NumberOfIds; i++)
    {
    this->Ids[i] = ids->Ids[i];
    }
}

int *vtkIdList::Resize(const int sz)
{
  int *newIds;
  int newSize;

  if ( sz > this->Size ) 
    {
    newSize = this->Size + sz;
    }
  else if (sz == this->Size)
    {
    return this->Ids;
    }
  else 
    {
    newSize = sz;
    }

  if (newSize <= 0)
    {
    this->Initialize();
    return 0;
    }

  if ( (newIds = new int[newSize]) == NULL )
    { 
    vtkErrorMacro(<< "Cannot allocate memory\n");
    return 0;
    }

  if (this->Ids)
    {
    memcpy(newIds, this->Ids,
           (sz < this->Size ? sz : this->Size) * sizeof(int));
    delete [] this->Ids;
    }

  this->Size = newSize;
  this->Ids = newIds;
  return this->Ids;
}

#define VTK_TMP_ARRAY_SIZE 500
// Intersect this list with another vtkIdList. Updates current list according
// to result of intersection operation.
void vtkIdList::IntersectWith(vtkIdList& otherIds)
{
  // Fast method due to Dr. Andreas Mueller of ISE Integrated Systems 
  // Engineering (CH).
  int thisNumIds = this->GetNumberOfIds();

  if (thisNumIds <= VTK_TMP_ARRAY_SIZE) 
    {//Use fast method if we can fit in temporary storage
    int  thisIds[VTK_TMP_ARRAY_SIZE];
    int  i, id;
    
    for (i=0; i < thisNumIds; i++)
      {
      thisIds[i] = this->GetId(i);
      }
    for (this->Reset(), i=0; i < thisNumIds; i++) 
      {
      id = thisIds[i];
      if ( otherIds.IsId(id) != (-1) )
        {
        this->InsertNextId(id);
        }
      }
    } 
  else 
    {//use slower method for extreme cases
    int  *thisIds = new int [thisNumIds];
    int  i, id;
    
    for (i=0; i < thisNumIds; i++)
      {
      *(thisIds + i) = this->GetId(i);
      }
    for (this->Reset(), i=0; i < thisNumIds; i++) 
      {
      id = *(thisIds + i);
      if ( otherIds.IsId(id) != (-1) )
        {
        this->InsertNextId(id);
        }
      }
    delete [] thisIds;
    }
}
#undef VTK_TMP_ARRAY_SIZE

void vtkIdList::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Number of Ids: " << this->NumberOfIds << "\n";
}
