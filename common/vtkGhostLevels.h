/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGhostLevels.h
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
// .NAME vtkGhostLevels - represent and manipulate ghost levels
// .SECTION Description
// vtkGhostLevels is used to keep track of whether a particular point in one
// piece is replicated in another piece.  One piece will "own" the point, and
// it's ghost level for that point will be 0.  For any other pieces containing
// this point, the ghost level will be non-zero.

#ifndef __vtkGhostLevels_h
#define __vtkGhostLevels_h

#include "vtkAttributeData.h"
#include "vtkUnsignedCharArray.h"

class vtkIdList;
class vtkGhostLevels;

class VTK_EXPORT vtkGhostLevels : public vtkAttributeData
{
public:
  static vtkGhostLevels *New();

  vtkTypeMacro(vtkGhostLevels, vtkAttributeData);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a copy of this object.
  vtkAttributeData *MakeObject(){return vtkGhostLevels::New();};
  
  // Description:
  // Return an unsigned char ghost level for a specific id.
  unsigned char GetGhostLevel(int id);
  
  // Description:
  // Insert ghost level into object. No range checking performed (fast!).
  void SetGhostLevel(int id, unsigned char lev);

  // Description:
  // Insert ghost level into object. Range checking performed and memory
  // allocated as necessary.
  void InsertGhostLevel(int id, unsigned char lev);

  // Description:
  // Insert ghost level into next available slot. Returns id of slot.
  int InsertNextGhostLevel(unsigned char lev);

  // Description:
  // Given a list of pt ids, return an array of ghost levels.
  void GetGhostLevels(vtkIdList *ptId, vtkGhostLevels *fn);

  // Description:
  // Specify the number of ghost levels for this object to hold.
  void SetNumberOfGhostLevels(int number) 
    {this->Data->SetNumberOfTuples(number);};

  // Description:
  // Return number of ghost levels in the array.
  int GetNumberOfGhostLevels() {return this->Data->GetNumberOfTuples();};
protected:
  vtkGhostLevels();
  ~vtkGhostLevels() {};
  vtkGhostLevels(const vtkGhostLevels&) {};
  void operator=(const vtkGhostLevels&) {};
};

inline unsigned char vtkGhostLevels::GetGhostLevel(int id)
{
  vtkUnsignedCharArray* array = (vtkUnsignedCharArray *)this->GetData();
  return array->GetValue(id);
}

inline void vtkGhostLevels::SetGhostLevel(int id, unsigned char lev)
{
  vtkUnsignedCharArray *array = (vtkUnsignedCharArray *)this->GetData();
  array->SetValue(id, lev);
}

inline void vtkGhostLevels::InsertGhostLevel(int id, unsigned char lev)
{
  vtkUnsignedCharArray *array = (vtkUnsignedCharArray *)this->GetData();
  array->InsertValue(id, lev);
}

inline int vtkGhostLevels::InsertNextGhostLevel(unsigned char lev)
{
  vtkUnsignedCharArray *array = (vtkUnsignedCharArray *)this->GetData();
  return array->InsertNextValue(lev);
}

// These include files are placed here so that if Normals.h is included 
// all other classes necessary for compilation are also included. 
#include "vtkIdList.h"

#endif
