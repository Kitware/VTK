/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestList.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen  
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkVector.txx"
#include "vtkLinkedList.txx"

int CheckName(const char *name, const char **names)
{
  int cc;
  if ( !name )
    {
    cout << "Trying to compare with empty name" << endl;
    return VTK_ERROR;
    }
  for ( cc = 0; names[cc]; cc++ )
    {
    if ( !strncmp(names[cc], name, strlen(names[cc])) )
      {
      return VTK_OK;
      }
    }
  return VTK_ERROR;
}

#define C_ERROR(c) cout << "Container: " << c->GetClassName() << " "

int TestVectorList()
{
  vtkIdType cc;
  int error = 0;
  const char* names[] = {
    "Amy",
    "Andy",
    "Berk",
    "Bill",
    "Brad",
    "Charles",
    "Ken",
    "Lisa",
    "Sebastien",
    "Will",
    0
  };

  const char separate[] = "separate";


  vtkVector<const char*> *strings = vtkVector<const char*>::New();

  for ( cc =0 ; cc< 10; cc++ )
    {
    if ( strings->AppendItem( names[cc] ) != VTK_OK )
      {
      C_ERROR(strings) << "Append failed" << endl;
      error = 1;
      }
    }
  //strings->DebugList();

  for ( cc=0; cc < 13; cc++ )
    {
    const char* name = 0;
    if ( cc < 10 )
      {
      if ( strings->GetItem(cc, name) != VTK_OK )
        {
        C_ERROR(strings) << "Problem accessing item: " << cc << endl;
        error = 1;
        }
      if ( !name )
        {
        C_ERROR(strings) << "Name is null" << endl;
        error = 1;
        }
      if ( name && strcmp(name, names[cc]) )
        {
        C_ERROR(strings) << "Got name but it is not what it should be" 
                         << endl;
        error = 1;
        }
      }
    else
      {
      if ( strings->GetItem(cc, name) == VTK_OK )
        {
        C_ERROR(strings) << "Should not be able to access item: " 
                         << cc << endl;
        C_ERROR(strings) << "Item: " << name << endl;
        error = 1;
        }
      }
    }
  //strings->DebugList();

  for ( cc=1; cc<10; cc+= 2 )
    {
    if ( cc < strings->GetNumberOfItems() )
      {
      if ( strings->RemoveItem(cc) != VTK_OK )
        {
        C_ERROR(strings) << "Problem removing item: " << cc << endl;
        C_ERROR(strings) << "Number of elements: " 
                         << strings->GetNumberOfItems() 
                         << endl;
        error = 1;
        }
      }
    else
      {
      if ( strings->RemoveItem(cc) == VTK_OK )
        {
        C_ERROR(strings) << "Should not be able to remove item: " 
                         << cc << endl;
        C_ERROR(strings) << "Number of elements: " 
                         << strings->GetNumberOfItems() 
             << endl;
        error = 1;
        }
      }
    
    }

  for ( cc=0; cc < 11; cc++ )
    {
    const char *name = 0;
    if ( cc < 7 )
      {
      if ( strings->GetItem(cc, name) != VTK_OK )
        {
        C_ERROR(strings) << "Problem accessing item: " << cc << endl;
        error = 1;
        }
      if ( !name )
        {
        C_ERROR(strings) << "Name is null" << endl;
        error = 1;
        }
      if ( !name || ::CheckName(name, names) != VTK_OK )
        {
        C_ERROR(strings) << "Got strange name at position: " 
                         << cc << endl;
        error = 1;
        }
      }
    else
      {
      if ( strings->GetItem(cc, name) == VTK_OK )
        {
        C_ERROR(strings) << "Should not be able to access item: " 
                         << cc << endl;
        C_ERROR(strings) << "Item: " << name << endl;
        error = 1;
        }
      }
    }
  
  if ( strings->GetNumberOfItems() != 7 )
    {
    C_ERROR(strings) << "Number of elements left: " 
                     << strings->GetNumberOfItems() << endl;
    error = 1;
    }

  for ( cc =0 ; cc< 100; cc++ )
    {
    if ( strings->PrependItem( separate ) != VTK_OK )
      {
      C_ERROR(strings) << "Problem prepending item: " << cc << endl;
      error = 1;
      }
    }

  for ( cc=0; cc < strings->GetNumberOfItems(); cc++ )
    {
    const char *name = 0;
    if ( strings->GetItem(cc, name) != VTK_OK )
      {
      C_ERROR(strings) << "Problem accessing item: " << cc << endl;
      error = 1;
      }
    if ( !name )
      {
      C_ERROR(strings) << "Name is null" << endl;
      error = 1;
      }
    if ( !name && ::CheckName(name, names) != VTK_OK )
      {
      C_ERROR(strings) << "Got strange name at position: " << cc << endl;
      error = 1;
      }
    }


  // Try the iterator
  vtkVector<const char*>::IteratorType *it = strings->NewIterator();
  //cout << "Try iterator" << endl;
  it->GoToFirstItem();
  while ( it->IsDoneWithTraversal() != VTK_OK )
    {
    const char* str = 0;
    vtkIdType idx = 0;
    if ( it->GetData(str) != VTK_OK )
      {
      C_ERROR(strings) << "Problem accessing data from iterator" << endl;
      error =1;
      }
    if ( it->GetKey(idx) != VTK_OK )
      {
      C_ERROR(strings) << "Problem accessing data from iterator" << endl;
      error =1;     
      }
    //cout << "Item: " << idx << " = " << str << endl;
    it->GoToNextItem();
    }
  it->GoToLastItem();
  while( it->IsDoneWithTraversal() != VTK_OK )
    {
    const char* str = 0;
    vtkIdType idx = 0;
    if ( it->GetData(str) != VTK_OK )
      {
      C_ERROR(strings) << "Problem accessing data from iterator" << endl;
      error =1;
      }
    if ( it->GetKey(idx) != VTK_OK )
      {
      C_ERROR(strings) << "Problem accessing data from iterator" << endl;
      error =1;     
      }
    //cout << "Item: " << idx << " = " << str << endl;    
    it->GoToPreviousItem();
    }
  it->GoToFirstItem();
  it->Delete();

  for ( ; strings->GetNumberOfItems(); )
    {
    if ( strings->RemoveItem(0) != VTK_OK )
      {
      C_ERROR(strings) << "Problem remove first element" << endl;
      error = 1;
      }
    }

  if ( strings->GetNumberOfItems() != 0 )
    {
    C_ERROR(strings) << "Number of elements left: " 
                     << strings->GetNumberOfItems() << endl;
    error = 1;
    }

  strings->Delete();

  strings = vtkVector<const char*>::New();
  vtkIdType maxsize = 0;
  if ( strings->SetSize(15) == VTK_OK )
    {
    maxsize = 15;
    }
  for ( cc = 0; cc < 20; cc ++ )
    {
    if ( !maxsize || cc < maxsize )
      {
      if ( strings->InsertItem( (cc)?(cc-1):0, separate ) != VTK_OK )
        {
        C_ERROR(strings) << "Problem inserting item: " << cc << endl;
        C_ERROR(strings) << "Size: " << strings->GetNumberOfItems() << endl;
        error = 1;
        }
      }
    else
      {
      if ( strings->InsertItem( (cc)?(cc-1):0, separate ) == VTK_OK )
        {
        C_ERROR(strings) << "Should not be able to insert item: " 
                         << cc << endl;
        C_ERROR(strings) << "Size: " << strings->GetNumberOfItems() << endl;
        error = 1;
        }
      }
    }
  
  strings->Delete();

  return error;
}

int TestLinkedList()
{
  vtkIdType cc;
  int error = 0;
  const char* names[] = {
    "Amy",
    "Andy",
    "Berk",
    "Bill",
    "Brad",
    "Charles",
    "Ken",
    "Lisa",
    "Sebastien",
    "Will",
    0
  };

  const char separate[] = "separate";


  vtkLinkedList<const char*> *strings = vtkLinkedList<const char*>::New();

  for ( cc =0 ; cc< 10; cc++ )
    {
    if ( strings->AppendItem( names[cc] ) != VTK_OK )
      {
      C_ERROR(strings) << "Append failed" << endl;
      error = 1;
      }
    }
  //strings->DebugList();

  for ( cc=0; cc < 13; cc++ )
    {
    const char* name = 0;
    if ( cc < 10 )
      {
      if ( strings->GetItem(cc, name) != VTK_OK )
        {
        C_ERROR(strings) << "Problem accessing item: " << cc << endl;
        error = 1;
        }
      if ( !name )
        {
        C_ERROR(strings) << "Name is null" << endl;
        error = 1;
        }
      if ( name && strcmp(name, names[cc]) )
        {
        C_ERROR(strings) << "Got name but it is not what it should be" 
                         << endl;
        error = 1;
        }
      }
    else
      {
      if ( strings->GetItem(cc, name) == VTK_OK )
        {
        C_ERROR(strings) << "Should not be able to access item: " 
                         << cc << endl;
        C_ERROR(strings) << "Item: " << name << endl;
        error = 1;
        }
      }
    }
  //strings->DebugList();

  for ( cc=1; cc<10; cc+= 2 )
    {
    if ( cc < strings->GetNumberOfItems() )
      {
      if ( strings->RemoveItem(cc) != VTK_OK )
        {
        C_ERROR(strings) << "Problem removing item: " << cc << endl;
        C_ERROR(strings) << "Number of elements: " 
                         << strings->GetNumberOfItems() 
                         << endl;
        error = 1;
        }
      }
    else
      {
      if ( strings->RemoveItem(cc) == VTK_OK )
        {
        C_ERROR(strings) << "Should not be able to remove item: " 
                         << cc << endl;
        C_ERROR(strings) << "Number of elements: " 
                         << strings->GetNumberOfItems() 
             << endl;
        error = 1;
        }
      }
    
    }

  for ( cc=0; cc < 11; cc++ )
    {
    const char *name = 0;
    if ( cc < 7 )
      {
      if ( strings->GetItem(cc, name) != VTK_OK )
        {
        C_ERROR(strings) << "Problem accessing item: " << cc << endl;
        error = 1;
        }
      if ( !name )
        {
        C_ERROR(strings) << "Name is null" << endl;
        error = 1;
        }
      if ( !name || ::CheckName(name, names) != VTK_OK )
        {
        C_ERROR(strings) << "Got strange name at position: " 
                         << cc << endl;
        error = 1;
        }
      }
    else
      {
      if ( strings->GetItem(cc, name) == VTK_OK )
        {
        C_ERROR(strings) << "Should not be able to access item: " 
                         << cc << endl;
        C_ERROR(strings) << "Item: " << name << endl;
        error = 1;
        }
      }
    }
  
  if ( strings->GetNumberOfItems() != 7 )
    {
    C_ERROR(strings) << "Number of elements left: " 
                     << strings->GetNumberOfItems() << endl;
    error = 1;
    }

  for ( cc =0 ; cc< 100; cc++ )
    {
    if ( strings->PrependItem( separate ) != VTK_OK )
      {
      C_ERROR(strings) << "Problem prepending item: " << cc << endl;
      error = 1;
      }
    }

  for ( cc=0; cc < strings->GetNumberOfItems(); cc++ )
    {
    const char *name = 0;
    if ( strings->GetItem(cc, name) != VTK_OK )
      {
      C_ERROR(strings) << "Problem accessing item: " << cc << endl;
      error = 1;
      }
    if ( !name )
      {
      C_ERROR(strings) << "Name is null" << endl;
      error = 1;
      }
    if ( !name && ::CheckName(name, names) != VTK_OK )
      {
      C_ERROR(strings) << "Got strange name at position: " << cc << endl;
      error = 1;
      }
    }


  // Try the iterator
  vtkLinkedList<const char*>::IteratorType *it = strings->NewIterator();
  //cout << "Try iterator" << endl;
  it->GoToFirstItem();
  while ( it->IsDoneWithTraversal() != VTK_OK )
    {
    const char* str = 0;
    vtkIdType idx = 0;
    if ( it->GetData(str) != VTK_OK )
      {
      C_ERROR(strings) << "Problem accessing data from iterator" << endl;
      error =1;
      }
    if ( it->GetKey(idx) != VTK_OK )
      {
      C_ERROR(strings) << "Problem accessing data from iterator" << endl;
      error =1;     
      }
    //cout << "Item: " << idx << " = " << str << endl;
    it->GoToNextItem();
    }
  it->GoToLastItem();
  while( it->IsDoneWithTraversal() != VTK_OK )
    {
    const char* str = 0;
    vtkIdType idx = 0;
    if ( it->GetData(str) != VTK_OK )
      {
      C_ERROR(strings) << "Problem accessing data from iterator" << endl;
      error =1;
      }
    if ( it->GetKey(idx) != VTK_OK )
      {
      C_ERROR(strings) << "Problem accessing data from iterator" << endl;
      error =1;     
      }
    //cout << "Item: " << idx << " = " << str << endl;    
    it->GoToPreviousItem();
    }
  it->GoToFirstItem();
  it->Delete();

  for ( ; strings->GetNumberOfItems(); )
    {
    if ( strings->RemoveItem(0) != VTK_OK )
      {
      C_ERROR(strings) << "Problem remove first element" << endl;
      error = 1;
      }
    }

  if ( strings->GetNumberOfItems() != 0 )
    {
    C_ERROR(strings) << "Number of elements left: " 
                     << strings->GetNumberOfItems() << endl;
    error = 1;
    }

  strings->Delete();

  strings = vtkLinkedList<const char*>::New();
  vtkIdType maxsize = 0;
  if ( strings->SetSize(15) == VTK_OK )
    {
    maxsize = 15;
    }
  for ( cc = 0; cc < 20; cc ++ )
    {
    if ( !maxsize || cc < maxsize )
      {
      if ( strings->InsertItem( (cc)?(cc-1):0, separate ) != VTK_OK )
        {
        C_ERROR(strings) << "Problem inserting item: " << cc << endl;
        C_ERROR(strings) << "Size: " << strings->GetNumberOfItems() << endl;
        error = 1;
        }
      }
    else
      {
      if ( strings->InsertItem( (cc)?(cc-1):0, separate ) == VTK_OK )
        {
        C_ERROR(strings) << "Should not be able to insert item: " 
                         << cc << endl;
        C_ERROR(strings) << "Size: " << strings->GetNumberOfItems() << endl;
        error = 1;
        }
      }
    }
  
  strings->Delete();

  return error;
}

int main()
{
  int res = 0;

  //cout << "Vector: " << endl;
  res += TestVectorList();

  //cout << "Linked List: " << endl;
  res += TestLinkedList();

  return res;
}
