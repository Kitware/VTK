/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAttributeDataToFieldDataFilter.h
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
// .NAME vtkAttributeDataToFieldDataFilter - map attribute data to field data
// .SECTION Description
// vtkAttributeDataToFieldDataFilter is a class that maps attribute data into
// field data. Since this filter is a subclass of vtkDataSetToDataSetFilter,
// the output dataset (whose structure is the same as the input dataset),
// will contain the field data that is generated. The filter will convert
// point and cell attribute data to field data and assign it as point and
// cell field data, replacing any point or field data that was there
// previously. By default, the original non-field point and cell attribute
// data will be passed to the output of the filter, although you can shut
// this behavior down.

// .SECTION Caveats
// Reference counting the underlying data arrays is used to create the field
// data.  Therefore, no extra memory is utilized.
//
// The original field data (if any) associated with the point and cell
// attribute data is placed into the generated fields along with the scalars,
// vectors, etc.

// .SECTION See Also
// vtkFieldData vtkDataObject vtkDataSet vtkFieldDataToAttributeDataFilter

#ifndef __vtkAttributeDataToFieldDataFilter_h
#define __vtkAttributeDataToFieldDataFilter_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_GRAPHICS_EXPORT vtkAttributeDataToFieldDataFilter : public vtkDataSetToDataSetFilter
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkAttributeDataToFieldDataFilter,vtkDataSetToDataSetFilter);

  // Description:
  // Construct this object.
  static vtkAttributeDataToFieldDataFilter *New();

  // Description:
  // Turn on/off the passing of point and cell non-field attribute data to the
  // output of the filter.
  vtkSetMacro(PassAttributeData,int);
  vtkGetMacro(PassAttributeData,int);
  vtkBooleanMacro(PassAttributeData,int);

protected:
  vtkAttributeDataToFieldDataFilter();
  ~vtkAttributeDataToFieldDataFilter() {};
  vtkAttributeDataToFieldDataFilter(const vtkAttributeDataToFieldDataFilter&);
  void operator=(const vtkAttributeDataToFieldDataFilter&);

  void Execute(); //generate output data

  int PassAttributeData;
};

#endif


