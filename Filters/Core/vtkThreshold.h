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
/**
 * @class   vtkThreshold
 * @brief   extracts cells where scalar value in cell satisfies threshold criterion
 *
 * vtkThreshold is a filter that extracts cells from any dataset type that
 * satisfy a threshold criterion. A cell satisfies the criterion if the
 * scalar value of (every or any) point satisfies the criterion. The
 * criterion can take three forms: 1) greater than a particular value; 2)
 * less than a particular value; or 3) between two values. The output of this
 * filter is an unstructured grid.
 *
 * Note that scalar values are available from the point and cell attribute
 * data.  By default, point data is used to obtain scalars, but you can
 * control this behavior. See the AttributeMode ivar below.
 *
 * By default only the first scalar value is used in the decision. Use the ComponentMode
 * and SelectedComponent ivars to control this behavior.
 *
 * @sa
 * vtkThresholdPoints vtkThresholdTextureCoords
*/

#ifndef vtkThreshold_h
#define vtkThreshold_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

#define VTK_ATTRIBUTE_MODE_DEFAULT         0
#define VTK_ATTRIBUTE_MODE_USE_POINT_DATA  1
#define VTK_ATTRIBUTE_MODE_USE_CELL_DATA   2

// order / values are important because of the SetClampMacro
#define VTK_COMPONENT_MODE_USE_SELECTED    0
#define VTK_COMPONENT_MODE_USE_ALL         1
#define VTK_COMPONENT_MODE_USE_ANY         2

class vtkDataArray;
class vtkIdList;

class VTKFILTERSCORE_EXPORT vtkThreshold : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkThreshold *New();
  vtkTypeMacro(vtkThreshold,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Criterion is cells whose scalars are less or equal to lower threshold.
   */
  void ThresholdByLower(double lower);

  /**
   * Criterion is cells whose scalars are greater or equal to upper threshold.
   */
  void ThresholdByUpper(double upper);

  /**
   * Criterion is cells whose scalars are between lower and upper thresholds
   * (inclusive of the end values).
   */
  void ThresholdBetween(double lower, double upper);

  //@{
  /**
   * Get the Upper and Lower thresholds.
   */
  vtkGetMacro(UpperThreshold,double);
  vtkGetMacro(LowerThreshold,double);
  //@}

  //@{
  /**
   * Control how the filter works with scalar point data and cell attribute
   * data.  By default (AttributeModeToDefault), the filter will use point
   * data, and if no point data is available, then cell data is
   * used. Alternatively you can explicitly set the filter to use point data
   * (AttributeModeToUsePointData) or cell data (AttributeModeToUseCellData).
   */
  vtkSetMacro(AttributeMode,int);
  vtkGetMacro(AttributeMode,int);
  void SetAttributeModeToDefault()
    {this->SetAttributeMode(VTK_ATTRIBUTE_MODE_DEFAULT);};
  void SetAttributeModeToUsePointData()
    {this->SetAttributeMode(VTK_ATTRIBUTE_MODE_USE_POINT_DATA);};
  void SetAttributeModeToUseCellData()
    {this->SetAttributeMode(VTK_ATTRIBUTE_MODE_USE_CELL_DATA);};
  const char *GetAttributeModeAsString();
  //@}

  //@{
  /**
   * Control how the decision of in / out is made with multi-component data.
   * The choices are to use the selected component (specified in the
   * SelectedComponent ivar), or to look at all components. When looking at
   * all components, the evaluation can pass if all the components satisfy
   * the rule (UseAll) or if any satisfy is (UseAny). The default value is
   * UseSelected.
   */
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
  //@}

  //@{
  /**
   * When the component mode is UseSelected, this ivar indicated the selected
   * component. The default value is 0.
   */
  vtkSetClampMacro(SelectedComponent,int,0,VTK_INT_MAX);
  vtkGetMacro(SelectedComponent,int);
  //@}

  //@{
  /**
   * If using scalars from point data, all scalars for all points in a cell
   * must satisfy the threshold criterion if AllScalars is set. Otherwise,
   * just a single scalar value satisfying the threshold criterion enables
   * will extract the cell.
   */
  vtkSetMacro(AllScalars,vtkTypeBool);
  vtkGetMacro(AllScalars,vtkTypeBool);
  vtkBooleanMacro(AllScalars,vtkTypeBool);
  //@}

  //@{
  /**
   * If this is on (default is off), we will use the continuous interval
   * [minimum cell scalar, maxmimum cell scalar] to intersect the threshold bound
   * , rather than the set of discrete scalar values from the vertices
   * *WARNING*: For higher order cells, the scalar range of the cell is
   * not the same as the vertex scalar interval used here, so the
   * result will not be accurate.
   */
  vtkSetMacro(UseContinuousCellRange,vtkTypeBool);
  vtkGetMacro(UseContinuousCellRange,vtkTypeBool);
  vtkBooleanMacro(UseContinuousCellRange,vtkTypeBool);
  //@}

  //@{
  /**
   * Set the data type of the output points (See the data types defined in
   * vtkType.h). The default data type is float.

   * These methods are deprecated. Please use the SetOutputPointsPrecision()
   * and GetOutputPointsPrecision() methods instead.
   */
  void SetPointsDataTypeToDouble() { this->SetPointsDataType( VTK_DOUBLE ); }
  void SetPointsDataTypeToFloat()  { this->SetPointsDataType( VTK_FLOAT  ); }
  void SetPointsDataType(int type);
  int GetPointsDataType();
  //@}

  //@{
  /**
   * Invert the threshold results. That is, cells that would have been in the output with this
   * option off are excluded, while cells that would have been excluded from the output are
   * included.
   */
  vtkSetMacro(Invert, bool);
  vtkGetMacro(Invert, bool);
  vtkBooleanMacro(Invert, bool);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  void SetOutputPointsPrecision(int precision);
  int GetOutputPointsPrecision() const;
  //@}

protected:
  vtkThreshold();
  ~vtkThreshold() override;

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

  int FillInputPortInformation(int port, vtkInformation *info) override;


  vtkTypeBool    AllScalars;
  double LowerThreshold;
  double UpperThreshold;
  int    AttributeMode;
  int    ComponentMode;
  int    SelectedComponent;
  int OutputPointsPrecision;
  vtkTypeBool UseContinuousCellRange;
  bool   Invert;

  int (vtkThreshold::*ThresholdFunction)(double s);

  int Lower(double s) {return ( s <= this->LowerThreshold ? 1 : 0 );};
  int Upper(double s) {return ( s >= this->UpperThreshold ? 1 : 0 );};
  int Between(double s) {return ( s >= this->LowerThreshold ?
                               ( s <= this->UpperThreshold ? 1 : 0 ) : 0 );};

  int EvaluateComponents( vtkDataArray *scalars, vtkIdType id );
  int EvaluateCell( vtkDataArray *scalars, vtkIdList* cellPts, int numCellPts );
  int EvaluateCell( vtkDataArray *scalars, int c, vtkIdList* cellPts, int numCellPts );
private:
  vtkThreshold(const vtkThreshold&) = delete;
  void operator=(const vtkThreshold&) = delete;
};

#endif
