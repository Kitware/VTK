## Fix vtkCompassWidget, vtkCompassRepresentation

Before this fix ```vtkCompassWidget``` and ```vtkCompassRepresentation``` were in a non-working state.

The following changes were applied:
- Moved ```vtkCompassWidget``` and ```vtkCompassRepresentation``` from ```Geovis/Core``` to  ```Interaction/Widgets``` module since the widget does not depend on anything else in ```Geovis/Core``` and vice versa. You can now use the widget without building VTK with the PROJ library.
- ```vtkCompassWidget``` and ```vtkCompassRepresentation``` have been fixed. They now react correctly to interaction with the mouse and render correctly.
- ```vtkSliderRepresentation``` and subclasses: Fixes were applied to correctly calculate the local coordinate for the slider position. They also now honour their ```Visibility``` parameter.
- In ```vtkCompassWidget``` you can now adjust the update ```TimerDuration``` as well as the ```TiltSpeed``` and the ```DistanceSpeed``` when clicking on the slider end caps.
