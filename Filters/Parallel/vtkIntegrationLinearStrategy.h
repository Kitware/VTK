// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkIntegrationLinearStrategy
 * @brief   Linear Integration Strategy
 *
 * This class inherits from `vtkIntegrationStrategy` and its integration
 * strategy assumes each cell to be linear. If a cell is not a simplex,
 * it divides the shape into simplices.
 */

#ifndef vtkIntegrationLinearStrategy_h
#define vtkIntegrationLinearStrategy_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkIntegrationStrategy.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkDataSetAttributes;
class vtkGenericCell;
class vtkIntegrateAttributesFieldList;
class vtkUnstructuredGrid;

class VTKFILTERSPARALLEL_EXPORT vtkIntegrationLinearStrategy : public vtkIntegrationStrategy
{
public:
  static vtkIntegrationLinearStrategy* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkIntegrationLinearStrategy, vtkIntegrationStrategy);

  ///@{
  /**
   * Integration functions operating on many cell types
   */
  void IntegratePolyLine(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3],
    vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
    int index) override;

  void IntegratePolygon(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3],
    vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
    int index) override;

  void IntegrateTriangleStrip(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3],
    vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
    int index) override;

  void IntegrateTriangle(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id, double& sum, double sumCenter[3],
    vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
    int index) override;

  void IntegrateQuad(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id, vtkIdType pt4Id, double& sum,
    double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
    vtkIntegrateAttributesFieldList& pointFieldList, int index) override;

  void IntegrateTetrahedron(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id, vtkIdType pt4Id, double& sum,
    double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
    vtkIntegrateAttributesFieldList& pointFieldList, int index) override;

  void IntegratePixel(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3],
    vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
    int index) override;

  void IntegrateVoxel(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3],
    vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
    int index) override;

  void IntegrateHexahedron(vtkDataSet* input, vtkUnstructuredGrid* output, vtkGenericCell* cell,
    vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds, vtkIdList* cellPtIdsList,
    double& sum, double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
    vtkIntegrateAttributesFieldList& pointFieldList, int index) override;

  void IntegrateWedge(vtkDataSet* input, vtkUnstructuredGrid* output, vtkGenericCell* cell,
    vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds, vtkIdList* cellPtIdsList,
    double& sum, double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
    vtkIntegrateAttributesFieldList& pointFieldList, int index) override;

  void IntegratePyramid(vtkDataSet* input, vtkUnstructuredGrid* output, vtkGenericCell* cell,
    vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds, vtkIdList* cellPtIdsList,
    double& sum, double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
    vtkIntegrateAttributesFieldList& pointFieldList, int index) override;

  void IntegrateGeneral1DCell(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3],
    vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
    int index) override;

  void IntegrateGeneral2DCell(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3],
    vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
    int index) override;

  void IntegrateGeneral3DCell(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3],
    vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
    int index) override;

  void IntegrateDefault(vtkDataSet* input, vtkUnstructuredGrid* output, vtkGenericCell* cell,
    vtkIdType cellId, vtkIdType numPts, vtkIdList* cellPtIds, double& sum, double sumCenter[3],
    vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
    int index) override;
  ///@}

  ///@{
  /**
   * Integrates on data arrays depending on the number of node of the shape
   */
  void IntegrateData1(vtkDataSetAttributes* inda, vtkDataSetAttributes* outda, vtkIdType pt1Id,
    double volume, vtkIntegrateAttributesFieldList& fieldlist, int fieldlist_index) override;

  void IntegrateData2(vtkDataSetAttributes* inda, vtkDataSetAttributes* outda, vtkIdType pt1Id,
    vtkIdType pt2Id, double volume, vtkIntegrateAttributesFieldList& fieldlist,
    int fieldlist_index) override;

  void IntegrateData3(vtkDataSetAttributes* inda, vtkDataSetAttributes* outda, vtkIdType pt1Id,
    vtkIdType pt2Id, vtkIdType pt3Id, double volume, vtkIntegrateAttributesFieldList& fieldlist,
    int fieldlist_index) override;

  void IntegrateData4(vtkDataSetAttributes* inda, vtkDataSetAttributes* outda, vtkIdType pt1Id,
    vtkIdType pt2Id, vtkIdType pt3Id, vtkIdType pt4Id, double volume,
    vtkIntegrateAttributesFieldList& fieldlist, int fieldlist_index) override;
  ///@}

protected:
  vtkIntegrationLinearStrategy() = default;
  ~vtkIntegrationLinearStrategy() override = default;

private:
  vtkIntegrationLinearStrategy(const vtkIntegrationLinearStrategy&) = delete;
  void operator=(const vtkIntegrationLinearStrategy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
