# `IOChemistry` Module

A new `IOChemistry` module has been added that contains all chemistry-related readers for improved cohesion.

## Moved Classes
* vtkCMLMoleculeReader: From DomainsChemistry to IOChemistry
* vtkGaussianCubeReader: From IOGeometry to IOChemistry
* vtkGaussianCubeReader2: From IOGeometry to IOChemistry
* vtkMoleculeReaderBase: From IOGeometry to IOChemistry
* vtkPDBReader: From From IOGeometry to IOChemistry
* vtkVASPAnimationReader: From DomainsChemistry to IOChemistry
* vtkVASPTessellationReader: From DomainsChemistry to IOChemistry
* vtkXYZMolReader: From IOGeometry to IOChemistry
* vtkXYZMolReader2: From IOGeometry to IOChemistry

## Refactored `vtkMoleculeReaderBase`

`vtkMoleculeReaderBase` was an old class that needed updating. There was also a bug where atom types were one less than they should be (for example, if you open a PDB file with ParaView you will see that atom types are wrong and rgb colors are incorrect). The class was changed to use the vtkPeriodicTable Blue Obelisk dataset instead of the previous hardcoded static arrays to reduce code duplication and improve accuracy.

* Used smart pointers
* Improved variable names
* Improved documentation
* Improved structure
* Replaced static arrays with `vtkPeriodicTable` in `vtkMoleculeReaderBase`
* Fixed `atomType` values being one less in `vtkMoleculeReaderBase`
* Updated `vtkProteinRibbonFilter` according to the `atomType` changes
* Added documentation to `vtkMoleculeReaderBase::ReadMolecule()`, `MakeAtomType()`, `MakeBonds()`
* Rename `vtkMoleculeReaderBase::MakeBonds()` `newPoints` parameter to `points` for clarity
* Changed "scanning" to "reading" in debug output
