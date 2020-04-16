# remove-legacy-apis

The following APIs have been removed:

  - `vtkDataArrayTemplate` (deprecated since Dec 2015)
  - `vtkObjectBase::PrintRevisions` and `vtkObjectBase::CollectRevisions`
    (deprecated since 2012)
  - `VTK___INT64` and `VTK_UNSIGNED___INT64` (deprecated since Mar 2017)
  - `vtkArrayCalculator::SetAttributeMode*` and `VTK_ATTRIBUTE_MODE_*` macros
    (deprecated in Jun 2017)
  - `vtkContourGrid::ComputeGradients` (deprecated in Dec 2018)
  - `vtkSMPContourGridManyPieces`, `vtkSMPTransform`,
    `vtkThreadedSynchronizedTemplates3D`, and
    `vtkThreadedSynchronizedTemplatesCutter3D` (deprecated in Sep 2017)
  - `vtkAbstractImageInterpolator::GetWholeExtent` (deprecated in Mar 2016)
  - `vtkImageStencilData::InsertLine` (an overload) (deprecated in Nov 2014)
  - The `RemoveBlockVisibilites` method from
    `vtkCompositeDataDisplayAttributes`,
    `vtkCompositeDataDisplayAttributesLegacy`, and
    `vtkCompositePolyDataMapper2` (deprecated in Jul 2017)
  - `vtkOpenVRPropPicker` (deprecated in Apr 2018)
