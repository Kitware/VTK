/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractList.h
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
// .NAME vtkAbstractList - a dynamic list data structure

#include "vtkContainer.h"

template<class DType>
class vtkAbstractList : public vtkContainer
{
public:
  // Description:
  // This is a prototype for a compare function. It has to
  // return true if objects are the same and false if not.
  typedef int (*CompareFunction)(DType item1, DType item2);

  // Description:
  // Append an Item to the end of the list.
  // It returns VTK_OK if successfull.
  virtual int AppendItem(DType a) = 0;
  
  // Description:
  // Insert an Item to the front of the list.
  // It returns VTK_OK if successfull.
  virtual int PrependItem(DType a) = 0;
  
  // Description:
  // Insert an Item to the specific location in the list.
  // It returns VTK_OK if successfull.
  virtual int InsertItem(unsigned long loc, DType a) = 0;
  
  // Description:
  // Remove an Item from the list
  // It returns VTK_OK if successfull.
  virtual int RemoveItem(unsigned long id) = 0;
  
  // Description:
  // Return an item that was previously added to this list. 
  // It returns VTK_OK if successfull.
  virtual int GetItem(unsigned long id, DType& ret) = 0;
      
  // Description:
  // Find an item in the list. Return one if it was found, zero if it was
  // not found. The location of the item is returned in res.
  // It returns VTK_OK if successfull.
  virtual int Find(DType a, unsigned long &res) = 0;

  // Description:
  // Find an item in the list using a comparison routine. 
  // Return one if it was found, zero if it was
  // not found. The location of the item is returned in res.
  // It returns VTK_OK if successfull.
  virtual int Find(DType a, CompareFunction compare, unsigned long &res) = 0;

  // Description:
  // Set the capacity of the list.
  // It returns VTK_OK if successfull.
  virtual int SetSize(unsigned long size) = 0;
  
protected:
  vtkAbstractList() {}
  ~vtkAbstractList() {}
};
