/*
 * Main.java
 *
 * Created on September 26, 2006, 2:02 PM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import org.web3d.parser.x3d.X3DReader;
import org.web3d.vrml.export.Exporter;
import org.web3d.vrml.export.X3DBinaryRetainedDirectExporter;
import org.web3d.vrml.export.X3DBinarySerializer;
import org.web3d.vrml.sav.InputSource;
import org.web3d.vrml.sav.VRMLReader;

/*
 * Need to include libraries in the project
 *
 *
 */

/* @author Franck Kolb */

public class vtkX3DBinaryConverter {

	// Default Largest acceptable error for float quantization
	private static float PARAM_FLOAT_LOSSY = 0.001f;

	/** The VRMLReader */
	private VRMLReader reader;

	private static Exporter writer;

	/** The compression method to use for binary */
	private int compressionMethod;

	/** The float lossy param */
	private float quantizeParam;

	private ByteArrayOutputStream writeStream;

	private BufferedOutputStream bos;

	/** Creates a new instance of vtkX3DBinaryConverter */
	public vtkX3DBinaryConverter(String outputFileName) throws Exception {

		this ( outputFileName, X3DBinarySerializer.METHOD_SMALLEST_NONLOSSY, PARAM_FLOAT_LOSSY);

	}


	/** Creates a new instance of vtkX3DBinaryConverter */
	private vtkX3DBinaryConverter(String outputFileName, int method,
			float quantizeParam) throws Exception {

		// Get the input stream for the XML document
		this.writeStream = new ByteArrayOutputStream();

		compressionMethod = method;
		this.quantizeParam = quantizeParam;

		reader = new X3DReader();

		// Set up output stream for fast infoset document
		FileOutputStream fos = null;

		try {
			fos = new FileOutputStream(outputFileName);
		} catch (FileNotFoundException fnfe) {
			fnfe.printStackTrace();
		}

		bos = new BufferedOutputStream(fos);

	}


	public void Write(byte[] b) throws IOException
	{
		this.writeStream.write(b, 0, b.length);

	}

	public void Close() throws Exception
    {
		InputStream inputStream = new ByteArrayInputStream(
			      this.writeStream.toByteArray());

		InputSource is = new InputSource(null, inputStream);

		System.out.println("Write X3D binary file");

		writer = new X3DBinaryRetainedDirectExporter(bos, 3, 1, null,
			   	compressionMethod, quantizeParam);

		reader.setContentHandler(writer);
		reader.setRouteHandler(writer);
		reader.setScriptHandler(writer);
		reader.setProtoHandler(writer);

		try {
			reader.parse(is);
		} catch (Exception e) {
			e.printStackTrace();
		}

    }


	/*
	 * Create an instance of this class and run it.
	 */
	public static void main(String[] args) {


		/*
		 * Set the compression method to use for binary compression. 4 mehods :
		 * Fasting parsing method Smallest parsing method Lossy parsing method
		 * Strings method
		 */
		// method = X3DBinarySerializer.METHOD_FASTEST_PARSING;
		// method = X3DBinarySerializer.METHOD_SMALLEST_NONLOSSY;
		// method = X3DBinarySerializer.METHOD_SMALLEST_LOSSY;
		// method = X3DBinarySerializer.METHOD_STRINGS;

	    try{
	        System.out.println("Create converter");
	        vtkX3DBinaryConverter x = new vtkX3DBinaryConverter("out.x3db");
	        InputStream is = new BufferedInputStream(new FileInputStream("export.x3d"));
	        System.out.println("Read file: " + is.available());
	        byte b[] = new byte[is.available()];
	        is.read(b);
	        x.Write(b);

	        System.out.println("Done...");
	        x.Close();
	      }
	      catch (Exception ex)
	        {
	        System.out.println(ex);
	        }
	}
}
