"""Test that enum property setters accept strings."""
from vtkmodules.vtkRenderingCore import vtkProperty
from vtkmodules.test import Testing


class TestEnumPropertySetter(Testing.vtkTest):
    def test_representation_string(self):
        """String assignment for representation enum."""
        prop = vtkProperty()
        prop.representation = "Surface"
        self.assertEqual(prop.GetRepresentation(), 2)

    def test_representation_case_insensitive(self):
        """Case-insensitive string assignment."""
        prop = vtkProperty()
        prop.representation = "wireframe"
        self.assertEqual(prop.GetRepresentation(), 1)
        prop.representation = "POINTS"
        self.assertEqual(prop.GetRepresentation(), 0)

    def test_representation_integer(self):
        """Integer assignment still works."""
        prop = vtkProperty()
        prop.representation = 1
        self.assertEqual(prop.GetRepresentation(), 1)

    def test_representation_invalid_string(self):
        """Invalid string raises ValueError."""
        prop = vtkProperty()
        with self.assertRaises(ValueError):
            prop.representation = "Invalid"

    def test_interpolation_string(self):
        """String assignment for interpolation enum."""
        prop = vtkProperty()
        prop.interpolation = "PBR"
        self.assertEqual(prop.GetInterpolation(), 3)
        prop.interpolation = "Flat"
        self.assertEqual(prop.GetInterpolation(), 0)
        prop.interpolation = "Gouraud"
        self.assertEqual(prop.GetInterpolation(), 1)
        prop.interpolation = "Phong"
        self.assertEqual(prop.GetInterpolation(), 2)

    def test_representation_roundtrip(self):
        """Verify getter returns integer after string assignment."""
        prop = vtkProperty()
        for name, expected in [("Points", 0), ("Wireframe", 1), ("Surface", 2)]:
            prop.representation = name
            self.assertEqual(prop.representation, expected)

    def test_keyword_constructor_with_string(self):
        """String enum values work in keyword constructor."""
        prop = vtkProperty(representation="Surface", interpolation="PBR")
        self.assertEqual(prop.GetRepresentation(), 2)
        self.assertEqual(prop.GetInterpolation(), 3)


if __name__ == "__main__":
    Testing.main([(TestEnumPropertySetter, "test")])
