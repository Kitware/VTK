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
#include "vtkmGradient.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#include "vtkmlib/ArrayConverters.h"
#include "vtkmlib/DataSetConverters.h"
#include "vtkmlib/PolyDataConverter.h"
#include "vtkmlib/Storage.h"

#include "vtkmCellSetExplicit.h"
#include "vtkmCellSetSingleType.h"
#include "vtkmFilterPolicy.h"

#include <vtkm/filter/Gradient.h>
#include <vtkm/filter/PointAverage.h>

vtkStandardNewMacro(vtkmGradient)

namespace
{
struct GradientTypes
    : vtkm::ListTagBase<
                        vtkm::Float32,
                        vtkm::Float64,
                        vtkm::Vec<vtkm::Float32,3>,
                        vtkm::Vec<vtkm::Float64,3>,
                        vtkm::Vec< vtkm::Vec<vtkm::Float32,3>, 3>,
                        vtkm::Vec< vtkm::Vec<vtkm::Float64,3>, 3>
                        >
{
};

//------------------------------------------------------------------------------
class vtkmGradientFilterPolicy
      : public vtkm::filter::PolicyBase<vtkmGradientFilterPolicy>
  {
  public:
    typedef GradientTypes FieldTypeList;
    typedef tovtkm::TypeListTagVTMOut FieldStorageList;

    typedef tovtkm::CellListStructuredInVTK StructuredCellSetList;
    typedef tovtkm::CellListUnstructuredInVTK UnstructuredCellSetList;
    typedef tovtkm::CellListAllInVTK AllCellSetList;

    typedef vtkm::TypeListTagFieldVec3 CoordinateTypeList;
    typedef tovtkm::PointListInVTK CoordinateStorageList;

    typedef vtkm::filter::PolicyDefault::DeviceAdapterList DeviceAdapterList;
  };
}

//------------------------------------------------------------------------------
vtkmGradient::vtkmGradient()
{
}

//------------------------------------------------------------------------------
vtkmGradient::~vtkmGradient()
{
}

//------------------------------------------------------------------------------
void vtkmGradient::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkmGradient::RequestData(vtkInformation* request,
                              vtkInformationVector** inputVector,
                              vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkDataSet* input =
      vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDataSet* output =
      vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  output->ShallowCopy(input);

  // grab the input array to process to determine the field want to compute
  // the gradient for
  int association = this->GetInputArrayAssociation(0, inputVector);

  // convert the input dataset to a vtkm::cont::DataSet. We explicitly drop
  // all arrays from the conversion as this algorithm doesn't change topology
  // and therefore doesn't need input fields converted through the VTK-m filter
  vtkm::cont::DataSet in = tovtkm::Convert(input, tovtkm::FieldsFlag::None);


  // convert the array over to vtkm
  vtkDataArray* inputArray = this->GetInputArrayToProcess(0, inputVector);
  vtkm::cont::Field field = tovtkm::Convert(inputArray, association);

  const bool dataSetValid =
      in.GetNumberOfCoordinateSystems() > 0 && in.GetNumberOfCellSets() > 0;

  const bool fieldIsPoint = field.GetAssociation() == vtkm::cont::Field::ASSOC_POINTS;
  const bool fieldIsCell = field.GetAssociation() == vtkm::cont::Field::ASSOC_CELL_SET;
  const bool fieldIsVec = (inputArray->GetNumberOfComponents() == 3);
  const bool fieldIsScalar = inputArray->GetDataType() == VTK_FLOAT ||
                             inputArray->GetDataType() == VTK_DOUBLE;
  const bool fieldValid = (fieldIsPoint || fieldIsCell) &&
                          fieldIsScalar &&
                          (field.GetName() != std::string());

  if(!(dataSetValid && fieldValid))
  {
    vtkWarningMacro(<< "Unable convert dataset over to VTK-m.\n"
                    << "Falling back to vtkGradientFilter."
                    );
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  vtkmGradientFilterPolicy policy;
  vtkm::filter::Gradient filter;
  filter.SetColumnMajorOrdering();

  if( fieldIsVec )
  { //this properties are only valid when processing a vec<3> field
    filter.SetComputeDivergence( this->ComputeDivergence != 0 );
    filter.SetComputeVorticity( this->ComputeVorticity != 0 );
    filter.SetComputeQCriterion( this->ComputeQCriterion != 0 );
  }

  if(this->ResultArrayName)
  {
    filter.SetOutputFieldName(this->ResultArrayName);
  }

  if(this->DivergenceArrayName)
  {
    filter.SetDivergenceName( this->DivergenceArrayName );
  }

  if(this->VorticityArrayName)
  {
    filter.SetVorticityName( this->VorticityArrayName );
  }

  if(this->QCriterionArrayName)
  {
    filter.SetQCriterionName( this->QCriterionArrayName );
  }
  else
  {
    filter.SetQCriterionName( "Q-criterion" );
  }

  // Run the VTK-m Gradient Filter
  // ----------------------------- //
  vtkm::filter::Result result;
  if(fieldIsPoint)
  {
    filter.SetComputePointGradient( !this->FasterApproximation );
    result = filter.Execute(in, field, policy);
  }
  else
  {
    //we need to convert the field to be a point field
    vtkm::filter::PointAverage cellToPoint;
    cellToPoint.SetOutputFieldName(field.GetName());
    vtkm::filter::Result toPoint = cellToPoint.Execute(in, field, policy);

    filter.SetComputePointGradient( false );
    result = filter.Execute(in, toPoint.GetField(), policy);
  }

  // Verify that the filter ran correctly
  // ----------------------------- //
  if(!result.IsFieldValid())
  {
    vtkWarningMacro(<< "VTK-m gradient computation failed for an unknown reason.\n"
                    << "Falling back to vtkGradientFilter."
                    );
    return this->Superclass::RequestData(request, inputVector, outputVector);
  }

  //When we have faster approximation enabled the VTK-m gradient will output
  //a cell field not a point field. So at that point we will need to convert
  //back to a point field
  if((fieldIsPoint && this->FasterApproximation))
  {
    vtkm::cont::DataSet const& resultData = result.GetDataSet();

    //We need to convert this field back to a point field
    vtkm::filter::PointAverage cellToPoint;
    cellToPoint.SetOutputFieldName(filter.GetOutputFieldName());
    vtkm::filter::Result toPointResult = cellToPoint.Execute(in,
                                                             result.GetField(),
                                                             policy);
    if(this->ComputeGradient)
      {
      vtkDataArray* gradientArray = fromvtkm::Convert(toPointResult.GetField());
      output->GetPointData()->AddArray(gradientArray);
      }

    if(this->ComputeDivergence && fieldIsVec)
      {
      vtkm::filter::Result dresult =
        cellToPoint.Execute(in, resultData.GetField(filter.GetDivergenceName()));
      vtkDataArray* divergenceArray = fromvtkm::Convert(dresult.GetField());
      output->GetPointData()->AddArray(divergenceArray);
      }

    if(this->ComputeVorticity  && fieldIsVec)
      {
      vtkm::filter::Result vresult =
        cellToPoint.Execute(in, resultData.GetField(filter.GetVorticityName()));
      vtkDataArray* vorticityArray = fromvtkm::Convert(vresult.GetField());
      output->GetPointData()->AddArray(vorticityArray);
      }

    if(this->ComputeQCriterion && fieldIsVec)
      {
      vtkm::filter::Result qresult =
        cellToPoint.Execute(in,resultData.GetField(filter.GetQCriterionName()));
      vtkDataArray* qcriterionArray = fromvtkm::Convert(qresult.GetField());
      output->GetPointData()->AddArray(qcriterionArray);
      }
  }
  else
  {
    // convert arrays back to VTK
    if (!fromvtkm::ConvertArrays(result.GetDataSet(), output))
    {
      vtkErrorMacro(<< "Unable to convert VTKm DataSet back to VTK");
      return 0;
    }
  }

  return 1;
}
