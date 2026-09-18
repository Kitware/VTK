// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGTranscribeUnstructuredCells.h"

#include "vtkCellIterator.h"
#include "vtkDGCell.h"
#include "vtkDataSetAttributes.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include "vtkStringScanner.h"
#include "vtkStringToken.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridFieldAnnotations.h"

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <map>
#include <sstream>
#include <type_traits>
#include <vector>

#include <iostream>

// Define the macro below to debug transcription
#undef VTK_DBG_TRANSCRIBE

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals; // for ""_token().

namespace
{

// constexpr char iossAnnotations[] = "Information Records";
constexpr char iossCurlPrefix[] = "EDGE_COEFF_";
constexpr char iossDivPrefix[] = "FACE_COEFF_";

// Describes how one VTK cell type is transcribed into a vtkDGCell: the DG shape
// that claims it; the basis ("C" = complete, "I" = incomplete/serendipity) and
// order of the shape attribute; the connectivity width; and a permutation taking
// DG node order to VTK node order (empty when the two orders coincide).
struct TranscriptionSource
{
  vtkDGCell::Shape Shape{ vtkDGCell::Shape::None };
  vtkStringToken Basis{ "C"_token };
  int Order{ 1 };
  int NumberOfNodes{ 0 };
  std::vector<int> NodePermutation; // DG slot ii takes VTK slot NodePermutation[ii].
};

const TranscriptionSource* transcriptionSourceForVTKCellType(int vtkCellType)
{
  // The VTK_QUADRATIC_HEXAHEDRON permutation reorders mid-edge nodes from VTK's
  // (bottom, top, vertical) edge order to the (bottom, vertical, top) order of
  // the HexI2 basis (see Basis/HGrad/HexI2Basis.h), which follows the
  // Exodus/IOSS HEX20 convention.
  static const std::map<int, TranscriptionSource> vtkCellTypeMap = {
    { VTK_VERTEX, { vtkDGCell::Shape::Vertex, "C"_token, 1, 1, {} } },
    { VTK_LINE, { vtkDGCell::Shape::Edge, "C"_token, 1, 2, {} } },
    { VTK_TRIANGLE, { vtkDGCell::Shape::Triangle, "C"_token, 1, 3, {} } },
    { VTK_QUAD, { vtkDGCell::Shape::Quadrilateral, "C"_token, 1, 4, {} } },
    { VTK_TETRA, { vtkDGCell::Shape::Tetrahedron, "C"_token, 1, 4, {} } },
    { VTK_HEXAHEDRON, { vtkDGCell::Shape::Hexahedron, "C"_token, 1, 8, {} } },
    { VTK_WEDGE, { vtkDGCell::Shape::Wedge, "C"_token, 1, 6, {} } },
    { VTK_PYRAMID, { vtkDGCell::Shape::Pyramid, "C"_token, 1, 5, {} } },
    // TODO: Add descriptors for the remaining quadratic and higher-order cells.
    { VTK_QUADRATIC_HEXAHEDRON,
      { vtkDGCell::Shape::Hexahedron, "I"_token, 2, 20,
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 16, 17, 18, 19, 12, 13, 14, 15 } } },
  };
  auto it = vtkCellTypeMap.find(vtkCellType);
  return it == vtkCellTypeMap.end() ? nullptr : &it->second;
}

bool findArrays(vtkStringToken fieldName, vtkDataSetAttributes* cellData,
  std::vector<vtkAbstractArray*>& arrays, vtkDGCell* dgCell,
  const vtkUnstructuredGridToCellGrid::TranscribeQuery::BlockAttributesValue& annotation)
{
  std::string prefix;
  int nn = 0;
  switch (annotation.FunctionSpace.GetId())
  {
    case "HCURL"_hash:
      prefix = iossCurlPrefix + fieldName.Data();
      nn = dgCell->GetNumberOfSidesOfDimension(1);
      // TODO: Multiply by factor for higher orders.
      break;
    case "HDIV"_hash:
      prefix = iossDivPrefix + fieldName.Data();
      nn = dgCell->GetNumberOfSidesOfDimension(2);
      // TODO: Multiply by factor for higher orders.
      break;
    case "HGRAD"_hash:
      prefix = fieldName.Data();
      nn = dgCell->GetNumberOfCorners();
      // TODO: Multiply by factor for higher orders.
      break;
    default:
    {
      vtkWarningWithObjectMacro(dgCell,
        << "Unsupported function space \"" << annotation.FunctionSpace.Data().c_str() << "\".");
      return false;
    }
  }
  int paddedWidth = static_cast<int>(std::floor(std::log10(nn))) + 1;
  bool missing = false;
  // Note 1-based indexing for array names:
  for (int ii = 1; ii <= nn; ++ii)
  {
    std::ostringstream inputName;
    if (nn == 1)
    {
      inputName << prefix;
    }
    else
    {
      inputName << prefix << "_" << std::setw(paddedWidth) << std::setfill('0') << ii;
    }
    auto* array = cellData->GetAbstractArray(inputName.str().c_str());
    if (array)
    {
      arrays.push_back(array);
    }
    else
    {
      missing = true;
      vtkWarningWithObjectMacro(dgCell, "Could not find \"" << inputName.str() << "\". Skipping.");
      break;
    }
  }
  if (missing)
  {
    arrays.clear();
    return false;
  }
#ifdef VTK_DBG_TRANSCRIBE
  std::cout << "    Found " << nn << " arrays for \"" << fieldName.Data() << "\".\n";
#endif
  return true;
}

std::size_t numberOfIntegrationPoints(vtkDGCell* dgCell,
  const vtkUnstructuredGridToCellGrid::TranscribeQuery::BlockAttributesValue& annotation)
{
  std::size_t nn = 0;
  switch (annotation.BasisSource.GetId())
  {
    case "Intrepid2"_hash:
    {
      std::size_t order = annotation.QuadratureScheme.Data().substr(1, 1)[0] - '0';
      switch (annotation.FunctionSpace.GetId())
      {
        case "HDIV"_hash:
          nn = order * dgCell->GetNumberOfSidesOfDimension(1);
          break;
        case "HCURL"_hash:
          nn = order * dgCell->GetNumberOfSidesOfDimension(dgCell->GetDimension() - 1);
          break;
        case "HGRAD"_hash:
          // TODO: Handle higher orders; this only works for order = 1:
          nn = order * dgCell->GetNumberOfCorners();
          break;
        default:
        {
          vtkWarningWithObjectMacro(dgCell,
            "Unsupported Intrepid function space \"" << annotation.FunctionSpace.Data() << "\".");
        }
      }
    }
    break;
    default:
    {
      vtkWarningWithObjectMacro(
        dgCell, "Unsupported basis source \"" << annotation.BasisSource.Data() << "\".");
    }
    break;
  }
  return nn;
}

bool findGlomArrays(vtkStringToken glomName,
  const vtkUnstructuredGridToCellGrid::TranscribeQuery::FieldGlom& glomData,
  const vtkUnstructuredGridToCellGrid::TranscribeQuery::BlockAttributesValue& annotation,
  vtkDGCell* dgCell, vtkDataSetAttributes* arrays, std::vector<vtkAbstractArray*>& found)
{
#ifdef VTK_DBG_TRANSCRIBE
  std::cout << "Glom \"" << glomName.Data() << "\"\n";
#else
  (void)glomName;
#endif
  found.clear();
  std::size_t mm = glomData.Members.size(); // Number of components in glom.
  std::size_t nn = ::numberOfIntegrationPoints(dgCell, annotation);
  found.resize(mm * nn);
  // "glommed" holds arrays corresponding to one component's integration points:
  std::vector<vtkAbstractArray*> glommed;
  glommed.reserve(nn);
  int cc = 0; // current component of glom.
  for (const auto& member : glomData.Members)
  {
    glommed.clear();
    if (!::findArrays(member, arrays, glommed, dgCell, annotation))
    {
      found.clear();
      return false;
    }
    // Copy arrays for the "member"-th component into \a found
    // interleaved by integration point.
    if (glommed.size() != nn)
    {
      vtkWarningWithObjectMacro(dgCell,
        "Expected " << nn << " arrays for " << member.Data() << ", got " << glommed.size()
                    << ". Ignoring.");
      found.clear();
      return false;
    }
    for (std::size_t ii = 0; ii < nn; ++ii)
    {
      found[cc + mm * ii] = glommed[ii];
    }
    ++cc;
  }
  // We found all the arrays for all the components in the glom.
  return true;
}

vtkDGCell::Shape intrepidShapeToDGShape(vtkStringToken intrepidShape)
{
  // clang-format off
  switch (intrepidShape.GetId())
  {
  case "VERT"_hash:     return vtkDGCell::Shape::Vertex;
  case "LINE"_hash:     return vtkDGCell::Shape::Edge;
  case "TRI"_hash:      return vtkDGCell::Shape::Triangle;
  case "QUAD"_hash:     return vtkDGCell::Shape::Quadrilateral;
  case "TET"_hash:      return vtkDGCell::Shape::Tetrahedron;
  case "HEX"_hash:      return vtkDGCell::Shape::Hexahedron;
  case "WEDGE"_hash:    return vtkDGCell::Shape::Wedge;
  case "PYR"_hash:      return vtkDGCell::Shape::Pyramid;
  default: break;
  }
  // clang-format on
  return vtkDGCell::Shape::None;
}

vtkSmartPointer<vtkDataArray> interleaveArrays(
  vtkStringToken nameOut, const std::vector<vtkAbstractArray*>& arraysIn)
{
  vtkSmartPointer<vtkDataArray> arrayOut;
  if (arraysIn.empty())
  {
    return arrayOut;
  }
  auto* src0 = vtkDataArray::SafeDownCast(arraysIn[0]);
  if (!src0)
  {
    // TODO: We only support vtkDataArray for now.
    vtkGenericWarningMacro("interleaveArrays only supports vtkDataArray.");
    return arrayOut;
  }
  arrayOut = vtk::TakeSmartPointer(vtkDataArray::CreateDataArray(src0->GetDataType()));
  arrayOut->SetName(nameOut.Data().c_str());
  arrayOut->SetNumberOfComponents(static_cast<int>(arraysIn.size()));
  arrayOut->SetNumberOfTuples(src0->GetNumberOfTuples());
  int component = 0;
  for (const auto& arrayIn : arraysIn)
  {
    auto* srcN = vtkDataArray::SafeDownCast(arrayIn);
    if (!srcN)
    {
      vtkGenericWarningMacro(
        "interleaveArrays only supports vtkDataArray (comp " << component << ").");
      arrayOut = nullptr; // Destroy our output array
      break;
    }
    arrayOut->CopyComponent(component, srcN, 0);
    ++component;
  }
  return arrayOut;
}

vtkStringToken uniquifyAttributeName(vtkStringToken nameIn, vtkCellGrid* grid)
{
  std::string badName = nameIn.Data();
  while (true)
  {
    auto markPos = badName.find("@@");
    std::ostringstream nameGen;
    if (markPos != std::string::npos)
    {
      int idx;
      VTK_FROM_CHARS_IF_ERROR_BREAK(std::string_view(badName).substr(markPos + 2), idx);
      nameGen << badName.substr(0, markPos) << "@@" << (idx + 1);
    }
    else
    {
      nameGen << badName << "@@1";
    }
    badName = nameGen.str();
    if (!grid->GetCellAttributeByName(badName))
    {
      break; // We have turned badName into a good name.
    }
  }
  return badName;
}

void uniquifyArrayName(vtkAbstractArray* valueArray, vtkDataSetAttributes* dsa)
{
  while (true)
  {
    std::string badName = valueArray->GetName();
    auto markPos = badName.find("@@");
    std::ostringstream nameGen;
    if (markPos != std::string::npos)
    {
      int idx;
      VTK_FROM_CHARS_IF_ERROR_BREAK(std::string_view(badName).substr(markPos + 2), idx);
      nameGen << badName.substr(0, markPos) << "@@" << (idx + 1);
    }
    else
    {
      nameGen << badName << "@@1";
    }
    if (!dsa->GetAbstractArray(nameGen.str().c_str()))
    {
#ifdef VTK_DBG_TRANSCRIBE
      std::cout << "        Renaming " << valueArray->GetName() << " to " << nameGen.str() << "\n";
#endif
      valueArray->SetName(nameGen.str().c_str());
      return;
    }
  }
}

vtkCellAttribute* createOrAppendCellAttribute(vtkCellGrid* cellGrid, vtkDGCell* dgCell,
  vtkStringToken arrayNameOut, vtkStringToken attributeSpace, int numberOfComponents,
  vtkStringToken dofSharing, vtkStringToken functionSpace, vtkStringToken basis, int order,
  vtkAbstractArray* valueArray)
{
  bool created = false;
  auto* attr = cellGrid->GetCellAttributeByName(arrayNameOut.Data());
  if (!attr)
  {
    created = true;
    attr = vtkCellAttribute::New();
    auto* other = cellGrid->GetCellAttributeByName(arrayNameOut.Data());
    if (other)
    {
      arrayNameOut = uniquifyAttributeName(arrayNameOut, cellGrid);
    }
    attr->Initialize(arrayNameOut, attributeSpace, numberOfComponents);
  }
  else
  {
    bool mismatch = false;
    if (attr->GetNumberOfComponents() != numberOfComponents)
    {
      vtkErrorWithObjectMacro(dgCell,
        "Existing cell-attribute "
          << attr << " " << attr->GetName().Data() << " has mismatched components ("
          << attr->GetNumberOfComponents() << " vs. " << numberOfComponents << ").");
      mismatch = true;
    }
    if (attr->GetSpace() != attributeSpace)
    {
      vtkErrorWithObjectMacro(dgCell,
        "Existing cell-attribute " << attr << " " << attr->GetName().Data()
                                   << " has mismatched space \"" << attr->GetSpace().Data()
                                   << "\" vs. \"" << attributeSpace.Data() << "\".");
      mismatch = true;
    }
    // Create a new cell-attribute in the case of a mismatch.
    if (mismatch)
    {
      created = true;
      attr = vtkCellAttribute::New();
      attr->Initialize(arrayNameOut, attributeSpace, numberOfComponents);
    }
  }
  if (created)
  {
    cellGrid->AddCellAttribute(attr);
    attr->FastDelete();
  }
  std::string longCellType = dgCell->GetClassName();
  auto* dsa = cellGrid->GetAttributes(longCellType);
  if (dsa->GetAbstractArray(valueArray->GetName()))
  {
    ::uniquifyArrayName(valueArray, dsa);
  }
  dsa->AddArray(valueArray);
  vtkCellAttribute::CellTypeInfo cellTypeInfo;
  cellTypeInfo.DOFSharing = dofSharing;
  cellTypeInfo.FunctionSpace = functionSpace;
  cellTypeInfo.Basis = basis;
  cellTypeInfo.Order = order;
  cellTypeInfo.ArraysByRole["values"_token] = valueArray;
  cellTypeInfo.ArraysByRole["connectivity"_token] = dsa->GetScalars();
  if (!attr->SetCellTypeInfo(longCellType, cellTypeInfo))
  {
    vtkWarningWithObjectMacro(
      dgCell, "Could not set arrays for \"" << dgCell->GetClassName() << "\".");
  }

  return attr;
}

} // anonymous namespace

vtkStandardNewMacro(vtkDGTranscribeUnstructuredCells);

void vtkDGTranscribeUnstructuredCells::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkDGTranscribeUnstructuredCells::ClaimMatchingCells(
  TranscribeQuery* query, vtkDGCell* cellType)
{
  for (auto& entry : query->CellTypeMap)
  {
#ifdef VTK_DBG_TRANSCRIBE
    std::cout << "Consider VTK cell type " << entry.first << " priority "
              << entry.second.CellTypePriority << "\n";
#endif
    if (entry.second.CellTypePriority <= 0)
    {
      const auto* source = ::transcriptionSourceForVTKCellType(entry.first);
      if (source && cellType->GetShape() == source->Shape)
      {
        vtkStringToken typeToken = cellType->GetClassName();
        entry.second.CellType = typeToken;
      }
    }
  }
  return true;
}

bool vtkDGTranscribeUnstructuredCells::TranscribeMatchingCells(
  TranscribeQuery* query, vtkDGCell* cellType)
{
  vtkStringToken typeToken = cellType->GetClassName();
  auto it = query->OutputAllocations.find(typeToken);
  if (it == query->OutputAllocations.end())
  {
    vtkLogF(TRACE, "  Skipping %s; no allocations.", typeToken.Data().c_str());
    return true; // No cells to transcribe.
  }
  // Create a set of all the cell types we are mapping to the cellType type.
  std::set<int> cellTypesToTranscribe;
  for (auto& entry : query->CellTypeMap)
  {
    if (entry.second.CellType == typeToken)
    {
      cellTypesToTranscribe.insert(entry.first);
    }
  }
  // Resolve the basis, order, and connectivity width shared by every claimed
  // input cell type. When claimed types disagree (e.g. linear and quadratic
  // hexahedra in one partition), fall back to a linear (order-1) transcription
  // that keeps only corner nodes, since each vtkDGCell holds a single
  // fixed-width connectivity array.
  vtkStringToken basis = "C"_token;
  int order = 1;
  int nn = cellType->GetNumberOfCorners();
  bool mixedOrders = false;
  bool firstSource = true;
  for (int inputCellType : cellTypesToTranscribe)
  {
    const auto* source = ::transcriptionSourceForVTKCellType(inputCellType);
    if (!source)
    {
      continue;
    }
    if (firstSource)
    {
      basis = source->Basis;
      order = source->Order;
      nn = source->NumberOfNodes;
      firstSource = false;
    }
    else if (basis != source->Basis || order != source->Order)
    {
      mixedOrders = true;
    }
  }
  if (mixedOrders)
  {
    vtkWarningMacro("Input has cells of mixed polynomial order mapped to "
      << typeToken.Data() << "; transcribing all of them at order 1. "
      << "Higher-order nodes will be ignored.");
    basis = "C"_token;
    order = 1;
    nn = cellType->GetNumberOfCorners();
  }
  vtkNew<vtkTypeInt64Array> conn;
  conn->SetNumberOfComponents(nn);
  conn->ReserveTuples(it->second);
  conn->SetName("conn");
  std::vector<vtkTypeInt64> element;
  element.resize(nn);
  // Iterate the input data and transcribe every cell of a proper type.
  auto cit = vtk::TakeSmartPointer(query->Input->NewCellIterator());
  for (cit->InitTraversal(); !cit->IsDoneWithTraversal(); cit->GoToNextCell())
  {
    if (cellTypesToTranscribe.find(cit->GetCellType()) == cellTypesToTranscribe.end())
    {
      continue; // Skip this cell
    }
    // Transcribe the cell, permuting nodes into the DG basis order when the
    // input cell type prescribes a permutation. When falling back to order 1,
    // copying the first \a nn point IDs is correct because every supported
    // input type lists the corner nodes first, in the same order.
    auto* pointIds = cit->GetPointIds();
    const std::vector<int>* perm = nullptr;
    if (order > 1)
    {
      const auto* source = ::transcriptionSourceForVTKCellType(cit->GetCellType());
      if (source && !source->NodePermutation.empty())
      {
        perm = &source->NodePermutation;
      }
    }
    for (int ii = 0; ii < nn; ++ii)
    {
      element[ii] = static_cast<vtkTypeInt64>(pointIds->GetId(perm ? (*perm)[ii] : ii));
    }
    conn->InsertNextTypedTuple(element.data());
  }
  // vtkStringToken arrayClass = typeToken.Data().substr(3);
  query->Output->GetAttributes(typeToken.GetId())->AddArray(conn);
  query->Output->GetAttributes(typeToken.GetId())->SetScalars(conn);

  // Mark the \a conn array as the source of cells for this metadata.
  auto& cellSpec = cellType->GetCellSpec();
  cellSpec.Connectivity = conn;
  cellSpec.Offset = 0;
  cellSpec.SideType = -1;
  cellSpec.Blanked = false;
  cellSpec.SourceShape = cellType->GetShape();
  auto* fieldData = query->Input->GetFieldData();
  auto* sideArrayNames =
    vtkStringArray::SafeDownCast(fieldData->GetAbstractArray("side_set_arrays"));
  if (sideArrayNames)
  {
#ifdef VTK_DBG_TRANSCRIBE
    std::cout << "  Mesh represents " << sideArrayNames->GetNumberOfTuples() / 2 << " side sets\n";
#endif
    auto& sideSpecs = cellType->GetSideSpecs();
    (void)sideSpecs;
    cellSpec.Blanked = true;
    vtkIdType offset = 0;
    for (vtkIdType ii = 0; ii <= sideArrayNames->GetMaxId(); ii += 2)
    {
      auto sideShape = vtkDGCell::GetShapeEnum(sideArrayNames->GetValue(ii + 1).c_str());
      int sideType = cellType->GetSideTypeForShape(sideShape);
      vtkDGCell::Source sideSpec{ fieldData->GetArray(
                                    sideArrayNames->GetValue(ii).c_str()), // Connectivity
        offset,
        false, // Blanked
        sideShape, sideType };
      sideSpecs.push_back(sideSpec);
#ifdef VTK_DBG_TRANSCRIBE
      std::cout << "    " << ii << " " << sideArrayNames->GetValue(ii) << " "
                << vtkDGCell::GetShapeName(
                     vtkDGCell::GetShapeEnum(sideArrayNames->GetValue(ii + 1).c_str()))
                     .Data()
                << "\n";
#endif
      offset += sideSpec.Connectivity->GetNumberOfTuples();
    }
  }
  // TODO: Handle side/node sets.
  // In this case:
  // 1. The input dataset will have field data indicating the source element block…
  //    but how will we find the matching output vtkCellGrid in order to reference
  //    its cells?
  // 2. Instead of cell connectivity, there should be an array of (cell, side) tuples
  //    that we add to dgCell->GetSideSpecs() – and we should blank the cells.

  // The point-coordinate array has been copied by reference to
  // query->Output, but we need the cell-attribute to refer to them
  // in the context of our newly-minted cells.
  auto* shape = query->Output->GetShapeAttribute();
  if (shape)
  {
    auto coords = query->Output->GetAttributes("coordinates"_hash)->GetVectors();
    vtkCellAttribute::CellTypeInfo cellTypeInfo;
    cellTypeInfo.DOFSharing = "CG"_token;
    cellTypeInfo.FunctionSpace = "HGRAD"_token;
    cellTypeInfo.Basis = basis;
    cellTypeInfo.Order = order;
    cellTypeInfo.ArraysByRole["values"] = coords;
    cellTypeInfo.ArraysByRole["connectivity"] = conn;
    shape->SetCellTypeInfo(typeToken, cellTypeInfo);
  }

  // The point-data arrays have all been copied by reference to
  // query->Output, but we need the cell-attribute to refer to them
  // in the context of our newly-minted cells.

  // The cell-data arrays have *not* been copied yet. Do so and also
  // add references in the cell-attribute's arrays for this cell type.
  // First, spelunk the input's field data for IOSS annotations
  // indicating some arrays have unusual function spaces.
  this->AddCellAttributes(query, cellType);
  this->AddPointAttributes(query, cellType, basis, order);
  return true;
}

bool vtkDGTranscribeUnstructuredCells::Query(
  TranscribeQuery* query, vtkCellMetadata* cellType, vtkCellGridResponders* vtkNotUsed(caches))
{
  bool ok = false;
  if (!query)
  {
    return ok;
  }

#ifdef VTK_DBG_TRANSCRIBE
  std::cout << "Transcribe " << cellType->GetClassName() << " phase " << query->Phase << " alloc "
            << query->OutputAllocations.size() << "\n";
  for (const auto& aent : query->OutputAllocations)
  {
    std::cout << "    " << aent.first.Data() << " (" << aent.first.GetId() << "): " << aent.second
              << "\n";
  }
#endif

  auto* dgCell = vtkDGCell::SafeDownCast(cellType);
  if (!dgCell)
  {
    return ok;
  }

  switch (query->Phase)
  {
    case 0:
      // Claim cells that have a matching vtkDGCell::Shape.
      ok = this->ClaimMatchingCells(query, dgCell);
      break;
    case 1:
      // Transcribe cells that were claimed in phase 0.
      ok = this->TranscribeMatchingCells(query, dgCell);
      break;
    default:
      vtkWarningMacro("Unknown phase " << query->Phase << ".");
      return ok;
  }

  return ok;
}

void vtkDGTranscribeUnstructuredCells::AddCellAttributes(TranscribeQuery* query, vtkDGCell* dgCell)
{
  using BlockAttributesKey = vtkUnstructuredGridToCellGrid::TranscribeQuery::BlockAttributesKey;
  using BlockAttributesValue = vtkUnstructuredGridToCellGrid::TranscribeQuery::BlockAttributesValue;
  std::map<BlockAttributesKey, BlockAttributesValue> empty;

  auto* cellData = query->Input->GetAttributes(vtkDataObject::CELL);
  if (cellData)
  {
    // I. Loop over annotations provided by the query which indicate how
    //    individual input arrays should be related to DG cell-attributes.
    auto it = query->Annotations->Data.find(query->FlatIndex);
    auto& localAnnotations(it == query->Annotations->Data.end() ? empty : it->second);
#ifdef VTK_DBG_TRANSCRIBE
    std::cout << "Block " << query->FlatIndex << " has " << localAnnotations.size()
              << " annotations.\n"
              << "  I. Annotated fields\n";
#endif
    std::set<vtkAbstractArray*> consumedInputs;
    for (const auto& annotation : localAnnotations)
    {
#ifdef VTK_DBG_TRANSCRIBE
      std::cout << "  " << annotation.first.DOFSharing.Data() << " "
                << annotation.first.FunctionSpace.Data() << ": "
                << annotation.second.FieldNames.size() << " attributes.\n";
#endif
      auto shape = intrepidShapeToDGShape(annotation.second.Shape);
      if (shape == vtkDGCell::Shape::None)
      {
        vtkWarningMacro(
          "Unsupported shape \"" << annotation.second.Shape.Data() << "\". Skipping.");
        continue;
      }
      if (shape != dgCell->GetShape())
      {
        vtkWarningMacro("Shape \""
          << vtkDGCell::GetShapeName(shape).Data() << "\" does not match \""
          << vtkDGCell::GetShapeName(dgCell->GetShape()).Data() << "\". Skipping.");
        continue;
      }
      auto dofSharing = annotation.first.DOFSharing;
      // Note that until we have an alternate search technique for finding HCurl/HDiv
      // arrays, we must modify the DOFSharing member because even though the simulation
      // may have used a CG technique, the ioss storage duplicates data in a way that
      // allows for discontinuous attributes.
      if (annotation.first.FunctionSpace == "HCURL"_token ||
        annotation.first.FunctionSpace == "HDIV"_token)
      {
        dofSharing = vtkStringToken(); // "DG"_token;
      }
      std::size_t order = annotation.second.QuadratureScheme.Data().substr(1, 1)[0] - '0';
      vtkStringToken basis = annotation.second.QuadratureScheme.Data().substr(0, 1);

      // A. Handle multi-component, multi-integration-point fields by interleaving
      // many arrays into a single array with M * N components (where M is the
      // number of integration points per cell and N is the number of values at
      // each integration point) and as many tuples as there are cells/sides.
      for (const auto& glomEntry : annotation.second.FieldGloms)
      {
        std::vector<vtkAbstractArray*> arrays;
        if (::findGlomArrays(
              glomEntry.first, glomEntry.second, annotation.second, dgCell, cellData, arrays))
        {
          // Create vector/tensor cell-attribute of the proper name and type.
          int numberOfComponents = static_cast<int>(glomEntry.second.Members.size());
          auto attributeSpace = vtkCellAttribute::EncodeSpace("ℝ", numberOfComponents);
          // Rewrite all the arrays into a single array.
          if (auto oneBigArray = ::interleaveArrays(glomEntry.first, arrays))
          {
            // Create a cell-attribute.
            auto* attr = ::createOrAppendCellAttribute(query->Output, dgCell, glomEntry.first,
              attributeSpace, numberOfComponents, dofSharing, annotation.first.FunctionSpace, basis,
              static_cast<int>(order), oneBigArray);
            (void)attr;
#ifdef VTK_DBG_TRANSCRIBE
            std::cout << "Found MIMC glom \"" << glomEntry.first.Data() << "\" with "
                      << arrays.size() << " arrays.\n";
            for (const auto& array : arrays)
            {
              std::cout << "  " << array << " " << array->GetName() << " ("
                        << array->GetNumberOfTuples() << "×" << array->GetNumberOfComponents()
                        << ")\n";
            }
#endif
          }
          consumedInputs.insert(arrays.begin(), arrays.end());
        }
      }

      // B. Handle single-component, multi-integration-point fields by interleaving
      // many arrays into a single array with M components (M as above) and as
      // many tuples as there are cells/sides.
      for (const auto& fieldName : annotation.second.FieldNames)
      {
        std::vector<vtkAbstractArray*> arrays;
        if (::findArrays(fieldName, cellData, arrays, dgCell, annotation.second))
        {
          int numberOfComponents = static_cast<int>(arrays.size());
          auto attributeSpace = vtkCellAttribute::EncodeSpace("ℝ", numberOfComponents);
          // Rewrite all the per-integration-point arrays into a single array.
          if (auto oneBigArray = ::interleaveArrays(fieldName, arrays))
          {
            // Create a cell-attribute.
            auto* attr = ::createOrAppendCellAttribute(query->Output, dgCell, fieldName,
              attributeSpace, numberOfComponents, dofSharing, annotation.first.FunctionSpace, basis,
              static_cast<int>(order), oneBigArray);
            (void)attr;
#ifdef VTK_DBG_TRANSCRIBE
            std::cout << "      Found SIMC scalar \"" << fieldName.Data() << "\" with "
                      << arrays.size() << " arrays.\n";
            for (const auto& array : arrays)
            {
              std::cout << "        " << array << " " << array->GetName() << " ("
                        << array->GetNumberOfTuples() << "×" << array->GetNumberOfComponents()
                        << ")\n";
            }
            std::cout << "      Converted into one array \"" << oneBigArray->GetName() << "\" ("
                      << oneBigArray->GetNumberOfTuples() << "×"
                      << oneBigArray->GetNumberOfComponents() << ")\n";
            vtkIndent indent(8);
            attr->PrintSelf(std::cout, indent);
#endif
            consumedInputs.insert(arrays.begin(), arrays.end());
          }
        }
      }
    }
    // II. Use any arrays unclaimed by the above as C-0 attributes defined over cells.
    int nn = cellData->GetNumberOfArrays();
#ifdef VTK_DBG_TRANSCRIBE
    std::cout << "  II. Traditional cell-data fields\n";
#endif
    for (int ii = 0; ii < nn; ++ii)
    {
      auto* arr = cellData->GetAbstractArray(ii);
      if (consumedInputs.find(arr) != consumedInputs.end())
      {
        continue;
      }
#ifdef VTK_DBG_TRANSCRIBE
      std::cout << "    array " << ii << " (" << arr->GetNumberOfTuples() << " × "
                << arr->GetNumberOfComponents() << "): " << arr->GetName() << " type "
                << arr->GetClassName() << "\n";
#endif

      // Create scalar cell-attribute of the proper name and type.
      int numberOfComponents = arr->GetNumberOfComponents();
      auto dofSharing = vtkStringToken();
      vtkStringToken attributeSpace = vtkCellAttribute::EncodeSpace("ℝ", numberOfComponents);
      auto* attr = ::createOrAppendCellAttribute(query->Output, dgCell, arr->GetName(),
        attributeSpace, numberOfComponents, dofSharing, "constant"_token, "C"_token, 0, arr);
      (void)attr;
#ifdef VTK_DBG_TRANSCRIBE
      vtkIndent indent(6);
      attr->PrintSelf(std::cout, indent);
#endif
    }
  }
}

void vtkDGTranscribeUnstructuredCells::AddPointAttributes(
  TranscribeQuery* query, vtkDGCell* dgCell, vtkStringToken basis, int order)
{
  using BlockAttributesKey = vtkUnstructuredGridToCellGrid::TranscribeQuery::BlockAttributesKey;
  using BlockAttributesValue = vtkUnstructuredGridToCellGrid::TranscribeQuery::BlockAttributesValue;
  std::map<BlockAttributesKey, BlockAttributesValue> empty;

  auto* pointData = query->Input->GetAttributes(vtkDataObject::POINT);
  if (!pointData)
  {
    return;
  }

  // I. Use point-data arrays as CG HGRAD cell-attributes matching the basis
  //    and order of the shape attribute (they share its connectivity array).
  int nn = pointData->GetNumberOfArrays();
  for (int ii = 0; ii < nn; ++ii)
  {
    auto* arr = pointData->GetAbstractArray(ii);
#ifdef VTK_DBG_TRANSCRIBE
    std::cout << "  array " << ii << " (" << arr->GetNumberOfTuples() << " × "
              << arr->GetNumberOfComponents() << "): " << arr->GetName() << " type "
              << arr->GetClassName() << "\n";
#endif

    int numberOfComponents = arr->GetNumberOfComponents();
    // Create scalar cell-attribute of the proper name and type.
    vtkStringToken dofSharing = "CG";
    vtkStringToken attributeSpace = vtkCellAttribute::EncodeSpace("ℝ", numberOfComponents);
    auto* attr = ::createOrAppendCellAttribute(query->Output, dgCell, arr->GetName(),
      attributeSpace, numberOfComponents, dofSharing, "HGRAD"_token, basis, order, arr);
    (void)attr;
  }
}

VTK_ABI_NAMESPACE_END
