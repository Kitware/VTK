from vtkmodules.test import Testing as vtkTesting

class TestInitialize(vtkTesting.vtkTest):

    def test(self):

        from vtkmodules.vtkSerializationManager import vtkObjectManager

        om = vtkObjectManager()
        self.assertEqual(om.Initialize(), True, "Initialization failed!")


if __name__ == "__main__":
    vtkTesting.main([(TestInitialize, 'test')])
