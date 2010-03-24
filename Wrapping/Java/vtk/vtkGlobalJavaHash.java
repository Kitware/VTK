package vtk;

import vtk.vtkGlobalJavaHash;
import vtk.vtkObject;
import vtk.vtkObjectBase;

import java.util.concurrent.ConcurrentHashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.lang.ref.WeakReference;

public class vtkGlobalJavaHash
{
  public static ConcurrentHashMap PointerToReference = new ConcurrentHashMap();
  public static vtkJavaGarbageCollector GarbageCollector = new vtkJavaGarbageCollector();
  
  /**
   * Delete all C++ objects, regardless of whether Java objects still exist
   * for them. Call this only when you are sure you are done using all
   * VTK objects.
   */
  public static int DeleteAll() {
    int deleted = 0;
    Set entries = vtkGlobalJavaHash.PointerToReference.entrySet();
    Iterator iter = entries.iterator();
    while (iter.hasNext()) {
      Map.Entry entry = (Map.Entry)iter.next();
      vtkObjectBase obj = (vtkObjectBase)((WeakReference)entry.getValue()).get();
      if (obj == null) {
        // Delete a garbage collected object using the raw C++ pointer.
        long id = ((Long)entry.getKey()).longValue();
        vtkObjectBase.VTKDeleteReference(id);
        entries.remove(entry);
        deleted++;
      } else {
        // Delete a non-garbage collected object.
        // We use Delete() which will call the "real" C++ Delete()
        // unless Delete() has been called on this Java object before.
        // Delete() will remove from the map so we don't have to.
        obj.Delete();
        deleted++;
      }
    }
    return deleted;
  }

  /**
   * Delete C++ objects whose Java objects have been garbage collected.
   */
  public static int GC() {
    int deleted = 0;
    Set entries = vtkGlobalJavaHash.PointerToReference.entrySet();
    Iterator iter = entries.iterator();
    while (iter.hasNext()) {
      Map.Entry entry = (Map.Entry)iter.next();
      vtkObjectBase obj = (vtkObjectBase)((WeakReference)entry.getValue()).get();
      if (obj == null) {
        // Delete a garbage collected object using the raw C++ pointer.
        long id = ((Long)entry.getKey()).longValue();
        vtkObjectBase.VTKDeleteReference(id);
        entries.remove(entry);
        deleted++;
      }
    }
    return deleted;
  }

}
