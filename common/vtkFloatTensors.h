/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFloatTensors.h
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
// .NAME vtkFloatTensors - floating point representation of tensor data
// .SECTION Description
// vtkFloatTensors is an (obsolete) concrete implementation of vtkTensors. 
// Tensor values are represented using float values.

#ifndef __vtkFloatTensors_h
#define __vtkFloatTensors_h

#include "vtkTensors.h"
#include "vtkFloatArray.h"

class VTK_EXPORT vtkFloatTensors : public vtkTensors
{
public:
  vtkFloatTensors():vtkTensors(VTK_FLOAT) {};
  static vtkFloatTensors *New() {return new vtkFloatTensors;};
  
  // overload vtkAttributeData API
  void SetDataType(int dataType);
  void SetData(vtkDataArray *);

  float *GetPointer(const int id);
  float *WritePointer(const int id, const int number);

};

// Description:
// Get pointer to array of data starting at data position "id".
inline float *vtkFloatTensors::GetPointer(const int id)
{
  return ((vtkFloatArray *)this->Data)->GetPointer(9*id);
} 

// Description:
// Get pointer to data array. Useful for direct writes of data. MaxId is
// bumped by number (and memory allocated if necessary). Id is the
// location you wish to write into; number is the number of tensors to
// write.
inline float *vtkFloatTensors::WritePointer(const int id, const int number)
{
  return ((vtkFloatArray *)this->Data)->WritePointer(9*id,9*number);
}


// Description:
// Set the data for this object. Only accepts VTK_FLOAT type.
inline void vtkFloatTensors::SetData(vtkDataArray *data)
{
  if ( data->GetDataType() != VTK_FLOAT )
    {
    vtkErrorMacro(<<"Float tensors only accepts float data type");
    return;
    }

  vtkTensors::SetData(data);
}

// Description:
// Set the data type for this object.
inline void vtkFloatTensors::SetDataType(int type)
{
  if ( type != VTK_FLOAT )
    {
    vtkErrorMacro(<<"Float tensors only accepts float data type");
    return;
    }

  vtkTensors::SetDataType(type);
}

#endif
