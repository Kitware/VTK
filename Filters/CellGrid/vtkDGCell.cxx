// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGCell.h"

#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkDGOperatorEntry.h"
#include "vtkDataSetAttributes.h"
#include "vtkStringToken.h"
#include "vtkTypeFloat32Array.h"
#include "vtkTypeInt32Array.h"
#include "vtkVectorOperators.h"

#include <token/Singletons.h>

// Switch this to defined to get some debug printouts.
#undef VTK_DBG_DGCELL

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

namespace
{

ostream& PrintSource(ostream& os, const vtkDGCell::Source& src, bool isCellSpec)
{
  os << "Connectivity: " << src.Connectivity;
  if (src.Connectivity)
  {
    if (isCellSpec)
    {
      os << " (pts/cell: " << src.Connectivity->GetNumberOfComponents()
         << ", cells: " << src.Connectivity->GetNumberOfTuples() << ")";
    }
    else
    {
      os << " (sides: " << src.Connectivity->GetNumberOfTuples() << ")";
    }
  }
  if (src.NodalGhostMarks)
  {
    os << ", NodalGhostMarks " << src.NodalGhostMarks;
  }
  os << ", Offset: " << src.Offset << ", Blanked: " << (src.Blanked ? "T" : "F")
     << ", Shape: " << src.SourceShape << ", SideType: " << src.SideType;
  return os;
}

} // anonymous namespace

vtkDGCell::Source::Source(vtkDataArray* conn, vtkIdType off, bool blank, Shape shape, int sideType)
  : Connectivity(conn)
  , Offset(off)
  , Blanked(blank)
  , SourceShape(shape)
  , SideType(sideType)
{
}

vtkDGCell::Source::Source(vtkDataArray* conn, vtkIdType off, bool blank, Shape shape, int sideType,
  int selnType, vtkDataArray* nodalGhostMarks)
  : Connectivity(conn)
  , NodalGhostMarks(nodalGhostMarks)
  , Offset(off)
  , Blanked(blank)
  , SourceShape(shape)
  , SideType(sideType)
  , SelectionType(selnType)
{
}

vtkDGCell::vtkDGCell()
{
  static bool registeredSideShapes = false;
  if (!registeredSideShapes)
  {
    registeredSideShapes = true;
    // Constructing these inserts the strings into the
    // vtkStringToken's manager so they are available for printing
    // even though GetShapeName() computes the hash at
    // compile time (which cannot insert strings into the
    // manager).
    (void)vtkStringToken("vertex");
    (void)vtkStringToken("edge");
    (void)vtkStringToken("triangle");
    (void)vtkStringToken("quadrilateral");
    (void)vtkStringToken("tetrahedron");
    (void)vtkStringToken("hexahedron");
    (void)vtkStringToken("wedge");
    (void)vtkStringToken("pyramid");
    (void)vtkStringToken("unknown");
  }
}

vtkDGCell::~vtkDGCell() = default;

void vtkDGCell::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CellSpec: ";
  PrintSource(os, this->CellSpec, true);
  os << "\n";
  os << indent << "SideSpecs:\n";
  vtkIndent i2 = indent.GetNextIndent();
  int ii = 0;
  for (const auto& sideSpec : this->SideSpecs)
  {
    os << i2 << ii << ": ";
    PrintSource(os, sideSpec, false);
    os << "\n";
    ++ii;
  }
}

const vtkDGCell::Source& vtkDGCell::GetCellSource(int sideType) const
{
  if (sideType < 0)
  {
    return this->CellSpec;
  }
  else if (sideType < static_cast<int>(this->SideSpecs.size()))
  {
    return this->SideSpecs[sideType];
  }
  static Source dummy;
  return dummy;
}

vtkDataArray* vtkDGCell::GetCellSourceConnectivity(int sideType) const
{
  return this->GetCellSource(sideType).Connectivity;
}

vtkDataArray* vtkDGCell::GetCellSourceNodalGhostMarks(int sideType) const
{
  return this->GetCellSource(sideType).NodalGhostMarks;
}

vtkIdType vtkDGCell::GetCellSourceOffset(int sideType) const
{
  return this->GetCellSource(sideType).Offset;
}

bool vtkDGCell::GetCellSourceIsBlanked(int sideType) const
{
  return this->GetCellSource(sideType).Blanked;
}

vtkDGCell::Shape vtkDGCell::GetCellSourceShape(int sideType) const
{
  return this->GetCellSource(sideType).SourceShape;
}

int vtkDGCell::GetCellSourceSideType(int sideType) const
{
  return this->GetCellSource(sideType).SideType;
}

int vtkDGCell::GetCellSourceSelectionType(int sideType) const
{
  return this->GetCellSource(sideType).SelectionType;
}

vtkIdType vtkDGCell::GetNumberOfCells()
{
  vtkIdType count = 0;
  if (!this->CellSpec.Connectivity)
  {
    return count;
  }
  if (!this->CellSpec.Blanked)
  {
    count += this->CellSpec.Connectivity->GetNumberOfTuples();
  }
  for (const auto& sideSpec : this->SideSpecs)
  {
    if (!sideSpec.Connectivity || sideSpec.Blanked)
    {
      continue;
    }
    count += sideSpec.Connectivity->GetNumberOfTuples();
  }
  return count;
}

void vtkDGCell::ShallowCopy(vtkCellMetadata* other)
{
  auto* dgOther = vtkDGCell::SafeDownCast(other);
  if (!dgOther || dgOther->GetClassName() != this->GetClassName())
  {
    vtkErrorMacro("Source of copy must be a " << this->GetClassName() << ".");
    return;
  }
  this->Superclass::ShallowCopy(dgOther);
  // We can directly reference the same arrays since this is a shallow copy.
  this->CellSpec = dgOther->CellSpec;
  this->SideSpecs = dgOther->SideSpecs;
}

void vtkDGCell::DeepCopy(vtkCellMetadata* other)
{
  auto* dgOther = vtkDGCell::SafeDownCast(other);
  if (!dgOther || dgOther->GetClassName() != this->GetClassName())
  {
    vtkErrorMacro("Source of copy must be a " << this->GetClassName() << ".");
    return;
  }
  auto* selfCG = this->GetCellGrid();
  auto* otherCG = other->GetCellGrid();

  this->Superclass::DeepCopy(dgOther);
  // We cannot directly reference the same arrays since this is a deep copy.
  // So, we must find corresponding arrays. We look for them in the same
  // array-group and with the same name.
  this->CellSpec = dgOther->CellSpec;
  this->CellSpec.Connectivity =
    vtkCellGrid::CorrespondingArray(selfCG, this->CellSpec.Connectivity, otherCG);
  if (!this->CellSpec.Connectivity)
  {
    vtkWarningMacro("Could not find cell-connectivity array \""
      << dgOther->CellSpec.Connectivity->GetName()
      << "\""
         " for cell type \""
      << this->GetClassName() << "\".");
  }
  this->SideSpecs = dgOther->SideSpecs;
  for (auto& sideSpec : this->SideSpecs)
  {
    auto* conn = sideSpec.Connectivity;
    sideSpec.Connectivity = vtkCellGrid::CorrespondingArray(selfCG, conn, otherCG);
    if (!sideSpec.Connectivity)
    {
      vtkWarningMacro("Could not find side-connectivity array \""
        << (conn ? conn->GetName() : "(null)")
        << "\""
           " for cell type \""
        << this->GetClassName() << "\".");
    }
  }
}

int vtkDGCell::GetShapeCornerCount(Shape shape)
{
  switch (shape)
  {
    case Vertex:
      return 1;
    case Edge:
      return 2;
    case Triangle:
      return 3;
    case Quadrilateral:
      return 4;
    case Tetrahedron:
      return 4;
    case Hexahedron:
      return 8;
    case Wedge:
      return 6;
    case Pyramid:
      return 5;
    default:
      break;
  }
  return 0;
}

vtkStringToken vtkDGCell::GetShapeName(Shape shape)
{
  switch (shape)
  {
    case Vertex:
      return "vertex"_token;
    case Edge:
      return "edge"_token;
    case Triangle:
      return "triangle"_token;
    case Quadrilateral:
      return "quadrilateral"_token;
    case Tetrahedron:
      return "tetrahedron"_token;
    case Hexahedron:
      return "hexahedron"_token;
    case Wedge:
      return "wedge"_token;
    case Pyramid:
      return "pyramid"_token;
    default:
      break;
  }
  return "unknown"_token;
}

vtkDGCell::Shape vtkDGCell::GetShapeEnum(vtkStringToken shapeName)
{
  // XXX(c++14)
#if __cplusplus < 201400L
  auto snid = shapeName.GetId();
  if (snid == "vert"_hash || snid == "vertex"_hash || snid == "sphere"_hash)
  {
    return Vertex;
  }
  else if (snid == "edge"_hash || snid == "line"_hash || snid == "spring"_hash)
  {
    return Edge;
  }
  else if (snid == "tri"_hash || snid == "triangle"_hash)
  {
    return Triangle;
  }
  else if (snid == "quad"_hash || snid == "quadrilateral"_hash)
  {
    return Quadrilateral;
  }
  else if (snid == "tet"_hash || snid == "tetrahedron"_hash)
  {
    return Tetrahedron;
  }
  else if (snid == "hex"_hash || snid == "hexahedron"_hash)
  {
    return Hexahedron;
  }
  else if (snid == "wdg"_hash || snid == "wedge"_hash)
  {
    return Wedge;
  }
  else if (snid == "pyr"_hash || snid == "pyramid"_hash)
  {
    return Pyramid;
  }
#else
  switch (shapeName.GetId())
  {
    case "vert"_hash:
    case "vertex"_hash:
    case "sphere"_hash:
      return Vertex;
    case "edge"_hash:
    case "line"_hash:
    case "spring"_hash:
      return Edge;
    case "tri"_hash:
    case "triangle"_hash:
      return Triangle;
    case "quad"_hash:
    case "quadrilateral"_hash:
      return Quadrilateral;
    case "tet"_hash:
    case "tetrahedron"_hash:
      return Tetrahedron;
    case "hex"_hash:
    case "hexahedron"_hash:
      return Hexahedron;
    case "wdg"_hash:
    case "wedge"_hash:
      return Wedge;
    case "pyr"_hash:
    case "pyramid"_hash:
      return Pyramid;
    default:
      break;
  }
#endif
  return None;
}

int vtkDGCell::GetShapeDimension(Shape shape)
{
  switch (shape)
  {
    case Vertex:
      return 0;

    case Edge:
      return 1;

    case Triangle:
    case Quadrilateral:
      return 2;

    case Tetrahedron:
    case Hexahedron:
    case Wedge:
    case Pyramid:
      return 3;

    default:
      break;
  }
  return -1;
}

vtkVector3d vtkDGCell::GetParametricCenterOfSide(int sideId) const
{
  const auto& sideConn = this->GetSideConnectivity(sideId);
  vtkVector3d pp(0, 0, 0);
  double scale = 1. / sideConn.size();
  for (const auto& corner : sideConn)
  {
    const auto& param = this->GetCornerParameter(corner);
    pp = pp + scale * vtkVector3d(param[0], param[1], param[2]);
  }
  return pp;
}

int* vtkDGCell::GetSideRangeForSideType(int sideType)
{
  static thread_local std::array<int, 2> result;
  auto range = this->GetSideRangeForType(sideType);
  result[0] = range.first;
  result[1] = range.second;
  return result.data();
}

std::pair<int, int> vtkDGCell::GetSideRangeForDimension(int dimension) const
{
  if (dimension < 0 || dimension > 3)
  {
    return { -1, -2 };
  }
  else if (dimension == this->GetDimension())
  {
    return { -1, 0 };
  }
  int nn = this->GetNumberOfSideTypes();
  int lo = std::numeric_limits<int>::max(), hi = -1;
  for (int sideType = 0; sideType < nn; ++sideType)
  {
    auto rr = this->GetSideRangeForType(sideType);
    auto shape = this->GetSideShape(rr.first);
    if (vtkDGCell::GetShapeDimension(shape) == dimension)
    {
      if (lo > rr.first)
      {
        lo = rr.first;
      }
      if (hi < rr.second)
      {
        hi = rr.second;
      }
    }
  }
  if (hi >= 0 && lo < hi)
  {
    return { lo, hi };
  }
  return { -1, -2 };
}

int* vtkDGCell::GetSideRangeForSideDimension(int sideDimension)
{
  static thread_local std::array<int, 2> result;
  auto range = this->GetSideRangeForDimension(sideDimension);
  result[0] = range.first;
  result[1] = range.second;
  return result.data();
}

int vtkDGCell::GetSideTypeForShape(Shape s) const
{
  int nn = this->GetNumberOfSideTypes();
  for (int ii = 0; ii < nn; ++ii)
  {
    auto sideRange = this->GetSideRangeForType(ii);
    if (this->GetSideShape(sideRange.first) == s)
    {
      return ii;
    }
  }
  return -1;
}

void vtkDGCell::FillReferencePoints(vtkTypeFloat32Array* arr) const
{
  int nn = this->GetNumberOfCorners();
  arr->SetNumberOfComponents(3);
  arr->SetNumberOfTuples(nn);
  for (int ii = 0; ii < nn; ++ii)
  {
    const auto& dCoord = this->GetCornerParameter(ii);
    std::array<float, 3> fCoord{ { static_cast<float>(dCoord[0]), static_cast<float>(dCoord[1]),
      static_cast<float>(dCoord[2]) } };
    arr->SetTypedTuple(ii, fCoord.data());
  }
}

void vtkDGCell::FillSideConnectivity(vtkTypeInt32Array* arr) const
{
  arr->SetNumberOfComponents(1);
  int tt = this->GetNumberOfSideTypes();
  int vv = 0; // Number of values to hold all side connectivities.
  for (int ii = 0; ii < tt; ++ii)
  {
    auto range = this->GetSideRangeForType(ii);
    if (range.second <= range.first)
    {
      continue;
    } // Ignore empty ranges.

    int pointsPerSide = vtkDGCell::GetShapeCornerCount(this->GetSideShape(range.first));
    vv += pointsPerSide * (range.second - range.first);
  }
  bool selfSide = this->GetDimension() < 3;
  if (selfSide)
  {
    vv += this->GetNumberOfCorners();
  }
  arr->SetNumberOfTuples(vv);

  // Fill in the array.
  vv = 0; // Index of current value.
  if (selfSide)
  {
    auto nc = static_cast<std::int32_t>(this->GetNumberOfCorners());
    for (std::int32_t jj = 0; jj < nc; ++jj)
    {
      arr->SetTypedTuple(vv++, &jj);
    }
  }
  for (int ii = 0; ii < tt; ++ii)
  {
    auto range = this->GetSideRangeForType(ii);

    for (int jj = range.first; jj < range.second; ++jj)
    {
      const auto& conn = this->GetSideConnectivity(jj);
      for (const auto& pointId : conn)
      {
        auto pointIdInt = static_cast<std::int32_t>(pointId);
        arr->SetTypedTuple(vv++, &pointIdInt);
      }
    }
  }
}

void vtkDGCell::FillSideOffsetsAndShapes(vtkTypeInt32Array* arr) const
{
  // Provide self-connectivity for cells of dimension 2 or lower
  // because these cells can be rendered directly with OpenGL primitives.
  int selfSide = this->GetDimension() <= 2 ? 1 : 0;
  int offset = selfSide ? 1 : 0;
  int numSideTypes = this->GetNumberOfSideTypes();
  // Allocate a tuple per side (perhaps plus one to include the cell itself if it is renderable).
  arr->SetNumberOfComponents(2);
  arr->SetNumberOfTuples(numSideTypes + 1 + offset);

  std::array<std::int32_t, 2> tuple{ { 0, this->GetSideShape(selfSide ? -1 : 0) } };
  for (int ii = selfSide ? -1 : 0; ii < numSideTypes; ++ii)
  {
    arr->SetTypedTuple(ii + offset, tuple.data());
#ifdef VTK_DBG_DGCELL
    std::cout << (ii + offset) << " → " << tuple[0] << " " << tuple[1] << " ("
              << vtkDGCell::GetShapeName(static_cast<Shape>(tuple[1])).Data() << ", "
              << vtkDGCell::GetShapeCornerCount(static_cast<Shape>(tuple[1])) << ")\n";
#endif

    auto range = this->GetSideRangeForType(ii);
    int pointsPerSide = vtkDGCell::GetShapeCornerCount(static_cast<Shape>(tuple[1]));
    tuple[0] += pointsPerSide * (range.second - range.first);
    tuple[1] = this->GetSideShape(range.second);
  }
  tuple[1] = this->GetShape(); // The final shape is the cell's shape.
  arr->SetTypedTuple(numSideTypes + offset, tuple.data());
#ifdef VTK_DBG_DGCELL
  std::cout << (numSideTypes + offset) << " → " << tuple[0] << " " << tuple[1] << " ("
            << vtkDGCell::GetShapeName(static_cast<Shape>(tuple[1])).Data() << ", "
            << vtkDGCell::GetShapeCornerCount(static_cast<Shape>(tuple[1])) << ")\n";
#endif
}

vtkCellGridResponders::TagSet vtkDGCell::GetAttributeTags(
  vtkCellAttribute* attribute, bool inheritedTypes)
{
  if (!attribute)
  {
    vtkCellGridResponders::TagSet tags;
    return tags;
  }

  std::unordered_set<vtkStringToken> typeMatches;
  if (inheritedTypes)
  {
    auto hh = this->InheritanceHierarchy();
    typeMatches.insert(hh.begin(), hh.end());
  }
  else
  {
    typeMatches.insert(this->GetClassName());
  }

  auto attributeInfo = attribute->GetCellTypeInfo(this->GetClassName());
  return vtkCellGridResponders::TagSet{ { "Type"_token, typeMatches },
    { "dof-sharing"_token, { attributeInfo.DOFSharing.GetId() } },
    { "function-space"_token, { attributeInfo.FunctionSpace.GetId() } },
    { "basis"_token, { attributeInfo.Basis.GetId() } },
    { "order"_token, { static_cast<vtkStringToken::Hash>(attributeInfo.Order) } } };
}

vtkDGOperatorEntry vtkDGCell::GetOperatorEntry(
  vtkStringToken opName, const vtkCellAttribute::CellTypeInfo& attributeInfo)
{
  std::vector<vtkStringToken> classNames = this->InheritanceHierarchy();
  auto& opMap = vtkDGCell::GetOperators();
  auto opit = opMap.find(opName);
  if (opit == opMap.end())
  {
    return vtkDGOperatorEntry();
  }
  auto fsit = opit->second.find(attributeInfo.FunctionSpace);
  if (fsit == opit->second.end())
  {
    return vtkDGOperatorEntry();
  }
  auto basisit = fsit->second.find(attributeInfo.Basis);
  if (basisit == fsit->second.end())
  {
    return vtkDGOperatorEntry();
  }
  auto orderit = basisit->second.find(attributeInfo.Order);
  if (orderit == basisit->second.end())
  {
    return vtkDGOperatorEntry();
  }
  for (const auto& className : classNames)
  {
    auto cellit = orderit->second.find(className);
    if (cellit == orderit->second.end())
    {
      continue;
    }
    return cellit->second;
  }
  return vtkDGOperatorEntry();
}

vtkDGCell::OperatorMap& vtkDGCell::GetOperators()
{
  return token_NAMESPACE::singletons().get<OperatorMap>();
}

VTK_ABI_NAMESPACE_END
