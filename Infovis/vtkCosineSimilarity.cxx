/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCosineSimilarity.cxx
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkArrayData.h"
#include "vtkCommand.h"
#include "vtkCosineSimilarity.h"
#include "vtkDenseArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include <vtksys/stl/algorithm>
#include <vtkstd/map>

// threshold_multimap
// This strange little fellow is used by the vtkCosineSimilarity
// implementation.  It provides the interface
// of a vtkstd::multimap, but it enforces several constraints on its contents:
//
// There is an upper-limit on the number of values stored in the container.
// There is a lower threshold on key-values stored in the container.
// The key threshold can be overridden by specifying a lower-limit on the
// number of values stored in the container.

template<typename KeyT, typename ValueT>
class threshold_multimap :
  public vtkstd::multimap<KeyT, ValueT, vtkstd::less<KeyT> >
{
  typedef vtkstd::multimap<KeyT, ValueT, vtkstd::less<KeyT> > container_t;

public:
  threshold_multimap(KeyT minimum_threshold, size_t minimum_count, size_t maximum_count) :
    MinimumThreshold(minimum_threshold),
    MinimumCount(vtkstd::max(static_cast<size_t>(0), minimum_count)),
    MaximumCount(vtkstd::max(static_cast<size_t>(0), maximum_count))
  {
  }

  void insert(const typename container_t::value_type& value)
  {
    // Insert the value into the container ...
    container_t::insert(value);

    // Prune small values down to our minimum size ...
    while((this->size() > this->MinimumCount) && (this->begin()->first < this->MinimumThreshold))
      this->erase(this->begin());
      
    // Prune small values down to our maximum size ...
    while(this->size() > this->MaximumCount)
      this->erase(this->begin());
  }

private:
  typename container_t::iterator insert(typename container_t::iterator where, const typename container_t::value_type& value);
  template<class InIt>
  void insert(InIt first, InIt last);

  KeyT MinimumThreshold;
  size_t MinimumCount;
  size_t MaximumCount;
};

// ----------------------------------------------------------------------

vtkCxxRevisionMacro(vtkCosineSimilarity, "1.4");
vtkStandardNewMacro(vtkCosineSimilarity);

// ----------------------------------------------------------------------

vtkCosineSimilarity::vtkCosineSimilarity() :
  VectorDimension(1),
  MinimumThreshold(1),
  MinimumCount(1),
  MaximumCount(10)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

// ----------------------------------------------------------------------

vtkCosineSimilarity::~vtkCosineSimilarity()
{
}

// ----------------------------------------------------------------------

void vtkCosineSimilarity::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VectorDimension: " << this->VectorDimension << endl;
  os << indent << "MinimumThreshold: " << this->MinimumThreshold << endl;
  os << indent << "MinimumCount: " << this->MinimumCount << endl;
  os << indent << "MaximumCount: " << this->MaximumCount << endl;
}

int vtkCosineSimilarity::FillInputPortInformation(int port, vtkInformation* info)
{
  switch(port)
    {
    case 0:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkArrayData");
      return 1;
    }

  return 0;
}

// ----------------------------------------------------------------------

int vtkCosineSimilarity::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  // Get input arrays ...
  vtkArrayData* const input = vtkArrayData::GetData(inputVector[0]);
  if(input->GetNumberOfArrays() != 1)
    {
    vtkErrorMacro(<< "vtkCosineSimilarity requires a vtkArrayData with exactly one array as input.");
    return 0;
    }
    
  vtkDenseArray<double>* const input_array = vtkDenseArray<double>::SafeDownCast(input->GetArray(0));
  if(!input_array)
    {
    vtkErrorMacro(<< "vtkCosineSimilarity requires a vtkDenseArray<double> as input.");
    return 0;
    }
  if(input_array->GetExtents().GetDimensions() != 2)
    {
    vtkErrorMacro(<< "vtkCosineSimilarity requires a matrix as input.");
    return 0;
    }

  // Get output arrays ...
  vtkTable* const output = vtkTable::GetData(outputVector);
 
  vtkIdTypeArray* const source_array = vtkIdTypeArray::New();
  source_array->SetName("source");
  
  vtkIdTypeArray* const target_array = vtkIdTypeArray::New();
  target_array->SetName("target");

  vtkDoubleArray* const similarity_array = vtkDoubleArray::New();
  similarity_array->SetName("similarity");

  vtkArrayCoordinates coordinates1(0, 0);
  vtkArrayCoordinates coordinates2(0, 0);

  // Okay let outside world know that I'm starting
  double progress = 0;
  this->InvokeEvent(vtkCommand::ProgressEvent, &progress);

  const int vector_dimension = vtkstd::min(1, vtkstd::max(0, this->VectorDimension));
  const int component_dimension = 1 - vector_dimension;

  const vtkIdType vector_count = input_array->GetExtents()[vector_dimension];
  const vtkIdType component_count = input_array->GetExtents()[component_dimension];

  // for each pair of vectors in the matrix ...
  for(vtkIdType vector1 = 0; vector1 < vector_count; ++vector1)
    {
    coordinates1[vector_dimension] = vector1;

    // Keep a sorted list of similarities as-we-go ...
    typedef threshold_multimap<double, vtkIdType> similarities_t;
    similarities_t similarities(this->MinimumThreshold, this->MinimumCount, this->MaximumCount);
    for(vtkIdType vector2 = vector1 + 1; vector2 < vector_count; ++vector2)
      {
      coordinates2[vector_dimension] = vector2;

      // Compute the dot-product of the two columns ...
      double dot_product = 0.0;
      for(vtkIdType component = 0; component < component_count; ++component)
        {
        coordinates1[component_dimension] = component;
        coordinates2[component_dimension] = component;
        dot_product += input_array->GetValue(coordinates1) * input_array->GetValue(coordinates2);
        }
#ifdef _RWSTD_NO_MEMBER_TEMPLATES
      // Deal with Sun Studio old libCstd.
      // http://sahajtechstyle.blogspot.com/2007/11/whats-wrong-with-sun-studio-c.html
      similarities.insert(vtkstd::pair<const double,vtkIdType>(dot_product, vector2));
#else
      similarities.insert(vtkstd::make_pair(dot_product, vector2));
#endif
      }
      
    // Now that we have our sorted list of similarities, store the results ...
    for(similarities_t::const_iterator similarity = similarities.begin(); similarity != similarities.end(); ++similarity)
      {
      source_array->InsertNextValue(vector1);
      target_array->InsertNextValue(similarity->second);
      similarity_array->InsertNextValue(similarity->first);
      }

    progress = static_cast<double>(vector1) / static_cast<double>(vector_count);
    this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
    }

  output->AddColumn(source_array);
  output->AddColumn(target_array);
  output->AddColumn(similarity_array);

  source_array->Delete();
  target_array->Delete();
  similarity_array->Delete();

  return 1;
}

