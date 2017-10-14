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
/**
 * @class   vtkAMRCutPlane
 *
 *
 *  A concrete instance of vtkMultiBlockDataSet that provides functionality for
 * cutting an AMR dataset (an instance of vtkOverlappingAMR) with user supplied
 * implicit plane function defined by a normal and center.
*/

#ifndef vtkAMRCutPlane_h
#define vtkAMRCutPlane_h

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
  void PrintSelf(ostream &oss, vtkIndent indent ) override;

  //@{
  /**
   * Sets the center
   */
  vtkSetVector3Macro(Center, double);
  //@}

  //@{
  /**
   * Sets the normal
   */
  vtkSetVector3Macro(Normal, double);
  //@}

  //@{
  /**
   * Sets the level of resolution
   */
  vtkSetMacro(LevelOfResolution, int);
  vtkGetMacro(LevelOfResolution, int);
  //@}

  //@{
  /**

   */
  vtkSetMacro(UseNativeCutter, bool);
  vtkGetMacro(UseNativeCutter, bool);
  vtkBooleanMacro(UseNativeCutter, bool);
  //@}

  //@{
  /**
   * Set/Get a multiprocess controller for parallel processing.
   * By default this parameter is set to nullptr by the constructor.
   */
  vtkSetMacro(Controller, vtkMultiProcessController*);
  vtkGetMacro(Controller, vtkMultiProcessController*);
  //@}

  // Standard pipeline routines

  int RequestData(
       vtkInformation*,vtkInformationVector**,vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation *info) override;
  int FillOutputPortInformation(int port, vtkInformation *info) override;


  /**
   * Gets the metadata from upstream module and determines which blocks
   * should be loaded by this instance.
   */
  int RequestInformation(
      vtkInformation *rqst,
      vtkInformationVector **inputVector,
      vtkInformationVector *outputVector ) override;

  /**
   * Performs upstream requests to the reader
   */
  int RequestUpdateExtent(
      vtkInformation*, vtkInformationVector**, vtkInformationVector* ) override;

protected:
  vtkAMRCutPlane();
  ~vtkAMRCutPlane() override;

  /**
   * Returns the cut-plane defined by a vtkCutPlane instance based on the
   * user-supplied center and normal.
   */
  vtkPlane* GetCutPlane( vtkOverlappingAMR *metadata );

  /**
   * Extracts cell
   */
  void ExtractCellFromGrid(
      vtkUniformGrid *grid, vtkCell* cell,
      std::map<vtkIdType,vtkIdType>& gridPntMapping,
      vtkPoints *nodes,
      vtkCellArray *cells );

  /**
   * Given the grid and a subset ID pair, grid IDs mapping to the extracted
   * grid IDs, extract the point data.
   */
  void ExtractPointDataFromGrid(
      vtkUniformGrid *grid,
      std::map<vtkIdType,vtkIdType>& gridPntMapping,
      vtkIdType NumNodes,
      vtkPointData *PD );

  /**
   * Given the grid and the list of cells that are extracted, extract the
   * corresponding cell data.
   */
  void ExtractCellDataFromGrid(
      vtkUniformGrid *grid,
      std::vector<vtkIdType>& cellIdxList,
      vtkCellData *CD);

  /**
   * Given a cut-plane, p, and the metadata, m, this method computes which
   * blocks need to be loaded. The corresponding block IDs are stored in
   * the internal STL vector, blocksToLoad, which is then propagated upstream
   * in the RequestUpdateExtent.
   */
  void ComputeAMRBlocksToLoad( vtkPlane* p, vtkOverlappingAMR* m);

  // Descriription:
  // Initializes the cut-plane center given the min/max bounds.
  void InitializeCenter( double min[3], double max[3] );

  //@{
  /**
   * Determines if a plane intersects with an AMR box
   */
  bool PlaneIntersectsAMRBox( vtkPlane* pl, double bounds[6] );
  bool PlaneIntersectsAMRBox( double plane[4], double bounds[6] );
  //@}

  /**
   * Determines if a plane intersects with a grid cell
   */
  bool PlaneIntersectsCell( vtkPlane *pl, vtkCell *cell );

  /**
   * A utility function that checks if the input AMR data is 2-D.
   */
  bool IsAMRData2D( vtkOverlappingAMR *input );

  /**
   * Applies cutting to an AMR block
   */
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

  std::vector<int> BlocksToLoad;

private:
  vtkAMRCutPlane(const vtkAMRCutPlane& ) = delete;
  void operator=(const vtkAMRCutPlane& ) = delete;
};

#endif /* vtkAMRCutPlane_h */
