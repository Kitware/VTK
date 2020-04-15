import vtk
from vtk.test import Testing

class TestPythonArchiver(Testing.vtkTest):
    def testArchiver(self):
        class MyArchiver(vtk.vtkArchiver):
            def __init__(self):
                vtk.vtkArchiver.__init__(self)
                self.calls = set()

            def OpenArchive(self, vtkself):
                self.calls.add('OpenArchive()')

            def CloseArchive(self, vtkself):
                self.calls.add('CloseArchive()')

            def InsertIntoArchive(self, vtkself, relativePath, data, size):
                self.calls.add('InsertIntoArchive(%s, %s, %d)' % (relativePath, data, size))

            def Contains(self, vtkself, relativePath):
                self.calls.add('Contains(%s)' % relativePath)
                return True

        pyArchiver = MyArchiver()
        ex = vtk.vtkPythonArchiver()
        ex.SetPythonObject(pyArchiver)

        ex.SetArchiveName('my/archive')
        ex.OpenArchive()
        data = 'bar'
        ex.InsertIntoArchive('foo', data, len(data.encode('utf-8')))
        self.assertTrue(ex.Contains('foo'))
        ex.CloseArchive()

        self.assertTrue('OpenArchive()' in pyArchiver.calls)
        self.assertTrue('InsertIntoArchive(%s, %s, %d)' %
                        ('foo', data.encode('utf-8'), len(data.encode('utf-8')))
                        in pyArchiver.calls)
        self.assertTrue('Contains(%s)' % 'foo' in pyArchiver.calls)
        self.assertTrue('CloseArchive()' in pyArchiver.calls)

if __name__ == "__main__":
    Testing.main([(TestPythonArchiver, 'test')])
