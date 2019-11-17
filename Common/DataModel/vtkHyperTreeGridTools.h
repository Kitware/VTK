/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridTools.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright Nonice for more information.

=========================================================================*/

#ifndef vtkHyperTreeGridTools_h
#define vtkHyperTreeGridTools_h

namespace vtk
{
namespace hypertreegrid
{

template <class T>
bool HasTree(const T& e)
{
  return e.GetTree() != nullptr;
}

} // namespace hypertreegrid
} // namespace vtk

#endif // vtHyperTreeGridTools_h
// VTK-HeaderTest-Exclude: vtkHyperTreeGridTools.h
