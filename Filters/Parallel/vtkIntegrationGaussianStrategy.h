// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkIntegrationGaussianStrategy
 * @brief   Gaussian Quadrature Integration Strategy
 *
 * This class inherits from `vtkIntegrationStrategy` and override
 * several function for specific cell types to use the Gaussian Quadrature
 * rule which enables the correct computation of higher order cells and non
 * simplicial shapes. Note that not all cells are properly handled by the
 * strategy, and the computation falls back to the linear strategy for those.
 * Reference to the quadrature method:
 *  https://en.wikipedia.org/wiki/Gaussian_quadrature
 * Reference for quadrature weights computation :
 *  https://www.mm.bme.hu/~gyebro/files/ans_help_v182/ans_thry/thy_et1.html
 * Note that we specifically implement the Gauss-Legendre quadrature
 */

#ifndef vtkIntegrationGaussianStrategy_h
#define vtkIntegrationGaussianStrategy_h

#include "vtkCellType.h"              // For VTKCellType
#include "vtkDataSet.h"               // For Intermediate
#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkIntegrationStrategy.h"
#include "vtkSmartPointer.h" // For Intermediate

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkDataSetAttributes;
class vtkIdTypeArray;
class vtkInformationQuadratureSchemeDefinitionVectorKey;
class vtkIntegrateAttributesFieldList;
class vtkIntegrationLinearStrategy;
class vtkQuadratureSchemeDefinition;
class vtkUnstructuredGrid;

class VTKFILTERSPARALLEL_EXPORT vtkIntegrationGaussianStrategy : public vtkIntegrationStrategy
{
public:
  static vtkIntegrationGaussianStrategy* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkIntegrationGaussianStrategy, vtkIntegrationStrategy);

  ///@{
  /**
   * Integration functions operating on some cell types. The Gaussian strategy is fully implemented.
   */
  void IntegrateQuad(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
    vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id, vtkIdType pt4Id, double& sum,
    double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
    vtkIntegrateAttributesFieldList& pointFieldList, int index) override;

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
  ///@}

  ///@{
  /**
   * Integration functions operating on some cell types. The Gaussian strategy falls back to linear
   * strategy but is strictly equivalent.
   */
  void IntegratePolyLine(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
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
  ///@}

  ///@{
  /**
   * Integration functions operating on some cell types. The Gaussian strategy is not implemented
   * and falls back to linear strategy.
   */
  void IntegratePolygon(vtkDataSet* input, vtkUnstructuredGrid* output, vtkIdType cellId,
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
   * Integrates on data arrays depending on the number of node of the shape.
   */
  void IntegrateData1(vtkDataSetAttributes* inda, vtkDataSetAttributes* outda, vtkIdType pt1Id,
    double k, vtkIntegrateAttributesFieldList& fieldlist, int fieldlist_index) override;

  void IntegrateData2(vtkDataSetAttributes* inda, vtkDataSetAttributes* outda, vtkIdType pt1Id,
    vtkIdType pt2Id, double k, vtkIntegrateAttributesFieldList& fieldlist,
    int fieldlist_index) override;

  void IntegrateData3(vtkDataSetAttributes* inda, vtkDataSetAttributes* outda, vtkIdType pt1Id,
    vtkIdType pt2Id, vtkIdType pt3Id, double k, vtkIntegrateAttributesFieldList& fieldlist,
    int fieldlist_index) override;

  void IntegrateData4(vtkDataSetAttributes* inda, vtkDataSetAttributes* outda, vtkIdType pt1Id,
    vtkIdType pt2Id, vtkIdType pt3Id, vtkIdType pt4Id, double k,
    vtkIntegrateAttributesFieldList& fieldlist, int fieldlist_index) override;
  ///@}

  /**
   * Setup necessary precomputations. In particular, interpolates data fields to quadratures points.
   */
  void Initialize(vtkDataSet* input) override;

protected:
  vtkIntegrationGaussianStrategy() = default;
  ~vtkIntegrationGaussianStrategy() override;

private:
  vtkIntegrationGaussianStrategy(const vtkIntegrationGaussianStrategy&) = delete;
  void operator=(const vtkIntegrationGaussianStrategy&) = delete;

  /**
   * Helper function to integrate using the Gaussian Quadrature on any cell
   */
  void IntegrateGaussian(vtkDataSet* input, vtkUnstructuredGrid* output, VTKCellType celltype,
    vtkIdType cellId, vtkIdType numPts, const vtkIdType* cellPtIds, vtkIdList* cellPtIdsList,
    double& sum, double sumCenter[3], vtkIntegrateAttributesFieldList& cellFieldList,
    vtkIntegrateAttributesFieldList& pointFieldList, int index);

  /**
   * Integrate over the point datas.
   */
  void IntegratePointDataGaussian(vtkDataSetAttributes* outda, vtkIdType cellId,
    int nQuadraturePoints, std::vector<double> partialArea,
    vtkIntegrateAttributesFieldList& fieldList, int index);

  ///@{
  /**
   * Helper functions for Jacobian determinant computation.
   */
  double ComputeJacobianDet2D(const double dN[8], const std::vector<std::array<double, 3>>& pts);
  double ComputeJacobianDet(
    const double* dN, const std::vector<std::array<double, 3>>& pts, int nNodes);
  ///@}

  ///@{
  /**
   * Helper function for the initialization.
   */
  void AddPointDataArray();
  void InitializeQuadratureOffsets();
  std::string GenerateUniqueArrayName(const std::string& baseName);
  vtkSmartPointer<vtkQuadratureSchemeDefinition> CreateQuadratureSchemeDefinition(int cellType);
  void InitializeQuadratureOffsetsArray(
    vtkInformation* info, vtkInformationQuadratureSchemeDefinitionVectorKey* key);
  void ComputeQuadratureInterpolation();
  ///@}

  vtkNew<vtkIntegrationLinearStrategy> LinearStrategy;
  vtkNew<vtkIdTypeArray> Offsets;
  vtkSmartPointer<vtkDataSet> Intermediate = nullptr;
  vtkQuadratureSchemeDefinition** CellDefinitionDictionnary = nullptr;
};

VTK_ABI_NAMESPACE_END
#endif
