/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBandFiltering.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkBandFiltering.h"

#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkTable.h"
#include "vtkTableFFT.h"

#include <string>

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkBandFiltering);

//----------------------------------------------------------------------------
void vtkBandFiltering::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ApplyFFT: " << this->ApplyFFT << std::endl;
  os << indent << "DefaultSamplingRate: " << this->DefaultSamplingRate << std::endl;
  os << indent << "WindowType: " << this->WindowType << std::endl;
  os << indent << "BandFilteringMode: " << this->BandFilteringMode << std::endl;
  os << indent << "ReferenceValue: " << this->ReferenceValue << std::endl;
  os << indent << "OutputInDecibel: " << this->OutputInDecibel << std::endl;
}

//----------------------------------------------------------------------------
int vtkBandFiltering::GenerateOctaveBands(double fmin, double fmax,
  std::vector<double>& lowerFrequencies, std::vector<double>& centerFrequencies,
  std::vector<double>& upperFrequencies)
{
  double ratioFrequency = std::pow(10, 3.0 / 10.0);
  double baseFrequency = 1000.0;
  double bandWidth = 1.0;
  if (this->BandFilteringMode == vtkBandFiltering::THIRD_OCTAVE)
  {
    bandWidth = 3;
  }

  double lowestBand =
    std::floor(bandWidth * std::log10(fmin / baseFrequency) / std::log10(ratioFrequency) + 0.5) + 1;
  double highestBand =
    std::floor(bandWidth * std::log10(fmax / baseFrequency) / std::log10(ratioFrequency) + 0.5) + 1;
  if (highestBand < lowestBand)
  {
    return 0;
  }

  std::vector<double> bandIndices;
  for (double x = lowestBand; x < highestBand; x++)
  {
    bandIndices.push_back(x);
  }

  for (int i = 0; i < bandIndices.size(); i++)
  {
    lowerFrequencies.push_back(
      baseFrequency * std::pow(ratioFrequency, (bandIndices[i] - 0.5) / bandWidth));
    centerFrequencies.push_back(
      baseFrequency * std::pow(ratioFrequency, (bandIndices[i]) / bandWidth));
    upperFrequencies.push_back(
      baseFrequency * std::pow(ratioFrequency, (bandIndices[i] + 0.5) / bandWidth));
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkBandFiltering::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  auto input = vtkTable::GetData(inputVector[0]);
  auto output = vtkTable::GetData(outputVector);
  if (!input || !output)
  {
    return 0;
  }
  if (input->GetNumberOfColumns() <= 0)
  {
    return 1;
  }

  vtkNew<vtkTable> processTable;
  processTable->ShallowCopy(input);

  // Check if the input have consistent column
  bool hasTimeColumn = false;
  bool couldOptimizeForRealInput = false;
  bool findColumnWithMultipleComponents = false;
  for (vtkIdType col = 0; col < processTable->GetNumberOfColumns(); col++)
  {
    auto* column = processTable->GetColumn(col);
    if (column)
    {
      continue;
    }
    if (vtksys::SystemTools::Strucmp(column->GetName(), "time") == 0)
    {
      hasTimeColumn = true;
      continue;
    }
    else if (column->GetNumberOfComponents() == 1)
    {
      couldOptimizeForRealInput = true;
    }

    if (column->GetNumberOfComponents() != 1)
    {
      findColumnWithMultipleComponents = true;
    }
  }

  if (couldOptimizeForRealInput && findColumnWithMultipleComponents)
  {
    vtkErrorMacro("The input contains column with differents number of components.");
    return 0;
  }
  if (this->ApplyFFT)
  {
    vtkNew<vtkTableFFT> tableFFT;
    tableFFT->SetInputData(processTable);
    tableFFT->SetOptimizeForRealInput(couldOptimizeForRealInput);
    tableFFT->CreateFrequencyColumnOn();
    tableFFT->SetWindowingFunction(this->WindowType);
    if (!hasTimeColumn)
    {
      tableFFT->SetDefaultSampleRate(this->DefaultSamplingRate);
    }

    tableFFT->Update();
    processTable->ShallowCopy(tableFFT->GetOutput());
    if (!processTable)
    {
      vtkErrorMacro("Failed to apply an FFT with vtkTableFFT filter.");
      return 0;
    }
  }

  // Generate each frequency bands
  std::vector<double> lowerFrequencies;
  std::vector<double> centerFrequencies;
  std::vector<double> upperFrequencies;
  double fmin = VTK_DOUBLE_MAX;
  double fmax = VTK_DOUBLE_MIN;
  auto* frequencies = vtkDoubleArray::SafeDownCast(processTable->GetColumnByName("Frequency"));
  if (frequencies)
  {
    for (vtkIdType i = 0; i < frequencies->GetNumberOfValues() / 2; i++)
    {
      if (frequencies->GetValue(i) == 0)
      {
        continue;
      }

      if (frequencies->GetValue(i) < fmin)
      {
        fmin = frequencies->GetValue(i);
      }
      if (frequencies->GetValue(i) > fmax)
      {
        fmax = frequencies->GetValue(i);
      }
    }
  }
  else
  {
    // Without frequency column generate by vtkTableFFT, we used by default the standard sound
    // pressure value used for these bands
    fmin = 16.0;
    if (this->BandFilteringMode == vtkBandFiltering::OCTAVE)
    {
      fmax = 16000;
    }
    else
    {
      fmax = 20000.0;
    }
  }
  if (!this->GenerateOctaveBands(fmin, fmax, lowerFrequencies, centerFrequencies, upperFrequencies))
  {
    vtkErrorMacro("Can't generate octave bands with the sample rate defined or the time column.");
    return 0;
  }

  vtkIdType maxIdFreq = frequencies->GetNumberOfValues();
  // Skip the second half of the fft because it is mirrored
  if (!couldOptimizeForRealInput)
  {
    maxIdFreq = frequencies->GetNumberOfValues() / 2 + 1;
  }

  vtkNew<vtkDoubleArray> xAxis;
  xAxis->SetNumberOfComponents(1);
  for (vtkIdType i = 0; i < centerFrequencies.size(); i++)
  {
    xAxis->InsertNextTuple1(centerFrequencies[i]);
  }
  xAxis->SetName("Frequency");
  output->AddColumn(xAxis);

  for (vtkIdType colID = 0; colID < processTable->GetNumberOfColumns(); colID++)
  {

    vtkDoubleArray* fftValues = vtkDoubleArray::SafeDownCast(processTable->GetColumn(colID));
    if (!fftValues)
    {
      continue;
    }
    if (vtksys::SystemTools::Strucmp(fftValues->GetName(), "Frequency") == 0)
    {
      continue;
    }

    // vtkTableFFT should to produce complex values
    if (fftValues->GetNumberOfComponents() != 2)
    {
      vtkErrorMacro("Processed column by a FFT isn't an array of complex value.");
      return 0;
    }

    // Compute in amplitudes from the complex array
    vtkNew<vtkDoubleArray> amplitudes;
    amplitudes->SetNumberOfComponents(1);
    amplitudes->SetNumberOfTuples(1 + fftValues->GetNumberOfTuples() / 2);
    for (vtkIdType i = 0; i < amplitudes->GetNumberOfTuples(); i++)
    {
      double* complex = fftValues->GetTuple2(i);
      double magnitude = vtkMath::Norm2D(complex);
      amplitudes->InsertValue(i, magnitude);
    }
    if (this->OutputInDecibel)
    {
      if (this->ReferenceValue == VTK_DBL_MIN)
      {
        vtkWarningMacro("Can't convert to decibel without an reference value setted.");
      }
      else
      {
        vtkSMPTools::For(0, amplitudes->GetNumberOfTuples(), [&](vtkIdType begin, vtkIdType end) {
          for (vtkIdType id = begin; id < end; id++)
          {
            double value = amplitudes->GetValue(id);
            amplitudes->SetValue(id, 20.0 * std::log10(value / ReferenceValue));
          }
        });
      }
    }

    vtkNew<vtkDoubleArray> octaveBand;
    octaveBand->SetNumberOfComponents(1);
    octaveBand->SetName(fftValues->GetName());

    vtkIdType currentIdFreq = 0;
    double currentFreq = frequencies->GetValue(currentIdFreq);
    while (currentFreq < lowerFrequencies[0] && currentIdFreq < maxIdFreq)
    {
      currentIdFreq++;
      currentFreq = frequencies->GetValue(currentIdFreq);
    }

    // Each octave band will just be the mean of all amplitude in the Hz range
    int numberOfValueInBand = 0;
    double acc = 0.0;
    vtkSMPTools::For(0, centerFrequencies.size(), [&](vtkIdType begin, vtkIdType end) {
      for (vtkIdType i = begin; i < end; i++)
      {
        while (currentIdFreq < maxIdFreq && currentFreq >= lowerFrequencies[i] &&
          currentFreq < upperFrequencies[i])
        {
          acc += amplitudes->GetValue(currentIdFreq);
          numberOfValueInBand++;
          currentIdFreq++;
          currentFreq = frequencies->GetValue(currentIdFreq);
        }

        if (numberOfValueInBand == 0)
        {
          octaveBand->InsertNextTuple1(0);
        }
        else
        {
          acc = acc / numberOfValueInBand;
          octaveBand->InsertNextTuple1(acc);
        }

        acc = 0.0;
        numberOfValueInBand = 0;
      }
    });

    output->AddColumn(octaveBand);
  }

  return 1;
}
