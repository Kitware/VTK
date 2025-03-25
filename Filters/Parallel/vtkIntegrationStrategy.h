// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkIntegrationStrategy
 * @brief   abstract class to define an integration strategy for vtkIntegrateAttributes
 *
 * Parent class for the integration strategies. It defines a pure virtual methods
 * to integrate over each handled cell type.
 */

#ifndef vtkIntegrationStrategy_h
#define vtkIntegrationStrategy_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkDataSetAttributes;
class vtkGenericCell;
class vtkIdList;
class vtkIntegrateAttributesFieldList;
class vtkUnstructuredGrid;

class VTKFILTERSPARALLEL_EXPORT vtkIntegrationStrategy : public vtkObject
{
public:
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkIntegrationStrategy, vtkObject);

  ///@{
  /**
   * Function to integrate the specified cell type
   */
  virtual void IntegratePolyLine(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3],
    vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
    int index) = 0;

  virtual void IntegratePolygon(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3],
    vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
    int index) = 0;

  virtual void IntegrateTriangleStrip(vtkDataSet* input, vtkUnstructuredGrid* output,
    vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds, double& sum,
    double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
    vtkIntegrateAttributesFieldList& pointFieldList, int index) = 0;

  virtual void IntegrateTriangle(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id, double& sum, double sumCenter[3],
    vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
    int index) = 0;

  virtual void IntegrateQuad(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id, vtkIdType pt4Id, double& sum,
    double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
    vtkIntegrateAttributesFieldList& pointFieldList, int index) = 0;

  virtual void IntegrateTetrahedron(vtkDataSet* input, vtkUnstructuredGrid* output,
    vtkIdType cellId, vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id, vtkIdType pt4Id,
    double& sum, double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
    vtkIntegrateAttributesFieldList& pointFieldList, int index) = 0;

  virtual void IntegratePixel(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3],
    vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
    int index) = 0;

  virtual void IntegrateVoxel(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType numPts, const vtkIdType* cellPtIds, double& sum, double sumCenter[3],
    vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
    int index) = 0;

  virtual void IntegrateHexahedron(vtkDataSet* input, vtkUnstructuredGrid* output,
    vtkGenericCell* cell, vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds,
    vtkIdList* cellPtIdsList, double& sum, double sumCenter[3],
    vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
    int index) = 0;

  virtual void IntegrateWedge(vtkDataSet* input, vtkUnstructuredGrid* output, vtkGenericCell* cell,
    vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds, vtkIdList* cellPtIdsList,
    double& sum, double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
    vtkIntegrateAttributesFieldList& pointFieldList, int index) = 0;

  virtual void IntegratePyramid(vtkDataSet* input, vtkUnstructuredGrid* output,
    vtkGenericCell* cell, vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds,
    vtkIdList* cellPtIdsList, double& sum, double sumCenter[3],
    vtkIntegrateAttributesFieldList& cellFieldList, vtkIntegrateAttributesFieldList& pointFieldList,
    int index) = 0;

  virtual void IntegrateGeneral1DCell(vtkDataSet* input, vtkUnstructuredGrid* output,
    vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds, double& sum,
    double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
    vtkIntegrateAttributesFieldList& pointFieldList, int index) = 0;

  virtual void IntegrateGeneral2DCell(vtkDataSet* input, vtkUnstructuredGrid* output,
    vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds, double& sum,
    double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
    vtkIntegrateAttributesFieldList& pointFieldList, int index) = 0;

  virtual void IntegrateGeneral3DCell(vtkDataSet* input, vtkUnstructuredGrid* output,
    vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds, double& sum,
    double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
    vtkIntegrateAttributesFieldList& pointFieldList, int index) = 0;

  virtual void IntegrateDefault(vtkDataSet* input, vtkUnstructuredGrid* output,
    vtkGenericCell* cell, vtkIdType cellId, vtkIdType numPts, vtkIdList* cellPtIds, double& sum,
    double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
    vtkIntegrateAttributesFieldList& pointFieldList, int index) = 0;
  ///@}

  ///@{
  /**
   * Integrates on data arrays depending on the number of node of the shape
   */
  virtual void IntegrateData1(vtkDataSetAttributes* inda, vtkDataSetAttributes* outda,
    vtkIdType pt1Id, double k, vtkIntegrateAttributesFieldList& fieldlist, int fieldlist_index) = 0;

  virtual void IntegrateData2(vtkDataSetAttributes* inda, vtkDataSetAttributes* outda,
    vtkIdType pt1Id, vtkIdType pt2Id, double k, vtkIntegrateAttributesFieldList& fieldlist,
    int fieldlist_index) = 0;

  virtual void IntegrateData3(vtkDataSetAttributes* inda, vtkDataSetAttributes* outda,
    vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id, double k,
    vtkIntegrateAttributesFieldList& fieldlist, int fieldlist_index) = 0;

  virtual void IntegrateData4(vtkDataSetAttributes* inda, vtkDataSetAttributes* outda,
    vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id, vtkIdType pt4Id, double k,
    vtkIntegrateAttributesFieldList& fieldlist, int fieldlist_index) = 0;
  ///@}

  /**
   * Setup necessary precomputations
   * Here does nothing, can be overriden if needed
   */
  virtual void Initialize(vtkDataSet* input);

protected:
  vtkIntegrationStrategy() = default;
  ~vtkIntegrationStrategy() override = default;

private:
  vtkIntegrationStrategy(const vtkIntegrationStrategy&) = delete;
  void operator=(const vtkIntegrationStrategy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
