//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================


#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/filter/field_conversion/CellAverage.h>
#include <viskores/filter/multi_block/AmrArrays.h>
#include <viskores/source/Amr.h>
#include <viskores/source/Wavelet.h>


namespace viskores
{
namespace source
{

Amr::Amr(viskores::IdComponent dimension,
         viskores::IdComponent cellsPerDimension,
         viskores::IdComponent numberOfLevels)
  : Dimension(dimension)
  , CellsPerDimension(cellsPerDimension)
  , NumberOfLevels(numberOfLevels)
{
}

template <viskores::IdComponent Dim>
viskores::cont::DataSet Amr::GenerateDataSet(unsigned int level, unsigned int amrIndex) const
{
  viskores::Id3 extent = { viskores::Id(this->CellsPerDimension / 2) };
  viskores::Id3 dimensions = { this->CellsPerDimension + 1 };
  viskores::Vec3f origin = { float(1. / pow(2, level) * amrIndex) };
  viskores::Vec3f spacing = { float(1. / this->CellsPerDimension / pow(2, level)) };
  viskores::Vec3f center = 0.5f - (origin + spacing * extent);
  viskores::Vec3f frequency = { 60.f, 30.f, 40.f };
  frequency = frequency * this->CellsPerDimension;
  viskores::FloatDefault deviation = 0.5f / this->CellsPerDimension;

  if (Dim == 2)
  {
    extent[2] = 0;
    dimensions[2] = 1;
    origin[2] = 0;
    spacing[2] = 1;
    center[2] = 0;
  }

  viskores::source::Wavelet waveletSource;
  waveletSource.SetOrigin(origin);
  waveletSource.SetSpacing(spacing);
  waveletSource.SetCenter(center);
  waveletSource.SetExtent(-extent, extent);
  waveletSource.SetFrequency(frequency);
  waveletSource.SetStandardDeviation(deviation);
  viskores::cont::DataSet wavelet = waveletSource.Execute();

  viskores::filter::field_conversion::CellAverage cellAverage;
  cellAverage.SetActiveField("RTData", viskores::cont::Field::Association::Points);
  cellAverage.SetOutputFieldName("RTDataCells");
  return cellAverage.Execute(wavelet);
}

viskores::cont::PartitionedDataSet Amr::Execute() const
{
  assert(this->CellsPerDimension > 1);
  assert(this->CellsPerDimension % 2 == 0);

  // Generate AMR
  std::vector<std::vector<viskores::Id>> blocksPerLevel(this->NumberOfLevels);
  unsigned int counter = 0;
  for (unsigned int l = 0; l < blocksPerLevel.size(); l++)
  {
    for (unsigned int b = 0; b < pow(2, l); b++)
    {
      blocksPerLevel.at(l).push_back(counter++);
    }
  }
  viskores::cont::PartitionedDataSet amrDataSet;

  // Fill AMR with data from the wavelet
  for (unsigned int l = 0; l < blocksPerLevel.size(); l++)
  {
    for (unsigned int b = 0; b < blocksPerLevel.at(l).size(); b++)
    {
      if (this->Dimension == 2)
      {
        amrDataSet.AppendPartition(this->GenerateDataSet<2>(l, b));
      }
      else if (this->Dimension == 3)
      {
        amrDataSet.AppendPartition(this->GenerateDataSet<3>(l, b));
      }
    }
  }

  // Generate helper arrays
  viskores::filter::multi_block::AmrArrays amrArrays;
  amrDataSet = amrArrays.Execute(amrDataSet);

  return amrDataSet;
}

} // namespace source
} // namespace viskores
