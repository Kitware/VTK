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

#ifndef viskores_filter_flow_testing_TestingFlow_h
#define viskores_filter_flow_testing_TestingFlow_h

#include <viskores/Particle.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/PartitionedDataSet.h>

#include <vector>

enum FilterType
{
  PARTICLE_ADVECTION,
  STREAMLINE,
  PATHLINE
};

viskores::cont::ArrayHandle<viskores::Vec3f> CreateConstantVectorField(viskores::Id num,
                                                                       const viskores::Vec3f& vec);

void AddVectorFields(viskores::cont::PartitionedDataSet& pds,
                     const std::string& fieldName,
                     const viskores::Vec3f& vec);

std::vector<viskores::cont::PartitionedDataSet> CreateAllDataSetBounds(viskores::Id nPerRank,
                                                                       bool useGhost);

std::vector<viskores::Range> ExtractMaxXRanges(const viskores::cont::PartitionedDataSet& pds,
                                               bool useGhost);

template <typename FilterType>
void SetFilter(FilterType& filter,
               viskores::FloatDefault stepSize,
               viskores::Id numSteps,
               const std::string& fieldName,
               viskores::cont::ArrayHandle<viskores::Particle> seedArray,
               bool useThreaded,
               bool useBlockIds,
               const std::vector<viskores::Id>& blockIds)
{
  filter.SetStepSize(stepSize);
  filter.SetNumberOfSteps(numSteps);
  filter.SetSeeds(seedArray);
  filter.SetActiveField(fieldName);
  filter.SetUseThreadedAlgorithm(useThreaded);

  if (useBlockIds)
    filter.SetBlockIDs(blockIds);
}

void ValidateOutput(const viskores::cont::DataSet& out,
                    viskores::Id numSeeds,
                    const viskores::Range& xMaxRange,
                    FilterType fType,
                    bool checkEndPoint,
                    bool blockDuplication);

void TestPartitionedDataSet(viskores::Id nPerRank,
                            bool useGhost,
                            FilterType fType,
                            bool useThreaded,
                            bool useBlockIds,
                            bool duplicateBlocks);

#endif // viskores_filter_flow_testing_TestingFlow_h
