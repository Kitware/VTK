# ResliceCursorWidget

- Add missing listener on the FourPlaneViewer which uses ResliceCursor in order to allow rendering refresh when scrolling
- Fixed a few issues with the vtkResliceCursorRepresentation and added a vtkResliceCursorRepresentation::BoundPlane method. This method allows to show the entire resliced image when rotation (means no crop) and show zero padding (show only the resliced image and no more)
- Fix RotateVectorAboutVector function. When combinate several rotations, errors were added and shown bad orientation
- Update how to setup the viewup so that when apply multiple rotation, it will rotate around the correct axis
