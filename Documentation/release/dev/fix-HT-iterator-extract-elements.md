# Fix HyperTree iterator in ExtractElements method

This branch fixes a bug when using Hover On Cell on a composite HTG Hercule base.
The indexes used in the ExtractElements method in order to sanitize the output HTG can be wrong for some bases.
One need to use the appropriate HyperTreeGridIterator to get correct indexes.
