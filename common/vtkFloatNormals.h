/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFloatNormals.h
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
// .NAME vtkFloatNormals - (obsolete)floating point representation of 3D normals
// .SECTION Description
// vtkFloatNormals is a concrete implementation of vtkNormals. Normals are
// represented using float values.

#ifndef __vtkFloatNormals_h
#define __vtkFloatNormals_h

#include "vtkNormals.h"
#include "vtkFloatArray.h"

#ifndef VTK_REMOVE_LEGACY_CODE
class VTK_EXPORT vtkFloatNormals : public vtkNormals
{
public:
  static vtkFloatNormals *New();
  
  // Description:
  // Set the data type for this object.
  void SetDataType(int dataType);

  // Description:
  // Set the data for this object. Only accepts VTK_FLOAT type.
  void SetData(vtkDataArray *);

  // Description:
  // Get pointer to array of data starting at data position "id".
  float *GetPointer(const int id);

  // Description:
  // Get pointer to data array. Useful for direct writes of data. MaxId is
  // bumped by number (and memory allocated if necessary). Id is the
  // location you wish to write into; number is the number of normals to
  // write.
  float *WritePointer(const int id, const int number);
  
protected:
  vtkFloatNormals() {};
  ~vtkFloatNormals() {};
  vtkFloatNormals(const vtkFloatNormals&) {};
  void operator=(const vtkFloatNormals&) {};

private:
  // hide the vtkNormal's New() method
  static vtkFloatNormals *New(int) { return vtkFloatNormals::New();};
};

inline float *vtkFloatNormals::GetPointer(const int id)
{
  return ((vtkFloatArray *)this->Data)->GetPointer(3*id);
} 

inline float *vtkFloatNormals::WritePointer(const int id, const int number)
{
  return ((vtkFloatArray *)this->Data)->WritePointer(3*id,3*number);
}

inline void vtkFloatNormals::SetData(vtkDataArray *data)
{
  if ( data->GetDataType() != VTK_FLOAT )
    {
    vtkErrorMacro(<<"Float normals only accepts float data type");
    return;
    }

  vtkNormals::SetData(data);
}

inline void vtkFloatNormals::SetDataType(int type)
{
  if ( type != VTK_FLOAT )
    {
    vtkErrorMacro(<<"Float normals only accepts float data type");
    return;
    }

  vtkNormals::SetDataType(type);
}
#endif

#endif

