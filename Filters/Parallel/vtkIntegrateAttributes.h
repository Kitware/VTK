// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
#include "vtkSmartPointer.h"          // For holding strategy
#include "vtkUnstructuredGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkDataSetAttributes;
class vtkIdList;
class vtkInformation;
class vtkInformationVector;
class vtkIntegrateAttributesFieldList;
class vtkIntegrationStrategy;
class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkIntegrateAttributes : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkIntegrateAttributes* New();
  vtkTypeMacro(vtkIntegrateAttributes, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the parallel controller to use. By default, set to.
   * `vtkMultiProcessController::GlobalController`.
   */
  void SetController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  ///@{
  /**
   * Get/Set the integration strategy.
   */
  void SetIntegrationStrategy(vtkIntegrationStrategy* strategy);
  vtkIntegrationStrategy* GetIntegrationStrategy();
  ///@}

  ///@{
  /**
   * If set to true then the filter will divide all output cell data arrays (the integrated values)
   * by the computed volume/area of the dataset.  Defaults to false.
   */
  vtkSetMacro(DivideAllCellDataByVolume, bool);
  vtkGetMacro(DivideAllCellDataByVolume, bool);
  ///@}

protected:
  vtkIntegrateAttributes();
  ~vtkIntegrateAttributes() override;

  vtkMultiProcessController* Controller;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  // Create a default executive.
  vtkExecutive* CreateDefaultExecutive() override;

  int FillInputPortInformation(int, vtkInformation*) override;

  static int CompareIntegrationDimension(vtkDataSet* output, int dim, double& totalSum,
    double totalSumCenter[3], int& integrationDimension);
  static void ZeroAttributes(vtkDataSetAttributes* outda);

  bool DivideAllCellDataByVolume;

  static void IntegrateSatelliteData(vtkDataSetAttributes* inda, vtkDataSetAttributes* outda);
  int PieceNodeMinToNode0(vtkUnstructuredGrid* data, double& totalSum, double totalSumCenter[3],
    int& integrationDimension);
  void SendPiece(vtkUnstructuredGrid* src, double totalSum, const double totalSumCenter[3],
    int integrationDimension);
  void ReceivePiece(vtkUnstructuredGrid* mergeTo, int fromId, double& totalSum,
    double totalSumCenter[3], int& integrationDimension);

  // This function assumes the data is in the format of the output of this filter with one
  // point/cell having the value computed as its only tuple.  It divides each value by sum,
  // skipping the last data array if requested (so the volume doesn't get divided by itself
  // and set to 1).
  static void DivideDataArraysByConstant(
    vtkDataSetAttributes* data, bool skipLastArray, double sum);

private:
  vtkIntegrateAttributes(const vtkIntegrateAttributes&) = delete;
  void operator=(const vtkIntegrateAttributes&) = delete;
  vtkSmartPointer<vtkIntegrationStrategy> IntegrationStrategy;

  class vtkIntegrateAttributesFunctor;

  static void AllocateAttributes(
    vtkIntegrateAttributesFieldList& fieldList, vtkDataSetAttributes* outda);
  static void InitializeAttributes(vtkDataSetAttributes* outda);
  void ExecuteBlock(vtkDataSet* input, vtkUnstructuredGrid* output, int fieldset_index,
    vtkIntegrateAttributesFieldList& pdList, vtkIntegrateAttributesFieldList& cdList,
    double& totalSum, double totalSumCenter[3], int integrationDimension);

public:
  enum CommunicationIds
  {
    IntegrateAttrInfo = 2000,
    IntegrateAttrData
  };
};

VTK_ABI_NAMESPACE_END
#endif
