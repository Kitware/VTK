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
// Copyright (c) 2018, The Regents of the University of California, through
// Lawrence Berkeley National Laboratory (subject to receipt of any required approvals
// from the U.S. Dept. of Energy).  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National
//     Laboratory, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
//=============================================================================
//
//  This code is an extension of the algorithm presented in the paper:
//  Parallel Peak Pruning for Scalable SMP Contour Tree Computation.
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.
//
//  The PPP2 algorithm and software were jointly developed by
//  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
//  Oliver Ruebel (LBNL)
//==============================================================================

#ifndef viskores_worklet_contourtree_augmented_process_contourtree_inc_piecewise_linear_function_h
#define viskores_worklet_contourtree_augmented_process_contourtree_inc_piecewise_linear_function_h

#include <viskores/Pair.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ContourTree.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{
namespace process_contourtree_inc
{


template <typename T>
struct PairComparator
{ // PairComparator()
  inline bool operator()(const std::pair<T, T>& s1, const std::pair<T, T>& s2) const
  {
    return s1.second > s2.second;
  }
}; // PairComparator()


// TODO Need to change the samples to use VISKORES data structures instead of std::vector
template <typename T>
class PiecewiseLinearFunction
{
  std::vector<std::pair<T, T>> samples;

public:
  void addSample(T sx, T sy)
  {
    auto it = std::find_if_not(
      samples.begin(), samples.end(), [sx](std::pair<T, T> s) { return sx > s.first; });
    samples.insert(it, std::pair<T, T>(sx, sy));
  }

  T operator()(T px) const
  {
    if (samples.size() < 2)
      return 0;
    else if (px < samples.front().first || px > samples.back().first)
      return 0;
    else
    {
      auto end = std::find_if_not(
        samples.begin(), samples.end(), [px](std::pair<T, T> s) { return px > s.first; });
      if (end == samples.begin())
      {
        std::cerr << "WARNING!" << std::endl;
        return 0;
      }
      else
      {
        auto begin = end - 1;
        T t = (px - begin->first) / (end->first - begin->first);
        return (1 - t) * begin->second + t * end->second;
      }
    }
  }

  PiecewiseLinearFunction& operator+=(const PiecewiseLinearFunction& other)
  {
    std::vector<std::pair<T, T>> new_samples;
    auto it1 = samples.begin();
    auto it2 = other.samples.begin();

    while (it1 != samples.end() && it2 != other.samples.end())
    {
      if (it1->first < it2->first)
      {
        new_samples.emplace_back(it1->first, it1->second + other(it1->first));
        ++it1;
      }
      else if (it2->first < it1->first)
      {
        new_samples.emplace_back(it2->first, it2->second + (*this)(it2->first));
        ++it2;
      }
      else
      {
        new_samples.emplace_back(it1->first, it1->second + it2->second);
        ++it1;
        ++it2;
      }
    }
    while (it1 != samples.end())
    {
      new_samples.push_back(*it1);
      ++it1;
    }
    while (it2 != other.samples.end())
    {
      new_samples.push_back(*it2);
      ++it2;
    }

    samples.swap(new_samples);
    return *this;
  }

  std::vector<T> nLargest(unsigned n)
  {
    std::vector<std::pair<T, T>> sCopy = samples;
    std::sort(sCopy.begin(), sCopy.end(), PairComparator<T>());
    std::vector<T> res;
    for (unsigned i = 0; i < n; ++i)
    {
      res.push_back(sCopy[i].first);
    }
    return res;
  }

  void print()
  {
    for (auto sample : samples)
      std::cout << "(" << sample.first << ", " << sample.second << ") ";
    std::cout << std::endl;
  }
};



} // process_contourtree_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
