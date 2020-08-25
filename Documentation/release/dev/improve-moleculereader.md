# Refactor `vtkMoleculeReaderBase`

`vtkMoleculeReaderBase` is an old class that needed updating. There was also a bug where atom types were one less than they should be (for example, if you open a PDB file with ParaView you will see that atom types are wrong and rgb colors are incorrect). The class was changed to use the `vtkPeriodicTable` Blue Obelisk dataset instead of the previous hardcoded static arrays to reduce code duplication and improve accuracy.

* Use smart pointers
* Improve variable names
* Improve comments
* Improve structure
* Replace static arrays with `vtkPeriodicTable` in `vtkMoleculeReaderBase`
* Fix atomType values being one less in `vtkMoleculeReaderBase`
* Update `vtkProteinRibbonFilter` according to the atomType changes
* Refactor `TestProteinRibbon`
* Add documentation to `vtkMoleculeReaderBase`: `ReadMolecule()`, `MakeAtomType()`, `MakeBonds()`
* Rename `vtkMoleculeReaderBase::MakeBonds()` newPoints parameter to points for clarity
* Change "scanning" to "reading" in debug output
