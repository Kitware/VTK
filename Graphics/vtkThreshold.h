/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThreshold.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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
//
// By default only the first scalar value is used in the decision. Use the ComponentMode
// and SelectedComponent ivars to control this behavior.

// .SECTION See Also
// vtkThresholdPoints vtkThresholdTextureCoords

#ifndef __vtkThreshold_h
#define __vtkThreshold_h

#include "vtkUnstructuredGridAlgorithm.h"

#define VTK_ATTRIBUTE_MODE_DEFAULT         0
#define VTK_ATTRIBUTE_MODE_USE_POINT_DATA  1
#define VTK_ATTRIBUTE_MODE_USE_CELL_DATA   2

// order / values are important because of the SetClampMacro
#define VTK_COMPONENT_MODE_USE_SELECTED    0
#define VTK_COMPONENT_MODE_USE_ALL         1
#define VTK_COMPONENT_MODE_USE_ANY         2

class vtkDataArray;

class VTK_GRAPHICS_EXPORT vtkThreshold : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkThreshold *New();
  vtkTypeMacro(vtkThreshold,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Criterion is cells whose scalars are less or equal to lower threshold.
  void ThresholdByLower(double lower);

  // Description:
  // Criterion is cells whose scalars are greater or equal to upper threshold.
  void ThresholdByUpper(double upper);

  // Description:
  // Criterion is cells whose scalars are between lower and upper thresholds
  // (inclusive of the end values).
  void ThresholdBetween(double lower, double upper);

  // Description:
  // Get the Upper and Lower thresholds.
  vtkGetMacro(UpperThreshold,double);
  vtkGetMacro(LowerThreshold,double);

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
  // Control how the decision of in / out is made with multi-component data.
  // The choices are to use the selected component (specified in the
  // SelectedComponent ivar), or to look at all components. When looking at
  // all components, the evaluation can pass if all the components satisfy
  // the rule (UseAll) or if any satisfy is (UseAny). The default value is
  // UseSelected.
  vtkSetClampMacro(ComponentMode,int,
                   VTK_COMPONENT_MODE_USE_SELECTED,
                   VTK_COMPONENT_MODE_USE_ANY);
  vtkGetMacro(ComponentMode,int);
  void SetComponentModeToUseSelected()
    {this->SetComponentMode(VTK_COMPONENT_MODE_USE_SELECTED);};
  void SetComponentModeToUseAll()
    {this->SetComponentMode(VTK_COMPONENT_MODE_USE_ALL);};
  void SetComponentModeToUseAny()
    {this->SetComponentMode(VTK_COMPONENT_MODE_USE_ANY);};
  const char *GetComponentModeAsString();

  // Description:
  // When the component mode is UseSelected, this ivar indicated the selected
  // component. The default value is 0.
  vtkSetClampMacro(SelectedComponent,int,0,VTK_INT_MAX);
  vtkGetMacro(SelectedComponent,int);

  // Description:
  // If using scalars from point data, all scalars for all points in a cell
  // must satisfy the threshold criterion if AllScalars is set. Otherwise,
  // just a single scalar value satisfying the threshold criterion enables
  // will extract the cell.
  vtkSetMacro(AllScalars,int);
  vtkGetMacro(AllScalars,int);
  vtkBooleanMacro(AllScalars,int);

  // Description:
  // Set the data type of the output points (See the data types defined in
  // vtkType.h). The default data type is float.
  void SetPointsDataTypeToDouble() { this->SetPointsDataType( VTK_DOUBLE ); }
  void SetPointsDataTypeToFloat()  { this->SetPointsDataType( VTK_FLOAT  ); }
  vtkSetMacro( PointsDataType, int );
  vtkGetMacro( PointsDataType, int );

  virtual int ProcessRequest(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

protected:
  vtkThreshold();
  ~vtkThreshold();

  // Usual data generation method
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  virtual int FillInputPortInformation(int port, vtkInformation *info);


  int    AllScalars;
  double LowerThreshold;
  double UpperThreshold;
  int    AttributeMode;
  int    ComponentMode;
  int    SelectedComponent;
  int    PointsDataType;

  //BTX
  int (vtkThreshold::*ThresholdFunction)(double s);
  //ETX

  int Lower(double s) {return ( s <= this->LowerThreshold ? 1 : 0 );};
  int Upper(double s) {return ( s >= this->UpperThreshold ? 1 : 0 );};
  int Between(double s) {return ( s >= this->LowerThreshold ?
                               ( s <= this->UpperThreshold ? 1 : 0 ) : 0 );};

  int EvaluateComponents( vtkDataArray *scalars, vtkIdType id );

private:
  vtkThreshold(const vtkThreshold&);  // Not implemented.
  void operator=(const vtkThreshold&);  // Not implemented.
};

#endif
