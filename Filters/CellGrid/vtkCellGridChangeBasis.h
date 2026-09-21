// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkCellGridChangeBasis_h
#define vtkCellGridChangeBasis_h

#include "vtkCellGridAlgorithm.h"

#include "vtkFiltersCellGrid.h" // For export macro
#include <map>                  // For ivar.

VTK_ABI_NAMESPACE_BEGIN

/**
 * @class   vtkCellGridChangeBasis
 * @brief   Project a cell-attribute's values from one basis to another.
 *
 * vtkCellGridChangeBasis is a filter that converts a cell-attribute's
 * degrees of freedom from one basis to another.
 *
 * The conversion is exact when the target basis spans at least the polynomials
 * the source basis does. The filter refuses combinations where it does not
 * rather than silently approximating, and reports why.
 *
 * For the polynomial order that means:
 * + The target may keep the source's order. The conversion is exact.
 * + The target may raise it. The conversion is still exact, since a space of
 *   higher order contains everything the source can represent.
 * + The target may not lower it. The result would be an approximation of the
 *   input rather than the same function, so the filter refuses instead.
 *
 * The same rule refuses an enriched ("F") source at its own order, which spans
 * more than the complete basis of that order does.
 *
 * The output keeps its degrees of freedom shared between cells when the input
 * does and the two bases place them at the same points, which is the case when
 * a complete basis is converted at its own order. When they do not - because
 * the input is already per-cell, or because an incomplete or enriched basis has
 * a different number of degrees of freedom than the target - each cell is given
 * its own coefficients instead.
 *
 * This filter was designed specifically to convert from HGRAD (Lagrange)
 * basis functions to Bernstein-Bezier basis functions of the same polynomial
 * order - specifically so that downstream processing, such as isocontour
 * computation, can use the convex hull and (for curves) variation diminishing
 * properties of the Bernstein basis to simplify their work.
 */
class VTKFILTERSCELLGRID_EXPORT vtkCellGridChangeBasis : public vtkCellGridAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information, and
   * printing.
   */
  static vtkCellGridChangeBasis* New();
  vtkTypeMacro(vtkCellGridChangeBasis, vtkCellGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Return the MTime also considering the query.
   */
  vtkMTimeType GetMTime() override;

  /** @class Query
   * @brief A cell-grid query for changing cell-attribute basis functions.
   */
  class Query : public vtkCellGridQuery
  {
  public:
    ///@{
    /**
     * Standard methods for instantiation, obtaining type information, and
     * printing.
     */
    static Query* New();
    vtkTypeMacro(vtkCellGridChangeBasis::Query, vtkCellGridQuery);
    void PrintSelf(ostream& os, vtkIndent indent) override;
    ///@}

    /// Construct a query.
    Query();
    ~Query() override;

    /**
     * Return the MTime also considering the cell-attribute.
     */
    vtkMTimeType GetMTime() override;

    ///@{
    /**
     * Specify the cell-attribute to be converted.
     */
    virtual void SetCellAttribute(vtkCellAttribute*);
    vtkGetObjectMacro(CellAttribute, vtkCellAttribute);
    ///@}

    ///@{
    /**
     * Set/get the function space that should be converted.
     */
    vtkSetStringTokenMacro(SourceFunctionSpace);
    vtkGetStringTokenMacro(SourceFunctionSpace);
    ///@}

    ///@{
    /**
     * Set/get the basis that should be converted.
     */
    vtkSetStringTokenMacro(SourceBasis);
    vtkGetStringTokenMacro(SourceBasis);
    ///@}

    ///@{
    /**
     * Set/get the order that should be converted.
     */
    vtkSetMacro(SourceOrder, int);
    vtkGetMacro(SourceOrder, int);
    ///@}

    ///@{
    /**
     * Set/get the destination function space.
     */
    vtkSetStringTokenMacro(TargetFunctionSpace);
    vtkGetStringTokenMacro(TargetFunctionSpace);
    ///@}

    ///@{
    /**
     * Set/get the destination basis.
     */
    vtkSetStringTokenMacro(TargetBasis);
    vtkGetStringTokenMacro(TargetBasis);
    ///@}

    ///@{
    /**
     * Set/get the order that should be converted.
     */
    vtkSetMacro(TargetOrder, int);
    vtkGetMacro(TargetOrder, int);
    ///@}

    /// A convenience method for responders to create arrays at
    /// the requested output precision given an \a input array.
    vtkDataArray* CreateNewDataArray(vtkDataArray* input) const;

    /// Output arrays holding shared degrees of freedom, keyed by the input array
    /// each one replaces.
    ///
    /// A responder handles one cell type at a time, but several cell types may
    /// share one group of degrees of freedom. They must then write into a single
    /// output array, so the first responder to need one records it here and the
    /// rest reuse it.
    std::map<vtkDataArray*, vtkSmartPointer<vtkDataArray>>& GetSharedOutputs()
    {
      return this->SharedOutputs;
    }

    /// Forget the arrays from any previous invocation.
    bool Initialize() override;

  protected:
    vtkCellAttribute* CellAttribute{ nullptr };
    vtkStringToken SourceFunctionSpace;
    vtkStringToken SourceBasis;
    int SourceOrder{ -1 };
    vtkStringToken TargetFunctionSpace;
    vtkStringToken TargetBasis;
    int TargetOrder{ -1 };
    int OutputPointsPrecision{ vtkAlgorithm::DesiredOutputPrecision::DEFAULT_PRECISION };
    std::map<vtkDataArray*, vtkSmartPointer<vtkDataArray>> SharedOutputs;
  };

  ///@{
  /// Methods on the algorithm that simply forward data to the request.
  void SetCellAttribute(vtkCellAttribute* att);
  void SetSourceFunctionSpace(const char* functionSpace);
  void SetSourceBasis(const char* basis);
  void SetSourceOrder(int order);
  void SetTargetFunctionSpace(const char* functionSpace);
  void SetTargetBasis(const char* basis);
  void SetTargetOrder(int order);
  ///@}

protected:
  vtkCellGridChangeBasis();
  ~vtkCellGridChangeBasis() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkNew<Query> Request;

private:
  vtkCellGridChangeBasis(const vtkCellGridChangeBasis&) = delete;
  void operator=(const vtkCellGridChangeBasis&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
