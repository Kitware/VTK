// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkEqualizerFilter.h"

#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFFT.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkVector.h"

#include <vtksys/SystemTools.hxx>

#include <sstream>
#include <string>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class vtkEqualizerFilter::vtkInternal
{
public:
  //------------------------------------------------------------------------------
  void ClearPoints() { this->Points.clear(); }

  //------------------------------------------------------------------------------
  void SetTable(vtkTable* input)
  {
    if (input != this->TableSrc)
    {
      this->TableSrc = input;
      this->OriginalSize = 0;
      this->SpectrumSize = 0;
      this->Spectrums.clear();
    }
  }

  //------------------------------------------------------------------------------
  vtkIdType GetHalfSize() const { return (OriginalSize + 1) / 2; }

  //------------------------------------------------------------------------------
  vtkIdType GetHalfSpectrumSize() const { return (SpectrumSize + 1) / 2; }

  //------------------------------------------------------------------------------
  const std::vector<vtkFFT::ComplexNumber>& GetSpectrum(vtkDataArray* array)
  {
    auto it = this->Spectrums.find(array->GetName());
    if (it == this->Spectrums.end())
    {
      const vtkIdType tuplesCount = array->GetNumberOfTuples();
      std::vector<double> values(tuplesCount);
      for (vtkIdType tupleId = 0; tupleId < tuplesCount; ++tupleId)
        values[tupleId] = array->GetTuple1(tupleId);

      this->Spectrums[array->GetName()] = vtkFFT::RFft(values);
      this->SpectrumSize = this->Spectrums[array->GetName()].size();
    }

    return this->Spectrums.at(array->GetName());
  }

  //------------------------------------------------------------------------------
  const std::vector<double>& GetNormalizedSpectrum(vtkDataArray* array)
  {
    auto it = this->NormalizedSpectrums.find(array->GetName());
    if (it == this->NormalizedSpectrums.end())
    {
      auto spectrum = this->GetSpectrum(array);
      auto maxModule = std::numeric_limits<double>::min();
      std::vector<double> modules;
      for (vtkIdType spectrumId = 0; spectrumId < this->GetHalfSpectrumSize(); ++spectrumId)
      {
        auto module = vtkFFT::Abs(spectrum[spectrumId]);
        maxModule = (maxModule > module ? maxModule : module);
        modules.push_back(module);
      }

      std::vector<double> normSpectrum(modules);
      std::for_each(normSpectrum.begin(), normSpectrum.end(),
        [maxModule](double& module) { module /= maxModule; });

      this->NormalizedSpectrums[array->GetName()] = std::move(normSpectrum);
    }

    return this->NormalizedSpectrums.at(array->GetName());
  }

  //------------------------------------------------------------------------------
  std::vector<std::pair<int, double>> GetModifiers(int samplingFrequency) const
  {
    std::vector<std::pair<int, double>> result;
    if (this->Points.size() < 2)
    {
      return result;
    }

    for (size_t i = 0; i < this->Points.size() - 1; i++)
    {
      vtkVector2f p1, p2;
      p1 = this->Points.at(i);
      p2 = this->Points.at(i + 1);
      // Cases for interval (freq[i], freq[i+1])

      int pos1 = p1.GetX() * this->SpectrumSize / samplingFrequency;
      int pos2 = p2.GetX() * this->SpectrumSize / samplingFrequency;
      // 1. Out of region
      if (pos2 < 0 || pos1 > this->GetHalfSpectrumSize())
      {
        continue;
      }
      // 2. Left is out of region, right is in region
      if (pos1 < 0 && pos2 <= this->GetHalfSpectrumSize())
      {
        double y =
          vtkInternal::lineYValue(0, vtkVector2f(pos1, p1.GetY()), vtkVector2f(pos2, p2.GetY()));
        pos1 = 0;
        p1.SetY(y);
      }
      // 3. Right is out of region, left is in region
      if (pos1 >= 0 && pos2 > this->GetHalfSpectrumSize())
      {
        double y = vtkInternal::lineYValue(
          this->GetHalfSpectrumSize(), vtkVector2f(pos1, p1.GetY()), vtkVector2f(pos2, p2.GetY()));
        pos2 = this->GetHalfSpectrumSize();
        p2.SetY(y);
      }
      // 4. Interval covers region
      if (pos1 < 0 && pos2 > this->GetHalfSpectrumSize())
      {
        double y1 =
          vtkInternal::lineYValue(0, vtkVector2f(pos1, p1.GetY()), vtkVector2f(pos2, p2.GetY()));
        double y2 = vtkInternal::lineYValue(
          this->GetHalfSpectrumSize(), vtkVector2f(pos1, p1.GetY()), vtkVector2f(pos2, p2.GetY()));
        pos1 = 0;
        p1.SetY(y1);
        pos2 = this->GetHalfSpectrumSize();
        p2.SetY(y2);
      }
      // 5. Otherwise, interval is inside region
      double delta = (p2.GetY() - p1.GetY()) / (pos2 - pos1);

      for (int j = pos1; j < pos2; j++)
      {
        float coeff = (p1.GetY() + delta * (j - pos1));
        result.emplace_back(j, coeff);
      }
    }

    return result;
  }

  //------------------------------------------------------------------------------
  static double lineYValue(double x, vtkVector2f le1, vtkVector2f le2)
  {
    return ((le2.GetY() - le1.GetY()) * x +
             (-le1.GetX() * (le2.GetY() - le1.GetY()) + le1.GetY() * (le2.GetX() - le1.GetX()))) /
      (le2.GetX() - le1.GetX());
  }

  //------------------------------------------------------------------------------
  static std::vector<std::string> splitStringByDelimiter(const std::string& source, char delim)
  {
    std::stringstream ss(source);
    std::string item;
    std::vector<std::string> result;
    while (std::getline(ss, item, delim))
    {
      result.push_back(std::move(item));
    }

    return result;
  }

  // attributes
  std::vector<vtkVector2f> Points;
  vtkIdType OriginalSize = 0;
  vtkIdType SpectrumSize = 0;
  vtkTable* TableSrc = nullptr;
  std::map<std::string, std::vector<vtkFFT::ComplexNumber>> Spectrums;
  std::map<std::string, std::vector<double>> NormalizedSpectrums;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkEqualizerFilter);

//------------------------------------------------------------------------------
vtkEqualizerFilter::vtkEqualizerFilter()
  : Internal(new vtkInternal())
{
  this->SetNumberOfOutputPorts(3);
}

//------------------------------------------------------------------------------
vtkEqualizerFilter::~vtkEqualizerFilter()
{
  delete this->Internal;
}

//------------------------------------------------------------------------------
void vtkEqualizerFilter::PrintSelf(std::ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "Sampling Frequency: " << this->SamplingFrequency << " Hz" << endl;
  os << indent << "All Columns: " << this->AllColumns << endl;
  os << indent << "Array: " << this->Array << endl;
  os << indent << "Spectrum Gain: " << this->SpectrumGain << " dB" << endl;
}

//------------------------------------------------------------------------------
void vtkEqualizerFilter::SetPoints(const std::string& pointsStr)
{
  // TODO: refactoring: replace to common function
  // linked to https://gitlab.kitware.com/vtk/vtk/-/merge_requests/5930#note_926052
  this->Internal->ClearPoints();
  std::vector<std::string> vecPointsStr{ vtkInternal::splitStringByDelimiter(pointsStr, ';') };

  for (const auto& point : vecPointsStr)
  {
    std::vector<std::string> pointStr{ vtkInternal::splitStringByDelimiter(point, ',') };
    if (pointStr.size() > 1)
    {
      float x = std::stof(pointStr.at(0));
      float y = std::stof(pointStr.at(1));
      this->Internal->Points.emplace_back(x, y);
    }
  }

  this->Modified();
}

//------------------------------------------------------------------------------
std::string vtkEqualizerFilter::GetPoints() const
{
  std::stringstream ss;
  for (auto point : this->Internal->Points)
  {
    ss << point.GetX() << "," << point.GetY() << ";";
  }

  return ss.str();
}

//------------------------------------------------------------------------------
int vtkEqualizerFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkTable* input = vtkTable::GetData(inputVector[0]);
  this->Internal->SetTable(input);

  vtkInformation* outInfo0 = outputVector->GetInformationObject(0);
  vtkInformation* outInfo1 = outputVector->GetInformationObject(1);
  vtkInformation* outInfo2 = outputVector->GetInformationObject(2);

  if (!outInfo0 || !outInfo1 || !outInfo2)
  {
    vtkWarningMacro(<< "No output info.");
    return 0;
  }

  vtkTable* spectrumTable = vtkTable::GetData(outInfo0);
  vtkTable* resultTable = vtkTable::GetData(outInfo1);
  vtkTable* normalizedSpectrumTable = vtkTable::GetData(outInfo2);

  if (!input || !spectrumTable || !resultTable || !normalizedSpectrumTable)
  {
    vtkWarningMacro(<< "No input or output.");
    return 0;
  }

  this->Internal->OriginalSize = input->GetNumberOfRows(); // orig size
  if (this->AllColumns)
  {
    vtkIdType numColumns = input->GetNumberOfColumns();

    for (vtkIdType col = 0; col < numColumns; col++)
    {
      this->UpdateProgress(static_cast<double>(col) / numColumns);
      if (this->CheckAbort())
      {
        break;
      }

      vtkDataArray* array = vtkArrayDownCast<vtkDataArray>(input->GetColumn(col));
      if (!array)
      {
        continue;
      }
      if (array->GetNumberOfComponents() != 1)
      {
        continue;
      }
      if (array->GetName())
      {
        if (vtksys::SystemTools::Strucmp(array->GetName(), "time") == 0)
        {
          resultTable->AddColumn(array);
          continue;
        }
        if (strcmp(array->GetName(), "vtkValidPointMask") == 0)
        {
          resultTable->AddColumn(array);
          continue;
        }
      }
      if (array->IsA("vtkIdTypeArray"))
      {
        continue;
      }

      ProcessColumn(array, spectrumTable, resultTable, normalizedSpectrumTable);
    }
  }
  else
  {
    vtkDataArray* array =
      vtkArrayDownCast<vtkDataArray>(input->GetColumnByName(this->Array.c_str()));
    if (!array)
    {
      vtkDebugMacro(<< " !array");
      return 1;
    }

    if (array->GetNumberOfComponents() != 1)
    {
      vtkDebugMacro(<< "Number of components != 1");
      return 1;
    }
    if (array->GetName())
    {
      if (vtksys::SystemTools::Strucmp(array->GetName(), "time") == 0)
      {
        resultTable->AddColumn(array);
        return 1;
      }
      if (strcmp(array->GetName(), "vtkValidPointMask") == 0)
      {
        resultTable->AddColumn(array);
        return 1;
      }
    }
    if (array->IsA("vtkIdTypeArray"))
    {
      vtkDebugMacro(<< "vtkIdTypeArray");
      return 1;
    }

    ProcessColumn(array, spectrumTable, resultTable, normalizedSpectrumTable);
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkEqualizerFilter::ProcessColumn(
  vtkDataArray* array, vtkTable* spectrumTable, vtkTable* resultTable, vtkTable* normalizedTable)
{
  // FFT
  auto spectrum = this->Internal->GetSpectrum(array);
  if (spectrum.empty())
  {
    vtkErrorMacro(<< "Spectrum is empty: " << array->GetName());
    return;
  }

  std::vector<double> normSpectrum = this->Internal->GetNormalizedSpectrum(array);
  if (normSpectrum.empty())
  {
    vtkErrorMacro(<< "Normalized spectrum is empty: " << array->GetName());
    return;
  }
  // end FFT

  // modify by equalizer
  auto modifiers = this->Internal->GetModifiers(this->SamplingFrequency);
  for (const auto& modifier : modifiers)
  {
    spectrum[modifier.first].r *= modifier.second;
    spectrum[modifier.first].i *= modifier.second;
    spectrum[this->Internal->SpectrumSize - modifier.first - 1].r *= modifier.second;
    spectrum[this->Internal->SpectrumSize - modifier.first - 1].i *= modifier.second;

    normSpectrum[modifier.first] *= modifier.second;
  }

  // fill spectrum table
  std::vector<double> freqArray =
    vtkFFT::RFftFreq(this->Internal->SpectrumSize, 1.0 / this->SamplingFrequency);

  vtkSmartPointer<vtkDoubleArray> freqColumn = vtkSmartPointer<vtkDoubleArray>::New();
  freqColumn->SetNumberOfComponents(1);
  freqColumn->SetNumberOfTuples(this->Internal->GetHalfSpectrumSize());
  freqColumn->SetName("Frequency");
  for (vtkIdType spectrumId = 0; spectrumId < this->Internal->GetHalfSpectrumSize(); ++spectrumId)
  {
    freqColumn->SetValue(spectrumId, freqArray.at(spectrumId));
  }

  spectrumTable->AddColumn(freqColumn);
  normalizedTable->AddColumn(freqColumn);

  vtkSmartPointer<vtkDoubleArray> leadArray = vtkSmartPointer<vtkDoubleArray>::New();
  leadArray->SetNumberOfComponents(1);
  leadArray->SetNumberOfTuples(this->Internal->GetHalfSpectrumSize());
  leadArray->SetName(array->GetName());

  vtkSmartPointer<vtkDoubleArray> normalizedArray = vtkSmartPointer<vtkDoubleArray>::New();
  normalizedArray->SetNumberOfComponents(1);
  normalizedArray->SetNumberOfTuples(this->Internal->GetHalfSpectrumSize());
  normalizedArray->SetName(array->GetName());

  double modifier = pow(10, 0.05 * this->SpectrumGain);
  for (vtkIdType spectrumId = 0; spectrumId < this->Internal->GetHalfSpectrumSize(); ++spectrumId)
  {
    if (this->CheckAbort())
    {
      break;
    }
    auto value = spectrum[spectrumId];
    // we are only interested in amplitude spectrum, so we use complex_module
    // divide by the number of elements so that the amplitudes are in millivolts, not Fourier sums.
    double module = vtkFFT::Abs(value) * modifier / this->Internal->GetHalfSpectrumSize();
    leadArray->SetValue(spectrumId, module);

    normalizedArray->SetValue(spectrumId, normSpectrum[spectrumId] * modifier);
  }
  spectrumTable->AddColumn(leadArray);
  normalizedTable->AddColumn(normalizedArray);
  // end fill spectrum table

  // fill result table
  auto num = vtkFFT::IRFft(spectrum);

  vtkSmartPointer<vtkDoubleArray> rfftArray = vtkSmartPointer<vtkDoubleArray>::New();
  rfftArray->SetNumberOfComponents(1);
  rfftArray->SetNumberOfTuples(this->Internal->OriginalSize);
  rfftArray->SetName(array->GetName());

  for (int spectrumId = 0; spectrumId < this->Internal->OriginalSize; ++spectrumId)
  {
    auto val = num[spectrumId];
    rfftArray->SetValue(spectrumId, val);
  }
  resultTable->AddColumn(rfftArray);
  // end fill result table
}
VTK_ABI_NAMESPACE_END
