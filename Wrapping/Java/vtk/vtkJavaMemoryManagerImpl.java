package vtk;

import java.lang.ref.WeakReference;
import java.lang.reflect.Constructor;
import java.util.HashMap;
import java.util.TreeSet;
import java.util.concurrent.locks.ReentrantLock;

/**
 * Provide a Java thread safe implementation of vtkJavaMemoryManager. This does
 * not make VTK thread safe. This only insure that the change of reference count
 * will never happen in two concurrent thread in the Java world.
 *
 * @see vtkJavaMemoryManager
 * @author sebastien jourdain - sebastien.jourdain@kitware.com
 */
public class vtkJavaMemoryManagerImpl implements vtkJavaMemoryManager {
  private vtkJavaGarbageCollector garbageCollector;
  private ReentrantLock lock;
  private vtkReferenceInformation lastGcResult;
  private HashMap<Long, WeakReference<vtkObjectBase>> objectMap;
  private HashMap<Long, String> objectMapClassName;

  public vtkJavaMemoryManagerImpl() {
    this.lock = new ReentrantLock();
    this.objectMap = new HashMap<Long, WeakReference<vtkObjectBase>>();
    this.objectMapClassName = new HashMap<Long, String>();
    this.garbageCollector = new vtkJavaGarbageCollector();
  }

  // Thread safe
  public vtkObjectBase getJavaObject(Long vtkId) {
    // Check pre-condition
    if (vtkId == null || vtkId.longValue() == 0) {
      throw new RuntimeException("Invalid ID, can not be null or equal to 0.");
    }

    // Check inside the map if the object is already there
    WeakReference<vtkObjectBase> value = objectMap.get(vtkId);
    vtkObjectBase resultObject = (value == null) ? null : value.get();

    // If not, we have to do something
    if (value == null || resultObject == null) {
      try {
        // Make sure no concurrency could happen inside that
        this.lock.lock();

        // Now that we have the lock make sure someone else didn't
        // create the object in between, if so just return the created
        // instance
        value = objectMap.get(vtkId);
        resultObject = (value == null) ? null : value.get();
        if (resultObject != null) {
          return resultObject;
        }

        // We need to do the work of the gc
        if (value != null && resultObject == null) {
          this.unRegisterJavaObject(vtkId);
        }

        // No-one did create it, so let's do it
        if (resultObject == null) {
          try {
            String className = vtkObjectBase.VTKGetClassNameFromReference(vtkId.longValue());
            Class<?> c = Class.forName("vtk." + className);
            Constructor<?> cons = c.getConstructor(new Class<?>[] { long.class });
            resultObject = (vtkObjectBase) cons.newInstance(new Object[] { vtkId });
          } catch (Exception e) {
            e.printStackTrace();
          }
        }
      } finally {
        this.lock.unlock();
      }
    }
    return resultObject;
  }

  // Thread safe
  public void registerJavaObject(Long id, vtkObjectBase obj) {
    try {
      this.lock.lock();
      this.objectMap.put(id, new WeakReference<vtkObjectBase>(obj));
      this.objectMapClassName.put(id, obj.GetClassName());
    } finally {
      this.lock.unlock();
    }
  }

  // Thread safe
  public void unRegisterJavaObject(Long id) {
    try {
      this.lock.lock();
      this.objectMapClassName.remove(id);
      WeakReference<vtkObjectBase> value = this.objectMap.remove(id);

      // Prevent double deletion...
      if (value != null) {
        vtkObjectBase.VTKDeleteReference(id.longValue());
      } else {
        throw new RuntimeException("You try to delete a vtkObject that is not referenced in the Java object Map. You may have call Delete() twice.");
      }
    } finally {
      this.lock.unlock();
    }
  }

  // Thread safe
  public vtkReferenceInformation gc(boolean debug) {
    System.gc();
    try {
      this.lock.lock();
      final vtkReferenceInformation infos = new vtkReferenceInformation(debug);
      for (Long id : new TreeSet<Long>(this.objectMap.keySet())) {
        vtkObjectBase obj = this.objectMap.get(id).get();
        if (obj == null) {
          infos.addFreeObject(this.objectMapClassName.get(id));
          this.unRegisterJavaObject(id);
        } else {
          infos.addKeptObject(this.objectMapClassName.get(id));
        }
      }

      this.lastGcResult = infos;
      return infos;
    } finally {
      this.lock.unlock();
    }
  }

  public vtkJavaGarbageCollector getAutoGarbageCollector() {
    return this.garbageCollector;
  }

  // Thread safe
  public int deleteAll() {
    int size = this.objectMap.size();
    try {
      this.lock.lock();
      for (Long id : new TreeSet<Long>(this.objectMap.keySet())) {
        this.unRegisterJavaObject(id);
      }
    } finally {
      this.lock.unlock();
    }
    return size;
  }

  public int getSize() {
    return objectMap.size();
  }

  public vtkReferenceInformation getLastReferenceInformation() {
    return this.lastGcResult;
  }
}
