/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataMapperNode.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPolyDataMapperNode
 * @brief   vtkViewNode specialized for vtkPolyDataMappers
 *
 * State storage and graph traversal for vtkPolyDataMapper/PolyDataMapper and Property
 * Made a choice to merge PolyDataMapper, PolyDataMapper and property together. If there
 * is a compelling reason to separate them we can.
*/

#ifndef vtkPolyDataMapperNode_h
#define vtkPolyDataMapperNode_h

#include "vtkRenderingSceneGraphModule.h" // For export macro
#include "vtkMapperNode.h"

#include <vector> //for results

class vtkActor;
class vtkPolyDataMapper;
class vtkPolyData;

class VTKRENDERINGSCENEGRAPH_EXPORT vtkPolyDataMapperNode :
  public vtkMapperNode
{
public:
  static vtkPolyDataMapperNode* New();
  vtkTypeMacro(vtkPolyDataMapperNode, vtkMapperNode);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkPolyDataMapperNode();
  ~vtkPolyDataMapperNode();

  //Utilities for children
  /**
   * Makes a cleaned up version of the polydata's geometry in which NaN are removed
   * (substituted with neighbor) and the PolyDataMapper's transformation matrix is applied.
   */
  static void TransformPoints(vtkActor *act,
                              vtkPolyData *poly,
                              std::vector<double> &vertices);

  /**
   * Homogenizes the entire polydata using internal CreateXIndexBuffer functions.
   * They flatten the input polydata's Points, Lines, Polys, and Strips contents into
   * the output arrays. The output "index" arrays contain indices into the Points. The
   * output "reverse" array contains indices into the original CellArray.
   */
  static void MakeConnectivity(vtkPolyData *poly,
                               int representation,
                               std::vector<unsigned int> &vertex_index,
                               std::vector<unsigned int> &vertex_reverse,
                               std::vector<unsigned int> &line_index,
                               std::vector<unsigned int> &line_reverse,
                               std::vector<unsigned int> &triangle_index,
                               std::vector<unsigned int> &triangle_reverse,
                               std::vector<unsigned int> &strip_index,
                               std::vector<unsigned int> &strip_reverse);

 private:
  vtkPolyDataMapperNode(const vtkPolyDataMapperNode&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPolyDataMapperNode&) VTK_DELETE_FUNCTION;
};

#endif
