/*
 * @(#)SimpleExample.java	1.23 99/04/23
 *
 * Copyright (c) 1997-1999 by Sun Microsystems, Inc. All Rights Reserved.
 * 
 * Sun grants you ("Licensee") a non-exclusive, royalty free, license to use,
 * modify and redistribute this software in source and binary code form,
 * provided that i) this copyright notice and license appear on all copies of
 * the software; and ii) Licensee does not utilize the software in a manner
 * which is disparaging to Sun.
 * 
 * This software is provided "AS IS," without a warranty of any kind. ALL
 * EXPRESS OR IMPLIED CONDITIONS, REPRESENTATIONS AND WARRANTIES, INCLUDING ANY
 * IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NON-INFRINGEMENT, ARE HEREBY EXCLUDED. SUN AND ITS LICENSORS SHALL NOT BE
 * LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING
 * OR DISTRIBUTING THE SOFTWARE OR ITS DERIVATIVES. IN NO EVENT WILL SUN OR ITS
 * LICENSORS BE LIABLE FOR ANY LOST REVENUE, PROFIT OR DATA, OR FOR DIRECT,
 * INDIRECT, SPECIAL, CONSEQUENTIAL, INCIDENTAL OR PUNITIVE DAMAGES, HOWEVER
 * CAUSED AND REGARDLESS OF THE THEORY OF LIABILITY, ARISING OUT OF THE USE OF
 * OR INABILITY TO USE SOFTWARE, EVEN IF SUN HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 * 
 * This software is not designed or intended for use in on-line control of
 * aircraft, air traffic, aircraft navigation or aircraft communications; or in
 * the design, construction, operation or maintenance of any nuclear
 * facility. Licensee represents and warrants that it will not use or
 * redistribute the Software for such purposes.
 */

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import vtk.*;

/**
 * An application that displays a JButton and several JRadioButtons.
 * The JRadioButtons determine the look and feel used by the application.
 */
public class SimpleVTK extends JPanel implements ActionListener {
    static JFrame frame;

    static String metal= "Metal";
    static String metalClassName = "javax.swing.plaf.metal.MetalLookAndFeel";

    Button exitButton;

    public SimpleVTK() {
	// Create the buttons.
	vtkPanel renWin = new vtkPanel();
        vtkConeSource cone = new vtkConeSource();
        cone.SetResolution(8);
        vtkPolyDataMapper coneMapper = new vtkPolyDataMapper();
        coneMapper.SetInput(cone.GetOutput());
        
        vtkActor coneActor = new vtkActor();
        coneActor.SetMapper(coneMapper);
        
        renWin.GetRenderer().AddActor(coneActor);

        exitButton = new Button("Exit");
        exitButton.addActionListener(this);

	add(renWin);
	add(exitButton);
    }


    /** An ActionListener that listens to the radio buttons. */
    public void actionPerformed(ActionEvent e) 
    {
        if (e.getSource().equals(exitButton)) 
            {
                System.exit(0);
            }
    }

    public static void main(String s[]) 
    {
	SimpleVTK panel = new SimpleVTK();
	
	frame = new JFrame("SimpleVTK");
	frame.addWindowListener(new WindowAdapter() 
            {
                public void windowClosing(WindowEvent e) {System.exit(0);}
            });
	frame.getContentPane().add("Center", panel);
	frame.pack();
	frame.setVisible(true);
    }
}

