package vtk;

import java.util.HashMap;
import java.util.Map.Entry;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Information object return by the Garbage collector. This allow the user to
 * monitor what kind of vtkObject and how many have been removed and kept during
 * the Garbage collection call.
 *
 * @author sebastien jourdain - sebastien.jourdain@kitware.com
 */
public class vtkReferenceInformation {
    private int numberOfObjectsToFree;
    private int numberOfObjectsStillReferenced;
    private HashMap<String, AtomicInteger> classesKept;
    private HashMap<String, AtomicInteger> classesRemoved;
    private boolean keepTrackOfClassNames;

    public vtkReferenceInformation(int nbToFree, int nbToKeep, int totalSize) {
        this.numberOfObjectsToFree = nbToFree;
        this.numberOfObjectsStillReferenced = nbToKeep;
        this.keepTrackOfClassNames = false;
    }

    public vtkReferenceInformation(boolean keepTrackOfClassNames) {
        this.numberOfObjectsToFree = 0;
        this.numberOfObjectsStillReferenced = 0;
        this.keepTrackOfClassNames = keepTrackOfClassNames;
    }

    public int getTotalNumberOfObjects() {
        return numberOfObjectsStillReferenced + numberOfObjectsToFree;
    }

    public int getNumberOfObjectsToFree() {
        return numberOfObjectsToFree;
    }

    public int getTotalNumberOfObjectsStillReferenced() {
        return numberOfObjectsStillReferenced;
    }

    public void setNumberOfObjectsStillReferenced(int numberOfObjectsStillReferenced) {
        this.numberOfObjectsStillReferenced = numberOfObjectsStillReferenced;
    }

    public void setNumberOfObjectsToFree(int numberOfObjectsToFree) {
        this.numberOfObjectsToFree = numberOfObjectsToFree;
    }

    public void addFreeObject(String className) {
        this.numberOfObjectsToFree++;
        if (keepTrackOfClassNames) {
            if (classesRemoved == null && className != null) {
                classesRemoved = new HashMap<String, AtomicInteger>();
            }
            AtomicInteger counter = classesRemoved.get(className);
            if (counter == null) {
                classesRemoved.put(className, new AtomicInteger(1));
            } else {
                counter.incrementAndGet();
            }
        }
    }

    public void addKeptObject(String className) {
        this.numberOfObjectsStillReferenced++;
        if (keepTrackOfClassNames && className != null) {
            if (classesKept == null) {
                classesKept = new HashMap<String, AtomicInteger>();
            }
            AtomicInteger counter = classesKept.get(className);
            if (counter == null) {
                classesKept.put(className, new AtomicInteger(1));
            } else {
                counter.incrementAndGet();
            }
        }
    }

    public String listKeptReferenceToString() {
        if (classesKept == null) {
            return "";
        }
        final StringBuilder builder = new StringBuilder(500);
        builder.append("List of classes kept in Java layer:\n");
        for (Entry<String, AtomicInteger> entry : classesKept.entrySet()) {
            builder.append(" - ").append(entry.getKey()).append(": ").append(entry.getValue().toString()).append("\n");
        }
        return builder.toString();
    }

    public String listRemovedReferenceToString() {
        if (classesRemoved == null) {
            return "";
        }
        final StringBuilder builder = new StringBuilder(500);
        builder.append("List of classes removed in Java layer:\n");
        for (Entry<String, AtomicInteger> entry : classesRemoved.entrySet()) {
            builder.append(" - ").append(entry.getKey()).append(": ").append(entry.getValue().toString()).append("\n");
        }
        return builder.toString();
    }

    @Override
    public String toString() {
        final StringBuilder builder = new StringBuilder(50);
        builder.append("VTK Gabage Collection: free(");
        builder.append(this.numberOfObjectsToFree);
        builder.append(") - keep(");
        builder.append(this.numberOfObjectsStillReferenced);
        builder.append(") - total(");
        builder.append(this.getTotalNumberOfObjects());
        builder.append(")");
        return builder.toString();
    }
}
