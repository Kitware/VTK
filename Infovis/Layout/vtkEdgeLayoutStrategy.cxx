// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkEdgeLayoutStrategy.h"

#include "vtkGraph.h"

VTK_ABI_NAMESPACE_BEGIN
void vtkEdgeLayoutStrategy::SetGraph(vtkGraph* graph)
{
  // This method is a cut and paste of vtkCxxSetObjectMacro
  // except for the call to Initialize in the middle :)
  if (graph != this->Graph)
  {
    vtkGraph* tmp = this->Graph;
    this->Graph = graph;
    if (this->Graph != nullptr)
    {
      this->Graph->Register(this);
      this->Initialize();
    }
    if (tmp != nullptr)
    {
      tmp->UnRegister(this);
    }
    this->Modified();
  }
}

vtkEdgeLayoutStrategy::vtkEdgeLayoutStrategy()
{
  this->Graph = nullptr;
  this->EdgeWeightArrayName = nullptr;
}

vtkEdgeLayoutStrategy::~vtkEdgeLayoutStrategy()
{
  // Unregister vtk objects that were passed in
  this->SetGraph(nullptr);
  this->SetEdgeWeightArrayName(nullptr);
}

void vtkEdgeLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Graph: " << (this->Graph ? "" : "(none)") << endl;
  if (this->Graph)
  {
    this->Graph->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "EdgeWeightArrayName: "
     << (this->EdgeWeightArrayName ? this->EdgeWeightArrayName : "(none)") << endl;
}
VTK_ABI_NAMESPACE_END
