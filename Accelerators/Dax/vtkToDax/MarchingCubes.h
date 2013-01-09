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

#ifndef vtkToDax_MarchingCubes_h
#define vtkToDax_MarchingCubes_h

#include "DataSetTypeToType.h"
#include "CellTypeToType.h"
#include "DataSetConverter.h"

#include "../daxToVtk/DataSetConverter.h"
#include "../daxToVtk/ArrayConverter.h"


#include <dax/cont/Scheduler.h>
#include <dax/cont/ScheduleGenerateTopology.h>
#include <dax/worklet/MarchingCubes.worklet>


namespace
{
template <typename T> struct MarchingCubesOuputType
{
  typedef dax::CellTagTriangle type;
};
}

namespace vtkToDax
{
template<typename FieldType_>
struct MarchingCubes
{
  public:
  typedef FieldType_ FieldType;
  //we expect FieldType_ to be an dax::cont::ArrayHandle
  typedef typename FieldType::ValueType T;

  MarchingCubes(const FieldType& f, T value):
    Result(NULL),
    Field(f),
    Value(value),
    Name()
    {
    }

  void setOutputGrid(vtkUnstructuredGrid* grid)
    {
    Result=grid;
    }

  void setFieldName(const char* name)
    {
    Name=std::string(name);
    }

  template<typename LHS, typename RHS>
  int operator()(LHS &dataSet, const RHS&) const
    {
    typedef CellTypeToType<RHS> VTKCellTypeStruct;
    typedef DataSetTypeToType<CellTypeToType<RHS>,LHS> DataSetTypeToTypeStruct;

    //get the mapped output type of this operation(threshold)
    //todo make this a typedef on the threshold
    typedef typename MarchingCubesOuputType< typename VTKCellTypeStruct::DaxCellType >::type OutCellType;

    //get the input dataset type
    typedef typename DataSetTypeToTypeStruct::DaxDataSetType InputDataSetType;

    //construct the output grid type to use the vtk containers
    //as we know we are going back to vtk. In a more general framework
    //we would want a tag to say what the destination container tag types
    //are
    dax::cont::UnstructuredGrid<OutCellType,
              vtkToDax::vtkTopologyContainerTag<OutCellType>,
              vtkToDax::vtkPointsContainerTag> resultGrid;

    //get from the Field the proper handle type
    FieldType outputHandle;
    InputDataSetType inputDaxData = vtkToDax::dataSetConverter(&dataSet,
                                                               DataSetTypeToTypeStruct());


    std::cout << "Input Data Set NumCells: " << inputDaxData.GetNumberOfCells() << std::endl;
    std::cout << "Input Data Set NumPoints: "  << inputDaxData.GetNumberOfPoints() << std::endl;


    int result=1;
    try
      {
      dax::cont::worklet::MarchingCubes(inputDaxData,resultGrid,this->Value,
                                          this->Field,outputHandle);
      }
    catch(...)
      {
      result=0;
      }

    if(result==1)
      {
      //if we converted correctly, copy the data back to VTK
      //remembering to add back in the output array to the generated
      //unstructured grid
      daxToVtk::addPointData(this->Result,outputHandle,this->Name);
      daxToVtk::dataSetConverter(resultGrid,this->Result);

      std::cout << "Dax Data Set NumCells: " << resultGrid.GetNumberOfCells() << std::endl;
      std::cout << "Dax Data Set NumPoints: "  << resultGrid.GetNumberOfPoints() << std::endl;

      std::cout << "VTK Data Set NumCells: " << this->Result->GetNumberOfCells() << std::endl;
      std::cout << "VTK Data Set NumPoints: "  << this->Result->GetNumberOfPoints() << std::endl;
      }

    return result;

    }
private:
  vtkUnstructuredGrid* Result;
  FieldType Field;
  T Value;
  std::string Name;

};
}

#endif // vtkToDax_MarchingCubes_h
