/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAttributeDataToFieldDataFilter.h
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
// .NAME vtkAttributeDataToFieldDataFilter - map attribute data to field data
// .SECTION Description
// vtkAttributeDataToFieldDataFilter is a class that maps attribute data
// into field data. Since this filter is a subclass of vtkDataSetToDataSetFilter,
// the output dataset (whose structure is the same as the input dataset), will
// contain the field data that is generated. The filter will convert point and 
// cell attribute data to field data and assign it as point and cell field data,
// replacing any point or field data that was there previously. By default, the 
// original non-field point and cell attribute data will be passed to the output 
// of the filter, although you can shut this behavior down.

// .SECTION Caveats
// Reference counting the underlying data arrays is used to create the field data.
// Therefore, no extra memory is utilized.
//
// The original field data (if any) associated with the point and cell attribute data
// is placed into the generated fields along with the scalars, vectors, etc.

// .SECTION See Also
// vtkFieldData vtkDataObject vtkDataSet vtkFieldDataToAttributeData

#ifndef __vtkAttributeDataToFieldDataFilter_h
#define __vtkAttributeDataToFieldDataFilter_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_EXPORT vtkAttributeDataToFieldDataFilter : public vtkDataSetToDataSetFilter
{
public:
  vtkAttributeDataToFieldDataFilter();
  void PrintSelf(ostream& os, vtkIndent indent);
  const char *GetClassName() {return "vtkAttributeDataToFieldDataFilter";};

  // Description:
  // Construct this object.
  static vtkAttributeDataToFieldDataFilter *New() {
    return new vtkAttributeDataToFieldDataFilter;};

  // Description:
  // Turn on/off the passing of point and cell non-field attribute data to the
  // output of the filter.
  vtkSetMacro(PassAttributeData,int);
  vtkGetMacro(PassAttributeData,int);
  vtkBooleanMacro(PassAttributeData,int);

protected:
  void Execute(); //generate output data

  int PassAttributeData;
};

#endif


