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
#include "DataSetConverters.h"

#include <daxToVtk/CellTypeToType.h>
#include <daxToVtk/DataSetConverters.h>

#include <dax/cont/Scheduler.h>
#include <dax/cont/ScheduleGenerateTopology.h>
#include <dax/worklet/MarchingCubes.worklet>

namespace
{
template <typename T> struct MarchingCubesOuputType
{
  typedef T type;
};
template <> struct MarchingCubesOuputType< dax::CellTagVoxel >
{
  typedef dax::CellTagHexahedron type;
};
}

namespace vtkToDax
{
  template<int B>
  struct DoMarchingCubes
  {
    template<class InGridType,
             class OutGridType,
             typename ValueType,
             typename TriangleValueType,
             class Container1,
             class Container2,
             class Adapter>
    int operator()(const InGridType &,
                   OutGridType &,
                   ValueType,
                   const dax::cont::ArrayHandle<ValueType,Container1,Adapter> &,
                   dax::cont::ArrayHandle<TriangleValueType,Container2,Adapter> &)
      {
      std::cout << "Not calling DAX, GridType and CellType combination not supported" << std::endl;
      return 0;
      }
  };
  template<>
  struct DoMarchingCubes<1>
  {
    template<class InGridType,
             class OutGridType,
             typename ValueType,
             typename TriangleValueType,
             class Container1,
             class Container2,
             class Adapter>
    int operator()(
        const InGridType &inGrid,
        OutGridType &outGeom,
        ValueType isoValue,
        const dax::cont::ArrayHandle<ValueType,Container1,Adapter> &mcHandle,
        dax::cont::ArrayHandle<TriangleValueType,Container2,Adapter> &mcResult)
      {
      std::cout << "GridType and CellType are supported by Dax"
                << std::endl;
      std::cout << "Doing MarchingCubes..."
                << std::endl;
      int result=1;
      dax::Scalar isoValueT(isoValue);

      dax::cont::UnstructuredGrid<dax::CellTagTriangle> resultGrid;

      try {

        typedef dax::cont::ScheduleGenerateTopology
          <dax::worklet::MarchingCubesTopology,Adapter> ScheduleGT;
        typedef typename ScheduleGT::ClassifyResultType  ClassifyResultType;

        // construct the scheduler that will execute all the worklets
        dax::cont::Scheduler<Adapter> scheduler;

        // construct the two worklets what will be used to do marching cubes
        dax::worklet::MarchingCubesClassify classifyWorklet(isoValueT);
        dax::worklet::MarchingCubesTopology generateWorklet(isoValueT);

        // run the first step
        ClassifyResultType classification; // array handle for the
                                           // first step
                                           // (classification)
        scheduler.Invoke(classifyWorklet,
                         inGrid,
                         mcHandle,
                         classification);

        // construct the topology generation worklet
        ScheduleGT generate(classification,generateWorklet);
        generate.SetRemoveDuplicatePoints(false);

        // run the second step
        scheduler.Invoke(generate,
                         inGrid,
                         resultGrid,
                         // outGeom,
                         mcHandle,
                         inGrid.GetPointCoordinates(),
                         mcResult);

        // convert mcResult to a sequential array.
        std::cout << " size of output = "
                  << mcResult.GetNumberOfValues()
                  << std::endl;
        // mcResult.CopyInto(mcResult.GetPortalControl().GetIteratorBegin());
      }

      catch(...){result=0; }
      return result;
      }
  };

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

      //get the mapped output type of this operation(MarchingCubes)
      //todo make this a typedef on the MarchingCubes
      typedef typename MarchingCubesOuputType< typename VTKCellTypeStruct::DaxCellType >::type OutCellType;

      //get the input dataset type
      typedef typename DataSetTypeToTypeStruct::DaxDataSetType InputDataSetType;

      //construct the output grid type to use the vtk containers
      //as we know we are going back to vtk. In a more general framework
      //we would want a tag to say what the destination container tag types
      //are
      typedef daxToVtk::CellTypeToType<OutCellType> VTKCellType;
      dax::cont::UnstructuredGrid<OutCellType,
                vtkToDax::vtkTopologyContainerTag<VTKCellType>,
                vtkToDax::vtkPointsContainerTag> resultGrid;

      //schedule marching cubes worklet generate step, saving
      //the coordinates into outputHandle.
      typedef dax::Tuple<dax::Vector3,3> TriCoordinatesType;
      dax::cont::ArrayHandle<TriCoordinatesType> outputHandle;

      InputDataSetType inputDaxData = vtkToDax::dataSetConverter(&dataSet,
                                                                 DataSetTypeToTypeStruct());


      std::cout << "Input Data Set NumCells:"
                << inputDaxData.GetNumberOfCells()
                << std::endl;
      std::cout << "Input Data Set NumPoints: "
                << inputDaxData.GetNumberOfPoints()
                << std::endl;

      vtkToDax::DoMarchingCubes<DataSetTypeToTypeStruct::Valid> mc;
      int result = mc(inputDaxData,
                       resultGrid,
                       this->Value,
                       this->Field,
                       outputHandle);
      std::cout << "result = " << result << std::endl;

      if(result==1)
        {
        //if we converted correctly, copy the data back to VTK
        //remembering to add back in the output array to the generated
        //unstructured grid

        //get from the Field the proper handle type
        // typedef FieldType::PortalConstControl<T> PortalType<T>;
        // FieldType pointOutputHandle(PortalType(outputHandle.GetPortalControl().GetIteratorBegin(),outputHandle.GetNumberOfValues()*3));
        FieldType outFieldType;

        daxToVtk::addPointData(this->Result,outputHandle,outFieldType,this->Name);
        // daxToVtk::dataSetConverter(resultGrid,this->Result);

        std::cout << "Dax Data Set NumCells: "
                  << resultGrid.GetNumberOfCells()
                  << std::endl;
        std::cout << "Dax Data Set NumPoints: "
                  << resultGrid.GetNumberOfPoints()
                  << std::endl;

        std::cout << "VTK Data Set NumCells: "
                  << this->Result->GetNumberOfCells()
                  << std::endl;
        std::cout << "VTK Data Set NumPoints: "
                  << this->Result->GetNumberOfPoints()
                  << std::endl;
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

#endif //vtkToDax_MarchingCubes_h
