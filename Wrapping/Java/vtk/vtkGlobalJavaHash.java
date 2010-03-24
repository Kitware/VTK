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
  
  public static int DeleteAll() {
    int deleted = 0;
    Set keys = PointerToReference.keySet();
    Iterator iter = keys.iterator();
    while (iter.hasNext()) {
      Long idLong = (Long)iter.next();
      long id = idLong.longValue();
      vtkObjectBase.VTKDeleteReference(id);
      keys.remove(idLong);
      deleted++;
    }
    return deleted;
  }

  public static int GC() {
    int deleted = 0;
    Set entries = vtkGlobalJavaHash.PointerToReference.entrySet();
    Iterator iter = entries.iterator();
    while (iter.hasNext()) {
      Map.Entry entry = (Map.Entry)iter.next();
      vtkObjectBase obj = (vtkObjectBase)((WeakReference)entry.getValue()).get();
      if (obj == null) {
        long id = ((Long)entry.getKey()).longValue();
        vtkObjectBase.VTKDeleteReference(id);
        entries.remove(entry);
        deleted++;
      }
    }
    return deleted;
  }

}
