/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClipPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkClipPolyData
 * @brief   clip polygonal data with user-specified implicit function or input scalar data
 *
 * vtkClipPolyData is a filter that clips polygonal data using either
 * any subclass of vtkImplicitFunction, or the input scalar
 * data. Clipping means that it actually "cuts" through the cells of
 * the dataset, returning everything inside of the specified implicit
 * function (or greater than the scalar value) including "pieces" of
 * a cell. (Compare this with vtkExtractGeometry, which pulls out
 * entire, uncut cells.) The output of this filter is polygonal data.
 *
 * To use this filter, you must decide if you will be clipping with an
 * implicit function, or whether you will be using the input scalar
 * data.  If you want to clip with an implicit function, you must:
 * 1) define an implicit function
 * 2) set it with the SetClipFunction method
 * 3) apply the GenerateClipScalarsOn method
 * If a ClipFunction is not specified, or GenerateClipScalars is off
 * (the default), then the input's scalar data will be used to clip
 * the polydata.
 *
 * You can also specify a scalar value, which is used to
 * decide what is inside and outside of the implicit function. You can
 * also reverse the sense of what inside/outside is by setting the
 * InsideOut instance variable. (The cutting algorithm proceeds by
 * computing an implicit function value or using the input scalar data
 * for each point in the dataset. This is compared to the scalar value
 * to determine inside/outside.)
 *
 * This filter can be configured to compute a second output. The
 * second output is the polygonal data that is clipped away. Set the
 * GenerateClippedData boolean on if you wish to access this output data.
 *
 * @warning
 * In order to cut all types of cells in polygonal data, vtkClipPolyData
 * triangulates some cells, and then cuts the resulting simplices
 * (i.e., points, lines, and triangles). This means that the resulting
 * output may consist of different cell types than the input data.
 *
 * @sa
 * vtkImplicitFunction vtkCutter vtkClipVolume vtkExtractGeometry
*/

#ifndef vtkClipPolyData_h
#define vtkClipPolyData_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkImplicitFunction;
class vtkIncrementalPointLocator;

class VTKFILTERSCORE_EXPORT vtkClipPolyData : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkClipPolyData,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct with user-specified implicit function; InsideOut turned off;
   * value set to 0.0; GenerateClipScalars turned off; GenerateClippedOutput
   * turned off.
   */
  static vtkClipPolyData *New();

  //@{
  /**
   * Set the clipping value of the implicit function (if clipping with
   * implicit function) or scalar value (if clipping with
   * scalars). The default value is 0.0.
   */
  vtkSetMacro(Value,double);
  vtkGetMacro(Value,double);
  //@}

  //@{
  /**
   * Set/Get the InsideOut flag. When off, a vertex is considered
   * inside the implicit function if its value is greater than the
   * Value ivar. When InsideOutside is turned on, a vertex is
   * considered inside the implicit function if its implicit function
   * value is less than or equal to the Value ivar.  InsideOut is off
   * by default.
   */
  vtkSetMacro(InsideOut,int);
  vtkGetMacro(InsideOut,int);
  vtkBooleanMacro(InsideOut,int);
  //@}

  //@{
  /**
   * Specify the implicit function with which to perform the
   * clipping. If you do not define an implicit function, then the input
   * scalar data will be used for clipping.
   */
  virtual void SetClipFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(ClipFunction,vtkImplicitFunction);
  //@}

  //@{
  /**
   * If this flag is enabled, then the output scalar values will be
   * interpolated from the implicit function values, and not the
   * input scalar data. If you enable this flag but do not provide an
   * implicit function an error will be reported.
   * GenerateClipScalars is off by default.
   */
  vtkSetMacro(GenerateClipScalars,int);
  vtkGetMacro(GenerateClipScalars,int);
  vtkBooleanMacro(GenerateClipScalars,int);
  //@}

  //@{
  /**
   * Control whether a second output is generated. The second output
   * contains the polygonal data that's been clipped away.
   * GenerateClippedOutput is off by default.
   */
  vtkSetMacro(GenerateClippedOutput,int);
  vtkGetMacro(GenerateClippedOutput,int);
  vtkBooleanMacro(GenerateClippedOutput,int);
  //@}

  /**
   * Return the Clipped output.
   */
  vtkPolyData *GetClippedOutput();

  /**
   * Return the output port (a vtkAlgorithmOutput) of the clipped output.
   */
  vtkAlgorithmOutput* GetClippedOutputPort()
  {
      return this->GetOutputPort(1);
  }

  //@{
  /**
   * Specify a spatial locator for merging points. By default, an
   * instance of vtkMergePoints is used.
   */
  void SetLocator(vtkIncrementalPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is specified. The
   * locator is used to merge coincident points.
   */
  void CreateDefaultLocator();

  /**
   * Return the mtime also considering the locator and clip function.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings. OutputPointsPrecision is DEFAULT_PRECISION
   * by default.
   */
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);
  //@}

protected:
  vtkClipPolyData(vtkImplicitFunction *cf=NULL);
  ~vtkClipPolyData() VTK_OVERRIDE;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  vtkImplicitFunction *ClipFunction;

  vtkIncrementalPointLocator *Locator;
  int InsideOut;
  double Value;
  int GenerateClipScalars;
  int GenerateClippedOutput;
  int OutputPointsPrecision;

private:
  vtkClipPolyData(const vtkClipPolyData&) VTK_DELETE_FUNCTION;
  void operator=(const vtkClipPolyData&) VTK_DELETE_FUNCTION;
};

#endif
