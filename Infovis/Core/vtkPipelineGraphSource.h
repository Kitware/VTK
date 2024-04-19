// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPipelineGraphSource
 * @brief   a graph constructed from a VTK pipeline
 *
 *
 */

#ifndef vtkPipelineGraphSource_h
#define vtkPipelineGraphSource_h

#include "vtkDirectedGraphAlgorithm.h"
#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkStdString.h"         // for vtkStdString

VTK_ABI_NAMESPACE_BEGIN
class vtkCollection;

class VTKINFOVISCORE_EXPORT vtkPipelineGraphSource : public vtkDirectedGraphAlgorithm
{
public:
  static vtkPipelineGraphSource* New();
  vtkTypeMacro(vtkPipelineGraphSource, vtkDirectedGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void AddSink(vtkObject* sink);
  void RemoveSink(vtkObject* sink);

  /**
   * Generates a GraphViz DOT file that describes the VTK pipeline
   * terminating at the given sink.
   */
  static void PipelineToDot(
    vtkAlgorithm* sink, ostream& output, const vtkStdString& graph_name = "");
  /**
   * Generates a GraphViz DOT file that describes the VTK pipeline
   * terminating at the given sinks.
   */
  static void PipelineToDot(
    vtkCollection* sinks, ostream& output, const vtkStdString& graph_name = "");

protected:
  vtkPipelineGraphSource();
  ~vtkPipelineGraphSource() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkCollection* Sinks;

private:
  vtkPipelineGraphSource(const vtkPipelineGraphSource&) = delete;
  void operator=(const vtkPipelineGraphSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
