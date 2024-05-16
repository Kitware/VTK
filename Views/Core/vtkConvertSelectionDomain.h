// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkConvertSelectionDomain
 * @brief   Convert a selection from one domain to another
 *
 *
 * vtkConvertSelectionDomain converts a selection from one domain to another
 * using known domain mappings. The domain mappings are described by a
 * vtkMultiBlockDataSet containing one or more vtkTables.
 *
 * The first input port is for the input selection (or collection of annotations
 * in a vtkAnnotationLayers object), while the second port
 * is for the multi-block of mappings, and the third port is for the
 * data that is being selected on.
 *
 * If the second or third port is not set, this filter will pass the
 * selection/annotation to the output unchanged.
 *
 * The second output is the selection associated with the "current annotation"
 * normally representing the current interactive selection.
 */

#ifndef vtkConvertSelectionDomain_h
#define vtkConvertSelectionDomain_h

#include "vtkPassInputTypeAlgorithm.h"
#include "vtkViewsCoreModule.h" // For export macro
#include "vtkWrappingHints.h"   // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkAnnotation;

class VTKVIEWSCORE_EXPORT VTK_MARSHALAUTO vtkConvertSelectionDomain
  : public vtkPassInputTypeAlgorithm
{
public:
  static vtkConvertSelectionDomain* New();
  vtkTypeMacro(vtkConvertSelectionDomain, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkConvertSelectionDomain();
  ~vtkConvertSelectionDomain() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;

private:
  vtkConvertSelectionDomain(const vtkConvertSelectionDomain&) = delete;
  void operator=(const vtkConvertSelectionDomain&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
