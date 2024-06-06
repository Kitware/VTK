// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkFiltersCellGrid.h"

#include "vtkCellGridBoundsQuery.h"
#include "vtkCellGridCopyQuery.h"
#include "vtkCellGridElevationQuery.h"
#include "vtkCellGridEvaluator.h"
#include "vtkCellGridRangeQuery.h"
#include "vtkCellGridSidesQuery.h"
#include "vtkDGAttributeInformation.h"
#include "vtkDGBoundsResponder.h"
#include "vtkDGCopyResponder.h"
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

#include "vtkDGConstantOperators.h"
#include "vtkDGHCurlOperators.h"
#include "vtkDGHDivOperators.h"
#include "vtkDGHGradOperators.h"

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

// Since cell-attribute calculators for the vtkDGCell subclasses all
// generally respond to the same sets of tags, this helper function
// registers them appropriately.
template <typename CalcType, typename ResponderType>
void registerCalculatorResponder(vtkCellGridResponders* responders, ResponderType* instance)
{
  // All the DG cells support constant and HGRAD function spaces:
  responders->RegisterCalculator<vtkDGCell, CalcType>(instance,
    { { { "function-space"_token, { "constant"_token, "HGRAD"_token } },
      { "basis"_token, { "I"_token, "C"_token } } } });
  // Only DeRham cells support HCURL and HGRAD function spaces:
  responders->RegisterCalculator<vtkDeRhamCell, CalcType>(instance,
    { { { "function-space"_token, { "HCURL"_token, "HDIV"_token } },
      { "basis"_token, { "I"_token } } } });
  // Only higher-order tet, wedge, and pyramid have "F"ull basis, and that only for HGRAD:
  responders->RegisterCalculator<vtkDGTet, CalcType>(instance,
    { { { "function-space"_token, { "HGRAD"_token } }, { "basis"_token, { "F"_token } } } });
  responders->RegisterCalculator<vtkDGPyr, CalcType>(instance,
    { { { "function-space"_token, { "HGRAD"_token } }, { "basis"_token, { "F"_token } } } });
  responders->RegisterCalculator<vtkDGWdg, CalcType>(instance,
    { { { "function-space"_token, { "HGRAD"_token } }, { "basis"_token, { "F"_token } } } });
}

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

    vtkStringToken basisI = "I";
    vtkStringToken basisC = "C";
    vtkStringToken basisF = "F";
    vtkStringToken fsGrad = "HGRAD";
    vtkStringToken fsCurl = "HCURL";
    vtkStringToken fsDiv = "HDIV";
    vtkStringToken fsConst = "constant";
    vtkStringToken dsCoord = "coordinates";
    vtkStringToken dsPData = "point-data";

    (void)basisI;
    (void)basisC;
    (void)basisF;
    (void)fsGrad;
    (void)fsCurl;
    (void)fsDiv;
    (void)fsConst;
    (void)dsCoord;
    (void)dsPData;

    // Register the basis function (and some gradient) operators in each of our function spaces.
    vtk::basis::constant::RegisterOperators();
    vtk::basis::hgrad::RegisterOperators();
    vtk::basis::hcurl::RegisterOperators();
    vtk::basis::hdiv::RegisterOperators();

    // Query responders
    vtkNew<vtkDGBoundsResponder> dgBds;
    vtkNew<vtkDGElevationResponder> dgElv;
    vtkNew<vtkDGEvaluator> dgEva;
    vtkNew<vtkDGRangeResponder> dgRng;
    vtkNew<vtkDGSidesResponder> dgSds;
    vtkNew<vtkDGTranscribeUnstructuredCells> dgTrs;
    vtkNew<vtkDGCopyResponder> dgCpy;

    // Attribute calculators
    vtkNew<vtkDGInterpolateCalculator> dgInterp;
    vtkNew<vtkDGAttributeInformation> dgAttInfo;

    auto* responders = vtkCellMetadata::GetResponders();

    responders->RegisterQueryResponder<vtkDGCell, vtkCellGridBoundsQuery>(dgBds.GetPointer());
    responders->RegisterQueryResponder<vtkDGCell, vtkCellGridCopyQuery>(dgCpy.GetPointer());
    responders->RegisterQueryResponder<vtkDGCell, vtkCellGridElevationQuery>(dgElv.GetPointer());
    responders->RegisterQueryResponder<vtkDGCell, vtkCellGridEvaluator>(dgEva.GetPointer());
    responders->RegisterQueryResponder<vtkDGCell, vtkCellGridRangeQuery>(dgRng.GetPointer());
    responders->RegisterQueryResponder<vtkDGCell, vtkCellGridSidesQuery>(dgSds.GetPointer());
    responders->RegisterQueryResponder<vtkDGCell, vtkCellGridTranscribeQuery>(dgTrs.GetPointer());

    // Register calculators
    // # Register vtkInterpolateCalculator responders.
    registerCalculatorResponder<vtkInterpolateCalculator>(responders, dgInterp.GetPointer());
    registerCalculatorResponder<vtkCellAttributeInformation>(responders, dgAttInfo.GetPointer());
  }

  return true;
}

VTK_ABI_NAMESPACE_END
