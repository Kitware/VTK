import vtkmodules.test.Testing as vtkTesting
from vtkmodules.util.misc import deprecated
import warnings

class TestDeprecated(vtkTesting.vtkTest):
    def test_deprecated_function_warning(self):
        @deprecated(version="1.2", message="Use 'new_function' instead.")
        def old_function():
            return "old func"

        with warnings.catch_warnings(record=True) as warn_info:
            warnings.simplefilter("always")  # Trigger all warnings
            result = old_function()

            self.assertEqual(result, "old func")
            self.assertEqual(len(warn_info), 1)
            self.assertTrue(issubclass(warn_info[0].category, DeprecationWarning))
            self.assertEqual(
                str(warn_info[0].message),
                "Function 'old_function' is deprecated since version 1.2. " \
                "Use 'new_function' instead."
            )

    def test_non_deprecated_function(self):
        def new_function():
            return "new func"

        with warnings.catch_warnings(record=True) as warn_info:
            warnings.simplefilter("always")
            result = new_function()

            self.assertEqual(result, "new func")
            self.assertEqual(len(warn_info), 0)

if __name__ == "__main__":
    vtkTesting.main([(TestDeprecated, "test")])
