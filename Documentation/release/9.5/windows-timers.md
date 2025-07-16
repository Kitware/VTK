# High precision Windows timers

The Windows interactor was creating timers using the native Win32 API `SetTimer`.
However, this timer isn't precise and the duration wasn't respected for short duration.
It now uses `CreateTimerQueueTimer` API which is much more precise.
