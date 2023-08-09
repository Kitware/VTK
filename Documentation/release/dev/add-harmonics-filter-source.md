## Add spatio-temporal harmonics filter/source

### Filter

You can now use a new filter to compute spatio-temporal
harmonics on each point of your input dataset. This avoids
some manual computation through the calculator which would
be hard to follow.

This filter allows you to add multiple harmonics defined
by their amplitude, temporal frequency, wave vector, and
phase. The sum of them will be computed using the sinus
function for each point. Note that there is no cosinus in
this function.

### Source

You can now generate an image data with harmonics data.
This new source allows you to specify the uniform grid
extent. It also lets you choose the harmonics you want,
the same way as in the filter (it is embedded).

If no harmonic is specified, the source will generate an
image data with default harmonics values.
