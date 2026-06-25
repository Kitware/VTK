# Refactor `vtkCollection` to use `std::vector` instead of a linked list

`vtkCollection` now uses an `std::vector` instead of a linked list, improving runtime performance.

Its public API and behaviour have not changed at all, except:

1. The class `vtkCollectionElement`, being a representation of a linked list node, has been removed, without any deprecation period. Although it appeared in `vtkCollection.h`, it wasn't exactly public API, and no uses of it could be found in various open source consumers of VTK (ParaView, Slicer, ITK, Horos).

2. Two `protected` member functions (RemoveElement and DeleteElement) and several `protected` member variables were removed. All of these involved `vtkCollectionElement` and thus could not be deprecated. This may affect subclassers. All of VTK's own subclasses have been updated.

3. A new `Sort()` member function was added. It takes an `std::function` and sorts itself using that. VTK subclasses (`vtkActor2DCollection` and `vtkImageSliceCollection`) that did their own sorting were updated to use this new mechanism.

4. The `NewIterator()` member function was deprecated since `vtkCollectionIterator` itself is now deprecated. Use `vtk::Range` for range-based for loops instead.

5. The default and `protected` constructors of the class `CollectionIterator`,
defined in `vtkCollectionRange.h`, have been removed. Use the new
constructor which properly initializes its new `Iterator` member.

6. `GetNextPath` is tagged with `VTK_MARSHAL_EXCLUDE_REASON_NOT_SUPPORTED`.
