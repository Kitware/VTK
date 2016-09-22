/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointCloudFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPointCloudFilter - abstract class for filtering a point cloud

// .SECTION Description
// vtkPointCloudFilter serves as a base for classes that filter point clouds.
// It takes as input any vtkPointSet (which represents points explicitly
// using vtkPoints) and produces as output an explicit representation of
// filtered points via a vtkPolyData. This output vtkPolyData will populate
// its instance of vtkPoints, but no cells will be defined (i.e., no
// vtkVertex or vtkPolyVertex are contained in the output). Also, after
// filter execution, the user can request a vtkIdType* point map which
// indicates how the input points were mapped to the output. A value of
// PointMap[i] < 0 (where i is the ith input point) means that the ith input
// point was removed. Otherwise PointMap[i] indicates the position in the
// output vtkPoints array (point cloud).
//
// Optionally the filter may produce a second output. This second output is
// another vtkPolyData with a vtkPoints that contains the points that were
// removed during processing. To produce this second output, you must enable
// GenerateOutliers. If this optional, second output is created, then the
// contents of the PointMap are modified as well. In this case, a PointMap[i]
// < 0 means that the ith input point has been mapped to the (-PointMap[i])-1
// position in the second output's vtkPoints.

// .SECTION Caveats
// This class has been threaded with vtkSMPTools. Using TBB or other
// non-sequential type (set in the CMake variable
// VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
//
// The filter copies point attributes from input to output consistent
// with the filtering operation.
//
// It is convenient to use vtkPointGaussianMapper to render the points (since
// this mapper does not require cells to be defined, and it is quite fast).

// .SECTION See Also
// vtkRadiusOutlierRemoval vtkPointGaussianMapper vtkThresholdPoints

#ifndef vtkPointCloudFilter_h
#define vtkPointCloudFilter_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkPointSet;


class VTKFILTERSPOINTS_EXPORT vtkPointCloudFilter : public vtkPolyDataAlgorithm
{
public:
  // Description:
  // Standard methods to obtain type information, and print information.
  vtkTypeMacro(vtkPointCloudFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Retrieve a map which indicates, on a point-by-point basis, where each
  // input point was placed into the output. In other words, map[i] indicates
  // where the ith input point is located in the output array of points. If
  // map[i] < 0, then the ith input point was removed during filter
  // execution.  This method returns valid information only after the filter
  // executes.
  const vtkIdType* GetPointMap();

  // Description:
  // Return the number of points removed after filter execution. The
  // information retuned is valid only after the filter executes.
  vtkIdType GetNumberOfPointsRemoved();

  // Description:
  // If this method is enabled (true), then a second output will be created
  // that contains the outlier points. By default this is off (false).  Note
  // that if enabled, the PointMap is modified as well: the outlier points
  // are listed as well, with similar meaning, except their value is negated
  // and shifted by -1.
  vtkSetMacro(GenerateOutliers,bool);
  vtkGetMacro(GenerateOutliers,bool);
  vtkBooleanMacro(GenerateOutliers,bool);

protected:
  vtkPointCloudFilter();
  ~vtkPointCloudFilter();

  // All derived classes must implement this method. Note that a side effect of
  // the class is to populate the PointMap. Zero is returned on error.
  virtual int FilterPoints(vtkPointSet *input) = 0;

  // Keep track of which points are removed through the point map
  vtkIdType *PointMap;
  vtkIdType NumberOfPointsRemoved;

  // Does a second output need to be created?
  bool GenerateOutliers;

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
    vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkPointCloudFilter(const vtkPointCloudFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPointCloudFilter&) VTK_DELETE_FUNCTION;

};

#endif
