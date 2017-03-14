/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVoxelGrid.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE file for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVoxelGrid
 * @brief   subsample points using uniform binning
 *
 *
 * vtkVoxelGrid is a filter that subsamples a point cloud based on a regular
 * binning of space. Basically the algorithm operates by dividing space into
 * a volume of M x N x O bins, and then for each bin averaging all of the
 * points positions into a single representative point. Several strategies for
 * computing the binning can be used: 1) manual configuration of a requiring
 * specifying bin dimensions (the bounds are calculated from the data); 2) by
 * explicit specification of the bin size in world coordinates (x-y-z
 * lengths); and 3) an automatic process in which the user specifies an
 * approximate, average number of points per bin and dimensions and bin size
 * are computed automatically. (Note that under the hood a
 * vtkStaticPointLocator is used.)
 *
 * While any vtkPointSet type can be provided as input, the output is
 * represented by an explicit representation of points via a
 * vtkPolyData. This output polydata will populate its instance of vtkPoints,
 * but no cells will be defined (i.e., no vtkVertex or vtkPolyVertex are
 * contained in the output).
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkStaticPointLocator vtkPointCloudFilter vtkQuadricClustering
*/

#ifndef vtkVoxelGrid_h
#define vtkVoxelGrid_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkStaticPointLocator;
class vtkInterpolationKernel;


class VTKFILTERSPOINTS_EXPORT vtkVoxelGrid : public vtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static vtkVoxelGrid *New();
  vtkTypeMacro(vtkVoxelGrid,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  /**
   * This enum is used to configure the operation of the filter.
   */
  enum Style
  {
    MANUAL=0,
    SPECIFY_LEAF_SIZE=1,
    AUTOMATIC=2
  };

  //@{
  /**
   * Configure how the filter is to operate. The user can choose to manually
   * specify the binning volume (by setting its dimensions via MANUAL style); or
   * specify a leaf bin size in the x-y-z directions (SPECIFY_LEAF_SIZE); or
   * in AUTOMATIC style, use a rough average number of points in each bin
   * guide the bin size and binning volume dimensions. By default, AUTOMATIC
   * configuration style is used.
   */
  vtkSetMacro(ConfigurationStyle,int);
  vtkGetMacro(ConfigurationStyle,int);
  void SetConfigurationStyleToManual()
    { this->SetConfigurationStyle(MANUAL); }
  void SetConfigurationStyleToLeafSize()
    { this->SetConfigurationStyle(SPECIFY_LEAF_SIZE); }
  void SetConfigurationStyleToAutomatic()
    { this->SetConfigurationStyle(AUTOMATIC); }
  //@}

  //@{
  /**
   * Set the number of divisions in x-y-z directions (the binning volume
   * dimensions). This data member is used when the configuration style is
   * set to MANUAL.
   */
  vtkSetVector3Macro(Divisions,int);
  vtkGetVectorMacro(Divisions,int,3);
  //@}

  //@{
  /**
   * Set the bin size in the x-y-z directions. This data member is
   * used when the configuration style is set to SPECIFY_LEAF_SIZE. The class will
   * use these x-y-z lengths, within the bounding box of the point cloud,
   * to determine the binning dimensions.
   */
  vtkSetVector3Macro(LeafSize,double);
  vtkGetVectorMacro(LeafSize,double,3);
  //@}

  //@{
  /**
   * Specify the average number of points in each bin. Larger values
   * result in higher rates of subsampling. This data member is used when the
   * configuration style is set to AUTOMATIC. The class will automatically
   * determine the binning dimensions in the x-y-z directions.
   */
  vtkSetClampMacro(NumberOfPointsPerBin,int,1,VTK_INT_MAX);
  vtkGetMacro(NumberOfPointsPerBin,int);
  //@}

  //@{
  /**
   * Specify an interpolation kernel to combine the point attributes. By
   * default a vtkLinearKernel is used (i.e., average values). The
   * interpolation kernel changes the basis of the interpolation.
   */
  void SetKernel(vtkInterpolationKernel *kernel);
  vtkGetObjectMacro(Kernel,vtkInterpolationKernel);
  //@}

protected:
  vtkVoxelGrid();
  ~vtkVoxelGrid() VTK_OVERRIDE;

  vtkStaticPointLocator *Locator;
  int ConfigurationStyle;

  int Divisions[3];
  double LeafSize[3];
  int NumberOfPointsPerBin;
  vtkInterpolationKernel *Kernel;

  int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

private:
  vtkVoxelGrid(const vtkVoxelGrid&) VTK_DELETE_FUNCTION;
  void operator=(const vtkVoxelGrid&) VTK_DELETE_FUNCTION;

};

#endif
