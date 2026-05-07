// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkDataObjectMeshCache_h
#define vtkDataObjectMeshCache_h

#include "vtkCommonCacheModule.h" // Export macro

#include "vtkObject.h"
#include "vtkSetGet.h"       // for Set/Get macros
#include "vtkSmartPointer.h" // for smart pointer
#include "vtkWeakPointer.h"  // for weak pointer

#include <set>    // for set
#include <string> // for string

VTK_ABI_NAMESPACE_BEGIN

class vtkAlgorithm;
class vtkDataObject;
class vtkDataSet;
class vtkCompositeDataSet;

/**
 * vtkDataObjectMeshCache is a class to store and reuse the mesh of a vtkDataSet,
 * while forwarding data arrays from another dataset. Composite structure
 * of vtkDataSet are also supported.
 * This is specially useful when working with static meshes and transient data.
 *
 * ## Example of use case
 * Think about the vtkGeometryFilter. On first execution, it extracts boundaries
 * of an input mesh alongside with the associated data.
 * On the second execution, if the input mesh didn't change and neither did the
 * vtkGeometryFitlers own properties, then there is no need for boundary extraction:
 * previous output mesh can be reused. Only the associated data should be forwarded.
 *
 * Instead of implementing such logic itself, filter like the vtkGeometryFilter can instead
 * rely on the vtkDataObjectMeshCache in order to easily reuse the previously computed mesh,
 * and forward the new data arrays.
 *
 * ## Details
 * This helper relies on different elements:
 * - `Consumer`: a vtkObject using the helper. Any modifications on it invalidate cache.
 * - `OriginalDataObject`: the input vtkDataObject. Should be either a vtkDataSet
 *   or a composite of vtkDataSet. The helper looks for its MeshMTime.
 * - `Cache`: the output vtkDataObject containing the mesh to reuse (or a composite)
 * - `OriginalIds`: a list of original ids array name per attribute types,
 *   to forward from OriginalDataObject to Cache when asked to.
 *
 * The `Status` structure reflects the state of those different elements.
 * It is the user responsibility to check for the status before
 * calling `CopyCacheToDataObject`.
 *
 * Attributes data are forwarded with `CopyAllocate` method. So output
 * should be a subset of the input. Support for `InterpolateAllocate`
 * is doable and may be added in the future.
 *
 * ## Requirements
 * The data arrays forwarding rely on GlobalIds arrays.
 *
 * When using vtkCompositeDataSet, every leaves should be of a supported
 * data set type.
 *
 * @warning usage with vtkCompositeDataPipeline
 * The vtkCompositeDataPipeline updates the filter for each block.
 * The filter cannot differentiate an update for a new block from an
 * update with a new input. Trying to use the vtkDataObjectMeshCache in this context
 * leads to cache invalidation at each call, because each block is considered as a new dataset.
 * To properly setup vtkDataObjectMeshCache in a filter, vtkCompositeDataSet should
 * be explicitly handled, so each RequestData can actually be interpreted as
 * a new input data to process.
 */
class VTKCOMMONCACHE_EXPORT vtkDataObjectMeshCache : public vtkObject
{
public:
  static vtkDataObjectMeshCache* New();
  vtkTypeMacro(vtkDataObjectMeshCache, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Structure to handle the current cache status.
   * Different conditions are mandatory to use the cache.
   * This offers a low-level access to those conditions as long
   * as the global state.
   */
  struct Status
  {
    /**
     * True if OriginalDataObject is set.
     */
    bool OriginalDataDefined = false;
    /**
     * True if Consumer is set.
     */
    bool ConsumerDefined = false;
    /**
     * True if Cache with initialized.
     * @sa UpdateCache
     */
    bool CacheDefined = false;
    /**
     * True if OriginalDataSet mesh was not modified since last cache update.
     */
    bool OriginalMeshUnmodified = false;
    /**
     * True if Consumer was not modified since last cache update.
     */
    bool ConsumerUnmodified = false;
    /**
     * True if attributes ids exists
     */
    bool AttributesIdsExists = true;

    /**
     * Return true if the cache can safely and meaningfully be used.
     */
    bool enabled()
    {
      return this->OriginalDataDefined && this->ConsumerDefined && this->CacheDefined &&
        this->ConsumerUnmodified && this->OriginalMeshUnmodified && this->AttributesIdsExists;
    }

    /**
     * Return true if and only if every members are equals.
     * @sa operator!=
     */
    bool operator==(const Status& other) const
    {
      return other.OriginalDataDefined == this->OriginalDataDefined &&
        other.ConsumerDefined == this->ConsumerDefined &&
        other.CacheDefined == this->CacheDefined &&
        other.OriginalMeshUnmodified == this->OriginalMeshUnmodified &&
        other.ConsumerUnmodified == this->ConsumerUnmodified &&
        other.AttributesIdsExists == this->AttributesIdsExists;
    }

    /**
     * Return true if both object are not equals.
     * @sa operator==
     */
    bool operator!=(const Status& other) const { return !(*this == other); }

    /**
     * Print members.
     */
    void PrintSelf(ostream& os, vtkIndent indent);
  };

  /**
   * @name Cache Configuration
   */
  ///@{
  /**
   * Set the consumer of this cache.
   * The status is invalid if the Consumer is modified after the last CopyCacheToDataObject call.
   * Required before any call to CopyCacheToDataObject.
   */
  void SetConsumer(vtkAlgorithm* consumer);

  /**
   * Set the original dataobject.
   * The status becomes invalid if the original dataobject mesh is modified.
   * Original dataobject is also used to copy data arrays to output,
   * if OriginalIds are configured.
   * Required before any call to CopyCacheToDataObject.
   * @sa AddOriginalIds, RemoveOriginalIds, ClearOriginalIds
   */
  void SetOriginalDataObject(vtkDataObject* original);
  ///@}

  /**
   * Original Ids.
   * When original ids are present for an attribute types, all arrays of this
   * attribute are forwarded to the output.
   * @sa CopyCacheToOutput
   */
  ///@{
  /**
   * Add original ids array name for attribute type.
   * @sa RemoveOriginalIds, ClearOriginalIds, CopyCacheToOutput
   */
  void AddOriginalIds(int attribute, const std::string& name);

  /**
   * Remove ids array name for attribute type.
   * @sa AddOriginalIds, ClearOriginalIds, CopyCacheToOutput
   */
  void RemoveOriginalIds(int attribute);

  /**
   * Clear all original ids.
   * @sa RemoveOriginalIds, AddOriginalIds
   */
  void ClearOriginalIds();

  /**
   * Add attribute in the forward list.
   * Arrays from this attribute will be forwarded to the output,
   * using a default `OriginalIds` name.
   * @see AddOriginalIds, GetTemporaryIdsName
   */
  void ForwardAttribute(int attribute);

  /**
   * Preserve input attribute, i.e. forward the whole input arrays
   * wihout reorder (original ids are not used).
   * Default is false.
   * @see AddOriginalIds, ForwardAttributes
   */
  ///@{
  void AddPreservedInputAttributes(int);
  void ClearPreservedInputAttributes();
  void PreservedInputAllAttributes();
  std::set<int> GetPreservedInputAttributes();
  ///@}

  /**
   * Return a default name for original ids.
   */
  static std::string GetDefaultIdsName();

  /**
   * Add an ids array on underlying PointData and CellData.
   * This is used by the vtkDataObjectMeshCache to forward
   * attributes data from a new input to the cached mesh.
   */
  static void CreateTemporaryOriginalIdsArrays(vtkDataObject* object);

  /**
   * Cleanup the temporary array, as we do not want it to exists outside
   * of this filter.
   */
  static void CleanupTemporaryOriginalIds(vtkDataObject* object);
  ///@}

  /**
   * Return the mesh MTime for the given dataobject, as used for the Cache.
   * If object is a vtkDataSet this is equivalent to vtkDataSet::GetMeshMTime()
   * If object is a composite, return the max of each underlying dataset MeshMTime.
   * Other types are ignored, a MTime of 0 is used.
   */
  static vtkMTimeType GetDataObjectMeshMTime(vtkDataObject* object);

  /**
   * Compute and returns the current cache status.
   * The cache status details whenever the cache is usable,
   * with detailed information.
   */
  Status GetStatus() const;

  /**
   * Fill given dataset with cached data.
   * If original ids are present, copy corresponding attributes.
   * It is the user responsibility to check the status before calling this.
   */
  void CopyCacheToDataObject(vtkDataObject* output);

  ///@{
  /**
   * Keep cached arrayName array when copying cache to output.
   *
   * By default, CopyCacheToDataObject clears output attributes arrays,
   * and then forward input arrays to output as configured.
   * Arrays added as PreservedCachedArrays are kept.
   */
  void AddPreservedCachedArray(const std::string& arrayName);
  void ClearPreservedCachedArray();
  std::set<std::string> GetPreservedCachedArrays();
  ///@}

  /**
   * Set given dataset as the new Cache.
   * Also update stored MTime from OriginalDataSet mesh and Consumer.
   */
  void UpdateCache(vtkDataObject* newObject);

  /**
   * Invalidate cache.
   * Remove cached dataset and reset cached MTimes.
   */
  void InvalidateCache();

  /**
   * Return true if dataobject is of a supported type.
   */
  bool IsSupportedData(vtkDataObject* dataobject) const;

protected:
  vtkDataObjectMeshCache() = default;
  ~vtkDataObjectMeshCache() override = default;
  /**
   * Forward dataset attributes from OriginalDataObject to output.
   * Uses original ids attribute arrays to copy data.
   * FieldData are always forwarded.
   */
  ///@{
  void ForwardAttributes(vtkDataSet* input, vtkDataSet* cache, vtkDataSet* output, int attribute,
    const std::string& name);
  void ForwardAttributesToDataSet(vtkDataSet* input, vtkDataSet* cache, vtkDataSet* output);
  void ForwardAttributesToComposite(vtkCompositeDataSet* input, vtkCompositeDataSet* output);
  ///@}

private:
  vtkDataObjectMeshCache(const vtkDataObjectMeshCache&) = delete;
  void operator=(const vtkDataObjectMeshCache&) = delete;

  /**
   * Get the original dataobject.
   */
  vtkDataObject* GetOriginalDataObject() const;

  /**
   * Get the OriginalDataSet mesh time.
   */
  vtkMTimeType GetOriginalMeshTime() const;

  /**
   * Return the number of datasets contained in dataobject.
   * Return 1 if dataobject is itself a vtkDataSet.
   * Return the number of non empty dataset leaves for a composite.
   * Return 0 otherwise.
   */
  vtkIdType GetNumberOfDataSets(vtkDataObject* dataobject) const;

  /**
   * Return true if the cached dataobject has the required ids arrays.
   * For composite, each leaf should have the required arrays.
   * Return false if no array were requested.
   * @sa SetOriginalIdsName HasRequestedIds
   */
  bool CacheHasRequestedIds() const;

  /**
   * Clear all dataset attributes from given data object.
   */
  void ClearAttributes(vtkDataObject*);

  /**
   * A consumer without any input port means it is most of the time a source.
   * This method is here to help us to determine specific behaviors for sources.
   */
  bool HasConsumerNoInputPort() const;

  vtkWeakPointer<vtkAlgorithm> Consumer;
  vtkSmartPointer<vtkDataObject> Cache;
  vtkWeakPointer<vtkDataSet> OriginalDataSet;
  vtkWeakPointer<vtkCompositeDataSet> OriginalCompositeDataSet;
  vtkMTimeType CachedOriginalMeshTime = 0;
  vtkMTimeType CachedConsumerTime = 0;
  std::map<int, std::string> OriginalIdsName;
  std::set<int> PreserveInputAttributes;
  std::set<std::string> PreserveCachedArrays;
};

VTK_ABI_NAMESPACE_END
#endif
