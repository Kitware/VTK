# StaticCellLocator Binner tolerance behavior default modification

The internal CellBinner of the vtkStaticCellLocator used to compute
a tolerance based only the diagolan length of the dataset. It will now use
the locator tolerance by default instead. To still
take the diagonal length of the dataset into account, a new boolean has been added:
UseDiagonalLengthTolerance
