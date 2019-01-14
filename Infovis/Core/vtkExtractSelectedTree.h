/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedTree.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExtractSelectedTree
 * @brief   return a subtree from a vtkTree
 *
 *
 * input 0 --- a vtkTree
 * input 1 --- a vtkSelection, containing selected vertices. It may have
 * FILED_type set to POINTS ( a vertex selection) or CELLS (an edge selection).
 * A vertex selection preserves the edges that connect selected vertices.
 * An edge selection perserves the vertices that are adjacent to at least one
 * selected edges.
*/

#ifndef vtkExtractSelectedTree_h
#define vtkExtractSelectedTree_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkTreeAlgorithm.h"

class vtkTree;
class vtkIdTypeArray;
class vtkMutableDirectedGraph;

class VTKINFOVISCORE_EXPORT vtkExtractSelectedTree : public vtkTreeAlgorithm
{
public:
  static vtkExtractSelectedTree* New();
  vtkTypeMacro(vtkExtractSelectedTree,vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * A convenience method for setting the second input (i.e. the selection).
   */
  void SetSelectionConnection(vtkAlgorithmOutput* in);

  int FillInputPortInformation(int port, vtkInformation* info) override;
protected:
  vtkExtractSelectedTree();
  ~vtkExtractSelectedTree() override;

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) override;

  int BuildTree(vtkTree * inputTree, vtkIdTypeArray * list, vtkMutableDirectedGraph * builder);


private:
  vtkExtractSelectedTree(const vtkExtractSelectedTree&) = delete;
  void operator=(const vtkExtractSelectedTree&) = delete;
};

#endif
