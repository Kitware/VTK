// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridWarp
 * @brief   Create a deformed copy of the input.
 *
 * This filter accepts a vector-valued cell-attribute (which you should
 * set by calling SetInputAttributeToProcess() with the name of the attribute)
 * and a scale factor.
 */

#ifndef vtkCellGridWarp_h
#define vtkCellGridWarp_h

#include "vtkCellGridAlgorithm.h"
#include "vtkCellGridQuery.h"         // For internal query class.
#include "vtkFiltersCellGridModule.h" // For export macro
#include "vtkNew.h"                   // for ivar

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSCELLGRID_EXPORT vtkCellGridWarp : public vtkCellGridAlgorithm
{
public:
  static vtkCellGridWarp* New();
  vtkTypeMacro(vtkCellGridWarp, vtkCellGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Overridden to include the Request's MTime in addition to our own.
  vtkMTimeType GetMTime() override;

  /**\brief Cell-grid query used to apply deformations to an input shape attribute.
   */
  class Query : public vtkCellGridQuery
  {
  public:
    static Query* New();
    vtkTypeMacro(vtkCellGridWarp::Query, vtkCellGridQuery);
    void PrintSelf(ostream& os, vtkIndent indent) override;

    /// Set/get the vector-valued attribute to apply as a deformation to the input shape.
    virtual void SetDeformationAttribute(vtkCellAttribute* deformation);
    vtkGetObjectMacro(DeformationAttribute, vtkCellAttribute);

    /// Set/get a scale factor applied to the deformation attribute.
    vtkSetMacro(ScaleFactor, double);
    vtkGetMacro(ScaleFactor, double);

  protected:
    ~Query() override;

    vtkCellAttribute* DeformationAttribute{ nullptr };
    double ScaleFactor{ 1.0 };
  };

  /// Set/get a scale factor applied to the deformation attribute.
  virtual void SetScaleFactor(double scaleFactor) { this->Request->SetScaleFactor(scaleFactor); }
  virtual double GetScaleFactor() const { return this->Request->GetScaleFactor(); }

protected:
  vtkCellGridWarp();
  ~vtkCellGridWarp() override = default;

  int RequestData(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* ouInfo) override;

  vtkNew<Query> Request;

private:
  vtkCellGridWarp(const vtkCellGridWarp&) = delete;
  void operator=(const vtkCellGridWarp&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridWarp_h
