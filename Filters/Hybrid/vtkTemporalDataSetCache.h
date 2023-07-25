// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTemporalDataSetCache
 * @brief   cache time steps
 *
 * vtkTemporalDataSetCache cache time step requests of a temporal dataset,
 * when cached data is requested it is returned using a shallow copy.
 * @par Thanks:
 * Ken Martin (Kitware) and John Bidiscombe of
 * CSCS - Swiss National Supercomputing Centre
 * for creating and contributing this class.
 * For related material, please refer to :
 * John Biddiscombe, Berk Geveci, Ken Martin, Kenneth Moreland, David Thompson,
 * "Time Dependent Processing in a Parallel Pipeline Architecture",
 * IEEE Visualization 2007.
 */

#ifndef vtkTemporalDataSetCache_h
#define vtkTemporalDataSetCache_h

#include "vtkFiltersHybridModule.h" // For export macro

#include "vtkAlgorithm.h"
#include <map>    // used for the cache
#include <vector> // used for the timestep records

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSHYBRID_EXPORT vtkTemporalDataSetCache : public vtkAlgorithm
{
public:
  static vtkTemporalDataSetCache* New();
  vtkTypeMacro(vtkTemporalDataSetCache, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * This is the maximum number of time steps that can be retained in memory.
   * it defaults to 10.
   */
  void SetCacheSize(int size);
  vtkGetMacro(CacheSize, int);
  ///@}

  ///@{
  /**
   * Tells the filter that it should store the dataobjects it holds in memkind
   * extended memory space rather than in normal memory space.
   */
  vtkSetMacro(CacheInMemkind, bool);
  vtkGetMacro(CacheInMemkind, bool);
  vtkBooleanMacro(CacheInMemkind, bool);
  ///@}

  ///@{
  /**
   * Tells the filter that needs to act as a pipeline source rather than a midpipline filter. In
   * that situation it needs to react differently in a few cases.
   */
  vtkSetMacro(IsASource, bool);
  vtkGetMacro(IsASource, bool);
  vtkBooleanMacro(IsASource, bool);
  ///@}

protected:
  vtkTemporalDataSetCache();
  ~vtkTemporalDataSetCache() override;

  int CacheSize;

  typedef std::map<double, std::pair<unsigned long, vtkDataObject*>> CacheType;
  CacheType Cache;
  std::vector<double> TimeStepValues;
  /**
   * see vtkAlgorithm for details
   */
  vtkTypeBool ProcessRequest(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info) override;

  virtual int RequestInformation(
    vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector);

  virtual int RequestDataObject(
    vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector);

  virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

private:
  vtkTemporalDataSetCache(const vtkTemporalDataSetCache&) = delete;
  void operator=(const vtkTemporalDataSetCache&) = delete;

  void ReplaceCacheItem(vtkDataObject* input, double inTime, vtkMTimeType outputUpdateTime);
  bool CacheInMemkind;
  bool IsASource;

  // a helper to deal with eviction smoothly. In effect we are an N+1 cache.
  void SetEjected(vtkDataObject*);
  vtkGetObjectMacro(Ejected, vtkDataObject);
  vtkDataObject* Ejected;
};

VTK_ABI_NAMESPACE_END
#endif
