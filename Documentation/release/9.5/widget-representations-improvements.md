# Implicit Widget Representations Improvements

VTK has better support for several vtkWidgetRepresentation subclasses.

## Bug fixes

Interactions are smoother thanks to a better Picker choice.
Also, interactively modifying radius (for Annulus, Cone and Cylinder) used to
be based on screen motion. Switching to mouse world position makes it
more intuitive: circle effectively follow the mouse cursor.

## New BoundedWidget design

vtkBoundedWidgetRepresentation is a new vtkWidgetRepresentation subclass,
to inherits from to benefit of a "crop at outline" feature.
This is useful for most vtkImplicitFunction representations.

## API breaking changes

The classes that were ported to the vtkBoundedWidgetRepresentation design
loose some protected member, as the new parent class has same member as `private`.
Please use the Getters instead.

Concerned classes:  vtkImplicitConeRepresentation, vtkImplicitCylinderRepresentation,
vtkImplicitAnnulusRepresentation and vtkImplicitPlaneRepresentation
