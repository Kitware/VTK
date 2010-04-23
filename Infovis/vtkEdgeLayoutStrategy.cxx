/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEdgeLayoutStrategy.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkEdgeLayoutStrategy.h"

#include "vtkGraph.h"


void vtkEdgeLayoutStrategy::SetGraph(vtkGraph *graph)
{
  // This method is a cut and paste of vtkCxxSetObjectMacro
  // except for the call to Initialize in the middle :)
  if (graph != this->Graph)
    {
    vtkGraph *tmp = this->Graph;
    this->Graph = graph;
    if (this->Graph != NULL)
      {
      this->Graph->Register(this);
      this->Initialize();
      }
    if (tmp != NULL)
      {
      tmp->UnRegister(this);
      }
    this->Modified();
    }
}

vtkEdgeLayoutStrategy::vtkEdgeLayoutStrategy()
{
  this->Graph = NULL;
  this->EdgeWeightArrayName = NULL;
}

vtkEdgeLayoutStrategy::~vtkEdgeLayoutStrategy()
{
  // Unregister vtk objects that were passed in
  this->SetGraph(NULL);
  this->SetEdgeWeightArrayName(NULL);
}

void vtkEdgeLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Graph: " << (this->Graph ? "" : "(none)") << endl;
  if (this->Graph)
    {
    this->Graph->PrintSelf(os, indent.GetNextIndent());
    }
  os << indent << "EdgeWeightArrayName: "
    << (this->EdgeWeightArrayName ? this->EdgeWeightArrayName : "(none)") << endl;
}
