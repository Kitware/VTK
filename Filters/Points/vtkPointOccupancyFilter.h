/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointOccupancyFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPointOccupancyFilter
 * @brief   produce occupancy bit mask from input point cloud
 *
 * vtkPointOccupancyFilter is a filter that generates an occupancy mask on a
 * volume from a point cloud. The output of the filter is an image/volume
 * that indicates for each pixel/voxel whether a point exists within the
 * pixel/voxel.
 *
 * To use this filter, specify an input of type vtkPointSet (i.e., has an
 * explicit representation of points). Then define the occupancy volume by
 * specifying the sample dimensions and bounds. Optionally you can specify
 * unsigned char values used to indicate whether a voxel is empty or occupied.
 *
 * @warning
 * During processing, if a point is found to be outside of the occupancy volume,
 * then it is skipped (i.e., it does not affect the occupancy mask).
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkPointDensityFilter vtkPointMaskFilter
*/

#ifndef vtkPointOccupancyFilter_h
#define vtkPointOccupancyFilter_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkImageAlgorithm.h"

class VTKFILTERSPOINTS_EXPORT vtkPointOccupancyFilter : public vtkImageAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static vtkPointOccupancyFilter *New();
  vtkTypeMacro(vtkPointOccupancyFilter,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Set / get the dimensions of the occupancy volume. Higher values generally
   * produce better results but may be much slower.
   */
  void SetSampleDimensions(int i, int j, int k);
  void SetSampleDimensions(int dim[3]);
  vtkGetVectorMacro(SampleDimensions,int,3);
  //@}

  //@{
  /**
   * Set / get the (xmin,xmax, ymin,ymax, zmin,zmax) bounding box in which
   * the sampling is performed. If any of the (min,max) bounds values are
   * min >= max, then the bounds will be computed automatically from the input
   * data. Otherwise, the user-specified bounds will be used.
   */
  vtkSetVector6Macro(ModelBounds,double);
  vtkGetVectorMacro(ModelBounds,double,6);
  //@}

  //@{
  /**
   * Set / get the values indicating whether a voxel is empty (i.e., does not
   * contain any points) or occupied. By default, an empty voxel has a zero
   * value; an occupied voxel has a value of one.
   */
  vtkSetMacro(EmptyValue,unsigned char);
  vtkGetMacro(EmptyValue,unsigned char);
  vtkSetMacro(OccupiedValue,unsigned char);
  vtkGetMacro(OccupiedValue,unsigned char);
  //@}

protected:
  vtkPointOccupancyFilter();
  ~vtkPointOccupancyFilter();

  int SampleDimensions[3]; // dimensions of volume over which to compute occupancy
  double ModelBounds[6]; // bounding box defining image
  double Origin[3], Spacing[3]; // output geometry
  unsigned char EmptyValue; // what value indicates a voxel is empty
  unsigned char OccupiedValue; // what value indicates a voxel is occupied

  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
  int RequestInformation (vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;
  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;

  void ComputeModelBounds(vtkDataSet *input, vtkImageData *output,
                          vtkInformation *outInfo);

private:
  vtkPointOccupancyFilter(const vtkPointOccupancyFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPointOccupancyFilter&) VTK_DELETE_FUNCTION;
};

#endif
