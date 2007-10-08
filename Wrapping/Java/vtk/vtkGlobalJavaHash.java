package vtk;

import vtk.vtkGlobalJavaHash;
import vtk.vtkObject;
import vtk.vtkObjectBase;

import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.lang.ref.WeakReference;

public class vtkGlobalJavaHash
{
  public static Map PointerToReference = Collections.synchronizedMap(new HashMap());
  
  public static void DeleteAll() {
    Iterator iter = PointerToReference.values().iterator();
    synchronized (PointerToReference) {
      while (iter.hasNext()) {
        WeakReference value = (WeakReference)iter.next();
        vtkObjectBase obj = (vtkObjectBase)value.get();
        if (obj != null) {
          obj.Delete();
        }
      }
    }
  }  
}