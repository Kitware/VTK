// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridTransform
 * @brief   transform points and associated normals and vectors
 *
 * vtkCellGridTransform is a filter that applies a transform to input cells
 * and generates transformed output cells.
 * Associated vector and tensor attributes may also be transformed.
 *
 * @sa
 * vtkAbstractTransform
 */

#ifndef vtkCellGridTransform_h
#define vtkCellGridTransform_h

#include "vtkCellGridAlgorithm.h"
#include "vtkFiltersCellGrid.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractTransform;

class VTKFILTERSCELLGRID_EXPORT vtkCellGridTransform : public vtkCellGridAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information, and
   * printing.
   */
  static vtkCellGridTransform* New();
  vtkTypeMacro(vtkCellGridTransform, vtkCellGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Return the MTime also considering the query.
   */
  vtkMTimeType GetMTime() override;

  /** @class Query
   * @brief A cell-grid query for applying a transform to a cell-grid.
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
    vtkTypeMacro(vtkCellGridTransform::Query, vtkCellGridQuery);
    void PrintSelf(ostream& os, vtkIndent indent) override;
    ///@}

    /// Construct a query.
    Query();
    ~Query() override;

    /**
     * Return the MTime also considering the transform.
     */
    vtkMTimeType GetMTime() override;

    ///@{
    /**
     * Specify the transform object used to transform points.
     */
    virtual void SetTransform(vtkAbstractTransform*);
    vtkGetObjectMacro(Transform, vtkAbstractTransform);
    ///@}

    ///@{
    /**
     * Specify the cell-attribute to be transformed.
     */
    virtual void SetCellAttribute(vtkCellAttribute*);
    vtkGetObjectMacro(CellAttribute, vtkCellAttribute);
    ///@}

    ///@{
    /**
     * Set/get the desired precision for the output types. See the documentation
     * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
     * the available precision settings.
     */
    vtkSetMacro(OutputPointsPrecision, int);
    vtkGetMacro(OutputPointsPrecision, int);
    ///@}

    // bool Initialize() override;
    // bool Finalize() override;

    /// A convenience method for responders to create arrays at
    /// the requested output precision given an \a input array.
    vtkDataArray* CreateNewDataArray(vtkDataArray* input) const;

  protected:
    vtkCellAttribute* CellAttribute{ nullptr };
    vtkAbstractTransform* Transform{ nullptr };
    int OutputPointsPrecision{ vtkAlgorithm::DesiredOutputPrecision::DEFAULT_PRECISION };
  };

  void SetTransform(vtkAbstractTransform* tfm);
  void SetCellAttribute(vtkCellAttribute* att);

protected:
  vtkCellGridTransform();
  ~vtkCellGridTransform() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkNew<Query> Request;

private:
  vtkCellGridTransform(const vtkCellGridTransform&) = delete;
  void operator=(const vtkCellGridTransform&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
