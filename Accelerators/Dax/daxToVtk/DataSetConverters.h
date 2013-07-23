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

#include <dax/cont/UnstructuredGrid.h>
#include <dax/cont/UniformGrid.h>
#include <dax/cont/ArrayHandle.h>

#include "CellTypeToType.h"

namespace daxToVtk
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
struct UniformDataSetConverter
{
  void operator()(const dax::cont::UniformGrid<>& grid,
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
};

//convert a UniformGrid to vtkImageData
inline void dataSetConverter(const dax::cont::UniformGrid<>& grid,
          vtkImageData* output)
{
  daxToVtk::UniformDataSetConverter()(grid,output);
}

//convert a UnstructuredGrid to vtkUnstructuredGrid
template<typename CellType, typename TopoTag, typename PointTag>
inline void dataSetConverter(dax::cont::UnstructuredGrid<CellType,TopoTag,PointTag>& grid,
          vtkUnstructuredGrid* output)
{
  //get the vtk cell type we are extracting
  enum{CELL_TYPE=CellTypeToType<CellType>::VTKCellType};

  //to properly set the points back into vtk we have to make
  //sure that for each cell we will fill in the part which states
  //how many points are in that cells
  vtkCellArray* cells = grid.GetCellConnections().GetPortalControl().GetVtkData();
  daxToVtk::writeCellTags<CellTypeToType<CellType> >(cells);
  output->SetCells(CELL_TYPE,cells);

  vtkPoints *p = grid.GetPointCoordinates().GetPortalControl().GetVtkData();
  output->SetPoints(p);
}

//convert a UnstructuredGrid to vtkPolyData
template<typename CellType, typename TopoTag, typename PointTag>
inline void dataSetConverter(
  dax::cont::UnstructuredGrid<CellType,TopoTag,PointTag>& grid,
  vtkPolyData* output)
{
  //get the vtk cell type we are extracting
  enum{CELL_TYPE=CellTypeToType<CellType>::VTKCellType};

  //to properly set the points back into vtk we have to make
  //sure that for each cell we will fill in the part which states
  //how many points are in that cells
  vtkCellArray* cells = grid.GetCellConnections().GetPortalControl().GetVtkData();
  daxToVtk::writeCellTags<CellTypeToType<CellType> >(cells);
  output->SetPolys(cells);

  vtkPoints *p = grid.GetPointCoordinates().GetPortalControl().GetVtkData();
  output->SetPoints(p);
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
