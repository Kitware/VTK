# Relative Mode for vtkTemporalArrayOperatorFilter

The `vtkTemporalArrayOperatorFilter` now has a `RelativeMode`, where it uses `UPDATE_TIME_STEP` time as first value alongside to a `TimeStepShift` shifted value, instead of two static time steps.

## Behavior Breaking Change

This filter was inconsistent on which input mesh and arrays were forwarded to output: Composite data used first input, while non composite used second.
The forwarded data is now the one given by `FirstTimeStepIndex` when `RelativeMode` is off or by `UPDATE_TIME_STEP` otherwise, as Composite used to do.

## Build Breaking Change

The protected variable members where moved to private. From inherited classes, please use Getter/Setter instead.
