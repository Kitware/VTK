/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDotProductSimilarity.cxx

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
#include "vtkDotProductSimilarity.h"
#include "vtkDenseArray.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include <algorithm>
#include <map>
#include <stdexcept>

// threshold_multimap
// This strange little fellow is used by the vtkDotProductSimilarity
// implementation.  It provides the interface
// of a std::multimap, but it enforces several constraints on its contents:
//
// There is an upper-limit on the number of values stored in the container.
// There is a lower threshold on key-values stored in the container.
// The key threshold can be overridden by specifying a lower-limit on the
// number of values stored in the container.

template<typename KeyT, typename ValueT>
class threshold_multimap :
  public std::multimap<KeyT, ValueT, std::less<KeyT> >
{
  typedef std::multimap<KeyT, ValueT, std::less<KeyT> > container_t;

public:
  threshold_multimap(KeyT minimum_threshold, size_t minimum_count, size_t maximum_count) :
    MinimumThreshold(minimum_threshold),
    MinimumCount(std::max(static_cast<size_t>(0), minimum_count)),
    MaximumCount(std::max(static_cast<size_t>(0), maximum_count))
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

vtkStandardNewMacro(vtkDotProductSimilarity);

// ----------------------------------------------------------------------

vtkDotProductSimilarity::vtkDotProductSimilarity() :
  VectorDimension(1),
  MinimumThreshold(1),
  MinimumCount(1),
  MaximumCount(10),
  UpperDiagonal(true),
  Diagonal(false),
  LowerDiagonal(false),
  FirstSecond(true),
  SecondFirst(true)
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(1);
}

// ----------------------------------------------------------------------

vtkDotProductSimilarity::~vtkDotProductSimilarity()
{
}

// ----------------------------------------------------------------------

void vtkDotProductSimilarity::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "VectorDimension: " << this->VectorDimension << endl;
  os << indent << "MinimumThreshold: " << this->MinimumThreshold << endl;
  os << indent << "MinimumCount: " << this->MinimumCount << endl;
  os << indent << "MaximumCount: " << this->MaximumCount << endl;
  os << indent << "UpperDiagonal: " << this->UpperDiagonal << endl;
  os << indent << "Diagonal: " << this->Diagonal << endl;
  os << indent << "LowerDiagonal: " << this->LowerDiagonal << endl;
  os << indent << "FirstSecond: " << this->FirstSecond << endl;
  os << indent << "SecondFirst: " << this->SecondFirst << endl;
}

int vtkDotProductSimilarity::FillInputPortInformation(int port, vtkInformation* info)
{
  switch(port)
  {
    case 0:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkArrayData");
      return 1;
    case 1:
      info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkArrayData");
      return 1;
  }

  return 0;
}

// ----------------------------------------------------------------------

static double DotProduct(
  vtkDenseArray<double>* input_a,
  vtkDenseArray<double>* input_b,
  const vtkIdType vector_a,
  const vtkIdType vector_b,
  const vtkIdType vector_dimension,
  const vtkIdType component_dimension,
  const vtkArrayRange range_a,
  const vtkArrayRange range_b)
{
  vtkArrayCoordinates coordinates_a(0, 0);
  vtkArrayCoordinates coordinates_b(0, 0);

  coordinates_a[vector_dimension] = vector_a;
  coordinates_b[vector_dimension] = vector_b;

  double dot_product = 0.0;
  for(vtkIdType component = 0; component != range_a.GetSize(); ++component)
  {
    coordinates_a[component_dimension] = component + range_a.GetBegin();
    coordinates_b[component_dimension] = component + range_b.GetBegin();
    dot_product += input_a->GetValue(coordinates_a) * input_b->GetValue(coordinates_b);
  }
  return dot_product;
}

int vtkDotProductSimilarity::RequestData(
  vtkInformation*,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  try
  {
    // Enforce our preconditions ...
    vtkArrayData* const input_a = vtkArrayData::GetData(inputVector[0]);
    if(!input_a)
      throw std::runtime_error("Missing array data input on input port 0.");
    if(input_a->GetNumberOfArrays() != 1)
      throw std::runtime_error("Array data on input port 0 must contain exactly one array.");
    vtkDenseArray<double>* const input_array_a = vtkDenseArray<double>::SafeDownCast(
      input_a->GetArray(static_cast<vtkIdType>(0)));
    if(!input_array_a)
      throw std::runtime_error("Array on input port 0 must be a vtkDenseArray<double>.");
    if(input_array_a->GetDimensions() != 2)
      throw std::runtime_error("Array on input port 0 must be a matrix.");

    vtkArrayData* const input_b = vtkArrayData::GetData(inputVector[1]);
    vtkDenseArray<double>* input_array_b = 0;
    if(input_b)
    {
      if(input_b->GetNumberOfArrays() != 1)
        throw std::runtime_error("Array data on input port 1 must contain exactly one array.");
      input_array_b = vtkDenseArray<double>::SafeDownCast(
        input_b->GetArray(static_cast<vtkIdType>(0)));
      if(!input_array_b)
        throw std::runtime_error("Array on input port 1 must be a vtkDenseArray<double>.");
      if(input_array_b->GetDimensions() != 2)
        throw std::runtime_error("Array on input port 1 must be a matrix.");
    }

    const vtkIdType vector_dimension = this->VectorDimension;
    if(vector_dimension != 0 && vector_dimension != 1)
      throw std::runtime_error("VectorDimension must be zero or one.");

    const vtkIdType component_dimension = 1 - vector_dimension;

    const vtkArrayRange vectors_a = input_array_a->GetExtent(vector_dimension);
    const vtkArrayRange components_a = input_array_a->GetExtent(component_dimension);

    const vtkArrayRange vectors_b = input_array_b ? input_array_b->GetExtent(vector_dimension) : vtkArrayRange();
    const vtkArrayRange components_b = input_array_b ? input_array_b->GetExtent(component_dimension) : vtkArrayRange();

    if(input_array_b && (components_a.GetSize() != components_b.GetSize()))
      throw std::runtime_error("Input array vector lengths must match.");

    // Get output arrays ...
    vtkTable* const output = vtkTable::GetData(outputVector);

    vtkIdTypeArray* const source_array = vtkIdTypeArray::New();
    source_array->SetName("source");

    vtkIdTypeArray* const target_array = vtkIdTypeArray::New();
    target_array->SetName("target");

    vtkDoubleArray* const similarity_array = vtkDoubleArray::New();
    similarity_array->SetName("similarity");

    // Okay let outside world know that I'm starting
    double progress = 0;
    this->InvokeEvent(vtkCommand::ProgressEvent, &progress);

    typedef threshold_multimap<double, vtkIdType> similarities_t;
    if(input_array_b)
    {
      // Compare the first matrix with the second matrix ...
      if(this->FirstSecond)
      {
        for(vtkIdType vector_a = vectors_a.GetBegin(); vector_a != vectors_a.GetEnd(); ++vector_a)
        {
          similarities_t similarities(this->MinimumThreshold, this->MinimumCount, this->MaximumCount);

          for(vtkIdType vector_b = vectors_b.GetBegin(); vector_b != vectors_b.GetEnd(); ++vector_b)
          {
            // Can't use std::make_pair - see http://sahajtechstyle.blogspot.com/2007/11/whats-wrong-with-sun-studio-c.html
            similarities.insert(std::pair<const double, vtkIdType>(DotProduct(input_array_a, input_array_b, vector_a, vector_b, vector_dimension, component_dimension, components_a, components_b), vector_b));
          }

          for(similarities_t::const_iterator similarity = similarities.begin(); similarity != similarities.end(); ++similarity)
          {
            source_array->InsertNextValue(vector_a);
            target_array->InsertNextValue(similarity->second);
            similarity_array->InsertNextValue(similarity->first);
          }
        }
      }
      // Compare the second matrix with the first matrix ...
      if(this->SecondFirst)
      {
        for(vtkIdType vector_b = vectors_b.GetBegin(); vector_b != vectors_b.GetEnd(); ++vector_b)
        {
          similarities_t similarities(this->MinimumThreshold, this->MinimumCount, this->MaximumCount);

          for(vtkIdType vector_a = vectors_a.GetBegin(); vector_a != vectors_a.GetEnd(); ++vector_a)
          {
            // Can't use std::make_pair - see http://sahajtechstyle.blogspot.com/2007/11/whats-wrong-with-sun-studio-c.html
            similarities.insert(std::pair<const double, vtkIdType>(DotProduct(input_array_b, input_array_a, vector_b, vector_a, vector_dimension, component_dimension, components_b, components_a), vector_a));
          }

          for(similarities_t::const_iterator similarity = similarities.begin(); similarity != similarities.end(); ++similarity)
          {
            source_array->InsertNextValue(vector_b);
            target_array->InsertNextValue(similarity->second);
            similarity_array->InsertNextValue(similarity->first);
          }
        }
      }
    }
    // Compare the one matrix with itself ...
    else
    {
      for(vtkIdType vector_a = vectors_a.GetBegin(); vector_a != vectors_a.GetEnd(); ++vector_a)
      {
        similarities_t similarities(this->MinimumThreshold, this->MinimumCount, this->MaximumCount);

        for(vtkIdType vector_b = vectors_a.GetBegin(); vector_b != vectors_a.GetEnd(); ++vector_b)
        {
          if((vector_b > vector_a) && !this->UpperDiagonal)
            continue;

          if((vector_b == vector_a) && !this->Diagonal)
            continue;

          if((vector_b < vector_a) && !this->LowerDiagonal)
            continue;

          // Can't use std::make_pair - see http://sahajtechstyle.blogspot.com/2007/11/whats-wrong-with-sun-studio-c.html
          similarities.insert(std::pair<const double, vtkIdType>(DotProduct(input_array_a, input_array_a, vector_a, vector_b, vector_dimension, component_dimension, components_a, components_a), vector_b));
        }

        for(similarities_t::const_iterator similarity = similarities.begin(); similarity != similarities.end(); ++similarity)
        {
          source_array->InsertNextValue(vector_a);
          target_array->InsertNextValue(similarity->second);
          similarity_array->InsertNextValue(similarity->first);
        }
      }
    }

    output->AddColumn(source_array);
    output->AddColumn(target_array);
    output->AddColumn(similarity_array);

    source_array->Delete();
    target_array->Delete();
    similarity_array->Delete();

    return 1;
  }
  catch(std::exception& e)
  {
    vtkErrorMacro(<< "unhandled exception: " << e.what());
    return 0;
  }
  catch(...)
  {
    vtkErrorMacro(<< "unknown exception");
    return 0;
  }
}

