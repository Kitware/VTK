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
// .NAME vtkPipelineGraphSource - a graph constructed from a VTK pipeline
//
// .SECTION Description

#ifndef __vtkPipelineGraphSource_h
#define __vtkPipelineGraphSource_h

#include "vtkDirectedGraphAlgorithm.h"
#include <vtkStdString.h>

class vtkCollection;

class VTK_VIEWS_EXPORT vtkPipelineGraphSource : public vtkDirectedGraphAlgorithm
{
public:
  static vtkPipelineGraphSource* New();
  vtkTypeMacro(vtkPipelineGraphSource,vtkDirectedGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  void AddSink(vtkObject* object);
  void RemoveSink(vtkObject* object);

//BTX
  // Description:
  // Generates a GraphViz DOT file that describes the VTK pipeline
  // terminating at the given sink.
  static void PipelineToDot(vtkAlgorithm* sink, ostream& output, const vtkStdString& graph_name = "");
  // Description:
  // Generates a GraphViz DOT file that describes the VTK pipeline
  // terminating at the given sinks.
  static void PipelineToDot(vtkCollection* sinks, ostream& output, const vtkStdString& graph_name = "");

protected:
  vtkPipelineGraphSource();
  ~vtkPipelineGraphSource();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

  vtkCollection* Sinks;

private:
  vtkPipelineGraphSource(const vtkPipelineGraphSource&); // Not implemented
  void operator=(const vtkPipelineGraphSource&);   // Not implemented
//ETX
};

#endif

