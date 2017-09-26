/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeContourSpectrumFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVolumeContourSpectrumFilter
 * @brief   compute an approximation of the
 * volume contour signature (evolution of the volume of the input tet-mesh
 * along an arc of the Reeb graph).
 *
 * The filter takes a vtkUnstructuredGrid as an input (port 0), along with a
 * vtkReebGraph (port 1).
 * The Reeb graph arc to consider can be specified with SetArcId() (default: 0).
 * The number of (evenly distributed) samples of the signature can be defined
 * with SetNumberOfSamples() (default value: 100).
 * The filter will first try to pull as a scalar field the vtkDataArray with Id
 * 'FieldId' of the vtkUnstructuredGrid, see SetFieldId (default: 0). The
 * filter will abort if this field does not exist.
 *
 * The filter outputs a vtkTable with the volume contour signature
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

#ifndef vtkVolumeContourSpectrumFilter_h
#define vtkVolumeContourSpectrumFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include  "vtkDataObjectAlgorithm.h"

class vtkReebGraph;
class vtkTable;

class VTKFILTERSGENERAL_EXPORT vtkVolumeContourSpectrumFilter :
  public vtkDataObjectAlgorithm
{
public:
  static vtkVolumeContourSpectrumFilter* New();
  vtkTypeMacro(vtkVolumeContourSpectrumFilter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
  vtkVolumeContourSpectrumFilter();
  ~vtkVolumeContourSpectrumFilter() override;

  vtkIdType ArcId, FieldId;
  int NumberOfSamples;

  int FillInputPortInformation(int portNumber, vtkInformation *) override;
  int FillOutputPortInformation(int portNumber, vtkInformation *info) override;

  int RequestData(vtkInformation *request,
    vtkInformationVector **inputVector, vtkInformationVector *outputVector) override;

private:
  vtkVolumeContourSpectrumFilter(const vtkVolumeContourSpectrumFilter&) = delete;
  void operator=(const vtkVolumeContourSpectrumFilter&) = delete;
};

#endif
