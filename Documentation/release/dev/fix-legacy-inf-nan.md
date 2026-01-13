# Fix legacy I/O handling of Inf/NaN values

VTK now properly handles reading of ASCII files in its "legacy" format
that contain Inf or NaN values. This has not worked in the past because
the std::istream `>>` operator used by the readers cannot parse them
even though the `<<` operator generates them.
