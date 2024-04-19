// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkGraphLayoutStrategy.h"

#include "vtkGraph.h"

VTK_ABI_NAMESPACE_BEGIN
void vtkGraphLayoutStrategy::SetGraph(vtkGraph* graph)
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

vtkGraphLayoutStrategy::vtkGraphLayoutStrategy()
{
  this->Graph = nullptr;
  this->EdgeWeightField = nullptr;
  this->WeightEdges = false;
}

vtkGraphLayoutStrategy::~vtkGraphLayoutStrategy()
{
  // Unregister vtk objects that were passed in
  this->SetGraph(nullptr);
  this->SetEdgeWeightField(nullptr);
}

void vtkGraphLayoutStrategy::SetWeightEdges(bool state)
{
  // This method is a cut and paste of vtkSetMacro
  // except for the call to Initialize at the end :)
  if (this->WeightEdges != state)
  {
    this->WeightEdges = state;
    this->Modified();
    if (this->Graph)
    {
      this->Initialize();
    }
  }
}

void vtkGraphLayoutStrategy::SetEdgeWeightField(const char* weights)
{
  // This method is a cut and paste of vtkSetStringMacro
  // except for the call to Initialize at the end :)
  if (this->EdgeWeightField == nullptr && weights == nullptr)
  {
    return;
  }
  if (this->EdgeWeightField && weights && (!strcmp(this->EdgeWeightField, weights)))
  {
    return;
  }
  delete[] this->EdgeWeightField;
  if (weights)
  {
    size_t n = strlen(weights) + 1;
    char* cp1 = new char[n];
    const char* cp2 = (weights);
    this->EdgeWeightField = cp1;
    do
    {
      *cp1++ = *cp2++;
    } while (--n);
  }
  else
  {
    this->EdgeWeightField = nullptr;
  }

  this->Modified();

  if (this->Graph)
  {
    this->Initialize();
  }
}

void vtkGraphLayoutStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Graph: " << (this->Graph ? "" : "(none)") << endl;
  if (this->Graph)
  {
    this->Graph->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "WeightEdges: " << (this->WeightEdges ? "True" : "False") << endl;
  os << indent << "EdgeWeightField: " << (this->EdgeWeightField ? this->EdgeWeightField : "(none)")
     << endl;
}
VTK_ABI_NAMESPACE_END
