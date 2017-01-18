/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPipelineGraphSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPipelineGraphSource
 * @brief   a graph constructed from a VTK pipeline
 *
 *
*/

#ifndef vtkPipelineGraphSource_h
#define vtkPipelineGraphSource_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkDirectedGraphAlgorithm.h"
#include "vtkStdString.h"

class vtkCollection;

class VTKINFOVISCORE_EXPORT vtkPipelineGraphSource : public vtkDirectedGraphAlgorithm
{
public:
  static vtkPipelineGraphSource* New();
  vtkTypeMacro(vtkPipelineGraphSource,vtkDirectedGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  void AddSink(vtkObject* object);
  void RemoveSink(vtkObject* object);

  /**
   * Generates a GraphViz DOT file that describes the VTK pipeline
   * terminating at the given sink.
   */
  static void PipelineToDot(vtkAlgorithm* sink, ostream& output, const vtkStdString& graph_name = "");
  /**
   * Generates a GraphViz DOT file that describes the VTK pipeline
   * terminating at the given sinks.
   */
  static void PipelineToDot(vtkCollection* sinks, ostream& output, const vtkStdString& graph_name = "");

protected:
  vtkPipelineGraphSource();
  ~vtkPipelineGraphSource() VTK_OVERRIDE;

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) VTK_OVERRIDE;

  vtkCollection* Sinks;

private:
  vtkPipelineGraphSource(const vtkPipelineGraphSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPipelineGraphSource&) VTK_DELETE_FUNCTION;

};

#endif

// VTK-HeaderTest-Exclude: vtkPipelineGraphSource.h
