/*
 * Main.java
 *
 * Created on September 26, 2006, 2:02 PM
 *
 * To change this template, choose Tools | Template Manager
 * and open the template in the editor.
 */
import java.io.*;
import javax.xml.parsers.*;
import com.sun.xml.fastinfoset.sax.*;

/*
 * Need to include libraries in the project
 *
 *
 **/

/* @author christian */

public class vtkX3DBinaryConverter {

  private ByteArrayOutputStream WriteStream;
  private SAXDocumentSerializer DocumentSerializer;

  /** Creates a new instance of Main */
  public vtkX3DBinaryConverter(String outputFileName) throws Exception
    {
    System.out.println("Write X3D binary file: " + outputFileName);
    // Get the input stream for the XML document
    this.WriteStream = new ByteArrayOutputStream();

    // Set up output stream for fast infoset document
    OutputStream fiDocument = new BufferedOutputStream(
      new FileOutputStream(outputFileName));

    // Create Fast Infoset SAX serializer
    this.DocumentSerializer = new SAXDocumentSerializer();
    // Set the output stream
    this.DocumentSerializer.setOutputStream(fiDocument);
    }

  public void Write(byte[] b) throws IOException
    {
    this.WriteStream.write(b, 0, b.length);
    }

  public void Close() throws Exception
    {
    InputStream inputStream = new ByteArrayInputStream(
      this.WriteStream.toByteArray());

    // Instantiate JAXP SAX parser factory
    SAXParserFactory saxParserFactory = SAXParserFactory.newInstance();
    /* Set parser to be namespace aware
     * Very important to do otherwise invalid FI documents will be
     * created by the SAXDocumentSerializer
     */
    saxParserFactory.setNamespaceAware(true);
    // Instantiate the JAXP SAX parser

    SAXParser parser = saxParserFactory.newSAXParser();
    parser.parse(inputStream, this.DocumentSerializer);
    }

  public static void main(String[] args) {

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
