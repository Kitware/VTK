"""Pythonic API for vtkMolecule.

Adds iteration, len, array properties, and interop with ASE and RDKit::

    from vtkmodules.vtkCommonDataModel import vtkMolecule

    # Construction from arrays:
    mol = vtkMolecule(atomic_numbers=[1, 1, 8],
                      positions=[[0, 0, 0], [1, 0, 0], [0.5, 0.8, 0]])
    mol = vtkMolecule(symbols=['H', 'H', 'O'],
                      positions=[[0, 0, 0], [1, 0, 0], [0.5, 0.8, 0]])

    # Sequence protocol:
    len(mol)                 # atom count
    mol[0]                   # vtkAtom proxy
    for atom in mol: ...     # iterate atoms

    # Array properties:
    mol.positions            # (N, 3) float32 numpy array
    mol.atomic_numbers       # (N,) uint16 numpy array
    mol.bond_orders          # (N_bonds,) uint16 numpy array

    # Views:
    mol.atoms[2]             # indexable atom view
    mol.bonds[0]             # indexable bond view

    # Data:
    mol.atom_data            # vtkDataSetAttributes
    mol.bond_data            # vtkDataSetAttributes

    # Interop:
    ase_atoms = mol.to_ase()
    mol2 = vtkMolecule().from_ase(ase_atoms)
    rdmol = mol.to_rdkit()
    mol3 = vtkMolecule().from_rdkit(rdmol)
"""

from vtkmodules.vtkCommonDataModel import vtkMolecule

_periodic_table = None


def _get_periodic_table():
    """Return a lazily-created singleton vtkPeriodicTable."""
    global _periodic_table
    if _periodic_table is None:
        from vtkmodules.vtkDomainsChemistry import vtkPeriodicTable

        _periodic_table = vtkPeriodicTable()
    return _periodic_table


# -- Bond order mapping for RDKit interop --

_RDKIT_BOND_TYPE_TO_VTK_ORDER = {
    "SINGLE": 1,
    "DOUBLE": 2,
    "TRIPLE": 3,
    "AROMATIC": 4,
}

_VTK_ORDER_TO_RDKIT_BOND_TYPE = {v: k for k, v in _RDKIT_BOND_TYPE_TO_VTK_ORDER.items()}


# -- Views --

class _AtomView:
    """Indexable, iterable view of atoms in a vtkMolecule."""

    __slots__ = ("_mol",)

    def __init__(self, mol):
        self._mol = mol

    def __len__(self):
        return self._mol.GetNumberOfAtoms()

    def __getitem__(self, key):
        n = self._mol.GetNumberOfAtoms()
        if isinstance(key, slice):
            return [self._mol.GetAtom(i) for i in range(*key.indices(n))]
        idx = key + n if key < 0 else key
        if idx < 0 or idx >= n:
            raise IndexError("atom index %d out of range [0, %d)" % (key, n))
        return self._mol.GetAtom(idx)

    def __iter__(self):
        for i in range(self._mol.GetNumberOfAtoms()):
            yield self._mol.GetAtom(i)

    def __repr__(self):
        return "_AtomView(%d atoms)" % len(self)


class _BondView:
    """Indexable, iterable view of bonds in a vtkMolecule."""

    __slots__ = ("_mol",)

    def __init__(self, mol):
        self._mol = mol

    def __len__(self):
        return self._mol.GetNumberOfBonds()

    def __getitem__(self, key):
        n = self._mol.GetNumberOfBonds()
        if isinstance(key, slice):
            return [self._mol.GetBond(i) for i in range(*key.indices(n))]
        idx = key + n if key < 0 else key
        if idx < 0 or idx >= n:
            raise IndexError("bond index %d out of range [0, %d)" % (key, n))
        return self._mol.GetBond(idx)

    def __iter__(self):
        for i in range(self._mol.GetNumberOfBonds()):
            yield self._mol.GetBond(i)

    def __repr__(self):
        return "_BondView(%d bonds)" % len(self)


# -- Mixin --

class _MoleculeMixin:

    def __init__(self, *args, atomic_numbers=None, symbols=None, positions=None,
                 **kwargs):
        super().__init__(*args, **kwargs)
        if positions is not None:
            import numpy as np

            positions = np.asarray(positions, dtype=np.float32)
            if positions.ndim == 1:
                positions = positions.reshape(-1, 3)
            n = len(positions)

            if atomic_numbers is not None:
                nums = np.asarray(atomic_numbers, dtype=np.uint16)
            elif symbols is not None:
                pt = _get_periodic_table()
                nums = np.array([pt.GetAtomicNumber(s) for s in symbols],
                                dtype=np.uint16)
            else:
                nums = np.ones(n, dtype=np.uint16)

            if len(nums) != n:
                which = "atomic_numbers" if atomic_numbers is not None else "symbols"
                raise ValueError(
                    "%s length %d != positions length %d"
                    % (which, len(nums), n)
                )

            for i in range(n):
                self.AppendAtom(int(nums[i]),
                                float(positions[i, 0]),
                                float(positions[i, 1]),
                                float(positions[i, 2]))
        elif atomic_numbers is not None or symbols is not None:
            raise ValueError(
                "positions must be provided when atomic_numbers or symbols are given"
            )

    # -- Sequence protocol (atom-centric, like ASE) --

    def __len__(self):
        return self.GetNumberOfAtoms()

    def __getitem__(self, key):
        n = self.GetNumberOfAtoms()
        if isinstance(key, slice):
            return [self.GetAtom(i) for i in range(*key.indices(n))]
        idx = key + n if key < 0 else key
        if idx < 0 or idx >= n:
            raise IndexError("atom index %d out of range [0, %d)" % (key, n))
        return self.GetAtom(idx)

    def __iter__(self):
        for i in range(self.GetNumberOfAtoms()):
            yield self.GetAtom(i)

    # -- Views --

    @property
    def atoms(self):
        """Indexable, iterable view of atoms."""
        return _AtomView(self)

    @property
    def bonds(self):
        """Indexable, iterable view of bonds."""
        return _BondView(self)

    # -- Array properties --

    @property
    def positions(self):
        """Atom positions as an (N, 3) float32 numpy array.

        This is a live view into VTK memory; mutating it changes the
        molecule in place (call ``Modified()`` afterwards to notify the
        pipeline). The view is invalidated if the atom array is
        reallocated (e.g. by ``AppendAtom``), so do not hold onto it
        across mutations. Use the ``positions`` setter for a safe,
        length-checked bulk assignment.
        """
        import numpy as np
        from vtkmodules.util.numpy_support import vtk_to_numpy

        pts = self.GetAtomicPositionArray()
        if pts is None or pts.GetNumberOfPoints() == 0:
            return np.empty((0, 3), dtype=np.float32)
        return vtk_to_numpy(pts.GetData()).reshape(-1, 3)

    @positions.setter
    def positions(self, value):
        """Set atom positions from an (N, 3) array."""
        import numpy as np
        from vtkmodules.util.numpy_support import vtk_to_numpy

        pos = np.asarray(value, dtype=np.float32)
        if pos.ndim == 1:
            pos = pos.reshape(-1, 3)
        n = self.GetNumberOfAtoms()
        if len(pos) != n:
            raise ValueError(
                "positions array length %d != atom count %d" % (len(pos), n)
            )
        if n:
            # Bulk-assign into the underlying (N, 3) array view.
            vtk_to_numpy(self.GetAtomicPositionArray().GetData())[:] = pos
            self.Modified()

    @property
    def atomic_numbers(self):
        """Atomic numbers as an (N,) uint16 numpy array.

        This is a live view into VTK memory; mutating it changes the
        molecule in place (call ``Modified()`` afterwards to notify the
        pipeline). The view is invalidated if the atom array is
        reallocated (e.g. by ``AppendAtom``), so do not hold onto it
        across mutations. Use the ``atomic_numbers`` setter for a safe,
        length-checked bulk assignment.
        """
        import numpy as np
        from vtkmodules.util.numpy_support import vtk_to_numpy

        arr = self.GetAtomicNumberArray()
        if arr is None or arr.GetNumberOfTuples() == 0:
            return np.empty(0, dtype=np.uint16)
        return vtk_to_numpy(arr)

    @atomic_numbers.setter
    def atomic_numbers(self, value):
        """Set atomic numbers from an (N,) array."""
        import numpy as np
        from vtkmodules.util.numpy_support import vtk_to_numpy

        nums = np.asarray(value, dtype=np.uint16)
        n = self.GetNumberOfAtoms()
        if len(nums) != n:
            raise ValueError(
                "atomic_numbers array length %d != atom count %d" % (len(nums), n)
            )
        if n:
            vtk_to_numpy(self.GetAtomicNumberArray())[:] = nums
            self.Modified()

    @property
    def bond_orders(self):
        """Bond orders as an (N_bonds,) uint16 numpy array.

        This is a live view into VTK memory; mutating it changes the
        molecule in place (call ``Modified()`` afterwards to notify the
        pipeline). The view is invalidated if the bond array is
        reallocated (e.g. by ``AppendBond``), so do not hold onto it
        across mutations.
        """
        import numpy as np
        from vtkmodules.util.numpy_support import vtk_to_numpy

        arr = self.GetBondOrdersArray()
        if arr is None or arr.GetNumberOfTuples() == 0:
            return np.empty(0, dtype=np.uint16)
        return vtk_to_numpy(arr)

    @property
    def symbols(self):
        """Element symbols as a list of strings.

        Requires vtkDomainsChemistry.
        """
        pt = _get_periodic_table()
        return [pt.GetSymbol(self.GetAtomAtomicNumber(i))
                for i in range(self.GetNumberOfAtoms())]

    @property
    def formula(self):
        """Molecular formula in Hill system order.

        Requires vtkDomainsChemistry.
        """
        from collections import Counter

        counts = Counter(self.symbols)

        def _term(elem, c):
            return elem if c == 1 else "%s%d" % (elem, c)

        parts = []
        # Hill system: when carbon is present, C comes first, then H, then
        # the remaining elements alphabetically.  When there is no carbon,
        # all elements (including H) are ordered alphabetically.
        if "C" in counts:
            parts.append(_term("C", counts.pop("C")))
            if "H" in counts:
                parts.append(_term("H", counts.pop("H")))
        for elem in sorted(counts):
            parts.append(_term(elem, counts[elem]))
        return "".join(parts)

    # -- Data access --

    @property
    def atom_data(self):
        """Atom data arrays (vtkDataSetAttributes)."""
        return self.GetAtomData()

    @property
    def bond_data(self):
        """Bond data arrays (vtkDataSetAttributes)."""
        return self.GetBondData()

    # -- Mutation --

    def append(self, atomic_number_or_symbol, position):
        """Append an atom and return the vtkAtom proxy.

        Parameters
        ----------
        atomic_number_or_symbol : int or str
            Atomic number or element symbol.
        position : sequence of 3 floats
            (x, y, z) position.
        """
        if isinstance(atomic_number_or_symbol, str):
            pt = _get_periodic_table()
            num = pt.GetAtomicNumber(atomic_number_or_symbol)
        else:
            num = int(atomic_number_or_symbol)
        return self.AppendAtom(num, float(position[0]), float(position[1]),
                               float(position[2]))

    def add_bond(self, atom1, atom2, order=1):
        """Add a bond between two atoms.

        Parameters
        ----------
        atom1, atom2 : int
            Atom indices.
        order : int
            Bond order (1=single, 2=double, 3=triple).
        """
        return self.AppendBond(int(atom1), int(atom2), int(order))

    # -- Lattice --

    @property
    def lattice(self):
        """Unit cell lattice as a 3x3 numpy array, or None if not set.

        VTK stores lattice vectors as columns of a 3x3 matrix.
        """
        if not self.HasLattice():
            return None
        import numpy as np

        mat = self.GetLattice()
        result = np.empty((3, 3), dtype=np.float64)
        for i in range(3):
            for j in range(3):
                result[i, j] = mat.GetElement(i, j)
        return result

    @lattice.setter
    def lattice(self, value):
        """Set or clear the unit cell lattice.

        Parameters
        ----------
        value : (3, 3) array-like or None
            Lattice vectors as columns. None clears the lattice.
        """
        if value is None:
            self.ClearLattice()
            return
        import numpy as np
        from vtkmodules.vtkCommonMath import vtkMatrix3x3

        mat_data = np.asarray(value, dtype=np.float64)
        mat = vtkMatrix3x3()
        for i in range(3):
            for j in range(3):
                mat.SetElement(i, j, float(mat_data[i, j]))
        self.SetLattice(mat)

    @property
    def lattice_origin(self):
        """Lattice origin as a (3,) numpy array, or None if no lattice is set.

        The origin is only meaningful together with a lattice, so this
        mirrors ``lattice`` and returns None when ``HasLattice()`` is false.
        """
        if not self.HasLattice():
            return None
        import numpy as np

        origin = self.GetLatticeOrigin()
        return np.array([origin[0], origin[1], origin[2]], dtype=np.float64)

    @lattice_origin.setter
    def lattice_origin(self, value):
        """Set the lattice origin."""
        from vtkmodules.vtkCommonDataModel import vtkVector3d

        v = vtkVector3d(float(value[0]), float(value[1]), float(value[2]))
        self.SetLatticeOrigin(v)

    # -- Repr --

    def __repr__(self):
        na = self.GetNumberOfAtoms()
        nb = self.GetNumberOfBonds()
        try:
            f = self.formula
            return "vtkMolecule(%s, %d atoms, %d bonds)" % (f, na, nb)
        except Exception:
            return "vtkMolecule(%d atoms, %d bonds)" % (na, nb)

    # -- ASE interop --

    def to_ase(self):
        """Convert to an ASE Atoms object.

        Positions and atomic numbers are copied. If a lattice is set,
        it is transferred as the ASE cell (transposed, since VTK stores
        vectors as columns and ASE as rows) and ``pbc=True``.

        Bonds are not transferred (ASE does not store bonds).
        """
        try:
            from ase import Atoms
        except ImportError:
            raise ImportError(
                "ASE is required for to_ase(). Install it with: pip install ase"
            )
        import numpy as np

        positions = np.array(self.positions, dtype=np.float64)
        numbers = np.array(self.atomic_numbers, dtype=int)

        kwargs = {"numbers": numbers, "positions": positions}
        if self.HasLattice():
            # VTK: vectors as columns → ASE: vectors as rows → transpose
            kwargs["cell"] = self.lattice.T
            kwargs["pbc"] = True

        return Atoms(**kwargs)

    def from_ase(self, atoms):
        """Populate this molecule from an ASE Atoms object.

        Existing atoms and bonds are cleared first. Bonds are not set
        (use ``vtkSimpleBondPerceiver`` to infer them). Returns ``self``
        so it can be chained, e.g. ``vtkMolecule().from_ase(atoms)``.
        """
        try:
            from ase import Atoms as _Atoms  # noqa: F401
        except ImportError:
            raise ImportError(
                "ASE is required for from_ase(). Install it with: pip install ase"
            )
        import numpy as np

        self.Initialize()
        numbers = np.asarray(atoms.get_atomic_numbers(), dtype=np.uint16)
        positions = np.asarray(atoms.get_positions(), dtype=np.float32)
        for i in range(len(numbers)):
            self.AppendAtom(int(numbers[i]), float(positions[i, 0]),
                            float(positions[i, 1]), float(positions[i, 2]))

        cell = atoms.get_cell()
        if cell is not None and np.any(cell):
            # ASE: vectors as rows → VTK: vectors as columns → transpose
            self.lattice = np.asarray(cell).T

        return self

    # -- RDKit interop --

    def to_rdkit(self, sanitize=False):
        """Convert to an RDKit Mol object.

        Atoms, bonds, and 3D coordinates (as a conformer) are transferred.
        Bond order mapping: 1→SINGLE, 2→DOUBLE, 3→TRIPLE, 4→AROMATIC.

        Parameters
        ----------
        sanitize : bool
            If True, run ``Chem.SanitizeMol`` on the result. The returned
            mol is otherwise unsanitized: perception of rings, valence, and
            aromaticity has not been run, and some downstream RDKit
            operations require a sanitized mol. Sanitization is off by
            default because molecules derived from geometry or bond
            perceivers need not satisfy RDKit's valence model, and
            ``SanitizeMol`` would raise on them.
        """
        try:
            from rdkit import Chem
        except ImportError:
            raise ImportError(
                "RDKit is required for to_rdkit(). "
                "Install it with: pip install rdkit"
            )

        emol = Chem.RWMol()

        # Add atoms
        for i in range(self.GetNumberOfAtoms()):
            atom = Chem.Atom(int(self.GetAtomAtomicNumber(i)))
            emol.AddAtom(atom)

        # Add bonds
        bond_type_map = {
            1: Chem.BondType.SINGLE,
            2: Chem.BondType.DOUBLE,
            3: Chem.BondType.TRIPLE,
            4: Chem.BondType.AROMATIC,
        }
        for i in range(self.GetNumberOfBonds()):
            bond = self.GetBond(i)
            order = int(bond.GetOrder())
            bt = bond_type_map.get(order, Chem.BondType.SINGLE)
            begin = int(bond.GetBeginAtomId())
            end = int(bond.GetEndAtomId())
            emol.AddBond(begin, end, bt)
            # Aromatic bonds require their endpoints to be flagged aromatic,
            # otherwise RDKit treats the mol as inconsistent.
            if bt == Chem.BondType.AROMATIC:
                emol.GetAtomWithIdx(begin).SetIsAromatic(True)
                emol.GetAtomWithIdx(end).SetIsAromatic(True)

        mol = emol.GetMol()
        if sanitize:
            Chem.SanitizeMol(mol)

        # Add 3D conformer
        conf = Chem.Conformer(self.GetNumberOfAtoms())
        positions = self.positions
        for i in range(len(positions)):
            conf.SetAtomPosition(i, (float(positions[i, 0]),
                                     float(positions[i, 1]),
                                     float(positions[i, 2])))
        mol.AddConformer(conf, assignId=True)

        return mol

    def from_rdkit(self, rdkit_mol, conformer_id=-1):
        """Populate this molecule from an RDKit Mol object.

        Existing atoms and bonds are cleared first. Returns ``self`` so it
        can be chained, e.g. ``vtkMolecule().from_rdkit(rdmol)``.

        Parameters
        ----------
        rdkit_mol : rdkit.Chem.Mol
            Source molecule.
        conformer_id : int
            Which conformer to use for positions (-1 = first/default).
            If no conformer exists, positions default to (0, 0, 0).
        """
        try:
            from rdkit import Chem
        except ImportError:
            raise ImportError(
                "RDKit is required for from_rdkit(). "
                "Install it with: pip install rdkit"
            )
        import numpy as np

        self.Initialize()
        n_atoms = rdkit_mol.GetNumAtoms()
        numbers = np.array(
            [rdkit_mol.GetAtomWithIdx(i).GetAtomicNum() for i in range(n_atoms)],
            dtype=np.uint16,
        )

        # Get positions from conformer if available
        if rdkit_mol.GetNumConformers() > 0:
            conf = rdkit_mol.GetConformer(conformer_id)
            positions = np.array(
                [conf.GetAtomPosition(i) for i in range(n_atoms)],
                dtype=np.float32,
            )
        else:
            positions = np.zeros((n_atoms, 3), dtype=np.float32)

        for i in range(n_atoms):
            self.AppendAtom(int(numbers[i]), float(positions[i, 0]),
                            float(positions[i, 1]), float(positions[i, 2]))

        # Add bonds
        order_map = {
            Chem.BondType.SINGLE: 1,
            Chem.BondType.DOUBLE: 2,
            Chem.BondType.TRIPLE: 3,
            Chem.BondType.AROMATIC: 4,
        }
        for bond in rdkit_mol.GetBonds():
            order = order_map.get(bond.GetBondType(), 1)
            self.AppendBond(bond.GetBeginAtomIdx(), bond.GetEndAtomIdx(), order)

        return self


# -- Apply override --

@vtkMolecule.override
class Molecule(_MoleculeMixin, vtkMolecule):
    pass
