// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkCellStatus_h
#define vtkCellStatus_h

#include "vtkCommonDataModelModule.h" // For export macro

#include <ostream>

VTK_ABI_NAMESPACE_BEGIN

/// Diagnostic values indicating how well-specified a cell is.
///
/// Bitwise combinations of these values are returned by methods of vtkPolygon,
/// vtkPolyhedron, and vtkCellValidator.
enum class vtkCellStatus : short
{
  Valid = 0x00,                       //!< Cell is in a good state.
  WrongNumberOfPoints = 0x01,         //!< Lines have <2 points, faces <3.
  IntersectingEdges = 0x02,           //!< Edges of a face self-intersect.
  IntersectingFaces = 0x04,           //!< Faces of a volume self-intersect.
  NoncontiguousEdges = 0x08,          //!< Edge vertices are not ordered head-to-tail.
  Nonconvex = 0x10,                   //!< The cell has a concavity.
  FacesAreOrientedIncorrectly = 0x20, //!< All faces should have CCW windings.
  NonPlanarFaces = 0x40,              //!< Vertices for a face do not all lie in the same plane.
  DegenerateFaces = 0x80,             //!< A face is collapsed to a line or a point.
  CoincidentPoints = 0x100,           //!< A cell is otherwise valid but has coincident points.
};

inline bool operator!=(short a, vtkCellStatus b)
{
  return a != static_cast<short>(b);
}

inline bool operator==(short a, vtkCellStatus b)
{
  return a == static_cast<short>(b);
}

inline bool operator!=(vtkCellStatus a, short b)
{
  return static_cast<short>(a) != b;
}

inline bool operator==(vtkCellStatus a, short b)
{
  return static_cast<short>(a) == b;
}

inline vtkCellStatus operator&(vtkCellStatus a, vtkCellStatus b)
{
  return static_cast<vtkCellStatus>(static_cast<short>(a) & static_cast<short>(b));
}

inline vtkCellStatus operator|(vtkCellStatus a, vtkCellStatus b)
{
  return static_cast<vtkCellStatus>(static_cast<short>(a) | static_cast<short>(b));
}

inline vtkCellStatus& operator&=(vtkCellStatus& a, vtkCellStatus b)
{
  a = static_cast<vtkCellStatus>(static_cast<short>(a) & static_cast<short>(b));
  return a;
}

inline vtkCellStatus& operator|=(vtkCellStatus& a, vtkCellStatus b)
{
  a = static_cast<vtkCellStatus>(static_cast<short>(a) | static_cast<short>(b));
  return a;
}

inline bool operator!(const vtkCellStatus& s)
{
  return s != vtkCellStatus::Valid;
}

inline std::ostream& operator<<(std::ostream& os, vtkCellStatus state)
{
  if (state == vtkCellStatus::Valid)
  {
    os << "valid";
  }
  else
  {
    bool comma = false;
    os << "invalid(";
    if ((state & vtkCellStatus::WrongNumberOfPoints) == vtkCellStatus::WrongNumberOfPoints)
    {
      os << "too few points";
      comma = true;
    }
    if ((state & vtkCellStatus::IntersectingEdges) == vtkCellStatus::IntersectingEdges)
    {
      if (comma)
      {
        os << ", ";
      }
      os << "intersecting edges";
      comma = true;
    }
    if ((state & vtkCellStatus::IntersectingFaces) == vtkCellStatus::IntersectingFaces)
    {
      if (comma)
      {
        os << ", ";
      }
      os << "intersecting faces";
      comma = true;
    }
    if ((state & vtkCellStatus::NoncontiguousEdges) == vtkCellStatus::NoncontiguousEdges)
    {
      if (comma)
      {
        os << ", ";
      }
      os << "noncontiguous edges";
      comma = true;
    }
    if ((state & vtkCellStatus::Nonconvex) == vtkCellStatus::Nonconvex)
    {
      if (comma)
      {
        os << ", ";
      }
      os << "nonconvex";
      comma = true;
    }
    if ((state & vtkCellStatus::FacesAreOrientedIncorrectly) ==
      vtkCellStatus::FacesAreOrientedIncorrectly)
    {
      if (comma)
      {
        os << ", ";
      }
      os << "misoriented faces";
      comma = true;
    }
    if ((state & vtkCellStatus::NonPlanarFaces) == vtkCellStatus::NonPlanarFaces)
    {
      if (comma)
      {
        os << ", ";
      }
      os << "non-planar faces";
    }
    if ((state & vtkCellStatus::DegenerateFaces) == vtkCellStatus::DegenerateFaces)
    {
      if (comma)
      {
        os << ", ";
      }
      os << "degenerate faces";
    }
    if ((state & vtkCellStatus::CoincidentPoints) == vtkCellStatus::CoincidentPoints)
    {
      if (comma)
      {
        os << ", ";
      }
      os << "coincident points";
    }
    os << ")";
  }
  return os;
}

VTK_ABI_NAMESPACE_END
#endif // vtkCellStatus_h
// VTK-HeaderTest-Exclude: vtkCellStatus.h
