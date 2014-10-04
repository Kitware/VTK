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

#ifndef vtkToDax_Threshold_h
#define vtkToDax_Threshold_h

#include "vtkToDax/CellTypeToType.h"
#include "vtkToDax/CompactPointField.h"
#include "vtkToDax/DataSetConverters.h"
#include "vtkToDax/DataSetTypeToType.h"

#include "daxToVtk/CellTypeToType.h"
#include "daxToVtk/DataSetConverters.h"

#include <dax/cont/DispatcherGenerateTopology.h>
#include <dax/cont/DispatcherMapCell.h>
#include <dax/worklet/Threshold.h>

namespace
{
template <typename T> struct ThresholdOuputType
{
  typedef T type;
};
template <> struct ThresholdOuputType< dax::CellTagVoxel >
{
  typedef dax::CellTagHexahedron type;
};
}

namespace vtkToDax
{

  template<int B>
  struct DoThreshold
  {
    template<class InGridType,
             class OutGridType,
             typename ValueType,
             class Container,
             class Adapter>
    int operator()(const InGridType &,
                   vtkDataSet *,
                   OutGridType &,
                   vtkUnstructuredGrid *,
                   ValueType,
                   ValueType,
                   const dax::cont::ArrayHandle<ValueType,Container,Adapter> &)
      {
      vtkGenericWarningMacro(
            << "Not calling Dax, GridType-CellType combination not supported");
      return 0;
      }
  };
  template<>
  struct DoThreshold<1>
  {
    template<class InGridType,
             class OutGridType,
             typename ValueType,
             class Container,
             class Adapter>
    int operator()(
        const InGridType &inDaxGrid,
        vtkDataSet *inVTKGrid,
        OutGridType &outDaxGeom,
        vtkUnstructuredGrid *outVTKGrid,
        ValueType thresholdMin,
        ValueType thresholdMax,
        const dax::cont::ArrayHandle<ValueType,Container,Adapter> &thresholdHandle)
      {
      int result=1;
      try
        {
        typedef dax::worklet::ThresholdCount<ValueType> ThresholdCountType;

        typedef dax::cont::DispatcherGenerateTopology<
                  dax::worklet::ThresholdTopology,
                  dax::cont::ArrayHandle< dax::Id >,
                  Adapter >                             DispatchGT;

        typedef typename DispatchGT::CountHandleType CountHandleType;

        ThresholdCountType countWorklet(thresholdMin,thresholdMax);
        dax::cont::DispatcherMapCell<ThresholdCountType,Adapter>
                                                  dispatchCount( countWorklet );

        CountHandleType count;
        dispatchCount.Invoke(inDaxGrid, thresholdHandle, count);

        DispatchGT resolveTopology(count);
        resolveTopology.Invoke(inDaxGrid,outDaxGeom);

        // Convert output geometry to VTK.
        daxToVtk::dataSetConverter(outDaxGeom, outVTKGrid);

        // Copy arrays where possible.
        vtkToDax::CompactPointField<DispatchGT> compact(resolveTopology,
                                                        outVTKGrid);

        vtkDispatcher<vtkAbstractArray,int> compactDispatcher;
        compactDispatcher.Add<vtkFloatArray>(compact);
        compactDispatcher.Add<vtkDoubleArray>(compact);
        compactDispatcher.Add<vtkUnsignedCharArray>(compact);
        compactDispatcher.Add<vtkIntArray>(compact);

        vtkPointData *pd = inVTKGrid->GetPointData();
        for (int arrayIndex = 0;
             arrayIndex < pd->GetNumberOfArrays();
             arrayIndex++)
          {
          vtkDataArray *array = pd->GetArray(arrayIndex);
          if (array == NULL) { continue; }

          compactDispatcher.Go(array);
          }

        // Pass information about attributes.
        for (int attributeType = 0;
             attributeType < vtkDataSetAttributes::NUM_ATTRIBUTES;
             attributeType++)
          {
          vtkDataArray *attribute = pd->GetAttribute(attributeType);
          if (attribute == NULL) { continue; }
          outVTKGrid->GetPointData()->SetActiveAttribute(attribute->GetName(),
                                                         attributeType);
          }
      }
      catch(dax::cont::ErrorControlOutOfMemory error)
        {
        std::cerr << "Ran out of memory trying to use the GPU" << std::endl;
        std::cerr << error.GetMessage() << std::endl;
        result = 0;
        }
      catch(dax::cont::ErrorExecution error)
        {
        std::cerr << "Got ErrorExecution from Dax." << std::endl;
        std::cerr << error.GetMessage() << std::endl;
        result = 0;
        }
      return result;
      }
  };

  template<typename FieldType_>
  struct Threshold
  {
    public:
    typedef FieldType_ FieldType;
    //we expect FieldType_ to be an dax::cont::ArrayHandle
    typedef typename FieldType::ValueType T;

    Threshold(const FieldType& f, T min, T max):
      Result(NULL),
      Field(f),
      Min(min),
      Max(max),
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
      typedef typename ThresholdOuputType< typename VTKCellTypeStruct::DaxCellType >::type OutCellType;

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

      InputDataSetType inputDaxData = vtkToDax::dataSetConverter(&dataSet,
                                                                 DataSetTypeToTypeStruct());

      vtkToDax::DoThreshold<DataSetTypeToTypeStruct::Valid> threshold;
      int result = threshold(inputDaxData,
                             &dataSet,
                             resultGrid,
                             this->Result,
                             this->Min,
                             this->Max,
                             this->Field);

      return result;

      }
  private:
    vtkUnstructuredGrid* Result;
    FieldType Field;
    T Min;
    T Max;
    std::string Name;

  };
}

#endif //vtkToDax_Threshold_h
