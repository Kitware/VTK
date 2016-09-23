/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellQuality.h"
#include "vtk_verdict.h"

#include "vtkCellData.h"
#include "vtkCell.h"
#include "vtkCellTypes.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMeshQuality.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include <utility>

vtkStandardNewMacro(vtkCellQuality);

double vtkCellQuality::CurrentTriNormal [3];

vtkCellQuality::~vtkCellQuality ()
{
  this->PointIds->Delete();
  this->Points->Delete();
}

vtkCellQuality::vtkCellQuality ()
{
  this->QualityMeasure = NONE;
  this->UnsupportedGeometry = -1;
  this->UndefinedQuality = -1;
  this->PointIds = vtkIdList::New();
  this->Points = vtkPoints::New();
}

void vtkCellQuality::PrintSelf (ostream& os, vtkIndent indent)
{
  static const char* CellQualityMeasureNames [] =
  {
    "None",
    "Area",
    "AspectBeta"
    "AspectFrobenius",
    "AspectGamma",
    "AspectRatio",
    "CollapseRatio",
    "Condition",
    "Diagonal",
    "Dimension",
    "Distortion",
    "EdgeRatio",
    "Jacobian",
    "MaxAngle",
    "MaxAspectFrobenius",
    "MaxEdgeRatio",
    "MedAspectFrobenius",
    "MinAngle",
    "Oddy",
    "RadiusRatio",
    "RelativeSizeSquared",
    "ScaledJacobian",
    "Shape",
    "ShapeAndSize",
    "Shear",
    "ShearAndSize",
    "Skew",
    "Stretch",
    "Taper",
    "Volume",
    "Warpage",
  };

  const char* name = CellQualityMeasureNames[this->QualityMeasure];
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TriangleQualityMeasure : " << name << endl;
  os << indent << "QuadQualityMeasure : " << name << endl;
  os << indent << "TetQualityMeasure : " << name << endl;
  os << indent << "HexQualityMeasure : " << name << endl;
  os << indent << "TriangleStripQualityMeasure : " << name << endl;
  os << indent << "PixelQualityMeasure : " << name << endl;

  os << indent << "UnsupportedGeometry : " << this->UnsupportedGeometry << endl;
  os << indent << "UndefinedQuality : " << this->UndefinedQuality << endl;
}

int vtkCellQuality::RequestData
  (vtkInformation* vtkNotUsed(request),
   vtkInformationVector** inputVector,
   vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* in = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* out = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Copy input to get a start point
  out->ShallowCopy(in);

  // Allocate storage for cell quality
  vtkIdType const nCells = in->GetNumberOfCells();
  vtkSmartPointer<vtkDoubleArray> quality =
    vtkSmartPointer<vtkDoubleArray>::New();
  quality->SetName("CellQuality");
  quality->SetNumberOfValues(nCells);

  vtkDataArray* CellNormals = in->GetCellData()->GetNormals();
  if (CellNormals)
  {
    v_set_tri_normal_func(reinterpret_cast<ComputeNormal>
      (vtkCellQuality::GetCurrentTriangleNormal));
  }
  else
  {
    v_set_tri_normal_func(0);
  }

  // Support progress and abort.
  vtkIdType const tenth = (nCells >= 10 ? nCells/10 : 1);
  double const nCellInv = 1./nCells;

  // Actual computation of the selected quality
  for (vtkIdType cid = 0; cid < nCells; ++cid)
  {
    // Periodically update progress and check for an abort request.
    if (0 == cid % tenth)
    {
      this->UpdateProgress((cid+1)*nCellInv);
      if (this->GetAbortExecute())
      {
        break;
      }
    }

    vtkCell* cell = out->GetCell(cid);
    double q = 0;
    switch (cell->GetCellType())
    {
      default:
        q = this->GetUnsupportedGeometry();
        break;

      // Below are supported types. Please be noted that not every quality is
      // defined for all supported geometries. For those quality that is not
      // defined with respective to a particular cell type,
      // this->GetUndefinedQuality() is returned.
      case VTK_TRIANGLE:
        if (CellNormals)
          CellNormals->GetTuple(cid, vtkCellQuality::CurrentTriNormal);
        q = this->ComputeTriangleQuality(cell);
        break;
      case VTK_TRIANGLE_STRIP:
        q = this->ComputeTriangleStripQuality(cell);
        break;
      case VTK_PIXEL:
        q = this->ComputePixelQuality(cell);
        break;
      case VTK_QUAD:
        q = this->ComputeQuadQuality(cell);
        break;
      case VTK_TETRA:
        q = this->ComputeTetQuality(cell);
        break;
      case VTK_HEXAHEDRON:
        q = this->ComputeHexQuality(cell);
        break;
    }
    quality->SetValue(cid, q);
  }

  out->GetCellData()->AddArray(quality);
  out->GetCellData()->SetActiveAttribute("CellQuality", vtkDataSetAttributes::SCALARS);

  return 1;
}

double vtkCellQuality::ComputeTriangleQuality (vtkCell* cell)
{
  switch (this->GetQualityMeasure())
  {
    case AREA:             return vtkMeshQuality::TriangleArea(cell);
    case ASPECT_FROBENIUS: return vtkMeshQuality::TriangleAspectFrobenius(cell);
    case ASPECT_RATIO:     return vtkMeshQuality::TriangleAspectRatio(cell);
    case CONDITION:        return vtkMeshQuality::TriangleCondition(cell);
    case DISTORTION:       return vtkMeshQuality::TriangleDistortion(cell);
    case EDGE_RATIO:       return vtkMeshQuality::TriangleEdgeRatio(cell);
    case MAX_ANGLE:        return vtkMeshQuality::TriangleMaxAngle(cell);
    case MIN_ANGLE:        return vtkMeshQuality::TriangleMinAngle(cell);
    case RADIUS_RATIO:     return vtkMeshQuality::TriangleRadiusRatio(cell);
    case RELATIVE_SIZE_SQUARED:
      return vtkMeshQuality::TriangleRelativeSizeSquared(cell);
    case SCALED_JACOBIAN:  return vtkMeshQuality::TriangleScaledJacobian(cell);
    case SHAPE_AND_SIZE:   return vtkMeshQuality::TriangleShapeAndSize(cell);
    case SHAPE:            return vtkMeshQuality::TriangleShape(cell);
    default:               return this->GetUndefinedQuality();
  }
}

double vtkCellQuality::ComputeQuadQuality (vtkCell* cell)
{
  switch (this->GetQualityMeasure())
  {
    case AREA:           return vtkMeshQuality::QuadArea(cell);
    case ASPECT_RATIO:   return vtkMeshQuality::QuadAspectRatio(cell);
    case CONDITION:      return vtkMeshQuality::QuadCondition(cell);
    case DISTORTION:     return vtkMeshQuality::QuadDistortion(cell);
    case EDGE_RATIO:     return vtkMeshQuality::QuadEdgeRatio(cell);
    case JACOBIAN:       return vtkMeshQuality::QuadJacobian(cell);
    case MAX_ANGLE:      return vtkMeshQuality::QuadMaxAngle(cell);
    case MAX_ASPECT_FROBENIUS:
      return vtkMeshQuality::QuadMaxAspectFrobenius(cell);
    case MAX_EDGE_RATIO: return vtkMeshQuality::QuadMaxEdgeRatios(cell);
    case MED_ASPECT_FROBENIUS:
      return vtkMeshQuality::QuadMedAspectFrobenius(cell);
    case MIN_ANGLE:      return vtkMeshQuality::QuadMinAngle(cell);
    case ODDY:           return vtkMeshQuality::QuadOddy(cell);
    case RADIUS_RATIO:   return vtkMeshQuality::QuadRadiusRatio(cell);
    case RELATIVE_SIZE_SQUARED:
      return vtkMeshQuality::QuadRelativeSizeSquared(cell);
    case SCALED_JACOBIAN:return vtkMeshQuality::QuadScaledJacobian(cell);
    case SHAPE_AND_SIZE: return vtkMeshQuality::QuadShapeAndSize(cell);
    case SHAPE:          return vtkMeshQuality::QuadShape(cell);
    case SHEAR_AND_SIZE: return vtkMeshQuality::QuadShearAndSize(cell);
    case SHEAR:          return vtkMeshQuality::QuadShear(cell);
    case SKEW:           return vtkMeshQuality::QuadSkew(cell);
    case STRETCH:        return vtkMeshQuality::QuadStretch(cell);
    case TAPER:          return vtkMeshQuality::QuadTaper(cell);
    case WARPAGE:        return vtkMeshQuality::QuadWarpage(cell);
    default:             return this->GetUndefinedQuality();
  }
}

double vtkCellQuality::ComputeTetQuality (vtkCell* cell)
{
  switch (this->GetQualityMeasure())
  {
    case ASPECT_BETA:      return vtkMeshQuality::TetAspectBeta(cell);
    case ASPECT_FROBENIUS: return vtkMeshQuality::TetAspectFrobenius(cell);
    case ASPECT_GAMMA:     return vtkMeshQuality::TetAspectGamma(cell);
    case ASPECT_RATIO:     return vtkMeshQuality::TetAspectRatio(cell);
    case COLLAPSE_RATIO:   return vtkMeshQuality::TetCollapseRatio(cell);
    case CONDITION:        return vtkMeshQuality::TetCondition(cell);
    case DISTORTION:       return vtkMeshQuality::TetDistortion(cell);
    case EDGE_RATIO:       return vtkMeshQuality::TetEdgeRatio(cell);
    case JACOBIAN:         return vtkMeshQuality::TetJacobian(cell);
    case MIN_ANGLE:        return vtkMeshQuality::TetMinAngle(cell);
    case RADIUS_RATIO:     return vtkMeshQuality::TetRadiusRatio(cell);
    case RELATIVE_SIZE_SQUARED:
      return vtkMeshQuality::TetRelativeSizeSquared(cell);
    case SCALED_JACOBIAN:  return vtkMeshQuality::TetScaledJacobian(cell);
    case SHAPE_AND_SIZE:   return vtkMeshQuality::TetShapeandSize(cell);
    case SHAPE:            return vtkMeshQuality::TetShape(cell);
    case VOLUME:           return vtkMeshQuality::TetVolume(cell);
    default:               return this->GetUndefinedQuality();
  }
}

double vtkCellQuality::ComputeHexQuality (vtkCell* cell)
{
  switch (this->GetQualityMeasure())
  {
    case CONDITION:      return vtkMeshQuality::HexCondition(cell);
    case DIAGONAL:       return vtkMeshQuality::HexDiagonal(cell);
    case DIMENSION:      return vtkMeshQuality::HexDimension(cell);
    case DISTORTION:     return vtkMeshQuality::HexDistortion(cell);
    case EDGE_RATIO:     return vtkMeshQuality::HexEdgeRatio(cell);
    case JACOBIAN:       return vtkMeshQuality::HexJacobian(cell);
    case MAX_ASPECT_FROBENIUS:
      return vtkMeshQuality::HexMaxAspectFrobenius(cell);
    case MAX_EDGE_RATIO: return vtkMeshQuality::HexMaxEdgeRatio(cell);
    case MED_ASPECT_FROBENIUS:
      return vtkMeshQuality::HexMedAspectFrobenius(cell);
    case ODDY:           return vtkMeshQuality::HexOddy(cell);
    case RELATIVE_SIZE_SQUARED:
      return vtkMeshQuality::HexRelativeSizeSquared(cell);
    case SCALED_JACOBIAN:return vtkMeshQuality::HexScaledJacobian(cell);
    case SHAPE_AND_SIZE: return vtkMeshQuality::HexShapeAndSize(cell);
    case SHAPE:          return vtkMeshQuality::HexShape(cell);
    case SHEAR_AND_SIZE: return vtkMeshQuality::HexShearAndSize(cell);
    case SHEAR:          return vtkMeshQuality::HexShear(cell);
    case SKEW:           return vtkMeshQuality::HexSkew(cell);
    case STRETCH:        return vtkMeshQuality::HexStretch(cell);
    case TAPER:          return vtkMeshQuality::HexTaper(cell);
    case VOLUME:         return vtkMeshQuality::HexVolume(cell);
    default:             return this->GetUndefinedQuality();
  }
}

double vtkCellQuality::ComputeTriangleStripQuality (vtkCell* cell)
{
  switch (this->GetQualityMeasure())
  {
    case AREA: return vtkCellQuality::TriangleStripArea(cell);
    default:   return this->GetUndefinedQuality();
  }
}

double vtkCellQuality::ComputePixelQuality (vtkCell* cell)
{
  switch (this->GetQualityMeasure())
  {
    case AREA: return vtkCellQuality::PixelArea(cell);
    default:   return this->GetUndefinedQuality();
  }
}

int vtkCellQuality::GetCurrentTriangleNormal (double [3], double normal [3])
{
  // copy the cell normal
  normal[0] = vtkCellQuality::CurrentTriNormal[0];
  normal[1] = vtkCellQuality::CurrentTriNormal[1];
  normal[2] = vtkCellQuality::CurrentTriNormal[2];
  return 1;
}

// Triangle strip quality metrics

double vtkCellQuality::TriangleStripArea (vtkCell* cell)
{
  return this->PolygonArea(cell);
}

// Pixel quality metrics

double vtkCellQuality::PixelArea (vtkCell* cell)
{
  return this->PolygonArea(cell);
}

// Polygon quality metrics

double vtkCellQuality::PolygonArea (vtkCell* cell)
{
  cell->Triangulate(0, this->PointIds, this->Points);
  double abc [3][3], quality = 0;
  for (vtkIdType i = 0, n = this->Points->GetNumberOfPoints(); i < n; i += 3)
  {
    this->Points->GetPoint(i+0, abc[0]);
    this->Points->GetPoint(i+1, abc[1]);
    this->Points->GetPoint(i+2, abc[2]);
    quality += vtkTriangle::TriangleArea(abc[0], abc[1], abc[2]);
  }
  return quality;
}
