// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkFiltersCellGrid.h"

#include "vtkCellGridBoundsQuery.h"
#include "vtkCellGridElevationQuery.h"
#include "vtkCellGridEvaluator.h"
#include "vtkCellGridRangeQuery.h"
#include "vtkCellGridSidesQuery.h"
#include "vtkDGAttributeInformation.h"
#include "vtkDGBoundsResponder.h"
#include "vtkDGEdge.h"
#include "vtkDGElevationResponder.h"
#include "vtkDGEvaluator.h"
#include "vtkDGHex.h"
#include "vtkDGInterpolateCalculator.h"
#include "vtkDGPyr.h"
#include "vtkDGQuad.h"
#include "vtkDGRangeResponder.h"
#include "vtkDGSidesResponder.h"
#include "vtkDGTet.h"
#include "vtkDGTranscribeUnstructuredCells.h"
#include "vtkDGTri.h"
#include "vtkDGVert.h"
#include "vtkDGWdg.h"
#include "vtkInterpolateCalculator.h"
#include "vtkUnstructuredGridToCellGrid.h"

void vtkFiltersCellGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkFiltersCellGrid::RegisterCellsAndResponders()
{
  using vtkCellGridTranscribeQuery = vtkUnstructuredGridToCellGrid::TranscribeQuery;

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
    vtkNew<vtkDGBoundsResponder> dgBds;
    vtkNew<vtkDGElevationResponder> dgElv;
    vtkNew<vtkDGEvaluator> dgEva;
    vtkNew<vtkDGRangeResponder> dgRng;
    vtkNew<vtkDGSidesResponder> dgSds;
    vtkNew<vtkDGTranscribeUnstructuredCells> dgTrs;

    // Attribute calculators
    vtkNew<vtkDGInterpolateCalculator> dgInterp;
    vtkNew<vtkDGAttributeInformation> dgAttInfo;

    auto* responders = vtkCellMetadata::GetResponders();

    // clang-format off
    responders->RegisterQueryResponder<vtkDGCell, vtkCellGridBoundsQuery>(dgBds.GetPointer());
    responders->RegisterQueryResponder<vtkDGCell, vtkCellGridElevationQuery>(dgElv.GetPointer());
    responders->RegisterQueryResponder<vtkDGCell, vtkCellGridEvaluator>(dgEva.GetPointer());
    responders->RegisterQueryResponder<vtkDGCell, vtkCellGridRangeQuery>(dgRng.GetPointer());
    responders->RegisterQueryResponder<vtkDGCell, vtkCellGridSidesQuery>(dgSds.GetPointer());
    responders->RegisterQueryResponder<vtkDGCell, vtkCellGridTranscribeQuery>(dgTrs.GetPointer());

    // Register calculators
    responders->RegisterCalculator<vtkDGCell, vtkInterpolateCalculator>("DG constant C0", dgInterp.GetPointer());
    responders->RegisterCalculator<vtkDGCell, vtkInterpolateCalculator>("DG HGRAD C0", dgInterp.GetPointer());
    responders->RegisterCalculator<vtkDGCell, vtkInterpolateCalculator>("CG HGRAD C1", dgInterp.GetPointer());
    responders->RegisterCalculator<vtkDGCell, vtkInterpolateCalculator>("DG HGRAD C1", dgInterp.GetPointer());
    responders->RegisterCalculator<vtkDGCell, vtkInterpolateCalculator>("DG HGRAD I2", dgInterp.GetPointer());
    responders->RegisterCalculator<vtkDeRhamCell, vtkInterpolateCalculator>("CG HCURL I1", dgInterp.GetPointer());
    responders->RegisterCalculator<vtkDeRhamCell, vtkInterpolateCalculator>("DG HCURL I1", dgInterp.GetPointer());
    responders->RegisterCalculator<vtkDeRhamCell, vtkInterpolateCalculator>("CG HDIV I1", dgInterp.GetPointer());
    responders->RegisterCalculator<vtkDeRhamCell, vtkInterpolateCalculator>("DG HDIV I1", dgInterp.GetPointer());

    responders->RegisterCalculator<vtkDGCell, vtkCellAttributeInformation>("DG constant C0", dgAttInfo.GetPointer());
    responders->RegisterCalculator<vtkDGCell, vtkCellAttributeInformation>("DG HGRAD C0", dgAttInfo.GetPointer());
    responders->RegisterCalculator<vtkDGCell, vtkCellAttributeInformation>("CG HGRAD C1", dgAttInfo.GetPointer());
    responders->RegisterCalculator<vtkDGCell, vtkCellAttributeInformation>("DG HGRAD C1", dgAttInfo.GetPointer());
    responders->RegisterCalculator<vtkDGCell, vtkCellAttributeInformation>("DG HGRAD I2", dgAttInfo.GetPointer());
    responders->RegisterCalculator<vtkDGCell, vtkCellAttributeInformation>("DG HGRAD C2", dgAttInfo.GetPointer());
    responders->RegisterCalculator<vtkDeRhamCell, vtkCellAttributeInformation>("CG HCURL I1", dgAttInfo.GetPointer());
    responders->RegisterCalculator<vtkDeRhamCell, vtkCellAttributeInformation>("DG HCURL I1", dgAttInfo.GetPointer());
    responders->RegisterCalculator<vtkDeRhamCell, vtkCellAttributeInformation>("CG HDIV I1", dgAttInfo.GetPointer());
    responders->RegisterCalculator<vtkDeRhamCell, vtkCellAttributeInformation>("DG HDIV I1", dgAttInfo.GetPointer());

    // clang-format on
  }

  return true;
}
