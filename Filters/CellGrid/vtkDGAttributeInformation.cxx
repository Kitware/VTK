// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGAttributeInformation.h"

#include "vtkAbstractArray.h"
#include "vtkBoundingBox.h"
#include "vtkCellAttribute.h"
#include "vtkCellMetadata.h"
#include "vtkDGCell.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkVector.h"

#include <cctype>  // for std::tolower/std::toupper
#include <cstdlib> // for strtol
#include <sstream> // for basisName

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkDGAttributeInformation);

void vtkDGAttributeInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "BasisOrder: " << this->BasisOrder << "\n";
  os << indent << "BasisValueSize: " << this->BasisValueSize << "\n";
  os << indent << "NumberOfBasisFunctions: " << this->NumberOfBasisFunctions << "\n";
  os << indent << "DegreeOfFreedomSize: " << this->DegreeOfFreedomSize << "\n";
  os << indent << "SharedDegreesOfFreedom: " << (this->SharedDegreesOfFreedom ? "T" : "F") << "\n";
}

std::string vtkDGAttributeInformation::BasisShapeName(vtkDGCell* cellType)
{
  auto shape = cellType ? cellType->GetShape() : vtkDGCell::Shape::None;
  switch (shape)
  {
    case vtkDGCell::Shape::Vertex:
      return "Vert";
    case vtkDGCell::Shape::Edge:
      return "Edge";
    case vtkDGCell::Shape::Triangle:
      return "Tri";
    case vtkDGCell::Shape::Quadrilateral:
      return "Quad";
    case vtkDGCell::Shape::Tetrahedron:
      return "Tet";
    case vtkDGCell::Shape::Hexahedron:
      return "Hex";
    case vtkDGCell::Shape::Wedge:
      return "Wdg";
    case vtkDGCell::Shape::Pyramid:
      return "Pyr";
    default:
    case vtkDGCell::Shape::None:
      break;
  }
  return "None";
}

vtkSmartPointer<vtkCellAttributeCalculator> vtkDGAttributeInformation::PrepareForGrid(
  vtkCellMetadata* metadata, vtkCellAttribute* attribute)
{
  vtkSmartPointer<vtkDGAttributeInformation> result;
  auto* cellType = vtkDGCell::SafeDownCast(metadata);
  if (!cellType || !attribute)
  {
    return result;
  }
  auto cellTypeInfo = attribute->GetCellTypeInfo(cellType->GetClassName());
  auto& arraysByRole = cellTypeInfo.ArraysByRole;
  vtkStringToken functionSpace = cellTypeInfo.FunctionSpace;
  if (functionSpace.HasData())
  {
    // Downcase the string so we are case-insensitive.
    functionSpace = vtksys::SystemTools::LowerCase(cellTypeInfo.FunctionSpace.Data());
  }
  // integrationScheme should be i or c or f:
  char integrationScheme = 'c';
  std::ostringstream basisName;

  bool sharedDOF = cellTypeInfo.DOFSharing.IsValid();
  int sideDim;
  int order = cellTypeInfo.Order;
  int op1 = order + 1;
  int op2 = order + 2;
  int op3 = order + 3;
  int basisValueSize;
  int numberOfBasisFunctions;
  int numberOfSides;
  int degreeOfFreedomSize = -1;
  switch (functionSpace.GetId())
  {
    case "hdiv"_hash:
    case "HDIV"_hash:
    case "HDiv"_hash:
      sideDim = cellType->GetDimension() - 1;
      numberOfSides = cellType->GetNumberOfSidesOfDimension(sideDim);
      basisValueSize = 3;
      basisName << "HDiv";
      integrationScheme = 'i';
      // NB: For now, we only support order 0 and 1
      numberOfBasisFunctions = order == 0 ? 1 : numberOfSides;
      break;
    case "hcurl"_hash:
    case "HCURL"_hash:
    case "HCurl"_hash:
      sideDim = 1;
      numberOfSides = cellType->GetNumberOfSidesOfDimension(sideDim);
      basisValueSize = 3;
      basisName << "HCurl";
      integrationScheme = 'i';
      // NB: For now, we only support order 0 and 1
      numberOfBasisFunctions = order == 0 ? 1 : numberOfSides;
      break;
    case "hgrad"_hash:
    case "HGRAD"_hash:
    case "HGrad"_hash:
    case "lagrange"_hash:
    case "Lagrange"_hash:
      sideDim = 0;
      basisValueSize = 1;
      numberOfSides = cellType->GetNumberOfSidesOfDimension(sideDim);
      basisName << "HGrad";
      switch (cellType->GetShape())
      {
        default:
        case vtkDGCell::Shape::None:
          numberOfBasisFunctions = 0;
          break;
        case vtkDGCell::Shape::Hexahedron:
          switch (cellTypeInfo.Basis.GetId())
          {
            case "I"_hash: // "I"ncomplete basis
            case "i"_hash: // "I"ncomplete basis
              // A basis function for each of 8 corners and (order - 1) mid-edge points.
              // No mid-face or mid-body points.
              numberOfBasisFunctions = 8 + (order - 1) * 12;
              integrationScheme = 'i';
              break;
            default:
            case "C"_hash: // "C"omplete basis
            case "c"_hash: // "C"omplete basis
              numberOfBasisFunctions = op1 * op1 * op1;
              integrationScheme = 'c';
          }
          break;
        case vtkDGCell::Shape::Tetrahedron:
          switch (cellTypeInfo.Basis.GetId())
          {
            case "F"_hash: // "F"ull basis
              numberOfBasisFunctions = 15;
              integrationScheme = 'f';
              break;
            default:
            case "C"_hash: // "C"omplete basis
              numberOfBasisFunctions = op1 * op2 * op3 / 6;
              integrationScheme = 'c';
              break;
          }
          break;
        case vtkDGCell::Shape::Pyramid:
          // We only handle order 0, 1, or 2 for Complete and Full bases:
          switch (cellTypeInfo.Basis.GetId())
          {
            case "F"_hash: // "F"ull basis
              numberOfBasisFunctions = (order == 2 ? 19 : (order == 1 ? 5 : 1));
              integrationScheme = 'f';
              break;
            default:
            case "C"_hash: // "C"omplete basis
              numberOfBasisFunctions = (order == 2 ? 18 : (order == 1 ? 5 : 1));
              integrationScheme = 'c';
              break;
          }
          break;
        case vtkDGCell::Shape::Wedge:
          switch (cellTypeInfo.Basis.GetId())
          {
            case "F"_hash:                 // "F"ull basis
              numberOfBasisFunctions = 21; // Only support wedge-21 for now.
              integrationScheme = 'f';
              break;
            default:
            case "C"_hash: // "C"omplete basis
              numberOfBasisFunctions = op1 * op1 * op2 / 2;
              integrationScheme = 'c';
              break;
          }
          break;
        case vtkDGCell::Shape::Quadrilateral:
          numberOfBasisFunctions = op1 * op1;
          integrationScheme = 'c';
          break;
        case vtkDGCell::Shape::Triangle:
          numberOfBasisFunctions = (order + 1) * (order + 2) / 2;
          integrationScheme = 'c';
          break;
        case vtkDGCell::Shape::Edge:
          numberOfBasisFunctions = order + 1;
          integrationScheme = 'c';
          break;
        case vtkDGCell::Shape::Vertex:
          numberOfBasisFunctions = 1;
          integrationScheme = 'c';
          break;
      }
      break;
    case "constant"_hash:
      // A single constant value over the entire cell.
      // There is one basis function and its value is 1.0.
      sideDim = 0;
      basisValueSize = 1;
      numberOfSides = 1;
      // We can use an order 0 "HGrad" interpolant for constant values
      // even though we do not have a basis function per cell corner.
      basisName << "HGrad";
      numberOfBasisFunctions = 1;
      break;
    default:
      basisValueSize = 1;
      numberOfSides = 0;
      numberOfBasisFunctions = 0;
      sideDim = 0;
      basisName << "None";
      break;
  }

  basisName << vtkDGAttributeInformation::BasisShapeName(cellType)
            << static_cast<char>(std::toupper(integrationScheme)) << order;

  auto valuesIt = arraysByRole.find("values");
  if (valuesIt != arraysByRole.end())
  {
    // The number of components per array value is
    // + the number of values per degree of freedom when sharedDOF is true
    // + the number of basis functions * the number of values per degree of freedom when sharedDOF
    // is false.
    vtkIdType valueSize = valuesIt->second->GetNumberOfComponents();
    if (functionSpace == "constant"_token)
    {
      degreeOfFreedomSize = valueSize;
    }
    else if (sharedDOF)
    {
      degreeOfFreedomSize = valueSize;
    }
    else
    {
      degreeOfFreedomSize = valueSize / numberOfBasisFunctions;
    }
  }

  if (degreeOfFreedomSize < 0)
  {
    vtkErrorMacro("Unsupported attribute. Could not determine DegreeOfFreedomSize.");
    return result;
  }
  if (degreeOfFreedomSize * basisValueSize != attribute->GetNumberOfComponents())
  {
    vtkErrorMacro("Unsupported attribute. Mismatched output size "
      << (degreeOfFreedomSize * basisValueSize) << " vs " << attribute->GetNumberOfComponents()
      << ".");
    return result;
  }
  result = vtkSmartPointer<vtkDGAttributeInformation>::New();
  result->BasisOrder = order;
  result->BasisValueSize = basisValueSize;
  result->NumberOfBasisFunctions = numberOfBasisFunctions;
  result->DegreeOfFreedomSize = degreeOfFreedomSize;
  result->SharedDegreesOfFreedom = sharedDOF;
  result->BasisName = basisName.str();
  return result;
}

VTK_ABI_NAMESPACE_END
