/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericProbeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericProbeFilter.h"

#include "vtkCell.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkGenericDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkGenericCellIterator.h"
#include "vtkGenericAdaptorCell.h"
#include "vtkGenericAttribute.h"
#include "vtkGenericAttributeCollection.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkGenericProbeFilter);

//----------------------------------------------------------------------------
vtkGenericProbeFilter::vtkGenericProbeFilter()
{
  this->ValidPoints = vtkIdTypeArray::New();
  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
vtkGenericProbeFilter::~vtkGenericProbeFilter()
{
  this->ValidPoints->Delete();
  this->ValidPoints = NULL;
}


//----------------------------------------------------------------------------
void vtkGenericProbeFilter::SetSourceData(vtkGenericDataSet *input)
{
  this->SetInputData(1, input);
}

//----------------------------------------------------------------------------
vtkGenericDataSet *vtkGenericProbeFilter::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return NULL;
  }

  return vtkGenericDataSet::SafeDownCast(
    this->GetExecutive()->GetInputData(1, 0));
}


//----------------------------------------------------------------------------
int vtkGenericProbeFilter::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // A variation of the bug fix from John Biddiscombe.
  // Make sure that the scalar type and number of components
  // are propagated from the source not the input.
  if (vtkImageData::HasScalarType(sourceInfo))
  {
    vtkImageData::SetScalarType(vtkImageData::GetScalarType(sourceInfo),
                                outInfo);
  }
  if (vtkImageData::HasNumberOfScalarComponents(sourceInfo))
  {
    vtkImageData::SetNumberOfScalarComponents(
      vtkImageData::GetNumberOfScalarComponents(sourceInfo),
      outInfo);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkGenericProbeFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkGenericDataSet *source = vtkGenericDataSet::SafeDownCast(
    sourceInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType ptId, numPts;
  double x[3], tol2;
  int subId;

  double pcoords[3];

  vtkDebugMacro(<<"Probing data");

  if (source == NULL)
  {
    vtkErrorMacro (<< "Source is NULL.");
    return 1;
  }


  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  numPts = input->GetNumberOfPoints();
  this->ValidPoints->Allocate(numPts);

  vtkPointData *outputPD = output->GetPointData();
  vtkCellData *outputCD = output->GetCellData();

  // prepare the output attributes
  vtkGenericAttributeCollection *attributes=source->GetAttributes();
  vtkGenericAttribute *attribute;
  vtkDataArray *attributeArray;

  int c=attributes->GetNumberOfAttributes();
  vtkDataSetAttributes *dsAttributes;

  int attributeType;

  double *tuples = new double[attributes->GetMaxNumberOfComponents()];

  for(int i = 0; i<c; ++i)
  {
    attribute=attributes->GetAttribute(i);
    attributeType=attribute->GetType();
    if(attribute->GetCentering()==vtkPointCentered)
    {
      dsAttributes=outputPD;
    }
    else // vtkCellCentered
    {
      dsAttributes=outputCD;
    }
    attributeArray=vtkDataArray::CreateDataArray(attribute->GetComponentType());
    attributeArray->SetNumberOfComponents(attribute->GetNumberOfComponents());
    attributeArray->SetName(attribute->GetName());
    dsAttributes->AddArray(attributeArray);
    attributeArray->Delete();

    if(dsAttributes->GetAttribute(attributeType)==0)
    {
      dsAttributes->SetActiveAttribute(dsAttributes->GetNumberOfArrays()-1,attributeType);
    }
  }


  // Use tolerance as a function of size of source data
  //
  tol2 = source->GetLength();
  tol2 = tol2 ? tol2*tol2 / 1000.0 : 0.001;
  cout<<"tol2="<<tol2<<endl;
  // Loop over all input points, interpolating source data
  //
  int abort=0;

  // Need to use source to create a cellIt since this class is virtual
  vtkGenericCellIterator *cellIt = source->NewCellIterator();

  vtkIdType progressInterval=numPts/20 + 1;
  for (ptId=0; ptId < numPts && !abort; ptId++)
  {
    if ( !(ptId % progressInterval) )
    {
      this->UpdateProgress(static_cast<double>(ptId)/numPts);
      abort = GetAbortExecute();
    }

    // Get the xyz coordinate of the point in the input dataset
    input->GetPoint(ptId, x);

    // Find the cell that contains xyz and get it
    if(source->FindCell(x,cellIt,tol2,subId,pcoords))
    {
      vtkGenericAdaptorCell *cellProbe = cellIt->GetCell();

      // for each cell-centered attribute: copy the value
      for(int attrib = 0; attrib<c; ++attrib)
      {
        if(attributes->GetAttribute(attrib)->GetCentering()==vtkCellCentered)
        {
          vtkDataArray *array=outputCD->GetArray(attributes->GetAttribute(attrib)->GetName());
          double *values=attributes->GetAttribute(attrib)->GetTuple(cellProbe);
          array->InsertNextTuple(values);
        }
      }

      // for each point-centered attribute: interpolate the value
      int j=0;
      for(int attribute_idx = 0; attribute_idx<c; ++attribute_idx)
      {
        vtkGenericAttribute *a = attributes->GetAttribute(attribute_idx);
        if(a->GetCentering()==vtkPointCentered)
        {
          cellProbe->InterpolateTuple(a,pcoords,tuples);
          outputPD->GetArray(j)->InsertTuple(ptId,tuples);
          ++j;
        }
      }
      this->ValidPoints->InsertNextValue(ptId);
    }
    else
    {
      outputPD->NullPoint(ptId);
    }
  }
  cellIt->Delete();
  delete[] tuples;

  return 1;
}


//----------------------------------------------------------------------------
void vtkGenericProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkGenericDataSet *source = this->GetSource();

  this->Superclass::PrintSelf(os,indent);

  os << indent << "Source: " << source << "\n";
  os << indent << "ValidPoints: " << this->ValidPoints << "\n";
}

//----------------------------------------------------------------------------
int vtkGenericProbeFilter::FillInputPortInformation(
  int port,
  vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  if(port==1)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGenericDataSet");
  }
  else
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  }
  return 1;
}
