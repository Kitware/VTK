// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDateToNumeric
 * @brief   Converts string dates to numeric values
 *
 *
 * This filter preserves all the topology of the input. All string arrays are
 * examined to see if their value is a date. If so an array is added with the
 * numeric value of that date. The new array is of type double and its name
 * is the source arrays name with _numeric appended.
 *
 * default date formats parsed include
 *
 *   "%Y-%m-%d %H:%M:%S"
 *   "%d/%m/%Y %H:%M:%S"
 */

#ifndef vtkDateToNumeric_h
#define vtkDateToNumeric_h

#include "vtkDataObject.h"           // for vtkDataObject::FieldAssociations
#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"
#include "vtkSmartPointer.h" // for ivar

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkDateToNumeric : public vtkPassInputTypeAlgorithm
{
public:
  static vtkDateToNumeric* New();
  vtkTypeMacro(vtkDateToNumeric, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * You can specify your own format to parse dates by. This string
   * follows the formatting conventions of std::get_time
   */
  vtkGetStringMacro(DateFormat);
  vtkSetStringMacro(DateFormat);
  ///@}

protected:
  vtkDateToNumeric();
  ~vtkDateToNumeric() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  char* DateFormat;

private:
  vtkDateToNumeric(const vtkDateToNumeric&) = delete;
  void operator=(const vtkDateToNumeric&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
