# Updated vtkmCleanGrid to manage point and cell fields

The `vtkmCleanGrid` filter now supports all the features of the underlying
Viskores clean grid filter. This includes the ability to merge coincident points
and remove degenerate cells. It also correctly handles when point and cell
fields would change size, which was not always handled correctly before.
