/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIntegrateAttributes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkIntegrateAttributes
 * @brief   Integrates lines, surfaces and volume.
 *
 * Integrates all point and cell data attributes while computing
 * length, area or volume.  Works for 1D, 2D or 3D.  Only one dimensionality
 * at a time.  For volume, this filter ignores all but 3D cells.  It
 * will not compute the volume contained in a closed surface.
 * The output of this filter is a single point and vertex.  The attributes
 * for this point and cell will contain the integration results
 * for the corresponding input attributes.
*/

#ifndef vtkIntegrateAttributes_h
#define vtkIntegrateAttributes_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkDataSet;
class vtkIdList;
class vtkInformation;
class vtkInformationVector;
class vtkDataSetAttributes;
class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkIntegrateAttributes : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkIntegrateAttributes* New();
  vtkTypeMacro(vtkIntegrateAttributes, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the parallel controller to use. By default, set to.
   * `vtkMultiProcessController::GlobalController`.
   */
  void SetController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  //@{
  /**
   * If set to true then the filter will divide all output cell data arrays (the integrated values)
   * by the computed volume/area of the dataset.  Defaults to false.
   */
  vtkSetMacro(DivideAllCellDataByVolume, bool);
  vtkGetMacro(DivideAllCellDataByVolume, bool);
  //@}

protected:
  vtkIntegrateAttributes();
  ~vtkIntegrateAttributes() override;

  vtkMultiProcessController* Controller;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  // Create a default executive.
  vtkExecutive* CreateDefaultExecutive() override;

  int FillInputPortInformation(int, vtkInformation*) override;

  int CompareIntegrationDimension(vtkDataSet* output, int dim);
  int IntegrationDimension;

  // The length, area or volume of the data set.  Computed by Execute;
  double Sum;
  // ToCompute the location of the output point.
  double SumCenter[3];

  bool DivideAllCellDataByVolume;

  void IntegratePolyLine(
    vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdList* cellPtIds);
  void IntegratePolygon(
    vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdList* cellPtIds);
  void IntegrateTriangleStrip(
    vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdList* cellPtIds);
  void IntegrateTriangle(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id);
  void IntegrateTetrahedron(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id, vtkIdType pt4Id);
  void IntegratePixel(
    vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdList* cellPtIds);
  void IntegrateVoxel(
    vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdList* cellPtIds);
  void IntegrateGeneral1DCell(
    vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdList* cellPtIds);
  void IntegrateGeneral2DCell(
    vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdList* cellPtIds);
  void IntegrateGeneral3DCell(
    vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId, vtkIdList* cellPtIds);
  void IntegrateSatelliteData(vtkDataSetAttributes* inda, vtkDataSetAttributes* outda);
  void ZeroAttributes(vtkDataSetAttributes* outda);
  int PieceNodeMinToNode0(vtkUnstructuredGrid* data);
  void SendPiece(vtkUnstructuredGrid* src);
  void ReceivePiece(vtkUnstructuredGrid* mergeTo, int fromId);

  // This function assumes the data is in the format of the output of this filter with one
  // point/cell having the value computed as its only tuple.  It divides each value by sum,
  // skipping the last data array if requested (so the volume doesn't get divided by itself
  // and set to 1).
  static void DivideDataArraysByConstant(
    vtkDataSetAttributes* data, bool skipLastArray, double sum);

private:
  vtkIntegrateAttributes(const vtkIntegrateAttributes&) = delete;
  void operator=(const vtkIntegrateAttributes&) = delete;

  class vtkFieldList;
  vtkFieldList* CellFieldList;
  vtkFieldList* PointFieldList;
  int FieldListIndex;

  void AllocateAttributes(vtkFieldList& fieldList, vtkDataSetAttributes* outda);
  void ExecuteBlock(vtkDataSet* input, vtkUnstructuredGrid* output, int fieldset_index,
    vtkFieldList& pdList, vtkFieldList& cdList);

  void IntegrateData1(vtkDataSetAttributes* inda, vtkDataSetAttributes* outda, vtkIdType pt1Id,
    double k, vtkFieldList& fieldlist, int fieldlist_index);
  void IntegrateData2(vtkDataSetAttributes* inda, vtkDataSetAttributes* outda, vtkIdType pt1Id,
    vtkIdType pt2Id, double k, vtkFieldList& fieldlist, int fieldlist_index);
  void IntegrateData3(vtkDataSetAttributes* inda, vtkDataSetAttributes* outda, vtkIdType pt1Id,
    vtkIdType pt2Id, vtkIdType pt3Id, double k, vtkFieldList& fieldlist, int fieldlist_index);
  void IntegrateData4(vtkDataSetAttributes* inda, vtkDataSetAttributes* outda, vtkIdType pt1Id,
    vtkIdType pt2Id, vtkIdType pt3Id, vtkIdType pt4Id, double k, vtkFieldList& fieldlist,
    int fieldlist_index);

public:
  enum CommunicationIds
  {
    IntegrateAttrInfo = 2000,
    IntegrateAttrData
  };
};

#endif
