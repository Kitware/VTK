/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreshold.h
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

class VTK_EXPORT vtkThreshold : public vtkDataSetToUnstructuredGridFilter
{
public:
  vtkThreshold();
  static vtkThreshold *New() {return new vtkThreshold;};
  const char *GetClassName() {return "vtkThreshold";};
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
  char *GetAttributeModeAsString();

  // Description:
  // If using scalars from point data, all scalars for all points in a cell 
  // must satisfy the threshold criterion if AllScalars is set. Otherwise, 
  // just a single scalar value satisfying the threshold criterion enables
  // will extract the cell.
  vtkSetMacro(AllScalars,int);
  vtkGetMacro(AllScalars,int);
  vtkBooleanMacro(AllScalars,int);
  
protected:
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
};

#endif


