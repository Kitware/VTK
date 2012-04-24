/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAttributeDataToFieldDataFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAttributeDataToFieldDataFilter - map attribute data to field data
// .SECTION Description
// vtkAttributeDataToFieldDataFilter is a class that maps attribute data into
// field data. Since this filter is a subclass of vtkDataSetAlgorithm,
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

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class VTKFILTERSCORE_EXPORT vtkAttributeDataToFieldDataFilter : public vtkDataSetAlgorithm
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkAttributeDataToFieldDataFilter,vtkDataSetAlgorithm);

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

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *); //generate output data

  int PassAttributeData;
private:
  vtkAttributeDataToFieldDataFilter(const vtkAttributeDataToFieldDataFilter&);  // Not implemented.
  void operator=(const vtkAttributeDataToFieldDataFilter&);  // Not implemented.
};

#endif
