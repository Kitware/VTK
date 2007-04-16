/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedGraph.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractSelectedGraph - return a subgraph of a vtkGraph
//
// .SECTION Description
// The first input is a vtkGraph to take a subgraph from.
// The second input is a vtkSelection containing the selected vertex indices.
//

#ifndef __vtkExtractSelectedGraph_h
#define __vtkExtractSelectedGraph_h

#include "vtkGraphAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkExtractSelectedGraph : public vtkGraphAlgorithm
{
public:
  static vtkExtractSelectedGraph* New();
  vtkTypeRevisionMacro(vtkExtractSelectedGraph,vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // A convenience method for setting the second input (i.e. the selection).
  void SetSelectionConnection(vtkAlgorithmOutput* in);

  // Description:
  // Specify the first vtkGraph input and the second vtkSelection input.
  int FillInputPortInformation(int port, vtkInformation* info);

protected:
  vtkExtractSelectedGraph();
  ~vtkExtractSelectedGraph();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkExtractSelectedGraph(const vtkExtractSelectedGraph&); // Not implemented
  void operator=(const vtkExtractSelectedGraph&);   // Not implemented
};

#endif

