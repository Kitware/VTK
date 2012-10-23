/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeCutter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkImplicitFunction.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkAppendPolyData.h"

#include <math.h>
#include "vtkObjectFactory.h"
#include "vtkCompositeDataIterator.h"
#include "vtkSmartPointer.h"
#include <assert.h>

vtkStandardNewMacro(vtkCompositeCutter);

#ifdef DEBUGME
#define PRINT(x) {cout<<x<<endl;}
#else
#define PRINT(x)
#endif
namespace
{
  inline double Sign(double a)
  {
    return a==0.0? 0.0 : (a<0.0? -1.0 : 1.0);
  }
  inline bool IntersectBox(vtkImplicitFunction* func, double bounds[6], double value)
  {
    double fVal[8];
    fVal[0] = func->EvaluateFunction(bounds[0],bounds[2],bounds[4]);
    fVal[1] = func->EvaluateFunction(bounds[0],bounds[2],bounds[5]);
    fVal[2] = func->EvaluateFunction(bounds[0],bounds[3],bounds[4]);
    fVal[3] = func->EvaluateFunction(bounds[0],bounds[3],bounds[5]);
    fVal[4] = func->EvaluateFunction(bounds[1],bounds[2],bounds[4]);
    fVal[5] = func->EvaluateFunction(bounds[1],bounds[2],bounds[5]);
    fVal[6] = func->EvaluateFunction(bounds[1],bounds[3],bounds[4]);
    fVal[7] = func->EvaluateFunction(bounds[1],bounds[3],bounds[5]);

    double sign0  = Sign(fVal[0]-value);
    for (int i=1; i<8;i++)
      {
      if (Sign(fVal[i]-value) != sign0)
        {
        //this corner is on different side than first, piece
        //intersects and cannot be rejected
        return true;
        }
      }
    return false;
  }
};

vtkCompositeCutter::vtkCompositeCutter(vtkImplicitFunction *cf):vtkCutter(cf)
{

}

//----------------------------------------------------------------------------
vtkCompositeCutter::~vtkCompositeCutter()
{

}

int vtkCompositeCutter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

int vtkCompositeCutter::RequestUpdateExtent(vtkInformation *, vtkInformationVector **inputVector, vtkInformationVector *)
{
  vtkDebugMacro("Request-Update");

  vtkInformation * inInfo = inputVector[0]->GetInformationObject(0);

  for (int c=0; c < this->ContourValues->GetNumberOfContours(); c++)
    {
    vtkDebugMacro("Contours "<<this->ContourValues->GetValue(c));
    }


  // Check if metadata are passed downstream
  if(inInfo->Has(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA() ) )
    {
    std::vector<int> intersected;

    vtkCompositeDataSet * meta= vtkCompositeDataSet::SafeDownCast(inInfo->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA()));
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(meta->NewIterator());
    iter->SetSkipEmptyNodes(false);
    for(iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      double* bb = iter->GetCurrentMetaData()->Get(vtkDataObject::BOUNDING_BOX());
      for (int c=0; c < this->ContourValues->GetNumberOfContours(); c++)
        {
        if(IntersectBox(this->GetCutFunction(),bb,this->ContourValues->GetValue(c)))
          {
          intersected.push_back(iter->GetCurrentFlatIndex());
          break;
          }
        }
      }
    PRINT("Cutter demand "<<intersected.size()<<" blocks");
    inInfo->Set(vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(), &intersected[0], static_cast<int>(intersected.size()));
    }
  return 1;
}

int vtkCompositeCutter::RequestData(vtkInformation *request,
                                    vtkInformationVector ** inputVector,
                                    vtkInformationVector * outputVector)
{
  vtkInformation * inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation * outInfo = outputVector->GetInformationObject(0);
  vtkSmartPointer<vtkCompositeDataSet> inData = vtkCompositeDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if(!inData)
    {
    return Superclass::RequestData(request,inputVector,outputVector);
    }

  vtkSmartPointer<vtkCompositeDataIterator> itr;
  itr.TakeReference(inData->NewIterator());
  itr->SetSkipEmptyNodes(true);

  vtkNew<vtkAppendPolyData> append;
  int numObjects(0);
  itr->GoToFirstItem();
  while(!itr->IsDoneWithTraversal())
    {
    vtkDataSet* data = vtkDataSet::SafeDownCast(itr->GetCurrentDataObject());
    assert(data);
    inInfo->Set(vtkDataObject::DATA_OBJECT(),data);
    vtkNew<vtkPolyData> out;
    outInfo->Set(vtkDataObject::DATA_OBJECT(),out.GetPointer());
    this->Superclass::RequestData(request,inputVector,outputVector);
    append->AddInputData(out.GetPointer());
    numObjects++;
    itr->GoToNextItem();
    }
  append->Update();

  vtkPolyData* appoutput = append->GetOutput();
  inInfo->Set(vtkDataObject::DATA_OBJECT(), inData); //set it back to the original input
  outInfo->Set(vtkDataObject::DATA_OBJECT(), appoutput);
  return 1;
}

void vtkCompositeCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
