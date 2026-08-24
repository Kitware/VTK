# VTK Key Press Repeat Counts

Previously, the Windows and remote window interactors started their repeat count at 0; others started at 1. Now, across all platforms, when the window interactors handle a key press (or key down) event, the repeat count starts at 0. When holding down a key, if it is considered a repeat (new key serial matches the previous key serial), the repeat count is increased by 1. When the key changes, the repeat count is set back to 0. This matches mouse event behavior.
