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
#include "vtkVectorOperators.h"

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
  auto arraysByRole = attribute->GetArraysForCellType(cellType->GetClassName());
  auto typeString = attribute->GetAttributeType().Data();
  auto typeParts = vtksys::SystemTools::SplitString(typeString, ' ');
  if (typeParts.size() != 3)
  {
    return result;
  }
  char* end = nullptr;
  std::string mode = vtksys::SystemTools::LowerCase(typeParts[0]);
  vtkStringToken functionSpace = vtksys::SystemTools::LowerCase(typeParts[1]);
  std::string orderString = typeParts[2].substr(1, std::string::npos);
  char integrationScheme = static_cast<char>(std::tolower(typeParts[2][0])); // i or c
  std::ostringstream basisName;

  bool sharedDOF = !(mode == "dg" || mode == "discontinuous");
  int sideDim;
  int order = static_cast<int>(strtol(orderString.c_str(), &end, 10));
  int op1 = order + 1;
  int op2 = order + 2;
  int op3 = order + 3;
  int basisValueSize;
  int numberOfBasisFunctions;
  int numberOfSides;
  int degreeOfFreedomSize = -1;
  if (functionSpace == "hdiv"_token)
  {
    sideDim = cellType->GetDimension() - 1;
    numberOfSides = cellType->GetNumberOfSidesOfDimension(sideDim);
    basisValueSize = 3;
    basisName << "HDiv";
    // NB: For now, we only support order 0 and 1
    numberOfBasisFunctions = order == 0 ? 1 : numberOfSides;
  }
  else if (functionSpace == "hcurl"_token)
  {
    sideDim = 1;
    numberOfSides = cellType->GetNumberOfSidesOfDimension(sideDim);
    basisValueSize = 3;
    basisName << "HCurl";
    // NB: For now, we only support order 0 and 1
    numberOfBasisFunctions = order == 0 ? 1 : numberOfSides;
  }
  else if (functionSpace == "hgrad"_token || functionSpace == "lagrange"_token)
  {
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
        switch (integrationScheme)
        {
          case 'i': // "I"ncomplete basis
            // A basis function for each of 8 corners and (order - 1) mid-edge points.
            // No mid-face or mid-body points.
            numberOfBasisFunctions = 8 + (order - 1) * 12;
            break;
          default:
          case 'c': // "C"omplete basis
            numberOfBasisFunctions = op1 * op1 * op1;
        }
        break;
      case vtkDGCell::Shape::Tetrahedron:
        numberOfBasisFunctions = op1 * op2 * op3 / 6;
        break;
      case vtkDGCell::Shape::Pyramid:
        // NB: For now we only support constant or linear pyramids.
        numberOfBasisFunctions = order == 0 ? 1 : 5;
        break;
      case vtkDGCell::Shape::Wedge:
        numberOfBasisFunctions = op1 * op1 * op2 / 2;
        break;
      case vtkDGCell::Shape::Quadrilateral:
        numberOfBasisFunctions = op1 * op1;
        break;
      case vtkDGCell::Shape::Triangle:
        numberOfBasisFunctions = (order + 1) * (order + 2) / 2;
        break;
      case vtkDGCell::Shape::Edge:
        numberOfBasisFunctions = order = 1;
        break;
      case vtkDGCell::Shape::Vertex:
        numberOfBasisFunctions = 1;
        break;
    }
  }
  else if (functionSpace == "constant"_token)
  {
    // A single constant value over the entire cell.
    // There is one basis function and its value is 1.0.
    sideDim = 0;
    basisValueSize = 1;
    numberOfSides = 1;
    // We can use an order 0 "HGrad" interpolant for constant values
    // even though we do not have a basis function per cell corner.
    basisName << "HGrad";
    numberOfBasisFunctions = 1;
  }
  else
  {
    basisValueSize = 1;
    numberOfSides = 0;
    numberOfBasisFunctions = 0;
    sideDim = 0;
    basisName << "None";
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
#if 1
      degreeOfFreedomSize = valueSize;
#else
      auto it = arraysByRole.find("connectivity");
      if (it != arraysByRole.end())
      {
        vtkIdType connSize = it->second->GetNumberOfComponents();
        degreeOfFreedomSize = connSize / numberOfBasisFunctions;
      }
#endif
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
