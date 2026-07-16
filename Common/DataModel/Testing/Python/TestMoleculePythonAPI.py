"""Tests for the Pythonic vtkMolecule API."""

import unittest

import numpy as np
from vtkmodules.vtkCommonDataModel import vtkMolecule


def _build_water():
    """Build a water molecule: O + 2 H, 2 bonds."""
    mol = vtkMolecule()
    o = mol.AppendAtom(8, 0.0, 0.0, 0.0)
    h1 = mol.AppendAtom(1, 0.96, 0.0, 0.0)
    h2 = mol.AppendAtom(1, -0.24, 0.93, 0.0)
    mol.AppendBond(o.GetId(), h1.GetId(), 1)
    mol.AppendBond(o.GetId(), h2.GetId(), 1)
    return mol


class TestSequenceProtocol(unittest.TestCase):
    def test_len(self):
        mol = _build_water()
        self.assertEqual(len(mol), 3)

    def test_getitem_positive(self):
        mol = _build_water()
        atom = mol[0]
        self.assertEqual(atom.GetAtomicNumber(), 8)

    def test_getitem_negative(self):
        mol = _build_water()
        atom = mol[-1]
        self.assertEqual(atom.GetAtomicNumber(), 1)

    def test_getitem_slice(self):
        mol = _build_water()
        atoms = mol[0:2]
        self.assertEqual(len(atoms), 2)
        self.assertEqual(atoms[0].GetAtomicNumber(), 8)
        self.assertEqual(atoms[1].GetAtomicNumber(), 1)

    def test_getitem_out_of_range(self):
        mol = _build_water()
        with self.assertRaises(IndexError):
            mol[10]

    def test_iter(self):
        mol = _build_water()
        numbers = [a.GetAtomicNumber() for a in mol]
        self.assertEqual(numbers, [8, 1, 1])


class TestAtomView(unittest.TestCase):
    def test_len(self):
        mol = _build_water()
        self.assertEqual(len(mol.atoms), 3)

    def test_getitem(self):
        mol = _build_water()
        self.assertEqual(mol.atoms[0].GetAtomicNumber(), 8)

    def test_getitem_negative(self):
        mol = _build_water()
        self.assertEqual(mol.atoms[-1].GetAtomicNumber(), 1)

    def test_iter(self):
        mol = _build_water()
        nums = [a.GetAtomicNumber() for a in mol.atoms]
        self.assertEqual(nums, [8, 1, 1])

    def test_slice(self):
        mol = _build_water()
        atoms = mol.atoms[1:]
        self.assertEqual(len(atoms), 2)


class TestBondView(unittest.TestCase):
    def test_len(self):
        mol = _build_water()
        self.assertEqual(len(mol.bonds), 2)

    def test_getitem(self):
        mol = _build_water()
        bond = mol.bonds[0]
        self.assertEqual(bond.GetOrder(), 1)

    def test_getitem_negative(self):
        mol = _build_water()
        bond = mol.bonds[-1]
        self.assertEqual(bond.GetOrder(), 1)

    def test_iter(self):
        mol = _build_water()
        orders = [b.GetOrder() for b in mol.bonds]
        self.assertEqual(orders, [1, 1])


class TestArrayProperties(unittest.TestCase):
    def test_positions_shape_dtype(self):
        mol = _build_water()
        pos = mol.positions
        self.assertEqual(pos.shape, (3, 3))
        self.assertEqual(pos.dtype, np.float32)

    def test_positions_values(self):
        mol = _build_water()
        pos = mol.positions
        np.testing.assert_allclose(pos[0], [0.0, 0.0, 0.0], atol=1e-6)
        np.testing.assert_allclose(pos[1], [0.96, 0.0, 0.0], atol=1e-6)

    def test_positions_setter(self):
        mol = _build_water()
        new_pos = np.array([[1, 2, 3], [4, 5, 6], [7, 8, 9]], dtype=np.float32)
        mol.positions = new_pos
        np.testing.assert_allclose(mol.positions, new_pos, atol=1e-6)

    def test_positions_setter_wrong_size(self):
        mol = _build_water()
        with self.assertRaises(ValueError):
            mol.positions = np.zeros((5, 3), dtype=np.float32)

    def test_atomic_numbers_shape_dtype(self):
        mol = _build_water()
        nums = mol.atomic_numbers
        self.assertEqual(nums.shape, (3,))
        self.assertEqual(nums.dtype, np.uint16)

    def test_atomic_numbers_values(self):
        mol = _build_water()
        np.testing.assert_array_equal(mol.atomic_numbers, [8, 1, 1])

    def test_atomic_numbers_setter(self):
        mol = _build_water()
        mol.atomic_numbers = np.array([6, 7, 8], dtype=np.uint16)
        np.testing.assert_array_equal(mol.atomic_numbers, [6, 7, 8])

    def test_bond_orders(self):
        mol = _build_water()
        orders = mol.bond_orders
        self.assertEqual(orders.shape, (2,))
        np.testing.assert_array_equal(orders, [1, 1])


class TestSymbolsFormula(unittest.TestCase):
    def setUp(self):
        try:
            from vtkmodules.vtkDomainsChemistry import vtkPeriodicTable  # noqa: F401
            self.has_chemistry = True
        except ImportError:
            self.has_chemistry = False

    def test_symbols(self):
        if not self.has_chemistry:
            self.skipTest("vtkDomainsChemistry not available")
        mol = _build_water()
        self.assertEqual(mol.symbols, ["O", "H", "H"])

    def test_formula(self):
        if not self.has_chemistry:
            self.skipTest("vtkDomainsChemistry not available")
        mol = _build_water()
        self.assertEqual(mol.formula, "H2O")

    def test_formula_organic(self):
        if not self.has_chemistry:
            self.skipTest("vtkDomainsChemistry not available")
        # Methane: CH4
        mol = vtkMolecule()
        mol.AppendAtom(6, 0.0, 0.0, 0.0)
        for _ in range(4):
            mol.AppendAtom(1, 1.0, 0.0, 0.0)
        self.assertEqual(mol.formula, "CH4")


class TestRepr(unittest.TestCase):
    def test_repr_contains_atoms_bonds(self):
        mol = _build_water()
        r = repr(mol)
        self.assertIn("atoms", r)
        self.assertIn("bonds", r)

    def test_repr_empty(self):
        mol = vtkMolecule()
        r = repr(mol)
        self.assertIn("0 atoms", r)


class TestConstructor(unittest.TestCase):
    def test_empty(self):
        mol = vtkMolecule()
        self.assertEqual(len(mol), 0)

    def test_from_atomic_numbers(self):
        mol = vtkMolecule(
            atomic_numbers=[8, 1, 1],
            positions=[[0, 0, 0], [1, 0, 0], [0, 1, 0]],
        )
        self.assertEqual(len(mol), 3)
        np.testing.assert_array_equal(mol.atomic_numbers, [8, 1, 1])

    def test_from_symbols(self):
        try:
            from vtkmodules.vtkDomainsChemistry import vtkPeriodicTable  # noqa: F401
        except ImportError:
            self.skipTest("vtkDomainsChemistry not available")
        mol = vtkMolecule(
            symbols=["O", "H", "H"],
            positions=[[0, 0, 0], [1, 0, 0], [0, 1, 0]],
        )
        self.assertEqual(len(mol), 3)
        np.testing.assert_array_equal(mol.atomic_numbers, [8, 1, 1])

    def test_positions_without_numbers_raises(self):
        # If no positions are given but atomic_numbers are, should raise
        with self.assertRaises(ValueError):
            vtkMolecule(atomic_numbers=[1, 2])

    def test_mismatched_atomic_numbers_raises(self):
        # A length mismatch must fail cleanly, not leave a partial molecule.
        with self.assertRaises(ValueError):
            vtkMolecule(
                atomic_numbers=[8, 1],
                positions=[[0, 0, 0], [1, 0, 0], [0, 1, 0]],
            )

    def test_mismatched_symbols_raises(self):
        try:
            from vtkmodules.vtkDomainsChemistry import vtkPeriodicTable  # noqa: F401
        except ImportError:
            self.skipTest("vtkDomainsChemistry not available")
        with self.assertRaises(ValueError):
            vtkMolecule(
                symbols=["O", "H"],
                positions=[[0, 0, 0], [1, 0, 0], [0, 1, 0]],
            )


class TestMutation(unittest.TestCase):
    def test_append_by_number(self):
        mol = vtkMolecule()
        atom = mol.append(6, [0, 0, 0])
        self.assertEqual(len(mol), 1)
        self.assertEqual(atom.GetAtomicNumber(), 6)

    def test_append_by_symbol(self):
        try:
            from vtkmodules.vtkDomainsChemistry import vtkPeriodicTable  # noqa: F401
        except ImportError:
            self.skipTest("vtkDomainsChemistry not available")
        mol = vtkMolecule()
        atom = mol.append("C", [0, 0, 0])
        self.assertEqual(atom.GetAtomicNumber(), 6)

    def test_add_bond(self):
        mol = vtkMolecule()
        mol.AppendAtom(6, 0.0, 0.0, 0.0)
        mol.AppendAtom(8, 1.0, 0.0, 0.0)
        bond = mol.add_bond(0, 1, order=2)
        self.assertEqual(len(mol.bonds), 1)
        self.assertEqual(bond.GetOrder(), 2)


class TestDataAccess(unittest.TestCase):
    def test_atom_data(self):
        mol = _build_water()
        self.assertIsNotNone(mol.atom_data)

    def test_bond_data(self):
        mol = _build_water()
        self.assertIsNotNone(mol.bond_data)


class TestLattice(unittest.TestCase):
    def test_no_lattice(self):
        mol = _build_water()
        self.assertIsNone(mol.lattice)

    def test_set_get_lattice(self):
        mol = _build_water()
        lat = np.eye(3) * 10.0
        mol.lattice = lat
        np.testing.assert_allclose(mol.lattice, lat, atol=1e-10)

    def test_clear_lattice(self):
        mol = _build_water()
        mol.lattice = np.eye(3) * 10.0
        self.assertTrue(mol.HasLattice())
        mol.lattice = None
        self.assertFalse(mol.HasLattice())
        self.assertIsNone(mol.lattice)

    def test_lattice_origin(self):
        mol = _build_water()
        mol.lattice = np.eye(3) * 10.0
        mol.lattice_origin = [1.0, 2.0, 3.0]
        origin = mol.lattice_origin
        np.testing.assert_allclose(origin, [1.0, 2.0, 3.0], atol=1e-10)

    def test_lattice_origin_none_without_lattice(self):
        mol = _build_water()
        self.assertIsNone(mol.lattice_origin)


class TestASEInterop(unittest.TestCase):
    def setUp(self):
        try:
            import ase  # noqa: F401
            self.has_ase = True
        except ImportError:
            self.has_ase = False

    def test_roundtrip(self):
        if not self.has_ase:
            self.skipTest("ASE not installed")
        mol = _build_water()
        atoms = mol.to_ase()
        self.assertEqual(len(atoms), 3)
        np.testing.assert_array_equal(atoms.get_atomic_numbers(), [8, 1, 1])

        mol2 = vtkMolecule().from_ase(atoms)
        self.assertEqual(len(mol2), 3)
        np.testing.assert_array_equal(mol2.atomic_numbers, [8, 1, 1])
        np.testing.assert_allclose(
            mol2.positions, mol.positions, atol=1e-5
        )

    def test_lattice_roundtrip(self):
        if not self.has_ase:
            self.skipTest("ASE not installed")
        mol = _build_water()
        # Non-symmetric (triclinic) cell so lat.T != lat and the
        # VTK-columns vs ASE-rows transpose is actually exercised.
        lat = np.array([[10, 1, 2], [0, 12, 3], [0, 0, 14]], dtype=np.float64)
        assert not np.array_equal(lat, lat.T)
        mol.lattice = lat

        atoms = mol.to_ase()
        # ASE cell has vectors as rows, VTK as columns → transposed
        np.testing.assert_allclose(atoms.get_cell()[:], lat.T, atol=1e-10)

        mol2 = vtkMolecule().from_ase(atoms)
        np.testing.assert_allclose(mol2.lattice, lat, atol=1e-5)


class TestRDKitInterop(unittest.TestCase):
    def setUp(self):
        try:
            from rdkit import Chem  # noqa: F401
            self.has_rdkit = True
        except ImportError:
            self.has_rdkit = False

    def test_roundtrip(self):
        if not self.has_rdkit:
            self.skipTest("RDKit not installed")
        mol = _build_water()
        rdmol = mol.to_rdkit()

        self.assertEqual(rdmol.GetNumAtoms(), 3)
        self.assertEqual(rdmol.GetNumBonds(), 2)
        self.assertEqual(rdmol.GetNumConformers(), 1)

        mol2 = vtkMolecule().from_rdkit(rdmol)
        self.assertEqual(len(mol2), 3)
        self.assertEqual(len(mol2.bonds), 2)
        np.testing.assert_allclose(
            mol2.positions, mol.positions, atol=1e-5
        )

    def test_no_conformer(self):
        if not self.has_rdkit:
            self.skipTest("RDKit not installed")
        from rdkit import Chem

        # Build an RDKit mol without conformer
        emol = Chem.RWMol()
        emol.AddAtom(Chem.Atom(6))
        emol.AddAtom(Chem.Atom(8))
        emol.AddBond(0, 1, Chem.BondType.DOUBLE)
        rdmol = emol.GetMol()

        mol = vtkMolecule().from_rdkit(rdmol)
        self.assertEqual(len(mol), 2)
        self.assertEqual(len(mol.bonds), 1)
        # Positions should be zeros
        np.testing.assert_allclose(mol.positions, np.zeros((2, 3)), atol=1e-10)

    def test_aromatic_sanitize(self):
        if not self.has_rdkit:
            self.skipTest("RDKit not installed")
        from rdkit import Chem

        # Benzene ring: 6 carbons, aromatic bonds (VTK order 4).
        mol = vtkMolecule()
        for i in range(6):
            mol.append("C", [float(i), 0.0, 0.0])
        for i in range(6):
            mol.add_bond(i, (i + 1) % 6, order=4)

        # Aromatic bonds flag their endpoints, so sanitization succeeds.
        rdmol = mol.to_rdkit(sanitize=True)
        self.assertEqual(rdmol.GetNumAtoms(), 6)
        self.assertEqual(rdmol.GetNumBonds(), 6)
        self.assertTrue(all(a.GetIsAromatic() for a in rdmol.GetAtoms()))


class TestEmptyMolecule(unittest.TestCase):
    def test_empty_positions(self):
        mol = vtkMolecule()
        pos = mol.positions
        self.assertEqual(pos.shape, (0, 3))

    def test_empty_atomic_numbers(self):
        mol = vtkMolecule()
        nums = mol.atomic_numbers
        self.assertEqual(nums.shape, (0,))

    def test_empty_bond_orders(self):
        mol = vtkMolecule()
        orders = mol.bond_orders
        self.assertEqual(orders.shape, (0,))

    def test_empty_iter(self):
        mol = vtkMolecule()
        self.assertEqual(list(mol), [])

    def test_empty_atoms_view(self):
        mol = vtkMolecule()
        self.assertEqual(len(mol.atoms), 0)
        self.assertEqual(len(mol.bonds), 0)


if __name__ == "__main__":
    unittest.main()
