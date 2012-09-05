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
// .NAME vtkAMRSliceFilter.h -- Creates slices from AMR datasets
//
// .SECTION Description
//  A concrete instance of vtkOverlappingAMRAlgorithm which implements
//  functionality for extracting slices from AMR data. Unlike the conventional
//  slice filter, the output of this filter is a 2-D AMR dataset itself.

#ifndef VTKAMRSLICEFILTER_H_
#define VTKAMRSLICEFILTER_H_

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
  void PrintSelf(ostream &os, vtkIndent indent );

  // Inline Gettters & Setters
  vtkSetMacro(OffSetFromOrigin,double);
  vtkGetMacro(OffSetFromOrigin,double);

  // Description:
  // Set/Get ForwardUpstream property
  vtkSetMacro( ForwardUpstream, int );
  vtkGetMacro( ForwardUpstream, int );
  vtkBooleanMacro( ForwardUpstream, int );

  // Description:
  // Set/Get EnablePrefetching property
  vtkSetMacro( EnablePrefetching, int );
  vtkGetMacro( EnablePrefetching, int );
  vtkBooleanMacro( EnablePrefetching, int );

  // Description:
  // Set/Get the maximum resolution used in this instance.
  vtkSetMacro(MaxResolution,int);
  vtkGetMacro(MaxResolution,int);

  // Description:
  // Set/Get the Axis normal. There are only 3 acceptable values
  // 1-(X-Normal); 2-(Y-Normal); 3-(Z-Normal)
  vtkSetMacro(Normal,int);
  vtkGetMacro(Normal,int);

  // Description:
  // Set/Get a multiprocess controller for paralle processing.
  // By default this parameter is set to NULL by the constructor.
  vtkSetMacro( Controller, vtkMultiProcessController* );
  vtkGetMacro( Controller, vtkMultiProcessController* );

  // Standard Pipeline methods
  virtual int RequestData(
     vtkInformation*,vtkInformationVector**,vtkInformationVector*);
  virtual int FillInputPortInformation(int port, vtkInformation *info);
  virtual int FillOutputPortInformation(int port, vtkInformation *info);

  // Description:
  // Makes upstream request to a source, typically, a concrete instance of
  // vtkAMRBaseReader, for which blocks to load.
  virtual int RequestInformation(
      vtkInformation *rqst,
      vtkInformationVector **inputVector,
      vtkInformationVector *outputVector );

  // Description:
  // Performs upstream requests to the reader
  virtual int RequestUpdateExtent(
      vtkInformation*, vtkInformationVector**,vtkInformationVector* );

protected:
  vtkAMRSliceFilter();
  ~vtkAMRSliceFilter();

  // Description:
  // Returns the cell index w.r.t. the given input grid which contains
  // the query point x. A -1 is returned if the point is not found.
  int GetDonorCellIdx( double x[3], vtkUniformGrid *ug );

  // Description:
  // Computes the cell center of the cell corresponding to the supplied
  // cell index w.r.t. the input uniform grid.
  void ComputeCellCenter(
      vtkUniformGrid *ug, const int cellIdx, double centroid[3] );

  // Description:
  // Gets the slice from the given grid given the plane origin & the
  // user-supplied normal associated with this class instance.
  vtkUniformGrid* GetSlice( double origin[3], int* dims, double* gorigin, double* spacing );

  // Description:
  // Copies the cell data for the cells in the slice from the 3-D grid.
  void GetSliceCellData( vtkUniformGrid *slice, vtkUniformGrid *grid3D );

  // Description:
  // Determines if a plane intersects with an AMR box
  bool PlaneIntersectsAMRBox( double plane[4], double bounds[6] );

  // Description:
  // Given the cut-plane and the metadata provided by a module upstream,
  // this method generates the list of linear AMR block indices that need
  // to be loaded.
  void ComputeAMRBlocksToLoad(
      vtkPlane *p, vtkOverlappingAMR *metadata );

  // Description:
  // Extracts a 2-D AMR slice from the dataset.
  void GetAMRSliceInPlane(
      vtkPlane *p, vtkOverlappingAMR *inp,
      vtkOverlappingAMR *out );

  // Description:
  // A utility function that checks if the input AMR data is 2-D.
  bool IsAMRData2D( vtkOverlappingAMR *input );

  // Description:
  // Returns the axis-aligned cut plane.
  vtkPlane* GetCutPlane( vtkOverlappingAMR *input );

  // Description:
  // Initializes the off-set to be at the center of the input data-set.
  void InitializeOffSet(
    vtkOverlappingAMR *inp, double *min, double *max );

  double OffSetFromOrigin;
  int    Normal; // 1=>X-Normal, 2=>Y-Normal, 3=>Z-Normal
  bool   initialRequest;
  int    MaxResolution;
  vtkMultiProcessController *Controller;

  int ForwardUpstream;
  int EnablePrefetching;
  // BTX
  std::vector< int > BlocksToLoad;
  // ETX

private:
  vtkAMRSliceFilter( const vtkAMRSliceFilter& ); // Not implemented
  void operator=( const vtkAMRSliceFilter& ); // Not implemented
};

#endif /* VTKAMRSLICEFILTER_H_ */
