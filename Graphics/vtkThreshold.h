/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreshold.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkThreshold - extracts cells where scalar value in cell satisfies threshold criterion
// .SECTION Description
// vtkThreshold is a filter that extracts cells from any dataset type that
// satisfy a threshold criterion. A cell satisfies the criterion if the
// scalar value of (every or any) point satisfies the criterion. The
// criterion can take three forms: 1) greater than a particular value; 2)
// less than a particular value; or 3) between two values. The output of this
// filter is an unstructured grid.
//
// Note that scalar values are available from the point and cell attribute
// data.  By default, point data is used to obtain scalars, but you can
// control this behavior. See the AttributeMode ivar below.

// .SECTION See Also
// vtkThresholdPoints vtkThresholdTextureCoords

#ifndef __vtkThreshold_h
#define __vtkThreshold_h

#include "vtkDataSetToUnstructuredGridFilter.h"

#define VTK_ATTRIBUTE_MODE_DEFAULT 0
#define VTK_ATTRIBUTE_MODE_USE_POINT_DATA 1
#define VTK_ATTRIBUTE_MODE_USE_CELL_DATA 2

class VTK_GRAPHICS_EXPORT vtkThreshold : public vtkDataSetToUnstructuredGridFilter
{
public:
  static vtkThreshold *New();
  vtkTypeRevisionMacro(vtkThreshold,vtkDataSetToUnstructuredGridFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Criterion is cells whose scalars are less or equal to lower threshold.
  void ThresholdByLower(float lower);

  // Description:
  // Criterion is cells whose scalars are greater or equal to upper threshold.
  void ThresholdByUpper(float upper);

  // Description:
  // Criterion is cells whose scalars are between lower and upper thresholds.
  void ThresholdBetween(float lower, float upper);

  // Description:
  // Get the Upper and Lower thresholds.
  vtkGetMacro(UpperThreshold,float);
  vtkGetMacro(LowerThreshold,float);

  // Description:
  // Control how the filter works with scalar point data and cell attribute
  // data.  By default (AttributeModeToDefault), the filter will use point
  // data, and if no point data is available, then cell data is
  // used. Alternatively you can explicitly set the filter to use point data
  // (AttributeModeToUsePointData) or cell data (AttributeModeToUseCellData).
  vtkSetMacro(AttributeMode,int);
  vtkGetMacro(AttributeMode,int);
  void SetAttributeModeToDefault() 
    {this->SetAttributeMode(VTK_ATTRIBUTE_MODE_DEFAULT);};
  void SetAttributeModeToUsePointData() 
    {this->SetAttributeMode(VTK_ATTRIBUTE_MODE_USE_POINT_DATA);};
  void SetAttributeModeToUseCellData() 
    {this->SetAttributeMode(VTK_ATTRIBUTE_MODE_USE_CELL_DATA);};
  const char *GetAttributeModeAsString();

  // Description:
  // If using scalars from point data, all scalars for all points in a cell 
  // must satisfy the threshold criterion if AllScalars is set. Otherwise, 
  // just a single scalar value satisfying the threshold criterion enables
  // will extract the cell.
  vtkSetMacro(AllScalars,int);
  vtkGetMacro(AllScalars,int);
  vtkBooleanMacro(AllScalars,int);
  
  // Description:
  // If you want to threshold by an arbitrary array, then set its name here.
  // By default this in NULL and the filter will use the active scalar array.
  vtkGetStringMacro(InputScalarsSelection);
  void SelectInputScalars(const char *fieldName) 
    {this->SetInputScalarsSelection(fieldName);}
  
protected:
  vtkThreshold();
  ~vtkThreshold();

  // Usual data generation method
  void Execute();

  int   AllScalars;
  float LowerThreshold;
  float UpperThreshold;
  int   AttributeMode;

  //BTX
  int (vtkThreshold::*ThresholdFunction)(float s);
  //ETX

  int Lower(float s) {return ( s <= this->LowerThreshold ? 1 : 0 );};
  int Upper(float s) {return ( s >= this->UpperThreshold ? 1 : 0 );};
  int Between(float s) {return ( s >= this->LowerThreshold ? 
                               ( s <= this->UpperThreshold ? 1 : 0 ) : 0 );};

  char *InputScalarsSelection;
  vtkSetStringMacro(InputScalarsSelection);

private:
  vtkThreshold(const vtkThreshold&);  // Not implemented.
  void operator=(const vtkThreshold&);  // Not implemented.
};

#endif
