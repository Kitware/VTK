## Fix surface LIC mapper when using more than one viewport

Fixed a bug in the surface LIC mapper when using multiple viewports. The viewport origin was not correctly initialized, causing surface LIC mappers in viewports with `y_min` > 0.0 to appear at `y_min` = 0.0 instead, making them invisible or clipped.
