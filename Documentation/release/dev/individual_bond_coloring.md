## Color vtkMolecule bonds individually

`vtkMoleculeMapper` has a new bond color mode, `DiscreteByBond`, which colors every bond from a
per-bond array you provide.

Pass one tuple per bond, in the same order as the molecule's bonds, with `SetIndividualBondColors()`:

* 3 or 4 components are used directly as RGB or RGBA colors, for example a `vtkUnsignedCharArray`.
* 1 component is mapped through the lookup table you set with `SetBondLookupTable()`, over that
  table's range. Without a lookup table, a default one spanning the data range is used.

If the array is missing or does not have one tuple per bond, the mapper warns and draws the bonds
in `BondColor`.

```cpp
mapper->SetBondColorModeToDiscreteByBond();
mapper->SetBondLookupTable(lut);
mapper->SetIndividualBondColors(bondStrain); // one value per bond
```
