from vtkmodules.vtkCommonDataModel import vtkPolyData
from vtkmodules.vtkIOLegacy import vtkPolyDataReader, vtkPolyDataWriter
from vtkmodules.test import Testing

import locale
from pathlib import Path

class TestWriterPreservesLocale(Testing.vtkTest):
    def test(self):
        l1 = locale.getlocale()
        e1 = locale.getencoding()
        print(f"Locale before: {l1}, Encoding: {e1}")

        poly = vtkPolyData()
        writer = vtkPolyDataWriter()
        file_name = Path(Testing.VTK_TEMP_DIR) / "poly.vtk"
        writer.SetFileName(str(file_name.absolute()))
        writer.SetInputData(poly)
        writer.Write()

        l2 = locale.getlocale()
        e2 = locale.getencoding()
        print(f"Locale after: {l2}, Encoding: {e2}")
        self.assertEqual(l1, l2)
        self.assertEqual(e1, e2)


class TestReaderPreservesLocale(Testing.vtkTest):
    def test(self):
        l1 = locale.getlocale()
        e1 = locale.getencoding()
        print(f"Locale before: {l1}, Encoding: {e1}")

        reader = vtkPolyDataReader()
        file_name = Path(Testing.VTK_TEMP_DIR) / "poly.vtk"
        reader.SetFileName(str(file_name.absolute()))
        reader.Update()

        l2 = locale.getlocale()
        e2 = locale.getencoding()
        print(f"Locale after: {l2}, Encoding: {e2}")
        self.assertEqual(l1, l2)
        self.assertEqual(e1, e2)

if __name__ == "__main__":
    import sys
    if sys.version_info < (3,11):
        Testing.skip()
    else:
        Testing.main([(TestWriterPreservesLocale, 'test'),
                     (TestReaderPreservesLocale, 'test')])
