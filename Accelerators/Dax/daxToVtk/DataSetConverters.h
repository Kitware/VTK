//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#ifndef daxToVtk_DataSetConverter_h
#define daxToVtk_DataSetConverter_h

class vtkLine;
class vtkHexahedron;
class vtkQuad;
class vtkTetra;
class vtkTriangle;
class vtkVoxel;
class vtkWedge;

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

#include <dax/cont/UnstructuredGrid.h>
#include <dax/cont/UniformGrid.h>
#include <dax/cont/ArrayHandle.h>

#include "CellTypeToType.h"
#include <algorithm>


namespace daxToVtk
{

namespace detail
{
//------------------------------------------------------------------------------
template<typename CellType>
void writeCellTags(vtkCellArray *cell)
{
  //in no place do we in dax write the number of points are in the cell
  //we don't want to that in the allocator. If the allocator does it
  //we create an affinity between the thread the allocator is on and
  //the memory, which will cause performance issues when we are in openMP

  //so instead we do it once we pull back to vtk
  vtkIdType* raw_ids = cell->GetPointer();

  for(vtkIdType i=0; i < cell->GetNumberOfCells(); ++i)
    {
    raw_ids[i*(CellType::NUM_POINTS+1)]=CellType::NUM_POINTS;
    }
}

//------------------------------------------------------------------------------
template<typename CellType>
void setCells(vtkCellArray* cells, vtkPolyData* output)
{
  //get the vtk cell type we are extracting
  const VTKCellType cell_type = static_cast<VTKCellType>(CellType::VTKCellType);
  if(cell_type == VTK_VERTEX)
    {
    output->SetVerts(cells);
    }
  else if(cell_type == VTK_LINE)
    {
    output->SetLines(cells);
    }
  else if(cell_type == VTK_TRIANGLE ||
          cell_type == VTK_QUAD )
    {
    output->SetPolys(cells);
    }
}

//------------------------------------------------------------------------------
template<typename CellType>
void setCells(vtkCellArray* cells, vtkUnstructuredGrid* output)
{
  //get the vtk cell type we are extracting
  const VTKCellType cell_type = static_cast<VTKCellType>(CellType::VTKCellType);
  output->SetCells(cell_type,cells);
}


//------------------------------------------------------------------------------
template<typename ContainerTag, typename GridType, typename OutputType>
void convertCells(ContainerTag, GridType& grid, OutputType* output)
{
  //we are dealing with a container type whose memory wasn't allocated by
  //vtk so we have to copy the data into a new vtk memory location just
  //to be safe.
  typedef typename ::daxToVtk::CellTypeToType<
                                typename GridType::CellTag > CellType;

  //determine amount of memory to allocate
  const vtkIdType num_cells = grid.GetNumberOfCells();
  const vtkIdType alloc_size = grid.GetCellConnections().GetNumberOfValues();

  //get the portal from the grid.
  typedef typename GridType::CellConnectionsType::PortalConstControl
                                                                DaxPortalType;
  DaxPortalType daxPortal = grid.GetCellConnections().GetPortalConstControl();

  //ask the vtkToDax allocator to make us memory
  ::vtkToDax::vtkAlloc<vtkCellArray, CellType::NUM_POINTS> alloc;
  vtkCellArray* cells = alloc.allocate(num_cells+alloc_size);

  vtkIdType* cellPointer = cells->GetPointer();
  vtkIdType index = 0;
  for(vtkIdType i=0; i < num_cells; ++i)
    {
    *cellPointer = CellType::NUM_POINTS;
    ++cellPointer;
    for(vtkIdType j=0; j < CellType::NUM_POINTS; ++j, ++cellPointer, ++index)
      {
      *cellPointer = daxPortal.Get(index);
      }
    }

  daxToVtk::detail::setCells<CellType>(cells,output);
}

//------------------------------------------------------------------------------
template<typename CellType, typename GridType, typename OutputType>
void convertCells(vtkToDax::vtkTopologyContainerTag<CellType>,
                  GridType& grid,
                  OutputType* output)
{
  //in this use case the cell container is of vtk type so we
  //can directly hook in and use the memory the container allocated
  //for the output. This is really nice when working with TBB and OpenMP
  //device adapters.
  vtkCellArray* cells = grid.GetCellConnections().GetPortalControl().GetVtkData();


  //to properly set the cells back into vtk we have to make
  //sure that for each cell we will fill in the part which states
  //how many points are in that cells
  daxToVtk::detail::writeCellTags<CellType>(cells);

  daxToVtk::detail::setCells<CellType>(cells,output);
}

//------------------------------------------------------------------------------
template<typename ContainerTag, typename GridType, typename OutputType>
void convertPoints(ContainerTag, GridType& grid, OutputType* output)
{
  //we are dealing with a container type whose memory wasn't allocated by
  //vtk so we have to copy the data into a new vtk memory location just
  //to be safe.

  //determine amount of memory to allocate
  const vtkIdType num_points = grid.GetNumberOfPoints();

  //ask vtkToDax to allocate the vtkPoints so it gets the float vs double
  //settings correct
  ::vtkToDax::vtkAlloc<vtkPoints,3> alloc;
  vtkPoints* points = alloc.allocate(num_points);

  dax::Vector3 *raw_pts = reinterpret_cast<dax::Vector3*>(
                                       points->GetData()->GetVoidPointer(0));

  //get the coord portal from the grid.
  typedef typename GridType::PointCoordinatesType::PortalConstControl
                                                                DaxPortalType;
  DaxPortalType daxPortal = grid.GetPointCoordinates().GetPortalConstControl();

  std::copy(daxPortal.GetIteratorBegin(),
            daxPortal.GetIteratorEnd(),
            raw_pts);

  output->SetPoints( points );
}


//------------------------------------------------------------------------------
template<typename GridType, typename OutputType>
void convertPoints(vtkToDax::vtkPointsContainerTag,
                  GridType& grid,
                  OutputType* output)
{
  vtkPoints *p = grid.GetPointCoordinates().GetPortalControl().GetVtkData();
  output->SetPoints(p);
}


} //namespace detail


//------------------------------------------------------------------------------
//convert a UniformGrid to vtkImageData
inline void dataSetConverter(const dax::cont::UniformGrid<>& grid,
          vtkImageData* output)
{
  dax::Vector3 origin = grid.GetOrigin();
  dax::Vector3 spacing = grid.GetSpacing();
  dax::Extent3 extent = grid.GetExtent();

  output->SetOrigin(origin[0],origin[1],origin[2]);
  output->SetSpacing(spacing[0],spacing[1],spacing[2]);
  output->SetExtent(extent.Min[0],extent.Max[0],
                    extent.Min[1],extent.Max[1],
                    extent.Min[2],extent.Max[2]);
}

//convert a UnstructuredGrid to vtkUnstructuredGrid
template<typename CellType, typename TopoTag, typename PointTag>
inline void dataSetConverter(dax::cont::UnstructuredGrid<CellType,TopoTag,PointTag>& grid,
          vtkUnstructuredGrid* output)
{
  daxToVtk::detail::convertCells(TopoTag(),grid,output);
  daxToVtk::detail::convertPoints(PointTag(),grid,output);
}

//convert a UnstructuredGrid to vtkPolyData
template<typename CellType, typename TopoTag, typename PointTag>
inline void dataSetConverter(
  dax::cont::UnstructuredGrid<CellType,TopoTag,PointTag>& grid,
  vtkPolyData* output)
{
  daxToVtk::detail::convertCells(TopoTag(),grid,output);
  daxToVtk::detail::convertPoints(PointTag(),grid,output);
}

template<typename FieldType>
void addCellData(vtkDataSet* output,
                 FieldType& outputArray,
                 const std::string& name)
{
  vtkDataArray *data = outputArray.GetPortalControl().GetVtkData();
  data->SetName(name.c_str());
  output->GetCellData()->AddArray(data);
}


template<typename FieldType>
void addPointData(vtkDataSet* output,
                  FieldType& outputArray,
                  const std::string& name)
{
  vtkDataArray *data = outputArray.GetPortalControl().GetVtkData();
  data->SetName(name.c_str());
  vtkPointData *pd = output->GetPointData();
  pd->AddArray(data);
}


}
#endif // daxToVtk_DataSetConverter_h
