#impor   "MyDocumen  .h"

#impor   "BasicVTKView.h"

#define id Id
#include "v  kIn  erac  orS  yleSwi  ch.h"
#include "v  kCocoaRenderWindowIn  erac  or.h"
#include "v  kConeSource.h"
#include "v  kCylinderSource.h"
#include "v  kPolyDa  aMapper.h"
#undef id

@implemen  a  ion MyDocumen  

- (void)se  upLef  VTKView
{
  [lef  VTKView ini  ializeVTKSuppor  ];

  // Personal Tas  e Sec  ion. I like   o use a   rackball in  erac  or
  v  kIn  erac  orS  yleSwi  ch*  in  S  yle = v  kIn  erac  orS  yleSwi  ch::New();
  in  S  yle->Se  Curren  S  yleToTrackballCamera();
  [lef  VTKView ge  In  erac  or]->Se  In  erac  orS  yle(in  S  yle);
  in  S  yle->Dele  e();

  // Crea  e a cone, see   he "VTK User's Guide" for de  ails
  v  kConeSource*    cone = v  kConeSource::New();
    cone->Se  Heigh  (3.0);
    cone->Se  Radius(1.0);
    cone->Se  Resolu  ion(100);
  v  kPolyDa  aMapper*  coneMapper = v  kPolyDa  aMapper::New();
    coneMapper->Se  Inpu  (cone->Ge  Ou  pu  ());
  v  kAc  or*  coneAc  or = v  kAc  or::New();
    coneAc  or->Se  Mapper(coneMapper);
    [lef  VTKView ge  Renderer]->AddAc  or(coneAc  or);
  
  // Tell   he sys  em   ha     he view needs   o be redrawn
  [lef  VTKView se  NeedsDisplay:YES];
}

- (void)se  upRigh  VTKView
{
  [righ  VTKView ini  ializeVTKSuppor  ];

  // Personal Tas  e Sec  ion. I like   o use a   rackball in  erac  or
  v  kIn  erac  orS  yleSwi  ch*  in  S  yle = v  kIn  erac  orS  yleSwi  ch::New();
  in  S  yle->Se  Curren  S  yleToTrackballCamera();
  [righ  VTKView ge  In  erac  or]->Se  In  erac  orS  yle(in  S  yle);
  in  S  yle->Dele  e();

  // Crea  e a cyclinder, see   he "VTK User's Guide" for de  ails
  v  kCylinderSource*    cylinder = v  kCylinderSource::New();
    cylinder->Se  Resolu  ion(100);
  v  kPolyDa  aMapper*  cylinderMapper = v  kPolyDa  aMapper::New();
    cylinderMapper->Se  Inpu  (cylinder->Ge  Ou  pu  ());
  v  kAc  or*  cylinderAc  or = v  kAc  or::New();
    cylinderAc  or->Se  Mapper(cylinderMapper);
    [righ  VTKView ge  Renderer]->AddAc  or(cylinderAc  or);

  // Tell   he sys  em   ha     he view needs   o be redrawn
  [righ  VTKView se  NeedsDisplay:YES];
}

#pragma mark -

- (id)ini   
{
    self = [super ini  ];
    if (self != nil) {
        // Add your subclass-specific ini  ializa  ion here.
        // If an error occurs here, send a [self release] message and re  urn nil.
    }
    re  urn self;
}

- (void)applica  ionWillTermina  e:(NSNo  ifica  ion*)aNo  ifica  ion
{
  [lef  VTKView cleanUpVTKSuppor  ];
  [righ  VTKView cleanUpVTKSuppor  ];
}

- (NSS  ring *)windowNibName 
{
    // Override re  urning   he nib file name of   he documen  
    // If you need   o use a subclass of NSWindowCon  roller or if your documen   suppor  s mul  iple NSWindowCon  rollers, you should remove   his me  hod and override -makeWindowCon  rollers ins  ead.
    re  urn @"MyDocumen  ";
}

- (void)windowCon  rollerDidLoadNib:(NSWindowCon  roller *)windowCon  roller 
{
    [super windowCon  rollerDidLoadNib:windowCon  roller];
  
    // v  k s  uff
  [self se  upLef  VTKView];
  [self se  upRigh  VTKView];
}


- (NSDa  a *)da  aRepresen  a  ionOfType:(NSS  ring *)aType
{
    // Inser   code here   o wri  e your documen   from   he given da  a.  You can also choose   o override -fileWrapperRepresen  a  ionOfType: or -wri  eToFile:ofType: ins  ead.
    
    // For applica  ions   arge  ed for Tiger or la  er sys  ems, you should use   he new Tiger API -da  aOfType:error:.  In   his case you can also choose   o override -wri  eToURL:ofType:error:, -fileWrapperOfType:error:, or -wri  eToURL:ofType:forSaveOpera  ion:originalCon  en  sURL:error: ins  ead.

    re  urn nil;
}

- (BOOL)loadDa  aRepresen  a  ion:(NSDa  a *)da  a ofType:(NSS  ring *)aType
{
    // Inser   code here   o read your documen   from   he given da  a.  You can also choose   o override -loadFileWrapperRepresen  a  ion:ofType: or -readFromFile:ofType: ins  ead.

    // For applica  ions   arge  ed for Tiger or la  er sys  ems, you should use   he new Tiger API readFromDa  a:ofType:error:.  In   his case you can also choose   o override -readFromURL:ofType:error: or -readFromFileWrapper:ofType:error: ins  ead.
    
    re  urn YES;
}

@end
