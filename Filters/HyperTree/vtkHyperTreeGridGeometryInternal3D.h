/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridGeometry.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkHyperTreeGridGeometryInternal3D_h
#define vtkHyperTreeGridGeometryInternal3D_h

#include "vtkHyperTreeGridGeometryInternal.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkBitArray;
class vtkHyperTreeGridNonOrientedVonNeumannSuperCursor;

//------------------------------------------------------------------------------
#include <map>

typedef unsigned int VTKEdgeIndex;

const VTKEdgeIndex VTK_DEFAULT_EDGE_INDEX = 42;

//------------------------------------------------------------------------------
class vtkHyperTreeGridGeometry::vtkInternal3D : public vtkHyperTreeGridGeometry::vtkInternal
{
public:
  vtkInternal3D(std::string _trace, bool _merging_points, vtkHyperTreeGrid* _input,
    vtkPolyData* _output, vtkPoints* _outputPoints, vtkCellArray* _outputCells,
    vtkDataSetAttributes* _inputCellDataAttributes, vtkDataSetAttributes* _outputCellDataAttributes,
    bool _passThroughCellIds, const std::string& _originalCellIdArrayName);

  //----------------------------------------------------------------------------------------------
  virtual ~vtkInternal3D() override;

private:
  //----------------------------------------------------------------------------------------------
  int m_branch_factor;
  vtkBitArray* m_inPureMaskArray;
  vtkHyperTreeGridNonOrientedVonNeumannSuperCursor* m_cursor;
  unsigned int m_number_of_children; // depend on branchfactor value

public: // pas pour une bonne raison
  class Point
  {
  public:
    Point() { this->reset(); }
    ~Point() {}

    void reset() { this->m_isValid = false; }

    void set(const Point& _point)
    {
      this->m_isValid = _point.m_isValid;
      if (this->m_isValid)
      {
        this->m_xyz[0] = _point.m_xyz[0];
        this->m_xyz[1] = _point.m_xyz[1];
        this->m_xyz[2] = _point.m_xyz[2];
        this->m_cell_interface_type = _point.m_cell_interface_type;
        this->m_id = _point.m_id;
        this->m_withInterfaceA = _point.m_withInterfaceA;
        this->m_scalarInterfaceA = _point.m_scalarInterfaceA;
        this->m_withInterfaceB = _point.m_withInterfaceB;
        this->m_scalarInterfaceB = _point.m_scalarInterfaceB;
        this->m_type_point = _point.m_type_point;
      }
    }

    // _interface=true -> interfaceA ; =false -> interfaceB
    void setIntersectXYZ(
      vtkHyperTreeGridGeometry::vtkInternal* _parent, const double* _xyz, bool _interface)
    {
      TRACE("setIntersectXYZ")
      this->m_xyz[0] = _xyz[0];
      this->m_xyz[1] = _xyz[1];
      this->m_xyz[2] = _xyz[2];
      this->m_cell_interface_type = _parent->getInterfaceTypeOnThisCell();
      this->m_id = -1; // _parent->insertPoint(this->m_xyz);
      TRACE(">>>> setXYZ ID#" << this->m_id << " [" << this->m_xyz[0] << " ; " << this->m_xyz[1]
                              << " ; " << this->m_xyz[2] << "]")
      // type=-1 c'est "B" distance[1]
      // type=1 c'est "A" distance[0]
      if (_interface)
      {
        this->m_type_point = 1;
        this->m_withInterfaceA = true;
        this->m_scalarInterfaceA = 0.;
        if (_parent->hasInterfaceOnThisCell() && this->m_cell_interface_type != -1.)
        {
          this->m_withInterfaceB = true;
          this->m_scalarInterfaceB = _parent->computeInterfaceB(this->m_xyz);
        }
        else
        {
          this->m_withInterfaceB = false;
        }
      }
      else
      {
        this->m_type_point = 2;
        this->m_withInterfaceB = true;
        this->m_scalarInterfaceB = 0.;
        if (_parent->hasInterfaceOnThisCell() && this->m_cell_interface_type != 1.)
        {
          this->m_withInterfaceA = true;
          this->m_scalarInterfaceA = _parent->computeInterfaceA(this->m_xyz); // 1 c'est A
        }
      }
      this->m_isValid = true;
    }

    void setXYZ(vtkHyperTreeGridGeometry::vtkInternal* _parent, const double* _xyz)
    {
      this->m_xyz[0] = _xyz[0];
      this->m_xyz[1] = _xyz[1];
      this->m_xyz[2] = _xyz[2];
      this->m_type_point = 0;

      this->m_cell_interface_type = _parent->getInterfaceTypeOnThisCell();
      // this->m_id = _parent->insertPoint(_xyz);
      this->m_id = -1;
      TRACE(">>>> setXYZ ID#" << this->m_id << " [" << this->m_xyz[0] << " ; " << this->m_xyz[1]
                              << " ; " << this->m_xyz[2] << "]")
      if (_parent->hasInterfaceOnThisCell())
      {
        TRACE("this->m_cell_interface_type: " << this->m_cell_interface_type << " =?= 1. if:"
                                              << (this->m_cell_interface_type != 1.))
        if (this->m_cell_interface_type != 1.)
        {
          this->m_withInterfaceA = true;
          this->m_scalarInterfaceA = _parent->computeInterfaceA(this->m_xyz);
        }
        TRACE("this->m_cell_interface_type: " << this->m_cell_interface_type << " =?= -1. if:"
                                              << (this->m_cell_interface_type != -1.))
        if (this->m_cell_interface_type != -1.)
        {
          this->m_withInterfaceB = true;
          this->m_scalarInterfaceB = _parent->computeInterfaceB(this->m_xyz);
        }
      }
      this->m_isValid = true;
      TRACE(">>>> setXYZ isValid#"
        << this->m_isValid << " ID#" << this->m_id << " [" << this->m_xyz[0] << " ; "
        << this->m_xyz[1] << " ; " << this->m_xyz[2] << "] scalarA:" << this->m_scalarInterfaceA
        << " (" << this->m_withInterfaceA << ")"
        << "] scalarB:" << this->m_scalarInterfaceB << " (" << this->m_withInterfaceB << ")")
    }

    bool existInterfaceA() const { return this->m_withInterfaceA; }

    double scalarInterfaceA() const
    {
      ERROR(!this->m_withInterfaceA, "This only makes sense if the interface A exists.");
      return this->m_scalarInterfaceA;
    }

    bool existInterfaceB() const { return this->m_withInterfaceB; }

    double scalarInterfaceB() const
    {
      ERROR(!this->m_withInterfaceB, "This only makes sense if the interface B exists.");
      return this->m_scalarInterfaceB;
    }
    // -1 /A/ 0 /B/ 1 normale à droite
    vtkIdType in(vtkHyperTreeGridGeometry::vtkInternal* _parent)
    {
      if (!this->m_isValid)
      {
        TRACE("in no valid (-1)")
        return -1;
      }
      TRACE("in ID#" << this->m_id)
      if (this->m_cell_interface_type == -1)
      {
        ERROR(!this->m_withInterfaceA, "in BAD BOY A (type=-1)")
        // ERROR(this->m_withInterfaceB, "in BAD BOY B (type=-1)")
        if (this->m_withInterfaceA)
        {
          TRACE("in type#-1 interfaceA#" << this->m_scalarInterfaceA << " <?< " << 0)
          if (this->m_scalarInterfaceA < 0)
          {
            TRACE("in out type#-1 interfaceA (-1)")
            return -1;
          }
        }
        return this->getId(_parent);
      }
      else if (this->m_cell_interface_type == 0)
      {
        ERROR(!this->m_withInterfaceA, "in BAD BOY A (type=0)")
        ERROR(!this->m_withInterfaceB, "in BAD BOY B (type=0)")
        TRACE("in type#0 interfaceA#" << this->m_scalarInterfaceA << " >?> " << 0)
        if (this->m_scalarInterfaceA > 0)
        {
          TRACE("in out type#0 interfaceA (-1)")
          return -1;
        }
        TRACE("in type#0 interfaceB#" << this->m_scalarInterfaceB << " <?< " << 0)
        if (this->m_scalarInterfaceB < 0)
        {
          TRACE("in out type#0 interfaceB (-1)")
          return -1;
        }
        return this->getId(_parent);
      }
      else if (this->m_cell_interface_type == 1)
      {
        // ERROR(this->m_withInterfaceA, "in BAD BOY A (type=1)")
        ERROR(!this->m_withInterfaceB, "in BAD BOY B (type=1)")
        if (this->m_withInterfaceB)
        {
          TRACE("in type#1 interfaceB#" << this->m_scalarInterfaceB << " >?> " << 0)
          if (this->m_scalarInterfaceB > 0)
          {
            TRACE("in out type#1 interfaceB (-1)")
            return -1;
          }
        }
        return this->getId(_parent);
      }
      else
      { // case type >= 2, pure cell
        TRACE("in out type#" << this->m_cell_interface_type)
        return this->getId(_parent);
      }
    }

    bool computeEdgeInterface(vtkHyperTreeGridGeometry::vtkInternal3D* _parent, double _scalar,
      const Point& _endpoint, double _scalar_endpoint,
      std::vector<std::pair<Point, Point>>& _edge_points, unsigned int _axisEdge,
      unsigned int _iEdgeCell,
      std::map<unsigned int, std::pair<Point*, unsigned int>>& _internalFace, Point& _pointInter,
      unsigned int& _iEdgePoint);

    void computeEdge(vtkHyperTreeGridGeometry::vtkInternal3D* _parent, const Point& _point,
      std::vector<std::pair<Point, Point>>& _edge_points, unsigned int _axisEdge,
      unsigned int _iEdgeCell,
      std::map<unsigned int, std::pair<Point*, unsigned int>>& _internalFaceA,
      std::map<unsigned int, std::pair<Point*, unsigned int>>& _internalFaceB,
      unsigned int& _crtEdgePointA, unsigned int& _crtEdgePointB);

    const double* getXYZ() const { return this->m_xyz; }

    vtkIdType getId(vtkHyperTreeGridGeometry::vtkInternal* _parent) const
    {
      TRACE("getId [" << this->m_xyz[0] << " ; " << this->m_xyz[1] << " ; " << this->m_xyz[2]
                      << "] id#" << this->m_id)
      if (this->m_isValid && this->m_id < 0)
      {
        this->m_id = _parent->insertPoint(this->m_xyz);
        TRACE("getId compute id#" << this->m_id)
      }
      return this->m_id;
    }

    const bool compare(const double* _xyz) const
    {
      bool res =
        (this->m_xyz[0] == _xyz[0] && this->m_xyz[1] == _xyz[1] && this->m_xyz[2] == _xyz[2]);
      if (res)
      {
        TRACE(">>>> Compare VALIDE")
      }
      else
      {
        TRACE(">>>> Compare NON VALIDE")
      }
      return res;
    }

    // Point of intersection between the edge and the interface plane A
    // which is NOT a vertex of the edge / face / hexagon.
    bool isPointEdgeFaceA() const { return this->m_type_point == 1; }

    // Point of intersection between the edge and the interface plane A
    // which is a vertex of the edge / face / hexagon.
    bool isPointCornerFaceA() const
    {
      return this->m_type_point == 0 && this->m_withInterfaceA && this->m_scalarInterfaceA == 0.;
    }

    // Point of intersection between the edge and the interface plane B
    // which is NOT a vertex of the edge / face / hexagon.
    bool isPointEdgeFaceB() const { return this->m_type_point == 2; }

    // Point of intersection between the edge and the interface plane B
    // which is a vertex of the edge / face / hexagon.
    bool isPointCornerFaceB() const
    {
      return this->m_type_point == 0 && this->m_withInterfaceB && this->m_scalarInterfaceB == 0.;
    }

    void setIsCorner() { this->m_type_point = 0; }

    bool isValid() const { return this->m_isValid; }

  private:
    const std::string m_trace = "vtkHyperTreeGridGeometry::vtkInternal3D::Point";
    bool m_isValid = false;
    double m_cell_interface_type = 2.;
    bool m_withInterfaceA = false;
    bool m_withInterfaceB = false;
    double m_xyz[3];
    mutable vtkIdType m_id = -1;
    double m_scalarInterfaceA = 0;
    double m_scalarInterfaceB = 0;
    unsigned int m_type_point =
      0; // 0 : corner, 1 : pt sur arete pour FaceA, 2 : pt sur arete pour FaceB
  };

private:
  vtkUnsignedCharArray* m_edge_flags;

  //----------------------------------------------------------------------------------------------
  void recursivelyProcessTree(unsigned char _coarse_cell_faces_to_be_treat);

  //------------------------------------------------------------------------------
  void processLeafCell(unsigned char _coarse_cell_faces_to_be_treat, vtkIdType _inputCellIndex);

  //------------------------------------------------------------------------------
  void processLeafCellAddFace(std::vector<Point>& _cell_points,
    std::vector<std::pair<Point, Point>>& _edge_points, unsigned int _neighbor_offset_local,
    vtkIdType _inputCellIndex, const double* _cell_origin, const double* _cell_size,
    unsigned int _front_plane_offset, unsigned int _orientation, unsigned char _hideEdge,
    std::map<unsigned int, std::pair<Point*, unsigned int>>& _internalFaceA,
    std::map<unsigned int, std::pair<Point*, unsigned int>>& _internalFaceB);

  //------------------------------------------------------------------------------
  void stateInterfaceFace(
    const std::string _str, std::map<unsigned int, std::pair<Point*, unsigned int>>& _internalFace)
  {
    TRACE("")
    TRACE("internalFace" + _str + " state")
    for (const auto& [key, value] : _internalFace)
    {
      TRACE("  iedge#" << key)
      vtkIdType pid = value.first->getId(this);
      TRACE("  iedge#" << key << " " << value.first << " valid#" << value.first->isValid() << " ID#"
                       << pid << " [" << value.first->getXYZ()[0] << " ; "
                       << value.first->getXYZ()[1] << " ; " << value.first->getXYZ()[2] << "] to "
                       << value.second)
    }
    TRACE("")
  }

  //------------------------------------------------------------------------------
  void setInterfaceFace(std::vector<std::pair<vtkHyperTreeGridGeometry::vtkInternal3D::Point,
                          vtkHyperTreeGridGeometry::vtkInternal3D::Point>>& _edge_points,
    unsigned int _iEdgeCell,
    std::map<unsigned int,
      std::pair<vtkHyperTreeGridGeometry::vtkInternal3D::Point*, unsigned int>>& _internalFace,
    vtkHyperTreeGridGeometry::vtkInternal3D::Point* _pt);

  //------------------------------------------------------------------------------
  void completeChainette(
    std::map<unsigned int,
      std::pair<vtkHyperTreeGridGeometry::vtkInternal3D::Point*, unsigned int>>& _internalFace,
    unsigned int _iEdgePoint1, unsigned int _iEdgePoint2);
};

VTK_ABI_NAMESPACE_END
#endif /* vtkHyperTreeGridGeometryInternal3D_h */
