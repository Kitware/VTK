// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDGTranscribeCellGridCells.h"

#include "vtkBoundingBox.h"
#include "vtkCellArray.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridElevationQuery.h"
#include "vtkDGInterpolateCalculator.h"
#include "vtkDGVert.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIncrementalOctreePointLocator.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkStringToken.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVector.h"

#include <algorithm>
#include <array>
#include <limits>
#include <sstream>
#include <string>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

namespace
{

int vtkCellTypeForDGShape(vtkDGCell::Shape shape)
{
  switch (shape)
  {
    case vtkDGCell::Shape::Vertex:
      return VTK_VERTEX;
    case vtkDGCell::Shape::Edge:
      return VTK_LINE;
    case vtkDGCell::Shape::Triangle:
      return VTK_TRIANGLE;
    case vtkDGCell::Shape::Quadrilateral:
      return VTK_QUAD;
    case vtkDGCell::Shape::Tetrahedron:
      return VTK_TETRA;
    case vtkDGCell::Shape::Hexahedron:
      return VTK_HEXAHEDRON;
    case vtkDGCell::Shape::Wedge:
      return VTK_WEDGE;
    case vtkDGCell::Shape::Pyramid:
      return VTK_PYRAMID;
    default:
      break;
  }
  return VTK_EMPTY_CELL;
}

/// A subdivision of a reference cell shape into linear cells.
///
/// The parameters are coordinates in the reference domain of the shape being
/// subdivided and the connectivity indexes into them. Every shape subdivides
/// into cells of the same shape except the pyramid, which subdivides into a
/// mix of pyramids and tetrahedra.
///
/// Working in reference coordinates means the result depends only on the shape
/// and the number of subdivisions, never on a particular cell, so one template
/// serves every cell of a type.
struct SubdivisionTemplate
{
  /// Where to sample the cell, in the reference coordinates of the shape.
  std::vector<vtkVector3d> Parameters;
  /// Flat connectivity; cell \a ii occupies [Offsets[ii], Offsets[ii + 1]).
  std::vector<vtkIdType> Connectivity;
  std::vector<vtkIdType> Offsets{ 0 };
  std::vector<unsigned char> CellTypes;

  vtkIdType GetNumberOfCells() const { return static_cast<vtkIdType>(this->CellTypes.size()); }
  vtkIdType GetNumberOfPoints() const { return static_cast<vtkIdType>(this->Parameters.size()); }
  /// The space this template occupies in a vtkCellArray (connectivity plus one size per cell).
  vtkIdType GetNumberOfConnectivityEntries() const
  {
    return static_cast<vtkIdType>(this->Connectivity.size() + this->CellTypes.size());
  }

  void AddPoint(double r, double s, double t) { this->Parameters.emplace_back(r, s, t); }

  void AddCell(unsigned char cellType, std::initializer_list<vtkIdType> conn)
  {
    this->Connectivity.insert(this->Connectivity.end(), conn);
    this->Offsets.push_back(static_cast<vtkIdType>(this->Connectivity.size()));
    this->CellTypes.push_back(cellType);
  }

  /// Return six times the signed volume of the tetrahedron \a a, \a b, \a c, \a d.
  ///
  /// Only the sign matters below. It is positive when \a d lies on the side of
  /// triangle (\a a, \a b, \a c) that VTK expects.
  double TripleProduct(vtkIdType a, vtkIdType b, vtkIdType c, vtkIdType d) const
  {
    const vtkVector3d& origin = this->Parameters[a];
    vtkVector3d u = this->Parameters[b] - origin;
    vtkVector3d v = this->Parameters[c] - origin;
    vtkVector3d w = this->Parameters[d] - origin;
    return u.Dot(v.Cross(w));
  }

  /// Append a tetrahedron, swapping two corners if needed to give it a positive
  /// volume.
  ///
  /// It is much easier to write the subdivisions below by listing which lattice
  /// points make up each piece than by working out the winding VTK wants for
  /// each of them, so let the arithmetic sort it out instead. This runs once per
  /// template, so the cost does not matter.
  ///
  /// Only the tetrahedron and the pyramid need this. The other shapes are built
  /// by walking a lattice in order, which already gives the right winding.
  void AddOrientedTetrahedron(vtkIdType a, vtkIdType b, vtkIdType c, vtkIdType d)
  {
    if (this->TripleProduct(a, b, c, d) < 0)
    {
      std::swap(c, d);
    }
    this->AddCell(VTK_TETRA, { a, b, c, d });
  }

  /// Append a pyramid, reversing its base if needed to give it a positive volume.
  void AddOrientedPyramid(vtkIdType a, vtkIdType b, vtkIdType c, vtkIdType d, vtkIdType apex)
  {
    if (this->TripleProduct(a, b, d, apex) < 0)
    {
      std::swap(b, d);
    }
    this->AddCell(VTK_PYRAMID, { a, b, c, d, apex });
  }
};

/// Return the number of points in a triangular lattice with \a n subdivisions per axis.
vtkIdType TriangleLatticeSize(int n)
{
  return static_cast<vtkIdType>((n + 1) * (n + 2) / 2);
}

/// Return the index of lattice point (\a i, \a j) in a triangular lattice with
/// \a n subdivisions per axis. Points are ordered by row (i.e., by constant \a j).
///
/// Row \a j holds (n + 1 - j) points, so it starts at (n + 1) * j less the
/// 1 + 2 + ... + (j - 1) points that the rows below it lack.
vtkIdType TriangleLatticeIndex(int i, int j, int n)
{
  return static_cast<vtkIdType>(j * (n + 1) - j * (j - 1) / 2 + i);
}

/// Subdivide the reference \a shape into \a m pieces along each parametric axis.
SubdivisionTemplate GenerateSubdivisionTemplate(vtkDGCell::Shape shape, int m)
{
  SubdivisionTemplate result;
  m = std::max(m, 1);
  const double dd = 1.0 / m; // The lattice spacing of parameterizations over [0, 1].
  switch (shape)
  {
    case vtkDGCell::Shape::Vertex:
      result.AddPoint(0., 0., 0.);
      result.AddCell(VTK_VERTEX, { 0 });
      break;

    case vtkDGCell::Shape::Edge:
      // Reference domain: [-1, 1].
      for (int ii = 0; ii <= m; ++ii)
      {
        result.AddPoint(-1. + 2 * ii * dd, 0., 0.);
      }
      for (int ii = 0; ii < m; ++ii)
      {
        result.AddCell(VTK_LINE, { ii, ii + 1 });
      }
      break;

    case vtkDGCell::Shape::Triangle:
      // Reference domain: the unit triangle.
      for (int jj = 0; jj <= m; ++jj)
      {
        for (int ii = 0; ii + jj <= m; ++ii)
        {
          result.AddPoint(ii * dd, jj * dd, 0.);
        }
      }
      for (int jj = 0; jj < m; ++jj)
      {
        for (int ii = 0; ii + jj < m; ++ii)
        {
          vtkIdType p00 = TriangleLatticeIndex(ii, jj, m);
          vtkIdType p10 = TriangleLatticeIndex(ii + 1, jj, m);
          vtkIdType p01 = TriangleLatticeIndex(ii, jj + 1, m);
          // Every lattice cell holds a triangle pointing the same way as the
          // parent and, unless it straddles the diagonal edge, a second one
          // pointing the other way to fill the rest of the square.
          result.AddCell(VTK_TRIANGLE, { p00, p10, p01 });
          if (ii + jj < m - 1)
          {
            vtkIdType p11 = TriangleLatticeIndex(ii + 1, jj + 1, m);
            result.AddCell(VTK_TRIANGLE, { p10, p11, p01 });
          }
        }
      }
      break;

    case vtkDGCell::Shape::Quadrilateral:
    {
      // Reference domain: [-1, 1]^2.
      auto idx = [m](int i, int j) { return static_cast<vtkIdType>(i + (m + 1) * j); };
      for (int jj = 0; jj <= m; ++jj)
      {
        for (int ii = 0; ii <= m; ++ii)
        {
          result.AddPoint(-1. + 2 * ii * dd, -1. + 2 * jj * dd, 0.);
        }
      }
      for (int jj = 0; jj < m; ++jj)
      {
        for (int ii = 0; ii < m; ++ii)
        {
          result.AddCell(
            VTK_QUAD, { idx(ii, jj), idx(ii + 1, jj), idx(ii + 1, jj + 1), idx(ii, jj + 1) });
        }
      }
    }
    break;

    case vtkDGCell::Shape::Hexahedron:
    {
      // Reference domain: [-1, 1]^3.
      auto idx = [m](int i, int j, int k)
      { return static_cast<vtkIdType>(i + (m + 1) * (j + (m + 1) * k)); };
      for (int kk = 0; kk <= m; ++kk)
      {
        for (int jj = 0; jj <= m; ++jj)
        {
          for (int ii = 0; ii <= m; ++ii)
          {
            result.AddPoint(-1. + 2 * ii * dd, -1. + 2 * jj * dd, -1. + 2 * kk * dd);
          }
        }
      }
      for (int kk = 0; kk < m; ++kk)
      {
        for (int jj = 0; jj < m; ++jj)
        {
          for (int ii = 0; ii < m; ++ii)
          {
            result.AddCell(VTK_HEXAHEDRON,
              { idx(ii, jj, kk), idx(ii + 1, jj, kk), idx(ii + 1, jj + 1, kk), idx(ii, jj + 1, kk),
                idx(ii, jj, kk + 1), idx(ii + 1, jj, kk + 1), idx(ii + 1, jj + 1, kk + 1),
                idx(ii, jj + 1, kk + 1) });
          }
        }
      }
    }
    break;

    case vtkDGCell::Shape::Wedge:
    {
      // Reference domain: the unit triangle crossed with [-1, 1], so each
      // layer of lattice points is a triangular lattice and the wedges are
      // simply the triangles of one layer extruded to the next. The upward
      // and downward triangles pair up exactly as in the triangle case.
      vtkIdType layerSize = TriangleLatticeSize(m);
      auto idx = [m, layerSize](int i, int j, int k)
      { return TriangleLatticeIndex(i, j, m) + layerSize * k; };
      for (int kk = 0; kk <= m; ++kk)
      {
        for (int jj = 0; jj <= m; ++jj)
        {
          for (int ii = 0; ii + jj <= m; ++ii)
          {
            result.AddPoint(ii * dd, jj * dd, -1. + 2 * kk * dd);
          }
        }
      }
      for (int kk = 0; kk < m; ++kk)
      {
        for (int jj = 0; jj < m; ++jj)
        {
          for (int ii = 0; ii + jj < m; ++ii)
          {
            result.AddCell(VTK_WEDGE,
              { idx(ii, jj, kk), idx(ii + 1, jj, kk), idx(ii, jj + 1, kk), idx(ii, jj, kk + 1),
                idx(ii + 1, jj, kk + 1), idx(ii, jj + 1, kk + 1) });
            if (ii + jj < m - 1)
            {
              result.AddCell(VTK_WEDGE,
                { idx(ii + 1, jj, kk), idx(ii + 1, jj + 1, kk), idx(ii, jj + 1, kk),
                  idx(ii + 1, jj, kk + 1), idx(ii + 1, jj + 1, kk + 1), idx(ii, jj + 1, kk + 1) });
            }
          }
        }
      }
    }
    break;

    case vtkDGCell::Shape::Tetrahedron:
    {
      // Reference domain: the unit tetrahedron. Layer kk is a triangular
      // lattice with (m - kk) subdivisions per axis.
      std::vector<vtkIdType> layerOffset(m + 2, 0);
      for (int kk = 0; kk <= m; ++kk)
      {
        layerOffset[kk + 1] = layerOffset[kk] + TriangleLatticeSize(m - kk);
      }
      auto idx = [m, &layerOffset](int i, int j, int k)
      { return layerOffset[k] + TriangleLatticeIndex(i, j, m - k); };
      for (int kk = 0; kk <= m; ++kk)
      {
        for (int jj = 0; jj + kk <= m; ++jj)
        {
          for (int ii = 0; ii + jj + kk <= m; ++ii)
          {
            result.AddPoint(ii * dd, jj * dd, kk * dd);
          }
        }
      }
      // Freudenthal subdivision. A tetrahedron cannot be cut into smaller
      // tetrahedra as tidily as a cube can be cut into cubes, so each lattice cell
      // contributes three kinds of piece: a corner tetrahedron, the octahedron
      // left over next to it (split into 4 along a diagonal), and an inverted
      // corner tetrahedron. Every piece is a lattice simplex of volume h^3/6,
      // and the counts add up to
      //   C(m+2,3) + 4*C(m+1,3) + C(m,3) == m^3
      // which is a good check that they tile the cell.
      //
      // The three families run out at different depths. A cell close to the
      // slanted face only has room for its corner tetrahedron, so the two
      // guards below keep the octahedron and the inverted tetrahedron from
      // spilling outside the parent.
      for (int kk = 0; kk < m; ++kk)
      {
        for (int jj = 0; jj + kk < m; ++jj)
        {
          for (int ii = 0; ii + jj + kk < m; ++ii)
          {
            result.AddOrientedTetrahedron(
              idx(ii, jj, kk), idx(ii + 1, jj, kk), idx(ii, jj + 1, kk), idx(ii, jj, kk + 1));
            if (ii + jj + kk >= m - 1)
            {
              continue;
            }
            // The octahedron. Points aa and ff are the endpoints of the chosen
            // diagonal, and bb, cc, ee and dd2 form the cycle around it, so
            // pairing consecutive cycle points with the diagonal gives the 4
            // tetrahedra. An octahedron has three diagonals and any of them
            // works, but every cell has to pick the same one or neighboring
            // cells disagree about the faces they share.
            vtkIdType aa = idx(ii + 1, jj, kk);
            vtkIdType bb = idx(ii, jj + 1, kk);
            vtkIdType cc = idx(ii, jj, kk + 1);
            vtkIdType dd2 = idx(ii + 1, jj + 1, kk);
            vtkIdType ee = idx(ii + 1, jj, kk + 1);
            vtkIdType ff = idx(ii, jj + 1, kk + 1);
            result.AddOrientedTetrahedron(aa, ff, bb, cc);
            result.AddOrientedTetrahedron(aa, ff, cc, ee);
            result.AddOrientedTetrahedron(aa, ff, ee, dd2);
            result.AddOrientedTetrahedron(aa, ff, dd2, bb);
            if (ii + jj + kk < m - 2)
            {
              result.AddOrientedTetrahedron(dd2, ee, ff, idx(ii + 1, jj + 1, kk + 1));
            }
          }
        }
      }
    }
    break;

    case vtkDGCell::Shape::Pyramid:
    {
      // Reference domain: the [-1, 1]^2 square at t = 0 with its apex at (0, 0, 1).
      //
      // The pyramid is the one shape that cannot be subdivided into pieces of
      // its own kind alone, so it is built up in layers instead. Layer kk is a
      // square lattice with (m - kk) subdivisions per axis, so the layers shrink
      // toward the apex, where the last one is a single point. The spacing stays
      // 2/m throughout, so every piece is the reference pyramid scaled by 1/m.
      // Between two layers the pieces come in four families, adding up to
      // (2n - 1)^2 cells for a layer of n squares.
      std::vector<vtkIdType> layerOffset(m + 2, 0);
      for (int kk = 0; kk <= m; ++kk)
      {
        layerOffset[kk + 1] = layerOffset[kk] + (m - kk + 1) * (m - kk + 1);
      }
      auto idx = [m, &layerOffset](int i, int j, int k)
      { return layerOffset[k] + i + (m - k + 1) * j; };
      for (int kk = 0; kk <= m; ++kk)
      {
        double tt = kk * dd;
        for (int jj = 0; jj <= m - kk; ++jj)
        {
          for (int ii = 0; ii <= m - kk; ++ii)
          {
            result.AddPoint(-(1. - tt) + 2 * ii * dd, -(1. - tt) + 2 * jj * dd, tt);
          }
        }
      }
      for (int kk = 0; kk < m; ++kk)
      {
        int nn = m - kk; // The number of sub-cells per axis in layer kk.
        // Family 1: a pyramid over every square of this layer, with its apex
        // the point of the next layer directly above the square's center. The
        // layers are staggered by half a square so that such a point exists.
        // This leaves gaps between the pyramids for the other families.
        for (int jj = 0; jj < nn; ++jj)
        {
          for (int ii = 0; ii < nn; ++ii)
          {
            result.AddOrientedPyramid(idx(ii, jj, kk), idx(ii + 1, jj, kk), idx(ii + 1, jj + 1, kk),
              idx(ii, jj + 1, kk), idx(ii, jj, kk + 1));
          }
        }
        // Families 2 and 3: two neighboring pyramids lean away from each other
        // and leave a tetrahedral gap between them, spanned by the base edge
        // they share and their two apexes. One family per axis.
        for (int jj = 0; jj < nn; ++jj)
        {
          for (int ii = 0; ii + 1 < nn; ++ii)
          {
            result.AddOrientedTetrahedron(idx(ii + 1, jj, kk), idx(ii + 1, jj + 1, kk),
              idx(ii, jj, kk + 1), idx(ii + 1, jj, kk + 1));
          }
        }
        for (int jj = 0; jj + 1 < nn; ++jj)
        {
          for (int ii = 0; ii < nn; ++ii)
          {
            result.AddOrientedTetrahedron(idx(ii, jj + 1, kk), idx(ii + 1, jj + 1, kk),
              idx(ii, jj, kk + 1), idx(ii, jj + 1, kk + 1));
          }
        }
        // Family 4: at every interior lattice point of this layer, 4 pyramids
        // and 4 tetrahedra surround a void whose base is the square joining
        // their 4 apexes on the next layer. A pyramid pointing the other way
        // fills it: base up, apex down onto the shared lattice point.
        for (int jj = 0; jj + 1 < nn; ++jj)
        {
          for (int ii = 0; ii + 1 < nn; ++ii)
          {
            result.AddOrientedPyramid(idx(ii, jj, kk + 1), idx(ii + 1, jj, kk + 1),
              idx(ii + 1, jj + 1, kk + 1), idx(ii, jj + 1, kk + 1), idx(ii + 1, jj + 1, kk));
          }
        }
      }
    }
    break;

    default:
      break;
  }
  return result;
}

/// The largest number of corners any DG shape has (the hexahedron's).
constexpr int MaxNumberOfCorners = 8;

/// Evaluate the linear (corner) basis of \a shape at \a rst into \a weights.
///
/// This is used to map a point in a side's reference coordinates into the
/// reference coordinates of the cell the side bounds: weighting the side's
/// corners (expressed in cell coordinates) by these values gives the sample
/// point in cell coordinates. Because a reference cell's sides are flat, the
/// linear basis reproduces that mapping exactly.
void EvaluateCornerBasis(
  vtkDGCell::Shape shape, const vtkVector3d& rst, std::array<double, MaxNumberOfCorners>& weights)
{
  double r = rst[0];
  double s = rst[1];
  double t = rst[2];
  switch (shape)
  {
    case vtkDGCell::Shape::Vertex:
      weights[0] = 1.;
      break;
    case vtkDGCell::Shape::Edge:
      weights[0] = (1. - r) / 2.;
      weights[1] = (1. + r) / 2.;
      break;
    case vtkDGCell::Shape::Triangle:
      weights[0] = 1. - r - s;
      weights[1] = r;
      weights[2] = s;
      break;
    case vtkDGCell::Shape::Quadrilateral:
      weights[0] = (1. - r) * (1. - s) / 4.;
      weights[1] = (1. + r) * (1. - s) / 4.;
      weights[2] = (1. + r) * (1. + s) / 4.;
      weights[3] = (1. - r) * (1. + s) / 4.;
      break;
    case vtkDGCell::Shape::Tetrahedron:
      weights[0] = 1. - r - s - t;
      weights[1] = r;
      weights[2] = s;
      weights[3] = t;
      break;
    case vtkDGCell::Shape::Hexahedron:
      weights[0] = (1. - r) * (1. - s) * (1. - t) / 8.;
      weights[1] = (1. + r) * (1. - s) * (1. - t) / 8.;
      weights[2] = (1. + r) * (1. + s) * (1. - t) / 8.;
      weights[3] = (1. - r) * (1. + s) * (1. - t) / 8.;
      weights[4] = (1. - r) * (1. - s) * (1. + t) / 8.;
      weights[5] = (1. + r) * (1. - s) * (1. + t) / 8.;
      weights[6] = (1. + r) * (1. + s) * (1. + t) / 8.;
      weights[7] = (1. - r) * (1. + s) * (1. + t) / 8.;
      break;
    case vtkDGCell::Shape::Wedge:
      weights[0] = (1. - r - s) * (1. - t) / 2.;
      weights[1] = r * (1. - t) / 2.;
      weights[2] = s * (1. - t) / 2.;
      weights[3] = (1. - r - s) * (1. + t) / 2.;
      weights[4] = r * (1. + t) / 2.;
      weights[5] = s * (1. + t) / 2.;
      break;
    case vtkDGCell::Shape::Pyramid:
    {
      // The base shrinks to a point at the apex, so the basis is rational.
      double denom = 1. - t;
      if (denom < 1e-12)
      {
        weights[0] = weights[1] = weights[2] = weights[3] = 0.;
        weights[4] = 1.;
        break;
      }
      double rr = r / denom;
      double ss = s / denom;
      weights[0] = (1. - rr) * (1. - ss) * denom / 4.;
      weights[1] = (1. + rr) * (1. - ss) * denom / 4.;
      weights[2] = (1. + rr) * (1. + ss) * denom / 4.;
      weights[3] = (1. - rr) * (1. + ss) * denom / 4.;
      weights[4] = t;
    }
    break;
    default:
      break;
  }
}

///@{
/// Return how many sample points and cells a subdivision of \a shape into \a m
/// pieces per axis produces.
///
/// These take and return doubles so that counts too large to represent show up
/// instead of overflowing silently.
double SubdivisionPointCount(vtkDGCell::Shape shape, double m)
{
  switch (shape)
  {
    case vtkDGCell::Shape::Vertex:
      return 1.;
    case vtkDGCell::Shape::Edge:
      return m + 1.;
    case vtkDGCell::Shape::Triangle:
      return (m + 1.) * (m + 2.) / 2.;
    case vtkDGCell::Shape::Quadrilateral:
      return (m + 1.) * (m + 1.);
    case vtkDGCell::Shape::Tetrahedron:
      return (m + 1.) * (m + 2.) * (m + 3.) / 6.;
    case vtkDGCell::Shape::Hexahedron:
      return (m + 1.) * (m + 1.) * (m + 1.);
    case vtkDGCell::Shape::Wedge:
      return (m + 1.) * (m + 1.) * (m + 2.) / 2.;
    case vtkDGCell::Shape::Pyramid:
      // The layers are squares of decreasing size: sum of n^2 for n in [1, m + 1].
      return (m + 1.) * (m + 2.) * (2. * m + 3.) / 6.;
    default:
      break;
  }
  return 0.;
}

double SubdivisionCellCount(vtkDGCell::Shape shape, double m)
{
  switch (shape)
  {
    case vtkDGCell::Shape::Vertex:
      return 1.;
    case vtkDGCell::Shape::Edge:
      return m;
    case vtkDGCell::Shape::Triangle:
    case vtkDGCell::Shape::Quadrilateral:
      return m * m;
    case vtkDGCell::Shape::Tetrahedron:
    case vtkDGCell::Shape::Hexahedron:
    case vtkDGCell::Shape::Wedge:
      return m * m * m;
    case vtkDGCell::Shape::Pyramid:
      // Each layer contributes (2n - 1)^2 cells: sum of odd squares.
      return m * (2. * m - 1.) * (2. * m + 1.) / 3.;
    default:
      break;
  }
  return 0.;
}
///@}

/// Return where to sample each side of \a source, in the coordinates of the
/// cell that side bounds, indexed by side.
///
/// The subdivision template gives coordinates in the reference domain of the
/// side's own shape, a square for instance, but attributes are interpolated in
/// the coordinates of the cell. Asked about a face of a hexahedron, the
/// evaluator needs to know where in the hexahedron a sample lies, not where in
/// the square. Weighting the side's corners, which are known in cell
/// coordinates, by the linear basis moves each sample across.
///
/// The mapping depends only on which side is being sampled, never on which
/// cell, so a source needs at most a couple of dozen of them. A source of cells
/// needs no mapping at all and returns an empty table.
std::vector<std::vector<vtkVector3d>> SideSampleParameters(vtkDGCell* cellType,
  const vtkDGCell::Source& source, vtkDGCell::Shape shape, const SubdivisionTemplate& subdivision)
{
  std::vector<std::vector<vtkVector3d>> parametersBySide;
  if (source.SideType < 0)
  {
    return parametersBySide;
  }
  auto sideRange = cellType->GetSideRangeForType(source.SideType);
  parametersBySide.resize(static_cast<std::size_t>(std::max(0, sideRange.second)));
  vtkIdType numSamples = subdivision.GetNumberOfPoints();
  std::array<double, MaxNumberOfCorners> weights;
  for (int side = std::max(0, sideRange.first); side < sideRange.second; ++side)
  {
    auto& parameters = parametersBySide[side];
    parameters.assign(numSamples, vtkVector3d(0., 0., 0.));
    const auto& sideConn = cellType->GetSideConnectivity(side);
    for (vtkIdType pp = 0; pp < numSamples; ++pp)
    {
      EvaluateCornerBasis(shape, subdivision.Parameters[pp], weights);
      for (std::size_t nn = 0; nn < sideConn.size(); ++nn)
      {
        vtkVector3d corner(cellType->GetCornerParameter(static_cast<int>(sideConn[nn])).data());
        parameters[pp] = parameters[pp] + corner * weights[nn];
      }
    }
  }
  return parametersBySide;
}

/// Return the shape of the cells or sides that \a source provides.
vtkDGCell::Shape vtkShapeForCellSource(vtkDGCell* dgCell, const vtkDGCell::Source& source)
{
  // Fetch the range of side indices that have the shape corresponding to source.SideType.
  // Note that a SideType of -1 yields the shape of the cell itself.
  auto sideRange = dgCell->GetSideRangeForType(source.SideType);
  return dgCell->GetSideShape(sideRange.first);
}

/// Return true if subdividing \a dgCell into \a subdivisions pieces per axis
/// yields point and cell counts a vtkUnstructuredGrid can represent.
///
/// There is no arbitrary limit on the level. This only rejects levels whose
/// output could not be indexed, and so could never be allocated either.
/// Failing here is friendlier than overflowing silently and returning a grid
/// that looks plausible but is not. The counts are computed in floating point
/// because the question is precisely whether they fit.
bool SubdivisionIsRepresentable(vtkDGCell* dgCell, vtkIdType subdivisions, std::string& problem)
{
  constexpr double idMax = static_cast<double>(VTK_ID_MAX);
  // Compare in floating point: when vtkIdType is 32 bits, an integer comparison
  // against int's maximum is tautological and warns.
  double mm = static_cast<double>(subdivisions);
  if (mm > static_cast<double>(std::numeric_limits<int>::max()))
  {
    problem = "the number of subdivisions per axis overflows";
    return false;
  }
  for (int ii = -1; ii < static_cast<int>(dgCell->GetNumberOfCellSources()); ++ii)
  {
    const auto& source(dgCell->GetCellSource(ii));
    if (source.Blanked)
    {
      continue;
    }
    auto shape = vtkShapeForCellSource(dgCell, source);
    double numCells = static_cast<double>(source.Connectivity->GetNumberOfTuples());
    double points = SubdivisionPointCount(shape, mm);
    double cells = SubdivisionCellCount(shape, mm);
    // Every sample point of one cell is held in memory at once, and the output
    // holds an entry (plus connectivity) for every sub-cell of every cell.
    if (points > idMax || cells * numCells > idMax ||
      cells * numCells * (vtkDGCell::GetShapeCornerCount(shape) + 1.) > idMax)
    {
      problem = "the output would have more points or cells than a vtkIdType can index";
      return false;
    }
  }
  return true;
}

void vtkCellInfoFromDGType(vtkCellGridToUnstructuredGrid::Query::OutputAllocation& alloc,
  vtkDGCell* dgCell, vtkIdType subdivisions)
{
  alloc.CellType = vtkCellTypeForDGShape(dgCell->GetShape());
  alloc.NumberOfCells = 0;
  alloc.NumberOfConnectivityEntries = 0;
  for (int ii = -1; ii < static_cast<int>(dgCell->GetNumberOfCellSources()); ++ii)
  {
    const auto& source(dgCell->GetCellSource(ii));
    if (source.Blanked)
    {
      continue;
    }

    auto shape = vtkShapeForCellSource(dgCell, source);
    vtkIdType numCells = source.Connectivity->GetNumberOfTuples();
    const SubdivisionTemplate subdivision =
      GenerateSubdivisionTemplate(shape, static_cast<int>(subdivisions));
    alloc.NumberOfCells += numCells * subdivision.GetNumberOfCells();
    alloc.NumberOfConnectivityEntries += numCells * subdivision.GetNumberOfConnectivityEntries();
  }
}

// The contributions of cell-grid corner points to
// corner points in the output unstructured grid.
// Attributes are interpolated using the cell IDs
// and parametric coordinates, then summed to the
// output points.
struct Contributions
{
  Contributions() { this->ParametricCoords->SetNumberOfComponents(3); }

  vtkIdType AddContribution(
    vtkIdType outputPointId, vtkIdType inputCellId, const vtkVector3d& pcoord)
  {
    vtkIdType nn = this->OutputPointIds->InsertNextValue(outputPointId);
    this->InputCellIds->InsertNextValue(inputCellId);
    this->ParametricCoords->InsertNextTuple(pcoord.GetData());
    return nn;
  }

  vtkNew<vtkIdTypeArray> OutputPointIds;
  vtkNew<vtkIdTypeArray> InputCellIds;
  vtkNew<vtkDoubleArray> ParametricCoords;
};

using ContributionMap = std::unordered_map<vtkStringToken, Contributions>;

class TranscribeCellGridPointCache : public vtkObject
{
public:
  vtkTypeMacro(TranscribeCellGridPointCache, vtkObject);
  void PrintSelf(std::ostream& os, vtkIndent indent) override
  {
    this->Superclass::PrintSelf(os, indent);
    os << indent << "ContributionsByType: " << this->ContributionsByType.size() << " entries\n";
  }
  static TranscribeCellGridPointCache* New();

  ContributionMap ContributionsByType;
};

Contributions& FetchPointContributionCache(
  vtkCellGridToUnstructuredGrid::Query* request, vtkDGCell* cellType, vtkCellGridResponders* caches)
{
  std::ostringstream cacheName;
  cacheName << "TranscribeCellGridPointCache_" << request;
  vtkStringToken cacheKey(cacheName.str());
  vtkStringToken cellTypeToken(cellType->GetClassName());
  auto data =
    caches->GetCacheDataAs<TranscribeCellGridPointCache>(cacheKey.GetId(), /*createIfAbsent*/ true);
  return data->ContributionsByType[cellTypeToken];
}

void FreePointContributionCache(
  vtkCellGridToUnstructuredGrid::Query* request, vtkDGCell* cellType, vtkCellGridResponders* caches)
{
  std::ostringstream cacheName;
  cacheName << "TranscribeCellGridPointCache_" << request;
  vtkStringToken cacheKey(cacheName.str());
  vtkStringToken cellTypeToken(cellType->GetClassName());
  auto data = caches->GetCacheDataAs<TranscribeCellGridPointCache>(
    cacheKey.GetId(), /*createIfAbsent*/ false);
  if (data)
  {
    data->ContributionsByType.erase(cellTypeToken);
    if (data->ContributionsByType.empty())
    {
      vtkSmartPointer<TranscribeCellGridPointCache> blank;
      caches->SetCacheData(cacheKey.GetId(), blank, /*overwrite*/ true);
    }
  }
}

} // anonymous namespace

vtkStandardNewMacro(vtkDGTranscribeCellGridCells);
vtkStandardNewMacro(TranscribeCellGridPointCache);

void vtkDGTranscribeCellGridCells::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

bool vtkDGTranscribeCellGridCells::Query(
  TranscribeQuery* request, vtkCellMetadata* cellType, vtkCellGridResponders* caches)
{
  (void)caches;

  auto* dgCell = vtkDGCell::SafeDownCast(cellType);
  if (!dgCell)
  {
    return false;
  }

  auto* grid = dgCell->GetCellGrid();
  if (!grid)
  {
    return false;
  }

  switch (request->GetPass())
  {
    case TranscribeQuery::PassType::CountOutputs:
    {
      std::string problem;
      if (!SubdivisionIsRepresentable(dgCell, request->GetSubdivisionCount(), problem))
      {
        vtkErrorMacro("Cannot convert " << dgCell->GetClassName() << " at subdivision level "
                                        << request->GetSubdivisionLevel() << " because " << problem
                                        << ".");
        return false;
      }
      auto& alloc(request->GetOutputAllocations());
      vtkCellInfoFromDGType(alloc[dgCell->GetClassName()], dgCell, request->GetSubdivisionCount());
    }
    break;
    case TranscribeQuery::PassType::GenerateConnectivity:
      this->GenerateConnectivity(request, dgCell, caches);
      break;
    case TranscribeQuery::PassType::GeneratePointData:
      this->GeneratePointData(request, dgCell, caches);
      break;
    default:
      vtkErrorMacro("Unknown pass " << request->GetPass());
  }

  return true;
}

/// Transcribe every cell and side of one DG cell type into the output grid.
///
/// Each one is handled in three steps:
///   1. Decide where to sample it. A SubdivisionTemplate says where, in the
///      reference coordinates of the shape; SideSampleParameters() moves those
///      into the coordinates of the cell when the source provides sides.
///   2. Ask the shape attribute where those samples lie in space.
///   3. Insert the resulting points and write the sub-cells that connect them.
void vtkDGTranscribeCellGridCells::GenerateConnectivity(
  TranscribeQuery* request, vtkDGCell* cellType, vtkCellGridResponders* caches)
{
  vtkStringToken cellTypeToken = cellType->GetClassName();
  auto& alloc = request->GetOutputAllocations();
  auto ait = alloc.find(cellTypeToken);
  if (ait == alloc.end())
  {
    return;
  }
  int subdivisions = static_cast<int>(request->GetSubdivisionCount());
  auto& contribs = FetchPointContributionCache(request, cellType, caches);

  auto* cellArray = request->GetOutput()->GetCells();
  auto* cellTypes = vtkUnsignedCharArray::FastDownCast(request->GetOutput()->GetCellTypes());
  auto* locator = request->GetLocator();
  auto& pointCounts = request->GetConnectivityCount();
  auto shapeAtt = request->GetInput()->GetShapeAttribute();

  // The sample points do not generally coincide with the shape attribute's
  // degrees of freedom, so we must interpolate the shape attribute in order to
  // obtain coordinates for the output points. (This is also what makes higher
  // levels approximate curved geometry. At the default level, the samples are
  // the cell's corners, where interpolation simply recovers the corner points.)
  vtkNew<vtkDGInterpolateCalculator> interpolateProto;
  auto rawCalc = interpolateProto->PrepareForGrid(cellType, shapeAtt);
  auto* shapeCalc = vtkDGInterpolateCalculator::SafeDownCast(rawCalc);
  if (!shapeCalc)
  {
    vtkErrorMacro("Could not interpolate the shape attribute for cells of type "
      << cellTypeToken.Data() << ".");
    return;
  }

  vtkNew<vtkIdTypeArray> cellIds;
  vtkNew<vtkDoubleArray> parameters;
  vtkNew<vtkDoubleArray> coordinates;
  parameters->SetNumberOfComponents(3);
  coordinates->SetNumberOfComponents(3);

  std::array<vtkTypeUInt64, 2> sideTuple; // (cellId, sideIndex)
  vtkVector3d xx;                         // A sample point's world coordinates.
  std::vector<vtkIdType> outputPointIds;
  std::vector<vtkIdType> outConn;

  // A cell type may offer its cells (source -1) and any number of side sources.
  // After vtkCellGridComputeSides has run, for example, the cells are usually
  // blanked and only their faces are meant to be drawn. Each source is
  // subdivided on its own, since they need not have the same shape.
  for (int ii = -1; ii < static_cast<int>(cellType->GetSideSpecs().size()); ++ii)
  {
    const auto& source(cellType->GetCellSource(ii));
    if (source.Blanked)
    {
      continue;
    }
    auto shape = vtkShapeForCellSource(cellType, source);
    const SubdivisionTemplate subdivision = GenerateSubdivisionTemplate(shape, subdivisions);
    vtkIdType numSamples = subdivision.GetNumberOfPoints();
    vtkIdType numSourceTuples = source.Connectivity->GetNumberOfTuples();
    if (numSamples == 0 || numSourceTuples == 0)
    {
      continue;
    }

    const auto sideParameters = SideSampleParameters(cellType, source, shape, subdivision);

    // One cell at a time: the scratch arrays below hold a single cell's samples,
    // so they stay small however large the mesh or the level is.
    cellIds->SetNumberOfValues(numSamples);
    parameters->SetNumberOfTuples(numSamples);
    coordinates->SetNumberOfTuples(numSamples);
    for (vtkIdType cc = 0; cc < numSourceTuples; ++cc)
    {
      if (request->IsAborted())
      {
        return;
      }

      // Step 1: where in its cell does each sample sit? Attributes are
      // evaluated by an ID that numbers cells and sides globally, so it must
      // include the source's offset. Given a side's ID the evaluator resolves
      // the cell that side bounds on its own, and it evaluates that cell's
      // basis: the parametric coordinates we supply are always the cell's,
      // never the side's.
      vtkIdType sourceId = cc + source.Offset;
      const std::vector<vtkVector3d>* sampleParameters = &subdivision.Parameters;
      if (source.SideType >= 0)
      {
        // source is a SideSpec; each tuple is a (cellId, sideIndex) pair.
        source.Connectivity->GetUnsignedTuple(cc, sideTuple.data());
        sampleParameters = &sideParameters[static_cast<int>(sideTuple[1])];
      }
      for (vtkIdType pp = 0; pp < numSamples; ++pp)
      {
        cellIds->SetValue(pp, sourceId);
        parameters->SetTuple(pp, (*sampleParameters)[pp].GetData());
      }

      // Step 2: where do those samples lie in space?
      shapeCalc->Evaluate(cellIds, parameters, coordinates);

      // Step 3: insert the sample points. Neighboring cells sample their shared
      // faces at exactly the same places, since they interpolate the same shape
      // attribute over the same parametric points. The locator merges those
      // samples, so the output is watertight instead of a pile of disconnected
      // cells.
      //
      // Contribute to each sample once per cell. A point inside a single cell
      // then keeps that cell's value, while a point on a shared face ends up
      // with the average of the cells meeting there. That is how a
      // discontinuous attribute becomes a continuous approximation.
      outputPointIds.resize(numSamples);
      for (vtkIdType pp = 0; pp < numSamples; ++pp)
      {
        vtkIdType outPointId;
        coordinates->GetTuple(pp, xx.GetData());
        locator->InsertUniquePoint(xx.GetData(), outPointId);
        outputPointIds[pp] = outPointId;
        ++pointCounts[outPointId];
        contribs.AddContribution(outPointId, sourceId, (*sampleParameters)[pp]);
      }

      // ... and write out the sub-cells connecting them.
      for (vtkIdType sc = 0; sc < subdivision.GetNumberOfCells(); ++sc)
      {
        outConn.clear();
        for (vtkIdType kk = subdivision.Offsets[sc]; kk < subdivision.Offsets[sc + 1]; ++kk)
        {
          outConn.push_back(outputPointIds[subdivision.Connectivity[kk]]);
        }
        cellArray->InsertNextCell(static_cast<vtkIdType>(outConn.size()), outConn.data());
        cellTypes->InsertNextValue(subdivision.CellTypes[sc]);
      }
    }
  }
}

void vtkDGTranscribeCellGridCells::GeneratePointData(
  TranscribeQuery* request, vtkDGCell* cellType, vtkCellGridResponders* caches)
{
  auto& alloc = request->GetOutputAllocations();
  auto ait = alloc.find(cellType->GetClassName());
  if (ait == alloc.end())
  {
    return;
  }
  auto& contribs = FetchPointContributionCache(request, cellType, caches);
  vtkIdType nn = contribs.InputCellIds->GetNumberOfTuples();
  auto& pointWeights = request->GetConnectivityWeights();

  vtkNew<vtkDGInterpolateCalculator> interpolateProto;
  for (const auto& inCellAtt : request->GetInput()->GetCellAttributeList())
  {
    if (inCellAtt == request->GetInput()->GetShapeAttribute())
    {
      continue;
    }
    // TODO: We could handle the "constant"_token function-space differently
    //       (by creating cell-data, not point-data, arrays).
    auto* outputArray = request->GetOutputArray(inCellAtt);
    auto rawCalc = interpolateProto->PrepareForGrid(cellType, inCellAtt);
    auto dgCalc = vtkDGInterpolateCalculator::SafeDownCast(rawCalc);
    vtkNew<vtkDoubleArray> interpResult;
    int nc = inCellAtt->GetNumberOfComponents();
    interpResult->SetNumberOfComponents(nc);
    interpResult->SetNumberOfTuples(nn);
    dgCalc->Evaluate(contribs.InputCellIds, contribs.ParametricCoords, interpResult);
    vtkSMPTools::For(0, nn,
      [&](vtkIdType begin, vtkIdType end)
      {
        std::vector<double> outTuple(nc, 0.);
        std::vector<double> inTuple(nc, 0.);
        for (vtkIdType ii = begin; ii < end; ++ii)
        {
          interpResult->GetTuple(ii, inTuple.data());
          vtkIdType outputPointId = contribs.OutputPointIds->GetValue(ii);
          outputArray->GetTuple(outputPointId, outTuple.data());
          double pw = pointWeights[outputPointId];
          for (int jj = 0; jj < nc; ++jj)
          {
            outTuple[jj] += pw * inTuple[jj];
          }
          outputArray->SetTuple(outputPointId, outTuple.data());
        }
      });
  }
  FreePointContributionCache(request, cellType, caches);
}

VTK_ABI_NAMESPACE_END
