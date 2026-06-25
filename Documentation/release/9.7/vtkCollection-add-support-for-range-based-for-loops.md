## Range-based for loops for `vtkCollection`; deprecation of `vtkCollectionIterator`

`vtkCollection` now supports C++11 range-based for loops, leveraging modern C++ syntax.

This, along with `vtk::Range`, should be preferred over using `vtkCollectionIterator` and `vtkCollectionSimpleIterator` and they are now deprecated.

Here's a typical example of the old way:

```
vtkCollectionIterator* iter = this->AnimationCuesIterator;
for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
{
  vtkAnimationCue* cue = vtkAnimationCue::SafeDownCast(iter->GetCurrentObject());
```

transformed:

```
for (vtkObject* obj : *(this->AnimationCues))
{
  vtkAnimationCue* cue = vtkAnimationCue::SafeDownCast(obj);
```

Note that since `vtkCollection` uses `vtkObject` (as opposed to being templated), the range-based for loops also use `vtkObject` and may require casting (as in both the old *and* new ways above).

All of VTK's own uses have been updated. The only slight backwards incompatible change is that the `protected` member `vtkCollectionIterator* AnimationCuesIterator` in `vtkAnimationScene` has been removed. This may cause compatibility issues for anyone subclassing this class.
