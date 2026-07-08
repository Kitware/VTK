## Fix gesture events handling in vtkRemoteInteractionAdapter

The EndRotate event now triggers vtkCommand::EndRotateEvent.

The gesture events (Pinch, Pan, Rotate) now have a correct position, the
calculation was wrong.

## Factored calculation

You can use the vtkRemoteInteractionAdapter::physicalToLogicalPosition function
to calculate the position of an event in the vtkRenderWindow from a position of
the event in its web render window.

## Add gesture events in the TestRemoteInteractionAdapter

`remote_events.json` the input of TestRemoteInteractionAdapter now has Pinch,
Pan and Rotate events.

TestRemoteInteractionAdapter now uses a InteractorStyleMultiTouchCamera to test
these gesture events.
