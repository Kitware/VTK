## Fix event processing in X11 interactor

The `ProcessEvents` method now dispatches all pending messages from the x11 event queue. Earlier,
it dispatched exactly one pending message and returned.
