// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridCellCenters
 * @brief   Output a vertex cell at the center of each input cell.
 *
 * Attributes common to all input cell-types should be interpolated to each output vertex.
 *
 * Because the query is simple, it is simply a child class of
 * the algorithm.
 */

#ifndef vtkCellGridCellCenters_h
#define vtkCellGridCellCenters_h

#include "vtkCellGridAlgorithm.h"
#include "vtkFiltersCellGridModule.h" // For export macro
#include "vtkNew.h"                   // for ivar

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSCELLGRID_EXPORT vtkCellGridCellCenters : public vtkCellGridAlgorithm
{
public:
  /// A query corresponding to this algorithm.
  ///
  /// This query gets run on the input cell-grid.
  class Query : public vtkCellGridQuery
  {
  public:
    static Query* New();
    vtkTypeMacro(vtkCellGridCellCenters::Query, vtkCellGridQuery);
    void PrintSelf(ostream& os, vtkIndent indent) override;

    /// A map used to allocate output vertex cells for various input cell types.
    using OutputAllocations =
      std::unordered_map<vtkStringToken, std::unordered_map<vtkStringToken, vtkIdType>>;

    enum PassType : int
    {
      CountOutputs = 0,
      AllocateOutputs = 1,
      GenerateOutputs = 2
    };

    bool Initialize() override;

    /// Force three passes through this query.
    bool IsAnotherPassRequired() override { return this->Pass < PassType::GenerateOutputs; }

    /// Get the request's output cell-grid.
    vtkCellGrid* GetOutput() const { return this->Output; }
    /// Get the request's input cell-grid.
    vtkCellGrid* GetInput() const { return this->Input; }

    /// Responders should call this method during the CountOutputs pass to
    /// allocate space in an output cell type.
    void AddOutputCenters(
      vtkStringToken inputCellType, vtkStringToken outputCellType, vtkIdType numberOfOutputs);

    /// Return the data structure that AddOutputCenters() modifies in
    /// the PassType::CountOutputs pass.
    OutputAllocations& GetOutputAllocations() { return this->OutputOffsets; }
    const OutputAllocations& GetOutputAllocations() const { return this->OutputOffsets; }

    /// Return an output attribute (or null).
    vtkCellAttribute* GetOutputAttribute(vtkCellAttribute* inputAttribute);

  protected:
    friend class vtkCellGridCellCenters;
    Query() = default;
    ~Query() override = default;

    vtkCellGrid* Input{ nullptr };
    vtkCellGrid* Output{ nullptr };

    /// Map output cell-typename to input cell-typename to count (offset after the
    /// AllocateOutputs pass has run).
    OutputAllocations OutputOffsets;
    // Map input to output attributes:
    std::unordered_map<vtkCellAttribute*, vtkCellAttribute*> AttributeMap;

  private:
    Query(const Query&) = delete;
    void operator=(const Query&) = delete;
  };

  static vtkCellGridCellCenters* New();
  vtkTypeMacro(vtkCellGridCellCenters, vtkCellGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkCellGridCellCenters();
  ~vtkCellGridCellCenters() override = default;

  int RequestData(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* ouInfo) override;

  vtkNew<Query> Request;

private:
  vtkCellGridCellCenters(const vtkCellGridCellCenters&) = delete;
  void operator=(const vtkCellGridCellCenters&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridCellCenters_h
