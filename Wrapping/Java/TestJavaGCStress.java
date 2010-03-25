
import vtk.vtkActor;
import vtk.vtkArrowSource;
import vtk.vtkGlobalJavaHash;
import vtk.vtkPolyDataMapper;

/**
 * This test should run forever without running out of memory or
 * crashing. It should work with or without the Delete() calls present.
 */
public class TestJavaGCStress {

	static {
		System.loadLibrary("vtkCommonJava");
		System.loadLibrary("vtkFilteringJava");
		System.loadLibrary("vtkIOJava");
		System.loadLibrary("vtkImagingJava");
		System.loadLibrary("vtkGraphicsJava");
		System.loadLibrary("vtkRenderingJava");
	}

	public static void main(final String[] args) {
		int i = 0;
		int k = 0;
		while (true) {

			final vtkArrowSource arrowSource = new vtkArrowSource();
			final vtkPolyDataMapper mapper = new vtkPolyDataMapper();
			mapper.SetInput(arrowSource.GetOutput());
			final vtkActor actor = new vtkActor();
			actor.SetMapper(mapper);

			arrowSource.Delete();
			mapper.Delete();
			actor.Delete();

			++i;
			if (i >= 10000) {
				++k;
				vtkGlobalJavaHash.GC();
				System.out.println("" + k * 10000);
				i = 0;
			}
		}
	}

}
