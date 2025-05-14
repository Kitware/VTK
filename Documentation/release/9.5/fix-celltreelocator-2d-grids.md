# Fix CellTreeLocator for 2d grids

A `std::vector` out of bounds access has been fixed when building the locator
for meshes whose x, y or z extent is zero.
