/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellQuality.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCellQuality.h"

#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkCellTypes.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCell.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkCellQuality);

//----------------------------------------------------------------------------
vtkCellQuality::~vtkCellQuality()
{
  this->PointIds->Delete();
  this->Points->Delete();
}

//----------------------------------------------------------------------------
vtkCellQuality::vtkCellQuality()
{
  this->QualityMeasure = vtkMeshQuality::NONE;
  this->UnsupportedGeometry = -1;
  this->UndefinedQuality = -1;
  this->PointIds = vtkIdList::New();
  this->Points = vtkPoints::New();
}

//----------------------------------------------------------------------------
void vtkCellQuality::PrintSelf(ostream& os, vtkIndent indent)
{
  const char* name = vtkMeshQuality::QualityMeasureNames[this->QualityMeasure];
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TriangleQualityMeasure : " << name << endl;
  os << indent << "QuadQualityMeasure : " << name << endl;
  os << indent << "TetQualityMeasure : " << name << endl;
  os << indent << "PyramidQualityMeasure : " << name << endl;
  os << indent << "WedgeQualityMeasure : " << name << endl;
  os << indent << "HexQualityMeasure : " << name << endl;
  os << indent << "TriangleStripQualityMeasure : " << name << endl;
  os << indent << "PixelQualityMeasure : " << name << endl;

  os << indent << "UnsupportedGeometry : " << this->UnsupportedGeometry << endl;
  os << indent << "UndefinedQuality : " << this->UndefinedQuality << endl;
}

//----------------------------------------------------------------------------
class vtkCellQualityFunctor
{
private:
  vtkSMPThreadLocalObject<vtkGenericCell> Cell;
  vtkCellQuality* CellQuality;
  vtkDataSet* Output;
  vtkSmartPointer<vtkDoubleArray> Quality;

public:
  vtkCellQualityFunctor(
    vtkCellQuality* cellQuality, vtkDataSet* output, vtkSmartPointer<vtkDoubleArray> quality)
    : CellQuality(cellQuality)
    , Output(output)
    , Quality(quality)
  {
    // instantiate any data-structure that needs to be cached for parallel execution.
    vtkNew<vtkGenericCell> cell;
    this->Output->GetCell(0, cell);
  }

  void operator()(vtkIdType begin, vtkIdType end)
  {
    auto& genericCell = this->Cell.Local();
    vtkCell* cell;
    double q;
    for (vtkIdType i = begin; i < end; ++i)
    {
      this->Output->GetCell(i, genericCell);
      cell = genericCell->GetRepresentativeCell();
      switch (cell->GetCellType())
      {
        default:
          q = this->CellQuality->GetUnsupportedGeometry();
          break;

        // Below are supported types. Please be noted that not every quality is
        // defined for all supported geometries. For those quality that is not
        // defined with respective to a particular cell type,
        // this->GetUndefinedQuality() is returned.
        case VTK_TRIANGLE:
          q = this->CellQuality->ComputeTriangleQuality(cell);
          break;
        case VTK_TRIANGLE_STRIP:
          q = this->CellQuality->ComputeTriangleStripQuality(cell);
          break;
        case VTK_PIXEL:
          q = this->CellQuality->ComputePixelQuality(cell);
          break;
        case VTK_QUAD:
          q = this->CellQuality->ComputeQuadQuality(cell);
          break;
        case VTK_TETRA:
          q = this->CellQuality->ComputeTetQuality(cell);
          break;
        case VTK_PYRAMID:
          q = this->CellQuality->ComputePyramidQuality(cell);
          break;
        case VTK_WEDGE:
          q = this->CellQuality->ComputeWedgeQuality(cell);
          break;
        case VTK_HEXAHEDRON:
          q = this->CellQuality->ComputeHexQuality(cell);
          break;
      }
      this->Quality->SetValue(i, q);
    }
  }
};

int vtkCellQuality::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet* in = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* out = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Copy input to get a start point
  out->ShallowCopy(in);

  // Allocate storage for cell quality
  vtkIdType const nCells = in->GetNumberOfCells();
  auto quality = vtkSmartPointer<vtkDoubleArray>::New();
  quality->SetName("CellQuality");
  quality->SetNumberOfValues(nCells);

  // Set the output quality array
  vtkCellQualityFunctor cellQualityFunctor(this, out, quality);
  vtkSMPTools::For(0, nCells, cellQualityFunctor);

  out->GetCellData()->AddArray(quality);
  out->GetCellData()->SetActiveAttribute("CellQuality", vtkDataSetAttributes::SCALARS);

  return 1;
}

//----------------------------------------------------------------------------
double vtkCellQuality::ComputeTriangleQuality(vtkCell* cell)
{
  switch (this->GetQualityMeasure())
  {
    case vtkMeshQuality::AREA:
      return vtkMeshQuality::TriangleArea(cell);
    case vtkMeshQuality::ASPECT_FROBENIUS:
      return vtkMeshQuality::TriangleAspectFrobenius(cell);
    case vtkMeshQuality::ASPECT_RATIO:
      return vtkMeshQuality::TriangleAspectRatio(cell);
    case vtkMeshQuality::CONDITION:
      return vtkMeshQuality::TriangleCondition(cell);
    case vtkMeshQuality::DISTORTION:
      return vtkMeshQuality::TriangleDistortion(cell);
    case vtkMeshQuality::EDGE_RATIO:
      return vtkMeshQuality::TriangleEdgeRatio(cell);
    case vtkMeshQuality::EQUIANGLE_SKEW:
      return vtkMeshQuality::TriangleEquiangleSkew(cell);
    case vtkMeshQuality::MAX_ANGLE:
      return vtkMeshQuality::TriangleMaxAngle(cell);
    case vtkMeshQuality::MIN_ANGLE:
      return vtkMeshQuality::TriangleMinAngle(cell);
    case vtkMeshQuality::NORMALIZED_INRADIUS:
      return vtkMeshQuality::TriangleNormalizedInradius(cell);
    case vtkMeshQuality::RADIUS_RATIO:
      return vtkMeshQuality::TriangleRadiusRatio(cell);
    case vtkMeshQuality::RELATIVE_SIZE_SQUARED:
      return vtkMeshQuality::TriangleRelativeSizeSquared(cell);
    case vtkMeshQuality::SCALED_JACOBIAN:
      return vtkMeshQuality::TriangleScaledJacobian(cell);
    case vtkMeshQuality::SHAPE_AND_SIZE:
      return vtkMeshQuality::TriangleShapeAndSize(cell);
    case vtkMeshQuality::SHAPE:
      return vtkMeshQuality::TriangleShape(cell);
    default:
      return this->GetUndefinedQuality();
  }
}

//----------------------------------------------------------------------------
double vtkCellQuality::ComputeQuadQuality(vtkCell* cell)
{
  switch (this->GetQualityMeasure())
  {
    case vtkMeshQuality::AREA:
      return vtkMeshQuality::QuadArea(cell);
    case vtkMeshQuality::ASPECT_RATIO:
      return vtkMeshQuality::QuadAspectRatio(cell);
    case vtkMeshQuality::CONDITION:
      return vtkMeshQuality::QuadCondition(cell);
    case vtkMeshQuality::DISTORTION:
      return vtkMeshQuality::QuadDistortion(cell);
    case vtkMeshQuality::EDGE_RATIO:
      return vtkMeshQuality::QuadEdgeRatio(cell);
    case vtkMeshQuality::EQUIANGLE_SKEW:
      return vtkMeshQuality::QuadEquiangleSkew(cell);
    case vtkMeshQuality::JACOBIAN:
      return vtkMeshQuality::QuadJacobian(cell);
    case vtkMeshQuality::MAX_ANGLE:
      return vtkMeshQuality::QuadMaxAngle(cell);
    case vtkMeshQuality::MAX_ASPECT_FROBENIUS:
      return vtkMeshQuality::QuadMaxAspectFrobenius(cell);
    case vtkMeshQuality::MAX_EDGE_RATIO:
      return vtkMeshQuality::QuadMaxEdgeRatio(cell);
    case vtkMeshQuality::MED_ASPECT_FROBENIUS:
      return vtkMeshQuality::QuadMedAspectFrobenius(cell);
    case vtkMeshQuality::MIN_ANGLE:
      return vtkMeshQuality::QuadMinAngle(cell);
    case vtkMeshQuality::ODDY:
      return vtkMeshQuality::QuadOddy(cell);
    case vtkMeshQuality::RADIUS_RATIO:
      return vtkMeshQuality::QuadRadiusRatio(cell);
    case vtkMeshQuality::RELATIVE_SIZE_SQUARED:
      return vtkMeshQuality::QuadRelativeSizeSquared(cell);
    case vtkMeshQuality::SCALED_JACOBIAN:
      return vtkMeshQuality::QuadScaledJacobian(cell);
    case vtkMeshQuality::SHAPE_AND_SIZE:
      return vtkMeshQuality::QuadShapeAndSize(cell);
    case vtkMeshQuality::SHAPE:
      return vtkMeshQuality::QuadShape(cell);
    case vtkMeshQuality::SHEAR_AND_SIZE:
      return vtkMeshQuality::QuadShearAndSize(cell);
    case vtkMeshQuality::SHEAR:
      return vtkMeshQuality::QuadShear(cell);
    case vtkMeshQuality::SKEW:
      return vtkMeshQuality::QuadSkew(cell);
    case vtkMeshQuality::STRETCH:
      return vtkMeshQuality::QuadStretch(cell);
    case vtkMeshQuality::TAPER:
      return vtkMeshQuality::QuadTaper(cell);
    case vtkMeshQuality::WARPAGE:
      return vtkMeshQuality::QuadWarpage(cell);
    default:
      return this->GetUndefinedQuality();
  }
}

//----------------------------------------------------------------------------
double vtkCellQuality::ComputeTetQuality(vtkCell* cell)
{
  switch (this->GetQualityMeasure())
  {
    case vtkMeshQuality::ASPECT_FROBENIUS:
      return vtkMeshQuality::TetAspectFrobenius(cell);
    case vtkMeshQuality::ASPECT_GAMMA:
      return vtkMeshQuality::TetAspectGamma(cell);
    case vtkMeshQuality::ASPECT_RATIO:
      return vtkMeshQuality::TetAspectRatio(cell);
    case vtkMeshQuality::COLLAPSE_RATIO:
      return vtkMeshQuality::TetCollapseRatio(cell);
    case vtkMeshQuality::CONDITION:
      return vtkMeshQuality::TetCondition(cell);
    case vtkMeshQuality::DISTORTION:
      return vtkMeshQuality::TetDistortion(cell);
    case vtkMeshQuality::EDGE_RATIO:
      return vtkMeshQuality::TetEdgeRatio(cell);
    case vtkMeshQuality::EQUIANGLE_SKEW:
      return vtkMeshQuality::TetEquiangleSkew(cell);
    case vtkMeshQuality::EQUIVOLUME_SKEW:
      return vtkMeshQuality::TetEquivolumeSkew(cell);
    case vtkMeshQuality::JACOBIAN:
      return vtkMeshQuality::TetJacobian(cell);
    case vtkMeshQuality::MEAN_RATIO:
      return vtkMeshQuality::TetMeanRatio(cell);
    case vtkMeshQuality::MIN_ANGLE:
      return vtkMeshQuality::TetMinAngle(cell);
    case vtkMeshQuality::NORMALIZED_INRADIUS:
      return vtkMeshQuality::TetNormalizedInradius(cell);
    case vtkMeshQuality::RADIUS_RATIO:
      return vtkMeshQuality::TetRadiusRatio(cell);
    case vtkMeshQuality::RELATIVE_SIZE_SQUARED:
      return vtkMeshQuality::TetRelativeSizeSquared(cell);
    case vtkMeshQuality::SCALED_JACOBIAN:
      return vtkMeshQuality::TetScaledJacobian(cell);
    case vtkMeshQuality::SHAPE_AND_SIZE:
      return vtkMeshQuality::TetShapeAndSize(cell);
    case vtkMeshQuality::SHAPE:
      return vtkMeshQuality::TetShape(cell);
    case vtkMeshQuality::SQUISH_INDEX:
      return vtkMeshQuality::TetSquishIndex(cell);
    case vtkMeshQuality::VOLUME:
      return vtkMeshQuality::TetVolume(cell);
    default:
      return this->GetUndefinedQuality();
  }
}

//----------------------------------------------------------------------------
double vtkCellQuality::ComputePyramidQuality(vtkCell* cell)
{
  switch (this->GetQualityMeasure())
  {
    case vtkMeshQuality::EQUIANGLE_SKEW:
      return vtkMeshQuality::PyramidEquiangleSkew(cell);
    case vtkMeshQuality::JACOBIAN:
      return vtkMeshQuality::PyramidJacobian(cell);
    case vtkMeshQuality::SCALED_JACOBIAN:
      return vtkMeshQuality::PyramidScaledJacobian(cell);
    case vtkMeshQuality::SHAPE:
      return vtkMeshQuality::PyramidShape(cell);
    case vtkMeshQuality::VOLUME:
      return vtkMeshQuality::PyramidVolume(cell);
    default:
      return this->GetUndefinedQuality();
  }
}

//----------------------------------------------------------------------------
double vtkCellQuality::ComputeWedgeQuality(vtkCell* cell)
{
  switch (this->GetQualityMeasure())
  {
    case vtkMeshQuality::CONDITION:
      return vtkMeshQuality::WedgeCondition(cell);
    case vtkMeshQuality::DISTORTION:
      return vtkMeshQuality::WedgeDistortion(cell);
    case vtkMeshQuality::EDGE_RATIO:
      return vtkMeshQuality::WedgeEdgeRatio(cell);
    case vtkMeshQuality::EQUIANGLE_SKEW:
      return vtkMeshQuality::WedgeEquiangleSkew(cell);
    case vtkMeshQuality::JACOBIAN:
      return vtkMeshQuality::WedgeJacobian(cell);
    case vtkMeshQuality::MAX_ASPECT_FROBENIUS:
      return vtkMeshQuality::WedgeMaxAspectFrobenius(cell);
    case vtkMeshQuality::MAX_STRETCH:
      return vtkMeshQuality::WedgeMaxStretch(cell);
    case vtkMeshQuality::MEAN_ASPECT_FROBENIUS:
      return vtkMeshQuality::WedgeMeanAspectFrobenius(cell);
    case vtkMeshQuality::SCALED_JACOBIAN:
      return vtkMeshQuality::WedgeScaledJacobian(cell);
    case vtkMeshQuality::SHAPE:
      return vtkMeshQuality::WedgeShape(cell);
    case vtkMeshQuality::VOLUME:
      return vtkMeshQuality::WedgeVolume(cell);
    default:
      return this->GetUndefinedQuality();
  }
}

//----------------------------------------------------------------------------
double vtkCellQuality::ComputeHexQuality(vtkCell* cell)
{
  switch (this->GetQualityMeasure())
  {
    case vtkMeshQuality::CONDITION:
      return vtkMeshQuality::HexCondition(cell);
    case vtkMeshQuality::DIAGONAL:
      return vtkMeshQuality::HexDiagonal(cell);
    case vtkMeshQuality::DIMENSION:
      return vtkMeshQuality::HexDimension(cell);
    case vtkMeshQuality::DISTORTION:
      return vtkMeshQuality::HexDistortion(cell);
    case vtkMeshQuality::EDGE_RATIO:
      return vtkMeshQuality::HexEdgeRatio(cell);
    case vtkMeshQuality::EQUIANGLE_SKEW:
      return vtkMeshQuality::HexEquiangleSkew(cell);
    case vtkMeshQuality::JACOBIAN:
      return vtkMeshQuality::HexJacobian(cell);
    case vtkMeshQuality::MAX_ASPECT_FROBENIUS:
      return vtkMeshQuality::HexMaxAspectFrobenius(cell);
    case vtkMeshQuality::MAX_EDGE_RATIO:
      return vtkMeshQuality::HexMaxEdgeRatio(cell);
    case vtkMeshQuality::MED_ASPECT_FROBENIUS:
      return vtkMeshQuality::HexMedAspectFrobenius(cell);
    case vtkMeshQuality::NODAL_JACOBIAN_RATIO:
      return vtkMeshQuality::HexNodalJacobianRatio(cell);
    case vtkMeshQuality::ODDY:
      return vtkMeshQuality::HexOddy(cell);
    case vtkMeshQuality::RELATIVE_SIZE_SQUARED:
      return vtkMeshQuality::HexRelativeSizeSquared(cell);
    case vtkMeshQuality::SCALED_JACOBIAN:
      return vtkMeshQuality::HexScaledJacobian(cell);
    case vtkMeshQuality::SHAPE_AND_SIZE:
      return vtkMeshQuality::HexShapeAndSize(cell);
    case vtkMeshQuality::SHAPE:
      return vtkMeshQuality::HexShape(cell);
    case vtkMeshQuality::SHEAR_AND_SIZE:
      return vtkMeshQuality::HexShearAndSize(cell);
    case vtkMeshQuality::SHEAR:
      return vtkMeshQuality::HexShear(cell);
    case vtkMeshQuality::SKEW:
      return vtkMeshQuality::HexSkew(cell);
    case vtkMeshQuality::STRETCH:
      return vtkMeshQuality::HexStretch(cell);
    case vtkMeshQuality::TAPER:
      return vtkMeshQuality::HexTaper(cell);
    case vtkMeshQuality::VOLUME:
      return vtkMeshQuality::HexVolume(cell);
    default:
      return this->GetUndefinedQuality();
  }
}

//----------------------------------------------------------------------------
double vtkCellQuality::ComputeTriangleStripQuality(vtkCell* cell)
{
  switch (this->GetQualityMeasure())
  {
    case vtkMeshQuality::AREA:
      return vtkCellQuality::TriangleStripArea(cell);
    default:
      return this->GetUndefinedQuality();
  }
}

//----------------------------------------------------------------------------
double vtkCellQuality::ComputePixelQuality(vtkCell* cell)
{
  switch (this->GetQualityMeasure())
  {
    case vtkMeshQuality::AREA:
      return vtkCellQuality::PixelArea(cell);
    default:
      return this->GetUndefinedQuality();
  }
}

//----------------------------------------------------------------------------
// Triangle strip quality metrics
double vtkCellQuality::TriangleStripArea(vtkCell* cell)
{
  return this->PolygonArea(cell);
}

//----------------------------------------------------------------------------
// Pixel quality metrics
double vtkCellQuality::PixelArea(vtkCell* cell)
{
  return this->PolygonArea(cell);
}

//----------------------------------------------------------------------------
// Polygon quality metrics
double vtkCellQuality::PolygonArea(vtkCell* cell)
{
  cell->Triangulate(0, this->PointIds, this->Points);
  double abc[3][3], quality = 0;
  for (vtkIdType i = 0, n = this->Points->GetNumberOfPoints(); i < n; i += 3)
  {
    this->Points->GetPoint(i + 0, abc[0]);
    this->Points->GetPoint(i + 1, abc[1]);
    this->Points->GetPoint(i + 2, abc[2]);
    quality += vtkTriangle::TriangleArea(abc[0], abc[1], abc[2]);
  }
  return quality;
}
