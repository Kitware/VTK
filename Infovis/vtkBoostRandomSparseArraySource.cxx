/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBoostRandomSparseArraySource.cxx
  
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

#include "vtkBoostRandomSparseArraySource.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSparseArray.h"

#include <boost/random.hpp>

// ----------------------------------------------------------------------

vtkStandardNewMacro(vtkBoostRandomSparseArraySource);

// ----------------------------------------------------------------------

vtkBoostRandomSparseArraySource::vtkBoostRandomSparseArraySource() :
  Extents(2, 2),
  ElementProbabilitySeed(123),
  ElementProbability(0.5),
  ElementValueSeed(456),
  MinValue(0.0),
  MaxValue(1.0)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

// ----------------------------------------------------------------------

vtkBoostRandomSparseArraySource::~vtkBoostRandomSparseArraySource()
{
}

// ----------------------------------------------------------------------

void vtkBoostRandomSparseArraySource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Extents: " << this->Extents << endl;
  os << indent << "ElementProbabilitySeed: " << this->ElementProbabilitySeed << endl;
  os << indent << "ElementProbability: " << this->ElementProbability << endl;
  os << indent << "ElementValueSeed: " << this->ElementValueSeed << endl;
  os << indent << "MinValue: " << this->MinValue << endl;
  os << indent << "MaxValue: " << this->MaxValue << endl;
}

void vtkBoostRandomSparseArraySource::SetExtents(const vtkArrayExtents& extents)
{
  if(extents == this->Extents)
    return;

  this->Extents = extents;
  this->Modified();
}

vtkArrayExtents vtkBoostRandomSparseArraySource::GetExtents()
{
  return this->Extents;
}

// ----------------------------------------------------------------------

int vtkBoostRandomSparseArraySource::RequestData(
  vtkInformation*, 
  vtkInformationVector**, 
  vtkInformationVector* outputVector)
{
  boost::mt19937 pattern_generator(static_cast<boost::uint32_t>(this->ElementProbabilitySeed));
  boost::bernoulli_distribution<> pattern_distribution(this->ElementProbability);
  boost::variate_generator<boost::mt19937&, boost::bernoulli_distribution<> > pattern(pattern_generator, pattern_distribution);

  boost::mt19937 value_generator(static_cast<boost::uint32_t>(this->ElementValueSeed));
  boost::uniform_real<> value_distribution(this->MinValue, this->MaxValue);
  boost::variate_generator<boost::mt19937&, boost::uniform_real<> > values(value_generator, value_distribution);

  vtkSparseArray<double>* const array = vtkSparseArray<double>::New();
  array->Resize(this->Extents);

  vtkArrayCoordinates coordinates;
  for(vtkIdType n = 0; n != this->Extents.GetSize(); ++n)
    {
    this->Extents.GetRightToLeftCoordinatesN(n, coordinates);

    // Although it seems wasteful, we calculate a value for every element in the array
    // so the results stay consistent as the ElementProbability varies
    const double value = values();
    if(pattern())
      {
      array->AddValue(coordinates, value);
      }
    }

  vtkArrayData* const output = vtkArrayData::GetData(outputVector);
  output->ClearArrays();
  output->AddArray(array);
  array->Delete();

  return 1;
}

