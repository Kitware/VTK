/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAreaContourSpectrumFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkAreaContourSpectrumFilter
 * @brief   compute an approximation of the area
 * contour signature (evolution of the area of the input surface along an arc of
 * the Reeb graph).
 *
 * The filter takes a vtkPolyData as an input (port 0), along with a
 * vtkReebGraph (port 1).
 * The Reeb graph arc to consider can be specified with SetArcId() (default: 0).
 * The number of (evenly distributed) samples of the signature can be defined
 * with SetNumberOfSamples() (default value: 100).
 * The filter will first try to pull as a scalar field the vtkDataArray with Id
 * 'FieldId' of the vtkPolyData, see SetFieldId (default: 0). The filter will
 * abort if this field does not exist.
 *
 * The filter outputs a vtkTable with the area contour signature
 * approximation, each sample being evenly distributed in the function span of
 * the arc.
 *
 * This filter is a typical example for designing your own contour signature
 * filter (with customized metrics). It also shows typical vtkReebGraph
 * traversals.
 *
 * Reference:
 * C. Bajaj, V. Pascucci, D. Schikore,
 * "The contour spectrum",
 * IEEE Visualization, 167-174, 1997.
*/

#ifndef vtkAreaContourSpectrumFilter_h
#define vtkAreaContourSpectrumFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include  "vtkDataObjectAlgorithm.h"

class vtkTable;

class VTKFILTERSGENERAL_EXPORT vtkAreaContourSpectrumFilter :
  public vtkDataObjectAlgorithm
{
public:
  static vtkAreaContourSpectrumFilter* New();
  vtkTypeMacro(vtkAreaContourSpectrumFilter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set the arc Id for which the contour signature has to be computed.
   * Default value: 0
   */
  vtkSetMacro(ArcId, vtkIdType);
  vtkGetMacro(ArcId, vtkIdType);
  //@}

  //@{
  /**
   * Set the number of samples in the output signature
   * Default value: 100
   */
  vtkSetMacro(NumberOfSamples, int);
  vtkGetMacro(NumberOfSamples, int);
  //@}

  //@{
  /**
   * Set the scalar field Id
   * Default value: 0
   */
  vtkSetMacro(FieldId, vtkIdType);
  vtkGetMacro(FieldId, vtkIdType);
  //@}

  vtkTable* GetOutput();

protected:
  vtkAreaContourSpectrumFilter();
  ~vtkAreaContourSpectrumFilter() VTK_OVERRIDE;

  vtkIdType ArcId, FieldId;
  int NumberOfSamples;

  int FillInputPortInformation(int portNumber, vtkInformation *) VTK_OVERRIDE;
  int FillOutputPortInformation(int portNumber, vtkInformation *info) VTK_OVERRIDE;

  int RequestData(vtkInformation *request,
    vtkInformationVector **inputVector, vtkInformationVector *outputVector) VTK_OVERRIDE;

private:
  vtkAreaContourSpectrumFilter(const vtkAreaContourSpectrumFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkAreaContourSpectrumFilter&) VTK_DELETE_FUNCTION;
};

#endif
