## Added new class to manually create a Cocoa NSAutoreleasePool

The new class vtkCocoaAutoreleasePool allows manually creating and draining a Cocoa autorelease pool.

The Cocoa event loop creates an NSAutoreleasePool at the beginning of every cycle of an event loop, and drains it at the end. But if you do lots of things before the end of the event loop, memory usage can temporarily grow high.

This class can be useful in scenarios where a loop creates, initializes, and then deletes a vtkCocoaRenderWindow, which doesn't yield back to the Cocoa event loop. Memory usage will thus increase with every iteration of such a loop. The memory will eventually be reclaimed (so there's no leak in that sense), but using this class allows manual control over that, reducing the high water mark of memory usage.

Other operating systems work differently, and so this class is only for macOS.
