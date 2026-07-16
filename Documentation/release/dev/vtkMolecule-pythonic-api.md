## Pythonic API for vtkMolecule

vtkMolecule now has a Pythonic API with array properties, iteration,
constructors, and interoperability with ASE and RDKit.

### Construction

```python
from vtkmodules.vtkCommonDataModel import vtkMolecule

# From arrays:
mol = vtkMolecule(atomic_numbers=[8, 1, 1],
                  positions=[[0, 0, 0], [0.96, 0, 0], [-0.24, 0.93, 0]])

# From element symbols (requires DomainsChemistry):
mol = vtkMolecule(symbols=['O', 'H', 'H'],
                  positions=[[0, 0, 0], [0.96, 0, 0], [-0.24, 0.93, 0]])
```

### Sequence protocol and views

```python
len(mol)                 # 3 (atom count)
mol[0]                   # vtkAtom proxy for oxygen
mol[-1]                  # last atom
for atom in mol: ...     # iterate atoms

mol.atoms[1]             # indexable atom view
mol.bonds[0]             # indexable bond view
```

### Array properties

```python
mol.positions            # (N, 3) float32 numpy array
mol.atomic_numbers       # (N,) uint16 numpy array
mol.bond_orders          # (N_bonds,) uint16 numpy array (live view)
mol.symbols              # ['O', 'H', 'H']
mol.formula              # 'H2O' (Hill system)

mol.positions = new_positions   # setter
mol.atomic_numbers = new_nums   # setter
```

### Mutation

```python
mol.append(6, [0, 0, 0])        # append atom by atomic number
mol.append('C', [0, 0, 0])      # append atom by symbol
mol.add_bond(0, 1, order=2)     # add double bond
```

### Lattice

```python
mol.lattice = np.eye(3) * 10.0  # set unit cell
mol.lattice                      # 3x3 numpy array or None
mol.lattice_origin = [1, 2, 3]  # set origin
```

### ASE interop

```python
ase_atoms = mol.to_ase()                  # convert to ASE Atoms
mol = vtkMolecule().from_ase(ase_atoms)   # convert from ASE Atoms
```

Lattice is transposed on conversion (VTK stores vectors as columns,
ASE as rows). Bonds are not transferred to ASE.

### RDKit interop

```python
rdmol = mol.to_rdkit()                    # convert to RDKit Mol
mol = vtkMolecule().from_rdkit(rdmol)     # convert from RDKit Mol
```

Bond order mapping: SINGLE=1, DOUBLE=2, TRIPLE=3, AROMATIC=4.
3D coordinates are transferred via RDKit conformers.
