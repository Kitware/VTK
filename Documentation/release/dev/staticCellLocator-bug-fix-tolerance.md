# Bug fix: staticCellLocator's binner tolerance for FindCellsAlongLine

Previously the tolerance was not used for intersection with boxes and detection of the end condition of the bins to process.
This could result in very different (and incorrect) result just by shifting very slightly a point of the line, even in the same direction.
Changing the tolerance could not solve the issue.

Behavior is now correct when tolerance is high enough to detect the previously ignored intersections.
