/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkReebGraphVolumeSkeletonFilter - compute a skeletal embedding of the
// Reeb graph of a scalar field defined on a tetrahedral mesh
// (vtkUnstructuredGrid).
// .SECTION Description
// The filter takes a vtkUnstructuredGrid as an input (port 0), along with a
// vtkReebGraph (port 1).
// The filter samples each arc of the Reeb graph and embeds the samples on the
// barycenter of the corresponding field contour.
// The number of (evenly distributed) arc samples  can be defined with
// SetNumberOfSamples() (default value: 10).
// The skeleton can be optionally smoothed with SetNumberOfSmoothingIterations()
// (default value: 10).
// The filter will first try to pull as a scalar field the vtkDataArray with Id
// 'FieldId' of the vtkPolyData, see SetFieldId() (default: 0). The filter will
// abort if this field does not exist.
//
// The filter outputs a vtkTable of points (double[3]). Each column contains the
// samples (sorted by function value) of the corresponding arc. The first and
// the last entry of the column corresponds to the critical nodes at the
// extremity of the arc (each column has NumberOfSamples + 2 entries).
//
// The skeleton can be rendered by linking the samples with geometrical
// primitives (for instance, spheres at critical nodes and cylinders between
// intermediary samples, see Graphics/Testing/Cxx/TestReebGraph.cxx).
//

#ifndef __vtkReebGraphVolumeSkeletonFilter_h
#define __vtkReebGraphVolumeSkeletonFilter_h

#include "vtkFiltersReebGraphModule.h" // For export macro
#include  "vtkDataObjectAlgorithm.h"

class vtkReebGraph;
class vtkTable;

class VTKFILTERSREEBGRAPH_EXPORT vtkReebGraphVolumeSkeletonFilter :
  public vtkDataObjectAlgorithm
{
public:
  static vtkReebGraphVolumeSkeletonFilter* New();
  vtkTypeMacro(vtkReebGraphVolumeSkeletonFilter,
    vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the number of samples along each arc of the Reeb graph
  // Default value: 5
  vtkSetMacro(NumberOfSamples, int);
  vtkGetMacro(NumberOfSamples, int);

  // Description:
  // Set the number of optional smoothing iterations
  // Default value: 30
  vtkSetMacro(NumberOfSmoothingIterations, int);
  vtkGetMacro(NumberOfSmoothingIterations, int);

  // Description:
  // Set the scalar field Id
  // Default value: 0
  vtkSetMacro(FieldId, vtkIdType);
  vtkGetMacro(FieldId, vtkIdType);

  vtkTable* GetOutput();

protected:
  vtkReebGraphVolumeSkeletonFilter();
  ~vtkReebGraphVolumeSkeletonFilter();

  vtkIdType FieldId;
  int NumberOfSamples, NumberOfSmoothingIterations;

  int FillInputPortInformation(int portNumber, vtkInformation *);
  int FillOutputPortInformation(int portNumber, vtkInformation *info);

  int RequestData(vtkInformation *request,
    vtkInformationVector **inputVector, vtkInformationVector *outputVector);

private:
  vtkReebGraphVolumeSkeletonFilter(const vtkReebGraphVolumeSkeletonFilter&); // Not implemented.
  void operator=(const vtkReebGraphVolumeSkeletonFilter&); // Not implemented.
};

#endif
