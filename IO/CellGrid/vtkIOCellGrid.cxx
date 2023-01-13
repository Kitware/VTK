// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkIOCellGrid.h"

#include "vtkCellGridIOQuery.h"
#include "vtkDGIOResponder.h"

// Cell types
#include "vtkDGEdge.h"
#include "vtkDGHex.h"
#include "vtkDGPyr.h"
#include "vtkDGQuad.h"
#include "vtkDGTet.h"
#include "vtkDGTri.h"
#include "vtkDGVert.h"
#include "vtkDGWdg.h"

void vtkIOCellGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkIOCellGrid::RegisterCellsAndResponders()
{
  // These are probably already registered, but it doesn't hurt to re-add them.
  vtkCellMetadata::RegisterType<vtkDGEdge>();
  vtkCellMetadata::RegisterType<vtkDGHex>();
  vtkCellMetadata::RegisterType<vtkDGPyr>();
  vtkCellMetadata::RegisterType<vtkDGQuad>();
  vtkCellMetadata::RegisterType<vtkDGTet>();
  vtkCellMetadata::RegisterType<vtkDGTri>();
  vtkCellMetadata::RegisterType<vtkDGVert>();
  vtkCellMetadata::RegisterType<vtkDGWdg>();

  static bool once = false;
  if (!once)
  {
    once = true;

    // Query responders
    vtkNew<vtkDGIOResponder> dgIO;

    auto* responders = vtkCellMetadata::GetResponders();

    // clang-format off
    // Edge responders
    responders->RegisterQueryResponder<vtkDGEdge, vtkCellGridIOQuery>(dgIO.GetPointer());

    // Hexahedral responders
    responders->RegisterQueryResponder<vtkDGHex, vtkCellGridIOQuery>(dgIO.GetPointer());

    // Pyramidal responders
    responders->RegisterQueryResponder<vtkDGPyr, vtkCellGridIOQuery>(dgIO.GetPointer());

    // Quadrilateral responders
    responders->RegisterQueryResponder<vtkDGQuad, vtkCellGridIOQuery>(dgIO.GetPointer());

    // Tetrahedral responders
    responders->RegisterQueryResponder<vtkDGTet, vtkCellGridIOQuery>(dgIO.GetPointer());

    // Triangle responders
    responders->RegisterQueryResponder<vtkDGTri, vtkCellGridIOQuery>(dgIO.GetPointer());

    // Vertex responders
    responders->RegisterQueryResponder<vtkDGVert, vtkCellGridIOQuery>(dgIO.GetPointer());

    // Wedge responders
    responders->RegisterQueryResponder<vtkDGWdg,  vtkCellGridIOQuery>(dgIO.GetPointer());
    // clang-format on
  }

  return true;
}
