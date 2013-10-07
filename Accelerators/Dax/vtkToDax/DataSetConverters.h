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

#ifndef vtkToDax_DataSetConverter_h
#define vtkToDax_DataSetConverter_h

//datasets we support
#include "vtkDataObjectTypes.h"
#include "vtkCellTypes.h"
#include "vtkCellArray.h"
#include "vtkImageData.h"
#include "vtkUniformGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkStructuredGrid.h"

#include <dax/cont/ArrayHandle.h>
#include <dax/cont/UniformGrid.h>
#include <dax/cont/UnstructuredGrid.h>

#include "CellTypeToType.h"
#include "DataSetTypeToType.h"

#include "Portals.h"
#include "Containers.h"


namespace vtkToDax
{

template<typename CellType>
inline void convertTopology(vtkUnstructuredGrid* input,
                     std::vector<dax::Id>& topo)
{
  enum{NUM_POINTS_IN_CELL=CellType::NUM_POINTS};
  vtkIdType size = input->GetNumberOfCells();
  topo.reserve(size*NUM_POINTS_IN_CELL); //reserver room so we don't have to realloc
  vtkCellArray *cells = input->GetCells();

  vtkIdType npts, *pts;
  cells->InitTraversal();
  while(cells->GetNextCell(npts,pts))
    {
    std::copy(pts,pts+npts,std::back_inserter(topo));
    }
}



//convert an image data type
template<typename VTKDataSetType>
inline typename VTKDataSetType::DaxDataSetType dataSetConverter(
    vtkImageData* input,
    VTKDataSetType)
  {
  typedef typename VTKDataSetType::DaxDataSetType DataSet;
  double origin[3];input->GetOrigin(origin);
  double spacing[3];input->GetSpacing(spacing);
  int extent[6];input->GetExtent(extent);

  //this would be image data
  DataSet output;
  output.SetOrigin(dax::make_Vector3(origin[0],origin[1],origin[2]));
  output.SetSpacing(dax::make_Vector3(spacing[0],spacing[1],spacing[2]));
  output.SetExtent(dax::make_Id3(extent[0],extent[2],extent[4]),
                   dax::make_Id3(extent[1],extent[3],extent[5]));
  return output;
  }

//convert an uniform grid type
template<typename VTKDataSetType>
inline typename VTKDataSetType::DaxDataSetType dataSetConverter(
    vtkUniformGrid* input,
    VTKDataSetType)
  {
  typedef typename VTKDataSetType::DaxDataSetType DataSet;
  double origin[3];input->GetOrigin(origin);
  double spacing[3];input->GetSpacing(spacing);
  int extent[6];input->GetExtent(extent);

  //this would be image data
  DataSet output;
  output.SetOrigin(dax::make_Vector3(origin[0],origin[1],origin[2]));
  output.SetSpacing(dax::make_Vector3(spacing[0],spacing[1],spacing[2]));
  output.SetExtent(dax::make_Id3(extent[0],extent[2],extent[4]),
                   dax::make_Id3(extent[1],extent[3],extent[5]));
  return output;
  }

//convert an unstructured grid type
template<typename VTKDataSetType>
inline typename VTKDataSetType::DaxDataSetType dataSetConverter(
    vtkUnstructuredGrid* input,
    VTKDataSetType)
  {
  //we convert to a hexahedron unstruction grid
  //this uses a vtkCellArrayContainer to get the needed topology information
  typedef typename VTKDataSetType::DaxDataSetType DataSet;
  typedef typename VTKDataSetType::CellTypeToType CellTypeToType;

  static const int NUM_POINTS = VTKDataSetType::CellTypeToType::NUM_POINTS;


  dax::cont::ArrayHandle<dax::Vector3,vtkToDax::vtkPointsContainerTag>
      pointsHandle(vtkToDax::vtkPointsPortal<dax::Vector3>(input->GetPoints(),
                                                           input->GetNumberOfPoints()));

  //
  dax::cont::ArrayHandle<dax::Id,vtkToDax::vtkTopologyContainerTag<CellTypeToType> >
      topoHandle(vtkToDax::vtkTopologyPortal<dax::Id, NUM_POINTS >(input->GetCells(),
                                              input->GetNumberOfCells()*NUM_POINTS));

  return DataSet(topoHandle,pointsHandle);
  }
}


#endif // vtkToDax_DataSetConverter_h
