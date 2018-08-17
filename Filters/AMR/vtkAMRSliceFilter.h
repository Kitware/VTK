/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRSliceFilter.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   vtkAMRSliceFilter
 *
 *
 *  A concrete instance of vtkOverlappingAMRAlgorithm which implements
 *  functionality for extracting slices from AMR data. Unlike the conventional
 *  slice filter, the output of this filter is a 2-D AMR dataset itself.
*/

#ifndef vtkAMRSliceFilter_h
#define vtkAMRSliceFilter_h

#include "vtkFiltersAMRModule.h" // For export macro
#include "vtkOverlappingAMRAlgorithm.h"

#include <vector> // For STL vector

class vtkInformation;
class vtkInformationVector;
class vtkOverlappingAMR;
class vtkMultiProcessController;
class vtkPlane;
class vtkAMRBox;
class vtkUniformGrid;

class VTKFILTERSAMR_EXPORT vtkAMRSliceFilter :
  public vtkOverlappingAMRAlgorithm
{
public:
  static vtkAMRSliceFilter* New();
  vtkTypeMacro( vtkAMRSliceFilter, vtkOverlappingAMRAlgorithm );
  void PrintSelf(ostream &os, vtkIndent indent ) override;

  // Inline Gettters & Setters

  //@{
  /**
   * Set/Get the offset-from-origin of the slicing plane.
   */
  vtkSetMacro(OffsetFromOrigin,double);
  vtkGetMacro(OffsetFromOrigin,double);
  //@}

  //@{
  /**
   * Set/Get the maximum resolution used in this instance.
   */
  vtkSetMacro(MaxResolution,unsigned int);
  vtkGetMacro(MaxResolution,unsigned int);
  //@}

  /**
   * Tags to identify normals along the X, Y and Z directions.
   */
  enum NormalTag : int
  {
    X_NORMAL = 1, Y_NORMAL = 2, Z_NORMAL = 4
  };

  //@{
  /**
   * Set/Get the Axis normal. The accpetable values are defined in the
   * NormalTag enum.
   */
  vtkSetMacro(Normal,int);
  vtkGetMacro(Normal,int);
  //@}

  //@{
  /**
   * Set/Get a multiprocess controller for paralle processing.
   * By default this parameter is set to nullptr by the constructor.
   */
  vtkSetMacro( Controller, vtkMultiProcessController* );
  vtkGetMacro( Controller, vtkMultiProcessController* );
  //@}

  // Standard Pipeline methods
  int RequestData(
     vtkInformation*,vtkInformationVector**,vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation *info) override;
  int FillOutputPortInformation(int port, vtkInformation *info) override;

  /**
   * Makes upstream request to a source, typically, a concrete instance of
   * vtkAMRBaseReader, for which blocks to load.
   */
  int RequestInformation(
      vtkInformation *rqst,
      vtkInformationVector **inputVector,
      vtkInformationVector *outputVector ) override;

  /**
   * Performs upstream requests to the reader
   */
  int RequestUpdateExtent(
      vtkInformation*, vtkInformationVector**,vtkInformationVector* ) override;

protected:
  vtkAMRSliceFilter();
  ~vtkAMRSliceFilter() override;

  /**
   * Returns the cell index w.r.t. the given input grid which contains
   * the query point x. A -1 is returned if the point is not found.
   */
  int GetDonorCellIdx( double x[3], vtkUniformGrid *ug );

  /**
   * Computes the cell center of the cell corresponding to the supplied
   * cell index w.r.t. the input uniform grid.
   */
  void ComputeCellCenter(
      vtkUniformGrid *ug, const int cellIdx, double centroid[3] );

  /**
   * Gets the slice from the given grid given the plane origin & the
   * user-supplied normal associated with this class instance.
   */
  vtkUniformGrid* GetSlice( double origin[3], int* dims, double* gorigin, double* spacing );

  /**
   * Copies the cell data for the cells in the slice from the 3-D grid.
   */
  void GetSliceCellData( vtkUniformGrid *slice, vtkUniformGrid *grid3D );

  /**
   * Determines if a plane intersects with an AMR box
   */
  bool PlaneIntersectsAMRBox( double plane[4], double bounds[6] );

  /**
   * Given the cut-plane and the metadata provided by a module upstream,
   * this method generates the list of linear AMR block indices that need
   * to be loaded.
   */
  void ComputeAMRBlocksToLoad(
      vtkPlane *p, vtkOverlappingAMR *metadata );

  /**
   * Extracts a 2-D AMR slice from the dataset.
   */
  void GetAMRSliceInPlane(
      vtkPlane *p, vtkOverlappingAMR *inp,
      vtkOverlappingAMR *out );

  /**
   * A utility function that checks if the input AMR data is 2-D.
   */
  bool IsAMRData2D( vtkOverlappingAMR *input );

  /**
   * Returns the axis-aligned cut plane.
   */
  vtkPlane* GetCutPlane( vtkOverlappingAMR *input );

  double OffsetFromOrigin;
  int    Normal;
  unsigned int MaxResolution;
  vtkMultiProcessController *Controller;

  std::vector< int > BlocksToLoad;

private:
  vtkAMRSliceFilter( const vtkAMRSliceFilter& ) = delete;
  void operator=( const vtkAMRSliceFilter& ) = delete;
};

#endif /* vtkAMRSliceFilter_h */
