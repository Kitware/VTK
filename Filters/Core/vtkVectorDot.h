/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorDot.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVectorDot
 * @brief   generate scalars from dot product of vectors and normals (e.g., show displacement plot)
 *
 * vtkVectorDot is a filter to generate point scalar values from a dataset.
 * The scalar value at a point is created by computing the dot product
 * between the normal and vector at each point. Combined with the appropriate
 * color map, this can show nodal lines/mode shapes of vibration, or a
 * displacement plot.
 *
 * Note that by default the resulting scalars are mapped into a specified
 * range. This requires an extra pass in the algorithm. This mapping pass can
 * be disabled (set MapScalars to off).
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
*/

#ifndef vtkVectorDot_h
#define vtkVectorDot_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class VTKFILTERSCORE_EXPORT vtkVectorDot : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkVectorDot,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct object with scalar range (-1,1).
   */
  static vtkVectorDot *New();

  //@{
  /**
   * Enable/disable the mapping of scalars into a specified range. This will
   * significantly improve the performance of the algorithm but the resulting
   * scalar values will strictly be a function of the vector and normal
   * data. By default, MapScalars is enabled, and the output scalar
   * values will fall into the range ScalarRange.
   */
  vtkSetMacro(MapScalars,int);
  vtkGetMacro(MapScalars,int);
  vtkBooleanMacro(MapScalars,int);
  //@}

  //@{
  /**
   * Specify the range into which to map the scalars. This mapping only
   * occurs if MapScalars is enabled.
   */
  vtkSetVector2Macro(ScalarRange,double);
  vtkGetVectorMacro(ScalarRange,double,2);
  //@}

  //@{
  /**
   * Return the actual range of the generated scalars (prior to mapping).
   * Note that the data is valid only after the filter executes.
   */
  vtkGetVectorMacro(ActualRange,double,2);
  //@}

protected:
  vtkVectorDot();
  ~vtkVectorDot() VTK_OVERRIDE {}

  int MapScalars;
  double ScalarRange[2];
  double ActualRange[2];

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

private:
  vtkVectorDot(const vtkVectorDot&) VTK_DELETE_FUNCTION;
  void operator=(const vtkVectorDot&) VTK_DELETE_FUNCTION;
};

#endif
