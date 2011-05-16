package vtk.test;

import java.util.concurrent.TimeUnit;

import vtk.vtkActor;
import vtk.vtkArrowSource;
import vtk.vtkObject;
import vtk.vtkPolyDataMapper;

/**
 * This test should run forever without running out of memory or
 * crashing. It should work with or without the Delete() calls present.
 */
public class JavaGCAndDelete {

	public static void main(final String[] args) {
	    vtkJavaTesting.Initialize(args);
	    vtkJavaTesting.StartTimeoutExit(1, TimeUnit.MINUTES);
		int i = 0;
		int k = 0;
		while (true) {
			final vtkArrowSource arrowSource = new vtkArrowSource();
			final vtkPolyDataMapper mapper = new vtkPolyDataMapper();
			mapper.SetInput(arrowSource.GetOutput());
			final vtkActor actor = new vtkActor();
			actor.SetMapper(mapper);

			arrowSource.GetOutput().Delete();
			arrowSource.Delete();
            mapper.Delete();
            actor.Delete();

			++i;
			if (i >= 10000) {
				++k;
				System.out.println(vtkObject.JAVA_OBJECT_MANAGER.gc(true).listKeptReferenceToString());
				System.out.println(k * 10000);
				i = 0;
			}
		}
	}

}
