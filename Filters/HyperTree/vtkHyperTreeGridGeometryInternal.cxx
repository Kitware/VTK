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
#include "vtkHyperTreeGridGeometryInternal.h"

#include "vtkBitArray.h"
#include "vtkCellArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkHyperTreeGrid.h"
#include "vtkMergePoints.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"

#include <limits>
#include <set>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

bool __trace_htg_geometry = false;
double __trace_htg_env_vtkcellid = -2; // -2:non init; -1:noactive; # cellid

//----------------------------------------------------------------------------------------------
vtkHyperTreeGridGeometry::vtkInternal::vtkInternal(std::string _trace, bool _merging_points,
  vtkHyperTreeGrid* _input, vtkPoints* _outputPoints, vtkCellArray* _outputCells,
  vtkDataSetAttributes* _inputCellDataAttributes, vtkDataSetAttributes* _outputCellDataAttributes,
  bool _passThroughCellIds, const std::string& _originalCellIdArrayName)
  : m_trace(_trace)
  , m_merging_points(_merging_points)
  , m_input(_input)
  , m_outputPoints(_outputPoints)
  , m_outputCells(_outputCells)
  , m_inputCellDataAttributes(_inputCellDataAttributes)
  , m_outputCellDataAttributes(_outputCellDataAttributes)
{
  TRACE("vtkInternal")
  // Retrieve orientation
  this->m_orientation = this->m_input->GetOrientation();
  // Retrieve ghost
  this->m_inGhostArray = this->m_input->GetGhostCells();
  // Retrieve mask
  this->m_inMaskArray = this->m_input->HasMask() ? this->m_input->GetMask() : nullptr;
  // Retrieve interface data when relevant
  this->m_hasInterface = this->m_input->GetHasInterface();
  if (this->m_hasInterface)
  {
    this->m_inputIntercepts =
      this->m_inputCellDataAttributes->GetArray(this->m_input->GetInterfaceInterceptsName());
    if (!this->m_inputIntercepts)
    {
      WARNING("vtkInternal HasInterface=true but no interface intercepts")
      this->m_hasInterface = false;
    }
    this->m_inputNormals =
      this->m_inputCellDataAttributes->GetArray(this->m_input->GetInterfaceNormalsName());
    if (!this->m_inputNormals)
    {
      WARNING("vtkInternal HasInterface=true but no interface normals")
      this->m_hasInterface = false;
    }
  }
  this->m_cell_intercepts.resize(3);
  this->m_cell_normal.resize(3);
  this->m_locator = nullptr;
  if (this->m_merging_points)
  {
    this->m_locator = vtkMergePoints::New();
    this->m_locator->InitPointInsertion(_outputPoints, _input->GetBounds());
  }
  if (_passThroughCellIds && _originalCellIdArrayName != "")
  {
    vtkNew<vtkIdTypeArray> originalCellIds;
    originalCellIds->SetName(_originalCellIdArrayName.c_str());
    originalCellIds->SetNumberOfComponents(1);
    this->m_outputCellDataAttributes->AddArray(originalCellIds);
    this->m_outputOriginalVtkCellLocalIdOnServer = vtkIdTypeArray::SafeDownCast(
      this->m_inputCellDataAttributes->GetArray(_originalCellIdArrayName.c_str()));
  }
  else
  {
    this->m_outputOriginalVtkCellLocalIdOnServer = nullptr;
  }
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::vtkInternal::finish(){ TRACE(
  "vtkInternal::finish Points #" << this->m_outputPoints->GetNumberOfPoints())
    TRACE("vtkInternal::finish Cells #" << this->m_outputCells->GetNumberOfCells()) }

//----------------------------------------------------------------------------------------------
vtkHyperTreeGridGeometry::vtkInternal::~vtkInternal()
{
  if (this->m_locator)
  {
    this->m_locator->Delete();
  }
}

//----------------------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridGeometry::vtkInternal::insertPoint(const double* xyz)
{
  // Insert a point
  TRACE("vtkInternal::insertPoint xyz [" << xyz[0] << " ; " << xyz[1] << " ; " << xyz[2] << "]")
  if (this->m_locator)
  {
    vtkIdType offset;
    this->m_locator->InsertUniquePoint(xyz, offset);
    TRACE("vtkInternal::insertPoint by InsertUniquePoint #" << offset)
    if (offset > this->m_max_id_point)
    {
      ERROR(offset != m_max_id_point + 1,
        "vtkInternal::insertPoint If a new record, an increment of one is always expected.")
      this->m_max_id_point = offset;
    }
    return offset;
  }
  vtkIdType offset = this->m_outputPoints->InsertNextPoint(xyz);
  TRACE("vtkInternal::insertPoint by InsertNextPoint #" << offset)
  ERROR(offset != m_max_id_point + 1,
    "vtkInternal::insertPoint We always expect an increment for any new point.")
  this->m_max_id_point = offset;
  return offset;
}

//----------------------------------------------------------------------------------------------
vtkIdType vtkHyperTreeGridGeometry::vtkInternal::insertPoint(const std::vector<double>& xyz)
{
  // Insert a point
  return insertPoint(xyz.data());
}

//----------------------------------------------------------------------------------------------
void vtkHyperTreeGridGeometry::vtkInternal::createNewCellAndCopyData(
  const std::vector<vtkIdType>& _outputIndexPoints, vtkIdType _inputCellIndex)
{
  TRACE("vtkInternal::createNewCellAndCopyData BEGIN")
  // This method creates a new 2D cell from a list of point offsets then attributes
  // to this cell the values of the fields to the cells assigned to the cell
  // as offset _inputCellIndex.
  // Insert new cell
  vtkIdType outputCellIndex =
    this->m_outputCells->InsertNextCell(_outputIndexPoints.size(), _outputIndexPoints.data());
  if (HAS_TRACE)
  {
    TRACE("vtkInternal::createNewCellAndCopyData m_outputCells#"
      << outputCellIndex << " Cells##" << this->m_outputCells->GetNumberOfCells() << " Pts##"
      << _outputIndexPoints.size() << " / Pts##" << this->m_max_id_point + 1)
    for (auto pt : _outputIndexPoints)
    {
      TRACE("vtkInternal::createNewCellAndCopyData    #" << pt)
    }
  }
  // Copy the data from the cell this face comes from
  this->m_outputCellDataAttributes->CopyData(
    this->m_inputCellDataAttributes, _inputCellIndex, outputCellIndex);
  // Insert value original VTK cell local index on server
  if (m_outputOriginalVtkCellLocalIdOnServer)
  {
    TRACE("vtkInternal::createNewCellAndCopyData m_outputOriginalVtkCellLocalIdOnServer #"
      << outputCellIndex << " = " << _inputCellIndex)
    m_outputOriginalVtkCellLocalIdOnServer->InsertValue(outputCellIndex, _inputCellIndex);
  }
  TRACE("vtkInternal::createNewCellAndCopyData END")
}

//----------------------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometry::vtkInternal::isMaskedOrGhosted(vtkIdType _global_node_index) const
{
  // This method determines if the _inputCellIndex offset cell is masked or ghosted.
  if (this->m_inMaskArray && this->m_inMaskArray->GetTuple1(_global_node_index))
  {
    TRACE("vtkInternal::isMaskedOrGhosted masked")
    return true;
  }
  if (this->m_inGhostArray && this->m_inGhostArray->GetTuple1(_global_node_index))
  {
    TRACE("vtkInternal::isMaskedOrGhosted ghosted")
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometry::vtkInternal::extractCellInterface(
  vtkIdType _inputCellIndex, bool _with_inversion)
{
  // This method determines if the cell with the _inputCellIndex offset cell is a mixed cell
  // and if so, its characteristics.
  // Return :
  // - if there is an interface on this cell (m_hasInterfaceOnThisCell);
  // - the type of the mixed cell (m_cell_interface_type):
  //   - 2 is pure cell;
  //   - -1 is mixed cell with an interface plane describes by m_cell_intercepts[0]; normals is
  //     entering;
  //   - 0 is mixed cell with the double interfaces plane describe by m_cell_intercepts[0]
  //     and m_cell_intercepts[1];
  //   - 1 is mixed cell with an interface describes plane by m_cell_intercepts[1]; normals is
  //     outgoing;
  // - the normals not zero; this same normals for all interfaces plane in the same mixed cell.
  TRACE("extractCellInterface BEGIN")
  if (!this->m_hasInterface)
  {
    TRACE("extractCellInterface no interface")
    TRACE("extractCellInterface END false")
    this->m_hasInterfaceOnThisCell = false;
    this->m_cell_interface_type = 2.; // we consider pure cell
    return false;
  }
  TRACE("extractCellInterface interface")
  double* intercepts = this->m_inputIntercepts->GetTuple(_inputCellIndex);
  if (intercepts == nullptr)
  {
    TRACE("extractCellInterface but intercepts=nullptr")
    TRACE("extractCellInterface END false")
    this->m_hasInterfaceOnThisCell = false;
    this->m_cell_interface_type = 2.; // we consider pure cell
    return false;
  }
  TRACE("extractCellInterface intercepts: " << intercepts)
  this->m_cell_intercepts[0] = intercepts[0];
  this->m_cell_intercepts[1] = intercepts[1];
  this->m_cell_intercepts[2] = intercepts[2];
  this->m_cell_interface_type = this->m_cell_intercepts[2];
  if (this->m_cell_interface_type >= 2)
  {
    TRACE("extractCellInterface END false")
    this->m_hasInterfaceOnThisCell = false;
    this->m_cell_interface_type = 2.; // we consider pure cell
    return false;
  }
  TRACE("extractCellInterface interface [" << this->m_cell_intercepts[0] << " ; "
                                           << this->m_cell_intercepts[1] << " ; "
                                           << this->m_cell_intercepts[2] << "]")
  double* normal = this->m_inputNormals->GetTuple(_inputCellIndex);
  if (normal == nullptr)
  {
    TRACE("extractCellInterface but normals=nullptr")
    TRACE("extractCellInterface END false")
    this->m_hasInterfaceOnThisCell = false;
    this->m_cell_interface_type = 2.; // we consider pure cell
    return false;
  }
  if (normal[0] == 0. && normal[1] == 0. && normal[2] == 0.)
  {
    TRACE("extractCellInterface but normals=[0.,0.,0.]")
    TRACE("extractCellInterface END false")
    this->m_hasInterfaceOnThisCell = false;
    this->m_cell_interface_type = 2.; // we consider pure cell
    return false;
  }
  TRACE("extractCellInterface normals: " << normal)
  this->m_cell_normal[0] = normal[0];
  this->m_cell_normal[1] = normal[1];
  this->m_cell_normal[2] = normal[2];
  TRACE("extractCellInterface normal [" << this->m_cell_normal[0] << " ; " << this->m_cell_normal[1]
                                        << " ; " << this->m_cell_normal[2] << "]")
  if (this->m_cell_interface_type == 0)
  {
    bool trace_htg_geometry = __trace_htg_geometry;
    __trace_htg_geometry = true;
    TRACE("extractCellInterface normal [" << this->m_cell_normal[0] << " ; "
                                          << this->m_cell_normal[1] << " ; "
                                          << this->m_cell_normal[2] << "]")
    TRACE("extractCellInterface mixed cell type# 0")
    double dD = this->m_cell_intercepts[1] - this->m_cell_intercepts[0];
    TRACE("extractCellInterface d2-d1# " << dD)
    if (!_with_inversion || dD >= 0)
    {
      TRACE("extractCellInterface dD valide")
    }
    else
    {
      // In the case of the "sandwich" material defined by two interfaces planes,
      // the implementation considers that :
      // - all interface planes are described by the same normal (u,v,w) ;
      // - an interface plane is described by the equation : u.x+v.y+w.z+d=0 ;
      // - in the direction of the normal, we first traverse the first interface plane
      //   defined by d1 (m_cell_intercepts[0]) then the second interface plane defined
      //   by d2 (m_cell_intercepts[1]).
      // It seems that sometimes the code makes a mistake in the attribution to
      // d1 and d2 which has the effect of disturbing the proper functioning of
      // the implementation.
      // This is why if d2 - d1 is negative, the assignment is reversed.
      // The demonstration of this is easy to achieve starting from the straight line
      // equation of each of the interfaces and the parametric equation of the straight
      // line starting from a point of the interface A towards the interface B.
      // The scalar product of BA by the normal is positive only if d2-d1 is.
      WARNING("extractCellInterface dD non valide (inversion)")
      double d = this->m_cell_intercepts[1];
      this->m_cell_intercepts[1] = this->m_cell_intercepts[0];
      this->m_cell_intercepts[0] = d;
    }
    __trace_htg_geometry = trace_htg_geometry;
  }
  TRACE("extractCellInterface END true")
  this->m_hasInterfaceOnThisCell = true;
  return true;
}

//----------------------------------------------------------------------------------------------
bool vtkHyperTreeGridGeometry::vtkInternal::hasInterface(vtkIdType _inputCellIndex) const
{
  // Only useful in 3D, this method makes it possible to know if the neighboring cell
  // of _inputCellIndex offset is pure or describes an interface.
  // It is pure if:
  // - there is no defined interface (m_hasInterface);
  // - there is no description of the interfaces (m_inputIntercepts);
  // - there is a description of the interfaces but the mixed cell type is not 2
  //   (pure cell) (m_inputIntercepts[2]); -1 and 1 describes a case of a mixed
  //   cell of a material with a single interface, 0 a case of a mixed cell of a
  //   material with a double interface;
  // - there is no description of the normals (m_inputNormals);
  // - there is a description of the normals but not zero.
  TRACE("vtkInternal::hasInterface #" << _inputCellIndex << " ")
  if (_inputCellIndex < 0)
  {
    TRACE("vtkInternal::hasInterface index not valid")
    return false;
  }
  if (!this->m_hasInterface)
  {
    TRACE("vtkInternal::hasInterface no interface")
    return false;
  }
  if (HAS_TRACE)
  {
    if (this->m_inputIntercepts)
    {
      TRACE("vtkInternal::hasInterface mixed cell type #"
        << _inputCellIndex << " " << (this->m_inputIntercepts->GetTuple(_inputCellIndex))[2])
    }
  }
  bool ret = (this->m_inputIntercepts &&
    ((this->m_inputIntercepts->GetTuple(_inputCellIndex))[2] < 2) && this->m_inputNormals);
  if (ret)
  {
    double* normal = this->m_inputNormals->GetTuple(_inputCellIndex);
    ret = !(normal[0] == 0. && normal[1] == 0. && normal[2] == 0.);
  }
  return ret;
}

//----------------------------------------------------------------------------------------------
double vtkHyperTreeGridGeometry::vtkInternal::computeInterfaceA(const double* xyz) const
{
  // Compute the value of the distance from a point to the interface plane A.
  double val = this->m_cell_intercepts[0] + this->m_cell_normal[0] * xyz[0] +
    this->m_cell_normal[1] * xyz[1] + this->m_cell_normal[2] * xyz[2];
  TRACE("vtkInternal::computeInterfaceA xyz [" << xyz[0] << " ; " << xyz[1] << " ; " << xyz[2]
                                               << "] val:" << val)
  return val;
}

//----------------------------------------------------------------------------------------------
double vtkHyperTreeGridGeometry::vtkInternal::computeInterfaceB(const double* xyz) const
{
  // Compute the value of the distance from a point to the interface plane B.
  double val = this->m_cell_intercepts[1] + this->m_cell_normal[0] * xyz[0] +
    this->m_cell_normal[1] * xyz[1] + this->m_cell_normal[2] * xyz[2];
  TRACE("vtkInternal::computeInterfaceB xyz [" << xyz[0] << " ; " << xyz[1] << " ; " << xyz[2]
                                               << "] val:" << val)
  return val;
}

VTK_ABI_NAMESPACE_END
