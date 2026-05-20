## Mesh cache improvements

`vtkDataObjectMeshCache` has better performance to forward attribute to output:
- `PreservedInputAttributes` flags a field from the OriginalDataObject so its arrays are shallow copied,
  without the need of the (time-costly) `OriginalIds` mapping.
- when using the `OriginalIds` map, the cache now uses SMP acceleration to forward
  attributes to the output.
- `PreservedCachedArray` flags some arrays from the cache to be preserved as is
  in the output. By default, every cached arrays are removed and the output is filled
  only by forwarding input arrays. When the consumer adds a new array that
  will not change in static mesh cases, it is useful to set it as a `PreservedCachedArray`.

`vtkMeshCacheRunner` is a RAII helper to use an existing `vtkDataObjectMeshCache`
with a new OriginalDataObject.
Its constructor does some sanity checks, can add implicit `OriginalIds`
if needed and copy the cache to the given output if the cache is valid.
Optionally performs cache update in its destructor.

### Fix for composite data

`vtkDataObjectMeshCache` stores the MeshMTime for each input leaf.
Previously, only the most recent (i.e. the greater) was stored for comparison.
This was a wrong approach: replacing an older leaf with another dataset also
older than the stored value, the cache was still considered as valid.
