## Improvements related to vtkPolarAxesActor

### Ticks and axes handling

#### Radial axes and arc ticks

You can now either specify a ***number of radial axes*** or a ***delta angle*** between them to automatically generate them. If both are specified, it will try to add *number of radial axes* every *delta angle*. This means you can have less radial axes than specified, if your *delta angle* is too big, or not enough to fill your arc, if your *delta angle* is too small. By default, *number of radial axes* is set to 0 and *delta angle* is set to 45.0, which means it will generate as much radial axes it can every 45 degrees. See `SetRequestedNumberOfRadialAxes(int)` and `SetRequestedDeltaAngleRadialAxes(double)`.

By default, major arc ticks will match radial axes and minor ones will be the half of a major tick. You can disable this behavior to specify your own *delta angle major* and *delta angle minor* using `SetArcTickMatchesRadialAxes(bool)`, `SetDeltaAngleMajor(double)` and `SetDeltaAngleMinor(double)`. In both cases, it will fill the arc with as much ticks it can.

#### Polar axes and axis ticks

You can now either specify a ***number of polar axes*** or a ***delta range*** between them to automatically generate them, similarly to radial axes. If both are specified, it will try to add *number of polar axes* every *delta range*. This means you can have less polar axes than specified, if your *delta range* is too big, or not enough to fill your arc, if your *delta range* is too small. By default, *number of polar axes* is set to 5 and *delta range* is set to 0.0, which means it will generate 5 polar axes equally spaced. See `SetRequestedNumberOfPolarAxes(int)` and `SetRequestedDeltaRangePolarAxes(double)`.

By default, major axis ticks will match polar axes and minor ones will be the half of a major tick. You can disable this behavior to specify your own *delta range major* and *delta range minor* using `SetAxisTickMatchesPolarAxes(bool)`, `SetDeltaRangeMajor(double)` and `SetDeltaRangeMinor(double)`. In both cases, it will fill axes with as much ticks it can.

#### Ticks size

If you don't specify a size for one type of tick, its size will be computed as a ratio of *maximum radius*. You can now specify a value for this ratio. See `SetTickRatioRadiusSize(double)`, default is 0.02.

### Polar arcs resolution

You can now change polar arcs resolution per degree. See `SetPolarArcResolutionPerDegree(double)`, default is 0.2, which means one line will be drawn every 5 degrees. If your ratio is different from 1.0, resolution will depend on it. E.g your ratio is set to 2.0, your arcs will be larger so the resolution will be multiplied by 2.0.

### Text offsets

You can now change the offsets of titles, labels and exponent texts. Any previous hard-coded offset **has been removed** and replaced with better default values for those offsets. Titles have a 2D offset, while labels and exponent have only a Y offset. Be aware that X offset of titles is applied only when not using VTK_ALIGN_TOP or VTK_ALIGN_BOTTOM, which stay aligned to center (X-wise). See `SetPolarTitleOffset`, `SetRadialTitleOffset`, `SetPolarLabelOffset` and `SetPolarExponentOffset`.
