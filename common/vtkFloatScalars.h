/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFloatScalars.h
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
// .NAME vtkFloatScalars - (obsolete)floating point representation of scalars
// .SECTION Description
// vtkFloatScalars is an (obsolete) concrete implementation of vtkScalars. Scalars
// coordinates are represented using float values.

#ifndef __vtkFloatScalars_h
#define __vtkFloatScalars_h

#include "vtkScalars.h"
#include "vtkFloatArray.h"

class VTK_EXPORT vtkFloatScalars : public vtkScalars
{
public:
  static vtkFloatScalars *New() {return new vtkFloatScalars;};
  vtkFloatScalars():vtkScalars(VTK_FLOAT) {};
  
  // overload vtkAttributeData API
  void SetDataType(int dataType);
  void SetData(vtkDataArray *);

  float *GetPointer(const int id);
  float *WritePointer(const int id, const int number);

};

// Description:
// Get pointer to array of data starting at data position "id".
inline float *vtkFloatScalars::GetPointer(const int id)
{
  return ((vtkFloatArray *)this->Data)->GetPointer(this->Data->GetNumberOfComponents()*id);
} 

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is
// bumped by number (and memory allocated if necessary). Id is the
// location you wish to write into; number is the number of scalars to
// write.
inline float *vtkFloatScalars::WritePointer(const int id, const int number)
{
  int num=this->Data->GetNumberOfComponents();
  
  return ((vtkFloatArray *)this->Data)->WritePointer(num*id,num*number);
}

// Description:
// Set the data for this object. Only accepts VTK_FLOAT type.
inline void vtkFloatScalars::SetData(vtkDataArray *data)
{
  if ( data->GetDataType() != VTK_FLOAT )
    {
    vtkErrorMacro(<<"Float scalars only accepts float data type");
    return;
    }

  vtkScalars::SetData(data);
}

// Description:
// Set the data type for this object.
inline void vtkFloatScalars::SetDataType(int type)
{
  if ( type != VTK_FLOAT )
    {
    vtkErrorMacro(<<"Float Scalars only accepts float data type");
    return;
    }

  vtkScalars::SetDataType(type);
}

#endif

