/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRCutPlane.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMRCutPlane.h -- Cuts an AMR dataset
//
// .SECTION Description
//  A concrete instance of vtkMultiBlockDataSet that provides functionality for
// cutting an AMR dataset (an instance of vtkOverlappingAMR) with user supplied
// implicit plane function defined by a normal and center.

#ifndef VTKAMRCUTPLANE_H_
#define VTKAMRCUTPLANE_H_

#include "vtkFiltersAMRModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

#include <vector> // For STL vector
#include <map> // For STL map

class vtkMultiBlockDataSet;
class vtkOverlappingAMR;
class vtkMultiProcessController;
class vtkInformation;
class vtkInformationVector;
class vtkIndent;
class vtkPlane;
class vtkUniformGrid;
class vtkCell;
class vtkPoints;
class vtkCellArray;
class vtkPointData;
class vtkCellData;

class VTKFILTERSAMR_EXPORT vtkAMRCutPlane : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkAMRCutPlane *New();
  vtkTypeMacro(vtkAMRCutPlane, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream &oss, vtkIndent indent );

  // Description:
  // Sets the center
  vtkSetVector3Macro(Center, double);

  // Description:
  // Sets the normal
  vtkSetVector3Macro(Normal, double);

  // Description:
  // Sets the level of resolution
  vtkSetMacro(LevelOfResolution, int);
  vtkGetMacro(LevelOfResolution, int);

  // Description:
  //
  vtkSetMacro(UseNativeCutter, bool);
  vtkGetMacro(UseNativeCutter, bool);
  vtkBooleanMacro(UseNativeCutter, bool);

  // Description:
  // Set/Get a multiprocess controller for parallel processing.
  // By default this parameter is set to NULL by the constructor.
  vtkSetMacro(Controller, vtkMultiProcessController*);
  vtkGetMacro(Controller, vtkMultiProcessController*);

  // Standard pipeline routines

  virtual int RequestData(
       vtkInformation*,vtkInformationVector**,vtkInformationVector*);
  virtual int FillInputPortInformation(int port, vtkInformation *info);
  virtual int FillOutputPortInformation(int port, vtkInformation *info);


  // Description:
  // Gets the metadata from upstream module and determines which blocks
  // should be loaded by this instance.
  virtual int RequestInformation(
      vtkInformation *rqst,
      vtkInformationVector **inputVector,
      vtkInformationVector *outputVector );

  // Description:
  // Performs upstream requests to the reader
  virtual int RequestUpdateExtent(
      vtkInformation*, vtkInformationVector**, vtkInformationVector* );

protected:
  vtkAMRCutPlane();
  virtual ~vtkAMRCutPlane();

  // Description:
  // Returns the cut-plane defined by a vtkCutPlane instance based on the
  // user-supplied center and normal.
  vtkPlane* GetCutPlane( vtkOverlappingAMR *metadata );

  // Description:
  // Extracts cell
  void ExtractCellFromGrid(
      vtkUniformGrid *grid, vtkCell* cell,
      std::map<vtkIdType,vtkIdType>& gridPntMapping,
      vtkPoints *nodes,
      vtkCellArray *cells );

  // Description:
  // Given the grid and a subset ID pair, grid IDs mapping to the extracted
  // grid IDs, extract the point data.
  void ExtractPointDataFromGrid(
      vtkUniformGrid *grid,
      std::map<vtkIdType,vtkIdType>& gridPntMapping,
      vtkIdType NumNodes,
      vtkPointData *PD );

  // Description:
  // Given the grid and the list of cells that are extracted, extract the
  // corresponding cell data.
  void ExtractCellDataFromGrid(
      vtkUniformGrid *grid,
      std::vector<vtkIdType>& cellIdxList,
      vtkCellData *CD);

  // Description:
  // Given a cut-plane, p, and the metadata, m, this method computes which
  // blocks need to be loaded. The corresponding block IDs are stored in
  // the internal STL vector, blocksToLoad, which is then propagated upstream
  // in the RequestUpdateExtent.
  void ComputeAMRBlocksToLoad( vtkPlane* p, vtkOverlappingAMR* m);

  // Descriription:
  // Initializes the cut-plane center given the min/max bounds.
  void InitializeCenter( double min[3], double max[3] );

  // Description:
  // Determines if a plane intersects with an AMR box
  bool PlaneIntersectsAMRBox( vtkPlane* pl, double bounds[6] );
  bool PlaneIntersectsAMRBox( double plane[4], double bounds[6] );

  // Description:
  // Determines if a plane intersects with a grid cell
  bool PlaneIntersectsCell( vtkPlane *pl, vtkCell *cell );

  // Description:
  // A utility function that checks if the input AMR data is 2-D.
  bool IsAMRData2D( vtkOverlappingAMR *input );

  // Description:
  // Applies cutting to an AMR block
  void CutAMRBlock(
      vtkPlane *cutPlane,
      unsigned int blockIdx,
      vtkUniformGrid *grid, vtkMultiBlockDataSet *dataSet );

  int    LevelOfResolution;
  double Center[3];
  double Normal[3];
  bool initialRequest;
  bool UseNativeCutter;
  vtkMultiProcessController *Controller;

// BTX
  std::vector<int> BlocksToLoad;
// ETX

private:
  vtkAMRCutPlane(const vtkAMRCutPlane& ); // Not implemented
  void operator=(const vtkAMRCutPlane& ); // Not implemented
};

#endif /* VTKAMRCUTPLANE_H_ */
