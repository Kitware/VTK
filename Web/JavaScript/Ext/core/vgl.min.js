//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

if(typeof ogs === 'undefined') {
  var ogs = {};
}

//////////////////////////////////////////////////////////////////////////////
/**
 * Create namespace for the given name
 *
 * @param ns_string
 * @returns {*|{}}
 */
//////////////////////////////////////////////////////////////////////////////
ogs.namespace = function(ns_string) {
  'use strict';

  var parts = ns_string.split('.'), parent = ogs, i;

  // strip redundant leading global
  if (parts[0] === "ogs") {
    parts = parts.slice(1);
  }
  for (i = 0; i < parts.length; i += 1) {
    // create a property if it doesn't exist
    if (typeof parent[parts[i]] === "undefined") {
      parent[parts[i]] = {};
    }
    parent = parent[parts[i]];
  }
  return parent;
};

/** ogs.vgl namespace */
var vglModule = ogs.namespace("vgl");

//////////////////////////////////////////////////////////////////////////////
/**
 * Convenient function to define JS inheritance
 *
 * @param C
 * @param P
 */
//////////////////////////////////////////////////////////////////////////////
function inherit(C, P) {
  "use strict";

  var F = function() {
  };
  F.prototype = P.prototype;
  C.prototype = new F();
  C.uber = P.prototype;
  C.prototype.constructor = C;
}

//////////////////////////////////////////////////////////////////////////////
/**
 * Convenient function to get size of an object
 *
 * @param obj
 * @returns {number} *
 */
//////////////////////////////////////////////////////////////////////////////
Object.size = function(obj) {
  "use strict";

  var size = 0, key = null;
  for (key in obj) {
    if (obj.hasOwnProperty(key)) {
      size++;
    }
  }
  return size;
};
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class timestamp
 *
 * @class
 * @returns {vglModule.timestamp}
 */
//////////////////////////////////////////////////////////////////////////////
var m_globalModifiedTime = 0;

vglModule.timestamp = function() {
  'use strict';

  if (!(this instanceof vglModule.timestamp)) {
    return new vglModule.timestamp();
  }

  var m_modifiedTime = 0;

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Update modified time
   */
  /////////////////////////////////////////////////////////////////////////////
  this.modified = function() {
    ++m_globalModifiedTime;
    m_modifiedTime = m_globalModifiedTime;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Get modified time
   *
   * @returns {number}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.getMTime = function() {
    return m_modifiedTime;
  };
};
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class object
 *
 * @class
 * @returns {vglModule.object}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.object = function() {
  'use strict';

  if (!(this instanceof vglModule.object)) {
    return new vglModule.object();
  }

  /** @private */
  var m_modifiedTime = vglModule.timestamp();
  m_modifiedTime.modified();

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Mark the object modified
   */
  ////////////////////////////////////////////////////////////////////////////
  this.modified = function() {
    m_modifiedTime.modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return modified time of the object
   *
   * @returns {*}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.getMTime = function() {
    return m_modifiedTime.getMTime();
  };

  return this;
};//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class command
 *
 * @class command
 * @returns {vglModule.command}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.command = function() {
  'use strict';

  if (!(this instanceof vglModule.command)) {
    return new vglModule.command();
  }
  vglModule.object.call(this);

  return this;
};

inherit(vglModule.command, vglModule.object);

//////////////////////////////////////////////////////////////////////////////
/**
 * Event types
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.command.keyPressEvent = "keyPressEvent";
vglModule.command.mousePressEvent = "mousePressEvent";
vglModule.command.mouseReleaseEvent = "mouseReleaseEvent";
vglModule.command.contextMenuEvent = "contextMenuEvent";
vglModule.command.configureEvent = "configureEvent";
vglModule.command.enableEvent = "enableEvent";
vglModule.command.mouseWheelBackwardEvent = "mouseWheelBackwardEvent";
vglModule.command.keyReleaseEvent = "keyReleaseEvent";
vglModule.command.middleButtonPressEvent = "middleButtonPressEvent";
vglModule.command.startInteractionEvent = "startInteractionEvent";
vglModule.command.enterEvent = "enterEvent";
vglModule.command.rightButtonPressEvent = "rightButtonPressEvent";
vglModule.command.middleButtonReleaseEvent = "middleButtonReleaseEvent";
vglModule.command.charEvent = "charEvent";
vglModule.command.disableEvent = "disableEvent";
vglModule.command.endInteractionEvent = "endInteractionEvent";
vglModule.command.mouseMoveEvent = "mouseMoveEvent";
vglModule.command.mouseWheelForwardEvent = "mouseWheelForwardEvent";
vglModule.command.exposeEvent = "exposeEvent";
vglModule.command.timerEvent = "timerEvent";
vglModule.command.leftButtonPressEvent = "leftButtonPressEvent";
vglModule.command.leaveEvent = "leaveEvent";
vglModule.command.rightButtonReleaseEvent = "rightButtonReleaseEvent";
vglModule.command.leftButtonReleaseEvent = "leftButtonReleaseEvent";
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class boundingObject
 *
 * @class
 * @return {vglModule.boundingObject}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.boundingObject = function() {
  'use strict';

  if (!(this instanceof vglModule.boundingObject)) {
    return new vglModule.boundingObject();
  }
  vglModule.object.call(this);

  /** @private */
  var m_bounds = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
      m_computeBoundsTimestamp = vglModule.timestamp(),
      m_boundsDirtyTimestamp = vglModule.timestamp();

  m_computeBoundsTimestamp.modified();
  m_boundsDirtyTimestamp.modified();

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get current bounds of the object
   */
  ////////////////////////////////////////////////////////////////////////////
  this.bounds = function() {
    return m_bounds;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set current bounds of the object
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setBounds = function(minX, maxX, minY, maxY, minZ, maxZ) {
    m_bounds[0] = minX;
    m_bounds[1] = maxX;
    m_bounds[2] = minY;
    m_bounds[3] = maxY;
    m_bounds[4] = minZ;
    m_bounds[5] = maxZ;

    this.modified();
    m_computeBoundsTimestamp.modified();

    return true;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Reset bounds to default values
   */
  ////////////////////////////////////////////////////////////////////////////
  this.resetBounds = function() {
    m_bounds[0] = Number.MAX_VALUE;
    m_bounds[1] = -Number.MAX_VALUE;
    m_bounds[2] = Number.MAX_VALUE;
    m_bounds[3] = -Number.MAX_VALUE;
    m_bounds[4] = Number.MAX_VALUE;
    m_bounds[5] = -Number.MAX_VALUE;

    this.modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Compute bounds of the object
   *
   * Should be implemented by the concrete class
   */
  ////////////////////////////////////////////////////////////////////////////
  this.computeBounds = function() {
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return bounds computation modification time
   *
   * @returns {vglModule.timestamp}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.computeBoundsTimestamp = function() {
    return m_computeBoundsTimestamp;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return bounds dirty timestamp
   *
   * @returns {vglModule.timestamp}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.boundsDirtyTimestamp = function() {
    return m_boundsDirtyTimestamp;
  };

  this.resetBounds();

  return this;
};

vglModule.boundingObject.ReferenceFrame = {
  "Relative" : 0,
  "Absolute" : 1
};

inherit(vglModule.boundingObject, vglModule.object);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class node
 *
 * @class
 * @returns {vglModule.node}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.node = function() {
  'use strict';

  if (!(this instanceof vglModule.node)) {
    return new vglModule.node();
  }
  vglModule.boundingObject.call(this);

  /** @private */
  var m_parent = null,
      m_material = null,
      m_visible = true,
      m_overlay = false;

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Accept visitor for scene traversal
   */
  ////////////////////////////////////////////////////////////////////////////
  this.accept = function(visitor) {
    visitor.visit(this);
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return active material used by the node
   */
  ////////////////////////////////////////////////////////////////////////////
  this.material = function() {
    return m_material;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set material to be used the node
   *
   * @param material
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setMaterial = function(material) {
    if (material !== m_material) {
      m_material = material;
      this.modified();
      return true;
    }

    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Check if the node is visible or node
   *
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.visible = function() {
    return m_visible;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Turn ON/OFF visibility of the node
   *
   * @param flag
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setVisible = function(flag) {
    if (flag !== m_visible) {
      m_visible = flag;
      this.modified();
      return true;
    }

    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return current parent of the node
   *
   * @returns {null}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.parent = function() {
    return m_parent;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set parent of the node
   *
   * @param parent
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setParent = function(parent) {
    if (parent !== m_parent) {
      if (m_parent !== null) {
        m_parent.removeChild(this);
      }
      m_parent = parent;
      this.modified();
      return true;
    }

    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Check if the node is an overlay node
   *
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.overlay = function() {
    return m_overlay;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set if the node is an overlay node or not
   *
   * @param flag
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setOverlay = function(flag) {
    if (m_overlay !== flag) {
      m_overlay = flag;
      this.modified();
      return true;
    }

    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /*
   * Traverse parent and their parent and so on
   */
  ////////////////////////////////////////////////////////////////////////////
  this.ascend = function(visitor) {
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Traverse children
   */
  ////////////////////////////////////////////////////////////////////////////
  this.traverse = function(visitor) {
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Mark that the bounds are modified
   *
   */
  ////////////////////////////////////////////////////////////////////////////
  this.boundsModified = function() {
    // @todo Implement this
    this.boundsDirtyTimestamp().modified();

    if (m_parent !== null) {
      m_parent.boundsModified();
    }
  };

  return this;
};

inherit(vglModule.node, vglModule.boundingObject);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class groupNode
 *
 * @class
 * @returns {vglModule.groupNode}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.groupNode = function() {
  'use strict';

  if (!(this instanceof vglModule.groupNode)) {
    return new vglModule.groupNode();
  }
  vglModule.node.call(this);

  var m_children = [];

  // Reference to base class methods
  this.b_setVisible = this.setVisible;

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Turn on / off visibility
   *
   * @param flag
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setVisible = function(flag) {
    var i;

    if (this.b_setVisible(flag) !== true) {
      return false;
    }

    for (i = 0; i < m_children.length; ++i) {
      m_children[i].setVisible(flag);
    }

    return true;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Make the incoming node as child of the group node
   *
   * @param childNode
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.addChild = function(childNode) {
    if (childNode instanceof vglModule.node) {
      if (m_children.indexOf(childNode) === -1) {
        childNode.setParent(this);
        m_children.push(childNode);
        this.boundsDirtyTimestamp().modified();
        return true;
      }
      return false;
    }

    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Remove parent-child relationship between the group and incoming node
   *
   * @param childNode
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.removeChild = function(childNode) {
    if (childNode.parent() === this) {
      var index = m_children.indexOf(childNode);
      m_children.splice(index, 1);
      this.boundsDirtyTimestamp().modified();
      return true;
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Remove parent-child relationship between child nodes and the group node
   */
  ////////////////////////////////////////////////////////////////////////////
  this.removeChildren = function() {
    var i;
    for (i = 0; i < m_children.length; ++i) {
      this.removeChild(m_children[i]);
    }

    this.modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return children of this group node
   *
   * @returns {Array}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.children = function() {
    return m_children;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Accept a visitor and traverse the scene tree
   *
   * @param visitor
   */
  ////////////////////////////////////////////////////////////////////////////
  this.accept = function(visitor) {
    visitor.visit(this);
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Traverse the scene
   *
   * @param visitor
   */
  ////////////////////////////////////////////////////////////////////////////
  this.traverse = function(visitor) {
    switch (visitor.type()) {
      case visitor.UpdateVisitor:
        this.traverseChildrenAndUpdateBounds(visitor);
        break;
      case visitor.CullVisitor:
        this.traverseChildren(visitor);
        break;
      default:
        break;
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Traverse all of the children and update the bounds for each
   *
   * @param visitor
   */
  ////////////////////////////////////////////////////////////////////////////
  this.traverseChildrenAndUpdateBounds = function(visitor) {
    var i;

    if (this.m_parent && this.boundsDirtyTimestamp().getMTime() >
      this.computeBoundsTimestamp().getMTime()) {
      // Flag parents bounds dirty.
      this.m_parent.boundsDirtyTimestamp.modified();
    }

    this.computeBounds();

    if (visitor.mode() === visitor.TraverseAllChildren) {
      for (i = 0; i < m_children.length(); ++i) {
        m_children[i].accept(visitor);
        this.updateBounds(m_children[i]);
      }
    }

    this.computeBoundsTimestamp().modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Traverse children of the group node
   *
   * @param visitor
   */
  ////////////////////////////////////////////////////////////////////////////
  this.traverseChildren = function(visitor) {
    var i;

    if (visitor.mode() === vglModule.vesVisitor.TraverseAllChildren) {
      for (i = 0; i < m_children.length(); ++i) {
        m_children[i].accept(visitor);
      }
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Compute bounds for the group node
   */
  ////////////////////////////////////////////////////////////////////////////
  this.computeBounds = function() {
    var i = 0;

    if (this.computeBoundsTimestamp().getMTime() >
        this.boundsDirtyTimestamp().getMTime()) {
      return;
    }

    for (i = 0; i < m_children.length; ++i) {
      this.updateBounds(m_children[i]);
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Update bounds for the group node
   *
   * This method is used internally to update bounds of the group node by
   * traversing each of its child.
   *
   * @param child
   */
  ////////////////////////////////////////////////////////////////////////////
  this.updateBounds = function(child) {
    // FIXME: This check should not be required and possibly is incorrect
    if (child.overlay()) {
      return;
    }

    // Make sure that child bounds are upto date
    child.computeBounds();

    var bounds = this.bounds(),
        childBounds = child.bounds(),
        istep = 0,
        jstep = 0,
        i;

    for (i = 0; i < 3; ++i) {
      istep = i * 2;
      jstep = i * 2 + 1;
      if (childBounds[istep] < bounds[istep]) {
        bounds[istep] = childBounds[istep];
      }
      if (childBounds[jstep] > bounds[jstep]) {
        bounds[jstep] = childBounds[jstep];
      }
    }

    this.setBounds(bounds[0], bounds[1], bounds[2], bounds[3],
                   bounds[4], bounds[5]);
  };

  return this;
};

inherit(vglModule.groupNode, vglModule.node);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec3, vec4, mat4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class actor
 *
 * @class
 * @returns {vglModule.actor}
 */
////////////////////////////////////////////////////////////////////////////
vglModule.actor = function() {
  'use strict';

  if (!(this instanceof vglModule.actor)) {
    return new vglModule.actor();
  }
  vglModule.node.call(this);

  /** @private */
  var m_center = [],
      m_rotation = [],
      m_scale = [],
      m_translation = [0, 0, 0],
      m_referenceFrame = vglModule.boundingObject.ReferenceFrame.Relative,
      m_mapper = null;

  m_center.length = 3;
  m_rotation.length = 4;
  m_scale.length = 3;
  m_translation.length = 3;

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get center of transformation
   *
   * @returns {Array}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.center = function() {
    return m_center;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set center of transformation
   *
   * @param {Number} x
   * @param {Number} y
   * @param {Number} z
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setCenter = function(x, y, z) {
    m_center[0] = x;
    m_center[1] = y;
    m_center[2] = z;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get rotation defined by axis & angle (radians)
   *
   * @returns {Array}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.rotation = function() {
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set rotation defined by axis & angle (radians)
   *
   * @param {Number} angle
   * @param {Number} x
   * @param {Number} y
   * @param {Number} z
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setRotation = function(angle, x, y, z) {
    this.boundsModified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get scale in x, y and z directions
   *
   * @returns {Array}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.scale = function() {
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set scale in x, y and z directions
   *
   * @param {Number} x Scale in x direction
   * @param {Number} y Scale in y direction
   * @param {Number} z Scale in z direction
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setScale = function(x, y, z) {
    this.boundsModified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get translation in x, y and z directions
   *
   * @returns {Array}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.translation = function() {
    return m_translation;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set translation in x, y and z directions
   *
   * @param {Number} x Translation in x direction
   * @param {Number} y Translation in y direction
   * @param {Number} z Translation in z direction
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setTranslation = function(x, y, z) {
    m_translation = [x, y, z];
    this.boundsModified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get reference frame for the transformations
   *
   * @returns {String} Possible values are Absolute or Relative
   */
  ////////////////////////////////////////////////////////////////////////////
  this.referenceFrame = function() {
    return m_referenceFrame;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set reference frame for the transformations
   *
   * @param {vglModule.boundingObject.ReferenceFrame}
   * referenceFrame Possible values are (Absolute | Relative)
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setReferenceFrame = function(referenceFrame) {
    if (referenceFrame !== m_referenceFrame) {
      m_referenceFrame = referenceFrame;
      this.modified();
      return true;
    }
    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Evaluate the transform associated with the actor.
   *
   * @returns {mat4}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.modelViewMatrix = function() {
    var mat = mat4.create();
    mat4.identity(mat);
    mat4.translate(mat, mat, m_translation);
    return mat;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return model-view matrix for the actor
   *
   * @returns {mat4}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.matrix = function() {
    return this.modelViewMatrix();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return mapper where actor gets it behavior and data
   *
   * @returns {vglModule.mapper}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.mapper = function() {
    return m_mapper;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Connect an actor to its data source
   *
   * @param {vglModule.mapper}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setMapper = function(mapper) {
    if (mapper !== m_mapper) {
      m_mapper = mapper;
      this.boundsModified();
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * @todo
   */
  ////////////////////////////////////////////////////////////////////////////
  this.accept = function(visitor) {
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * @todo
   */
  ////////////////////////////////////////////////////////////////////////////
  this.ascend = function(visitor) {
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Compute object space to world space matrix
   * @todo
   */
  ////////////////////////////////////////////////////////////////////////////
  this.computeLocalToWorldMatrix = function(matrix, visitor) {
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Compute world space to object space matrix
   * @todo
   */
  ////////////////////////////////////////////////////////////////////////////
  this.computeWorldToLocalMatrix = function(matrix, visitor) {
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Compute actor bounds
   */
  ////////////////////////////////////////////////////////////////////////////
  this.computeBounds = function() {
    if (m_mapper === null || m_mapper === undefined) {
      this.resetBounds();
      return;
    }

    var computeBoundsTimestamp = this.computeBoundsTimestamp(),
        mapperBounds, minPt, maxPt, actorMatrix, newBounds;

    if (this.boundsDirtyTimestamp().getMTime() > computeBoundsTimestamp.getMTime() ||
      m_mapper.boundsDirtyTimestamp().getMTime() > computeBoundsTimestamp.getMTime()) {

      m_mapper.computeBounds();
      mapperBounds = m_mapper.bounds();

      minPt = [mapperBounds[0], mapperBounds[2], mapperBounds[4]];
      maxPt = [mapperBounds[1], mapperBounds[3], mapperBounds[5]];

      actorMatrix = this.modelViewMatrix();
      vec3.transformMat4(minPt, minPt, actorMatrix);
      vec3.transformMat4(maxPt, maxPt, actorMatrix);

      newBounds = [
        minPt[0] > maxPt[0] ? maxPt[0] : minPt[0],
        minPt[0] > maxPt[0] ? minPt[0] : maxPt[0],
        minPt[1] > maxPt[1] ? maxPt[1] : minPt[1],
        minPt[1] > maxPt[1] ? minPt[1] : maxPt[1],
        minPt[2] > maxPt[2] ? maxPt[2] : minPt[2],
        minPt[2] > maxPt[2] ? minPt[2] : maxPt[2]
      ];

      this.setBounds(newBounds[0], newBounds[1],
                     newBounds[2], newBounds[3],
                     newBounds[4], newBounds[5]);

      computeBoundsTimestamp.modified();
    }
  };

  return this;
};

inherit(vglModule.actor, vglModule.node);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec3, vec4, mat4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class camera
 *
 * @class
 * @returns {vglModule.camera}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.camera = function() {
  'use strict';

  if (!(this instanceof vglModule.camera)) {
    return new vglModule.camera();
  }
  vglModule.groupNode.call(this);

  /** @private */
  var m_viewAngle = (Math.PI * 30) / 180.0,
      m_position = vec4.fromValues(0.0, 0.0, 0.0, 1.0),
      m_focalPoint = vec4.fromValues(0.0, 0.0, -5.0, 1.0),
      m_centerOfRotation = vec3.fromValues(0.0, 0.0, 0.0),
      m_viewUp = vec4.fromValues(0.0, 1.0, 0.0, 0.0),
      m_rightDir = vec4.fromValues(1.0, 0.0, 0.0, 0.0),
      m_near = 0.01,
      m_far = 10000.0,
      m_viewAspect = 1.0,
      m_directionOfProjection = vec4.fromValues(0.0, 0.0, -1.0, 0.0),
      m_viewPlaneNormal = vec4.fromValues(0.0, 0.0, 1.0, 0.0),
      m_viewMatrix = mat4.create(),
      m_projectionMatrix = mat4.create(),
      m_computeModelViewMatrixTime = ogs.vgl.timestamp(),
      m_computeProjectMatrixTime = ogs.vgl.timestamp(),
      m_left = -1.0,
      m_right = 1.0,
      m_top = +1.0,
      m_bottom = -1.0,
      m_enableParallelProjection = false;

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get view angle of the camera
   */
  ////////////////////////////////////////////////////////////////////////////
  this.viewAngle = function() {
    return m_viewAngle;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set view angle of the camera in degrees, which is converted to radians.
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setViewAngleDegrees = function(a) {
    m_viewAngle = (Math.PI * a) / 180.0;
    this.modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set view angle of the camera in degrees, which is converted to radians.
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setViewAngle = function(a) {
    m_viewAngle = a;
    this.modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get position of the camera
   */
  ////////////////////////////////////////////////////////////////////////////
  this.position = function() {
    return m_position;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set position of the camera
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setPosition = function(x, y, z) {
    m_position = vec4.fromValues(x, y, z, 1.0);
    this.modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get focal point of the camera
   */
  ////////////////////////////////////////////////////////////////////////////
  this.focalPoint = function() {
    return m_focalPoint;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set focal point of the camera
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setFocalPoint = function(x, y, z) {
    m_focalPoint = vec4.fromValues(x, y, z, 1.0);
    this.modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get view-up direction of camera
   */
  ////////////////////////////////////////////////////////////////////////////
  this.viewUpDirection = function() {
    return m_viewUp;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set view-up direction of the camera
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setViewUpDirection = function(x, y, z) {
    m_viewUp = vec4.fromValues(x, y, z, 0);
    this.modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get center of rotation for camera
   */
  ////////////////////////////////////////////////////////////////////////////
  this.centerOfRotation = function() {
    return m_centerOfRotation;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set center of rotation for camera
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setCenterOfRotation = function(centerOfRotation) {
    m_centerOfRotation = centerOfRotation;
    this.modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get clipping range of the camera
   */
  ////////////////////////////////////////////////////////////////////////////
  this.getClippingRange = function() {
    return [m_near, m_far];
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set clipping range of the camera
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setClippingRange = function(near, far) {
    m_near = near;
    m_far = far;
    this.modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get view aspect
   */
  ////////////////////////////////////////////////////////////////////////////
  this.viewAspect = function() {
    return m_viewAspect;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set view aspect
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setViewAspect = function(aspect) {
    m_viewAspect = aspect;
    this.modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return if parallel projection is enabled
   */
  ////////////////////////////////////////////////////////////////////////////
  this.isEnabledParallelProjection = function() {
    return m_enableParallelProjection;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Enable / disable parallel projection
   */
  ////////////////////////////////////////////////////////////////////////////
  this.enableParallelProjection = function(flag) {
    if (flag !== m_enableParallelProjection) {
      m_enableParallelProjection = flag;
      this.modified();
      return true;
    }

    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set parallel projection parameters
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setParallelProjection = function(left, right, top, bottom) {
    m_left = left;
    m_right = right;
    m_top = top;
    m_bottom = bottom;
    this.modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return direction of projection
   */
  ////////////////////////////////////////////////////////////////////////////
  this.directionOfProjection = function() {
    this.computeDirectionOfProjection();
    return m_directionOfProjection;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return view plane normal direction
   */
  ////////////////////////////////////////////////////////////////////////////
  this.viewPlaneNormal = function() {
    this.computeViewPlaneNormal();
    return m_viewPlaneNormal;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return view-matrix for the camera This method does not compute the
   * view-matrix for the camera. It is assumed that a call to computeViewMatrix
   * has been made earlier.
   *
   * @returns {mat4}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.viewMatrix = function() {
    return this.computeViewMatrix();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return camera projection matrix This method does not compute the
   * projection-matrix for the camera. It is assumed that a call to
   * computeProjectionMatrix has been made earlier.
   *
   * @returns {mat4}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.projectionMatrix = function() {
    return this.computeProjectionMatrix();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Compute direction of projection
   */
  ////////////////////////////////////////////////////////////////////////////
  this.computeDirectionOfProjection = function() {
    vec3.subtract(m_directionOfProjection, m_focalPoint, m_position);
    vec3.normalize(m_directionOfProjection, m_directionOfProjection);
    this.modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Compute view plane normal
   */
  ////////////////////////////////////////////////////////////////////////////
  this.computeViewPlaneNormal = function() {
    m_viewPlaneNormal[0] = -m_directionOfProjection[0];
    m_viewPlaneNormal[1] = -m_directionOfProjection[1];
    m_viewPlaneNormal[2] = -m_directionOfProjection[2];
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Move camera closer or further away from the scene
   */
  ////////////////////////////////////////////////////////////////////////////
  this.zoom = function(d) {
    if (d === 0) {
      return;
    }

    d = d * vec3.distance(m_focalPoint, m_position);
    m_position[0] = m_focalPoint[0] - d * m_directionOfProjection[0];
    m_position[1] = m_focalPoint[1] - d * m_directionOfProjection[1];
    m_position[2] = m_focalPoint[2] - d * m_directionOfProjection[2];

    this.modified();
    // TODO: If the distance between focal point and the camera position
    // goes really low then we run into issues
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Move camera sideways
   */
  ////////////////////////////////////////////////////////////////////////////
  this.pan = function(dx, dy, dz) {
    m_position[0] += dx;
    m_position[1] += dy;
    m_position[2] += dz;

    m_focalPoint[0] += dx;
    m_focalPoint[1] += dy;
    m_focalPoint[2] += dz;

    this.modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Compute camera coordinate axes
   */
  ////////////////////////////////////////////////////////////////////////////
  this.computeOrthogonalAxes = function() {
    this.computeDirectionOfProjection();
    vec3.cross(m_rightDir, m_directionOfProjection, m_viewUp);
    vec3.normalize(m_rightDir, m_rightDir);
    this.modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Rotate camera around center of rotation
   * @param dx Rotation around vertical axis in degrees
   * @param dy Rotation around horizontal axis in degrees
   */
  ////////////////////////////////////////////////////////////////////////////
  this.rotate = function(dx, dy) {

    // Convert degrees into radians
    dx = 0.5 * dx * (Math.PI / 180.0);
    dy = 0.5 * dy * (Math.PI / 180.0);

    var mat = mat4.create(),
        inverseCenterOfRotation = new vec3.create();

    mat4.identity(mat);

    inverseCenterOfRotation[0] = -m_centerOfRotation[0];
    inverseCenterOfRotation[1] = -m_centerOfRotation[1];
    inverseCenterOfRotation[2] = -m_centerOfRotation[2];

    mat4.translate(mat, mat, m_centerOfRotation);
    mat4.rotate(mat, mat, dx, m_viewUp);
    mat4.rotate(mat, mat, dy, m_rightDir);
    mat4.translate(mat, mat, inverseCenterOfRotation);

    vec4.transformMat4(m_position, m_position, mat);
    vec4.transformMat4(m_focalPoint, m_focalPoint, mat);

    // Update viewup vector
    vec4.transformMat4(m_viewUp, m_viewUp, mat);
    vec4.normalize(m_viewUp, m_viewUp);

    // Update direction of projection
    this.computeOrthogonalAxes();

    // Mark modified
    this.modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Compute camera view matrix
   */
  ////////////////////////////////////////////////////////////////////////////
  this.computeViewMatrix = function() {
    if (m_computeModelViewMatrixTime.getMTime() < this.getMTime()) {
      mat4.lookAt(m_viewMatrix, m_position, m_focalPoint, m_viewUp);
      m_computeModelViewMatrixTime.modified();
    }

    return m_viewMatrix;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Compute camera projection matrix
   */
  ////////////////////////////////////////////////////////////////////////////
  this.computeProjectionMatrix = function() {
    if (m_computeProjectMatrixTime.getMTime() < this.getMTime()) {
      if (!m_enableParallelProjection) {
        mat4.perspective(m_projectionMatrix, m_viewAngle, m_viewAspect, m_near, m_far);
      } else {
        console.log('paralle projection');
        mat4.ortho(m_projectionMatrix,
          m_left, m_right, m_bottom, m_top, m_near, m_far);
      }

      m_computeProjectMatrixTime.modified();
    }

    return m_projectionMatrix;
  };

  this.computeDirectionOfProjection();

  return this;
};

inherit(vglModule.camera, vglModule.groupNode);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Freeze javascript object
 *
 * @param obj
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.freezeObject = function(obj) {
  'use strict';

  /**
   * Freezes an object, using Object.freeze if available, otherwise returns
   * the object unchanged.  This function should be used in setup code to prevent
   * errors from completely halting JavaScript execution in legacy browsers.
   *
   * @exports freezeObject
   */
  var freezedObject = Object.freeze(obj);
  if (typeof freezedObject === 'undefined') {
    freezedObject = function(o) {
      return o;
    };
  }

  return freezedObject;
};
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Returns the first parameter if not undefined,
 * otherwise the second parameter.
 *
 * @class
 * @returns {vglModule.defaultValue}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.defaultValue = function(a, b) {
  'use strict';

  if (typeof a !== 'undefined') {
    return a;
  }
  return b;
};

vglModule.defaultValue.EMPTY_OBJECT = vglModule.freezeObject({});
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $, Uint16Array*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of geojson reader
 *
 * This contains code that reads a geoJSON file and produces rendering
 * primitives from it.
 *
 * @class
 * @returns {vglModule.geojsonReader}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.geojsonReader = function() {
  'use strict';

  if (!(this instanceof vglModule.geojsonReader)) {
    return new vglModule.geojsonReader();
  }

  var m_scalarFormat = "none",
      m_scalarRange = null;

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Read scalars
   *
   * @param coordinates
   * @param geom
   * @param size_estimate
   * @param idx
   */
  ////////////////////////////////////////////////////////////////////////////
  this.readScalars = function(coordinates, geom, size_estimate, idx) {
    var array = null,
        s = null,
        r  = null,
        g = null,
        b = null;

    if (this.m_scalarFormat === "values" && coordinates.length === 4)
    {
      s = coordinates[3];
      array = geom.sourceData(vglModule.vertexAttributeKeys.Scalar);

      if (!array) {
        array = new vglModule.sourceDataSf();
        if (this.m_scalarRange) {
          array.setScalarRange(this.m_scalarRange[0],this.m_scalarRange[1]);
        }
        if (size_estimate !== undefined) {
          array.length = size_estimate;
        }
        geom.addSource(array);
      }
      if (size_estimate === undefined) {
        array.pushBack(s);
      } else {
        array.insertAt(idx, s);
      }
    } else if (this.m_scalarFormat === "rgb" && coordinates.length === 6) {
      array = geom.sourceData(vglModule.vertexAttributeKeys.Color);
      if (!array) {
        array = new vglModule.sourceDataC3fv();
        if (size_estimate !== undefined) {
          array.length = size_estimate*3;
        }
        geom.addSource(array);
      }
      r = coordinates[3];
      g = coordinates[4];
      b = coordinates[5];
      if (size_estimate === undefined) {
        array.pushBack([r,g,b]);
      } else {
        array.insertAt(idx, [r,g,b]);
      }
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Read point data
   *
   * @param coordinates
   * @returns {vglModule.geometryData}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.readPoint = function(coordinates) {
    var geom = new vglModule.geometryData(),
        vglpoints = new vglModule.points(),
        vglcoords = new vglModule.sourceDataP3fv(),
        indices = new Uint16Array(1),
        x = null,
        y = null,
        z = null,
        i = null;

    geom.addSource(vglcoords);
    for (i = 0; i < 1; i++) {
      indices[i] = i;

      x = coordinates[0];
      y = coordinates[1];
      z = 0.0;
      if (coordinates.length>2) {
        z = coordinates[2];
      }

      //console.log("read " + x + "," + y + "," + z);
      vglcoords.pushBack([x,y,z]);

      //attributes
      this.readScalars(coordinates, geom);
    }

    vglpoints.setIndices(indices);
    geom.addPrimitive(vglpoints);
    geom.setName("aPoint");
    return geom;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Read multipoint data
   *
   * @param coordinates
   * @returns {vglModule.geometryData}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.readMultiPoint = function(coordinates) {
    var geom = new vglModule.geometryData(),
        vglpoints = new vglModule.points(),
        vglcoords = new vglModule.sourceDataP3fv(),
        indices = new Uint16Array(coordinates.length),
        pntcnt = 0,
        estpntcnt = coordinates.length,
        x = null,
        y = null,
        z = null,
        i;

    //preallocate with size estimate
    vglcoords.data().length = estpntcnt * 3; //x,y,z

    for (i = 0; i < coordinates.length; i++) {
      indices[i] = i;
      x = coordinates[i][0];
      y = coordinates[i][1];
      z = 0.0;
      if (coordinates[i].length>2) {
        z = coordinates[i][2];
      }

      //console.log("read " + x + "," + y + "," + z);
      vglcoords.insertAt(pntcnt, [x,y,z]);

      //attributes
      this.readScalars(coordinates[i], geom, estpntcnt, pntcnt);

      pntcnt++;
    }

    vglpoints.setIndices(indices);
    geom.addPrimitive(vglpoints);
    geom.addSource(vglcoords);
    geom.setName("manyPoints");
    return geom;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Read line string data
   *
   * @param coordinates
   * @returns {vglModule.geometryData}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.readLineString = function(coordinates) {
    var geom = new vglModule.geometryData(),
        vglline = new vglModule.lineStrip(),
        vglcoords = new vglModule.sourceDataP3fv(),
        indices = [],
        i = null,
        x = null,
        y = null,
        z = null;

    vglline.setIndicesPerPrimitive(coordinates.length);

    for (i = 0; i < coordinates.length; i++) {
      indices.push(i);
      x = coordinates[i][0];
      y = coordinates[i][1];
      z = 0.0;
      if (coordinates[i].length>2) {
        z = coordinates[i][2];
      }

      //console.log("read " + x + "," + y + "," + z);
      vglcoords.pushBack([x,y,z]);

      //attributes
      this.readScalars(coordinates[i], geom);
    }

    vglline.setIndices(indices);
    geom.addPrimitive(vglline);
    geom.addSource(vglcoords);
    geom.setName("aLineString");
    return geom;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Read multi line string
   *
   * @param coordinates
   * @returns {vglModule.geometryData}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.readMultiLineString = function(coordinates) {
    var geom = new vglModule.geometryData(),
        vglcoords = new vglModule.sourceDataP3fv(),
        pntcnt = 0,
        //lines should be at least 2 verts long, underest OK
        estpntcnt = coordinates.length * 2,
        i = null,
        j = null,
        x = null,
        y = null,
        z = null,
        indices = null,
        vglline = null,
        thisLineLength = null;

    // Preallocate with size estimate
    vglcoords.data().length = estpntcnt*3; //x,y,z

    for (j = 0; j < coordinates.length; j++) {
      indices = [];
      //console.log("getting line " + j);
      vglline = new vglModule.lineStrip();
      thisLineLength = coordinates[j].length;
      vglline.setIndicesPerPrimitive(thisLineLength);
      for (i = 0; i < thisLineLength; i++) {
        indices.push(pntcnt);
        x = coordinates[j][i][0];
        y = coordinates[j][i][1];
        z = 0.0;
        if (coordinates[j][i].length>2) {
          z = coordinates[j][i][2];
        }

        //console.log("read " + x + "," + y + "," + z);
        vglcoords.insertAt(pntcnt, [x,y,z]);

        //attributes
        this.readScalars(coordinates[j][i], geom, estpntcnt*2, pntcnt);

        pntcnt++;
      }

      vglline.setIndices(indices);
      geom.addPrimitive(vglline);
    }

    geom.setName("aMultiLineString");
    geom.addSource(vglcoords);
    return geom;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Read polygon data
   *
   * @param coordinates
   * @returns {vglModule.geometryData}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.readPolygon = function(coordinates) {
    //TODO: ignoring holes given in coordinates[1...]
    //TODO: ignoring convex
    //TODO: implement ear clipping in VGL instead of this to handle both
    var geom = new vglModule.geometryData(),
        vglcoords = new vglModule.sourceDataP3fv(),
        x = null,
        y = null,
        z  = null,
        thisPolyLength = coordinates[0].length,
        vl = 1,
        i = null,
        indices = null,
        vgltriangle = null;


    for (i = 0; i < thisPolyLength; i++) {
      x = coordinates[0][i][0];
      y = coordinates[0][i][1];
      z = 0.0;
      if (coordinates[0][i].length>2) {
        z = coordinates[0][i][2];
      }

      //console.log("read " + x + "," + y + "," + z);
      vglcoords.pushBack([x,y,z]);

      //attributes
      this.readScalars(coordinates[0][i], geom);

      if (i > 1) {
        //console.log("Cutting new triangle 0,"+ vl+ ","+ i);
        indices = new Uint16Array([0,vl,i]);
        vgltriangle = new vglModule.triangles();
        vgltriangle.setIndices(indices);
        geom.addPrimitive(vgltriangle);
        vl = i;
      }
    }

    geom.setName("POLY");
    geom.addSource(vglcoords);
    return geom;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Read multi polygon data
   *
   * @param coordinates
   * @returns {vglModule.geometryData}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.readMultiPolygon = function(coordinates) {
    var geom = new vglModule.geometryData(),
        vglcoords = new vglModule.sourceDataP3fv(),
        ccount = 0,
        numPolys = coordinates.length,
        pntcnt = 0,
        estpntcnt = numPolys* 3, // assume triangles, underest is fine
        vgltriangle = new vglModule.triangles(),
        indexes = [],
        i = null,
        j = null,
        x = null,
        y = null,
        z  = null,
        thisPolyLength = null,
        vf = null,
        vl = null,
        flip = null,
        flipped = false;


    //var time1 = new Date().getTime()
    //var a = 0;
    //var b = 0;
    //var c = 0;
    //var d = 0;

    //preallocate with size estimate
    vglcoords.data().length = numPolys*3; //x,y,z

    for (j = 0; j < numPolys; j++) {
      //console.log("getting poly " + j);

      thisPolyLength = coordinates[j][0].length;
      vf = ccount;
      vl = ccount+1;
      flip = [false,false,false];
      for (i = 0; i < thisPolyLength; i++) {
        //var timea = new Date().getTime()

        x = coordinates[j][0][i][0];
        y = coordinates[j][0][i][1];
        z = 0.0;
        if (coordinates[j][0][i].length>2) {
          z = coordinates[j][0][i][2];
        }
         flipped = false;
        if (x > 180) {
          flipped = true;
          x = x - 360;
        }
        if (i === 0) {
          flip[0] = flipped;
        } else {
          flip[1+(i-1)%2] = flipped;
        }
        //var timeb = new Date().getTime();
        //console.log("read " + x + "," + y + "," + z);

        vglcoords.insertAt(pntcnt, [x,y,z]);
        //var timec = new Date().getTime();

        //attributes
        this.readScalars(coordinates[j][0][i], geom, estpntcnt, pntcnt);
        pntcnt++;
        //var timed = new Date().getTime()

        if (i > 1) {
          //console.log("Cutting new triangle "+ vf + "," + vl + "," + ccount);
          if (flip[0] === flip[1] && flip[1] === flip[2]) {
            indexes = indexes.concat([vf,vl,ccount]);
          }
          //else {
          //  //TODO: duplicate triangles that straddle boundary on either side
          //}

          vl = ccount;
        }
        ccount++;
        //var timee = new Date().getTime()
        //a = a + (timeb-timea)
        //b = b + (timec-timeb)
        //c = c + (timed-timec)
        //d = d + (timee-timed)
      }
    }
    vgltriangle.setIndices(indexes);
    geom.addPrimitive(vgltriangle);

    //console.log("NUMPOLYS " + pntcnt);
    //console.log("RMP: ", a, ",", b, ",", c, ",", d)
    //var time2 = new Date().getTime()

    geom.setName("aMultiPoly");
    geom.addSource(vglcoords);
    //var time3 = new Date().getTime()
    //console.log("RMP: ", time2-time1, ",", time3-time2)

    return geom;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * @param object
   * @returns {*}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.readGJObjectInt = function(object) {
    if (!object.hasOwnProperty('type')) {
      //console.log("uh oh, not a geojson object");
      return null;
    }

    //look for properties type annotation
    if (object.properties &&
        object.properties.ScalarFormat &&
        object.properties.ScalarFormat === "values") {
      this.m_scalarFormat = "values";
      if (object.properties.ScalarRange) {
        this.m_scalarRange = object.properties.ScalarRange;
      }
    }
    if (object.properties &&
        object.properties.ScalarFormat &&
        object.properties.ScalarFormat === "rgb") {
      this.m_scalarFormat = "rgb";
    }

    //TODO: ignoring "crs" and "bbox" and misc meta data on all of these,
    //best to handle as references into original probably
    var ret,
        type = object.type,
        next = null,
        nextset = null,
        i = null;

    switch (type) {
      case "Point":
        //console.log("parsed Point");
        ret = this.readPoint(object.coordinates);
        break;
      case "MultiPoint":
        //console.log("parsed MultiPoint");
        ret = this.readMultiPoint(object.coordinates);
        break;
      case "LineString":
        //console.log("parsed LineString");
        ret = this.readLineString(object.coordinates);
        break;
      case "MultiLineString":
        //console.log("parsed MultiLineString");
        ret = this.readMultiLineString(object.coordinates);
        break;
      case "Polygon":
        //console.log("parsed Polygon");
        ret = this.readPolygon(object.coordinates);
        break;
      case "MultiPolygon":
        //console.log("parsed MultiPolygon");
        ret = this.readMultiPolygon(object.coordinates);
        break;
      case "GeometryCollection":
        //console.log("parsed GeometryCollection");
        nextset = [];
        for (i = 0; i < object.geometries.length; i++) {
          next = this.readGJObject(object.geometries[i]);
          nextset.push(next);
        }
        ret = nextset;
        break;
      case "Feature":
        //console.log("parsed Feature");
        next = this.readGJObject(object.geometry);
        ret = next;
        break;
      case "FeatureCollection":
        //console.log("parsed FeatureCollection");
        nextset = [];
        for (i = 0; i < object.features.length; i++) {
          next = this.readGJObject(object.features[i]);
          nextset.push(next);
        }
        ret = nextset;
        break;
      default:
        console.log("Don't understand type " + type);
        ret = null;
      break;
    }
    return ret;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * @param object
   * @returns {*}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.readGJObject = function(object) {
    //var time1, time2;
    var ret;
    //time1 = new Date().getTime()
    ret = this.readGJObjectInt(object);
    //time2 = new Date().getTime()
    //console.log("ELAPSED: ", time2-time1)
    return ret;
  };

  /**
   * Linearize geometries
   *
   * @param geoms
   * @param geom
   */
  this.linearizeGeoms = function(geoms, geom) {
    var i = null;

    if( Object.prototype.toString.call( geom ) === '[object Array]' ) {
      for (i = 0; i < geom.length; i++) {
        this.linearizeGeoms(geoms, geom[i]);
      }
    }
    else {
     geoms.push(geom);
   }
 };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Read geometries from geojson object
   *
   * @param object
   * @returns {Array}
   */
 ////////////////////////////////////////////////////////////////////////////
 this.readGeomObject = function(object) {
    var geom,
        geoms = [];

    geom = this.readGJObject(object);
    this.linearizeGeoms(geoms, geom);
    return geoms;
 };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Given a buffer get rendering primitives
   *
   * @param buffer
   * @returns {*}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.getPrimitives = function(buffer) {
    //console.log("Parsing geoJSON");
    if (!buffer) {
      return [];
    }

    var obj = JSON.parse(buffer),
      geom = this.readGJObject(obj),
      geoms = [];

    this.m_scalarFormat = "none";
    this.m_scalarRange = null;

    this.linearizeGeoms(geoms, geom);

    return { "geoms":geoms,
             "scalarFormat":this.m_scalarFormat,
             "scalarRange":this.m_scalarRange };
  };

  return this;
};
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

vglModule.data = function() {
  'use strict';

  if (!(this instanceof vglModule.data)) {
    return new vglModule.data();
  }

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return data type. Should be implemented by the derived class
   */
  ////////////////////////////////////////////////////////////////////////////
  this.type = function() {
  };
};

vglModule.data.raster = 0;
vglModule.data.point = 1;
vglModule.data.lineString = 2;
vglModule.data.polygon = 3;
vglModule.data.geometry = 10;//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, Uint16Array, gl, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Vertex attribute keys
 *
 * @type {{Position: number, Normal: number, TextureCoordinate: number,
 *         Color: number, Scalar: number}}
 */
//////////////////////////////////////////////////////////////////////////////
var vertexAttributeKeys = {
  "Position" : 0,
  "Normal" : 1,
  "TextureCoordinate" : 2,
  "Color" : 3,
  "Scalar" : 4
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class primitive
 *
 * @class
 * @return {vglModule.primitive}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.primitive = function() {
  'use strict';

  if (!(this instanceof vglModule.primitive)) {
    return new vglModule.primitive();
  }

  /** @private */
  var m_indicesPerPrimitive = 0,
      m_primitiveType = 0,
      m_indicesValueType = 0,
      m_indices = null;

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get indices of the primitive
   *
   * @returns {null}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.indices = function() {
    return m_indices;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Create indices array for the primitive
   * @param type
   */
  ////////////////////////////////////////////////////////////////////////////
  this.createIndices = function(type) {
    // TODO Check for the type
    m_indices = new Uint16Array();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return the number of indices
   */
  ////////////////////////////////////////////////////////////////////////////
  this.numberOfIndices = function() {
    return m_indices.length;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return size of indices in bytes
   */
  ////////////////////////////////////////////////////////////////////////////
  this.sizeInBytes = function() {
    return m_indices.length * Uint16Array.BYTES_PER_ELEMENT;
  };

  ////////////////////////////////////////////////////////////////////////////
  /*
   * Return primitive type g
   */
  ////////////////////////////////////////////////////////////////////////////
  this.primitiveType = function() {
    return m_primitiveType;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set primitive type
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setPrimitiveType = function(type) {
    m_primitiveType = type;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return count of indices that form a primitives
   */
  ////////////////////////////////////////////////////////////////////////////
  this.indicesPerPrimitive = function() {
    return m_indicesPerPrimitive;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set count of indices that form a primitive
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setIndicesPerPrimitive = function(count) {
    m_indicesPerPrimitive = count;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return indices value type
   */
  ////////////////////////////////////////////////////////////////////////////
  this.indicesValueType = function() {
    return m_indicesValueType;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set indices value type
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setIndicesValueType = function(type) {
    m_indicesValueType = type;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set indices from a array
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setIndices = function(indicesArray) {
    // TODO Check for the type
    m_indices = new Uint16Array(indicesArray);
  };

  return this;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class triangleStrip
 *
 * @returns {vglModule.triangleStrip}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.triangleStrip = function() {
  'use strict';

  if (!(this instanceof vglModule.triangleStrip)) {
    return new vglModule.triangleStrip();
  }

  vglModule.primitive.call(this);

  this.setPrimitiveType(gl.TRIANGLE_STRIP);
  this.setIndicesValueType(gl.UNSIGNED_SHORT);
  this.setIndicesPerPrimitive(3);

  return this;
};

inherit(vglModule.triangleStrip, vglModule.primitive);

////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class triangles
 *
 * @returns {vglModule.triangles}
 */
////////////////////////////////////////////////////////////////////////////
vglModule.triangles = function() {
  'use strict';

  if (!(this instanceof vglModule.triangles)) {
    return new vglModule.triangles();
  }
  vglModule.primitive.call(this);

  this.setPrimitiveType(gl.TRIANGLES);
  this.setIndicesValueType(gl.UNSIGNED_SHORT);
  this.setIndicesPerPrimitive(3);

  return this;
};

inherit(vglModule.triangles, vglModule.primitive);

//////////////////////////////////////////////////////////////////////////////
/**
 * create a instance of lines primitive type
 *
 * @returns {vglModule.lines}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.lines = function() {
  'use strict';

  if (!(this instanceof vglModule.lines)) {
    return new vglModule.lines();
  }
  vglModule.primitive.call(this);

  this.setPrimitiveType(gl.LINES);
  this.setIndicesValueType(gl.UNSIGNED_SHORT);
  this.setIndicesPerPrimitive(2);

  return this;
};
inherit(vglModule.lines, vglModule.primitive);

//////////////////////////////////////////////////////////////////////////////
/**
 * create a instance of line strip primitive type
 *
 * @returns {vglModule.lineStrip}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.lineStrip = function() {
  'use strict';

  if (!(this instanceof vglModule.lineStrip)) {
    return new vglModule.lineStrip();
  }
  vglModule.primitive.call(this);

  this.setPrimitiveType(gl.LINE_STRIP);
  this.setIndicesValueType(gl.UNSIGNED_SHORT);
  this.setIndicesPerPrimitive(2);

  return this;
};
inherit(vglModule.lineStrip, vglModule.primitive);

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class points
 *
 * @returns {vglModule.points}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.points = function() {
  'use strict';

  if (!(this instanceof vglModule.points)) {
    return new vglModule.points();
  }
  vglModule.primitive.call(this);

  this.setPrimitiveType(gl.POINTS);
  this.setIndicesValueType(gl.UNSIGNED_SHORT);
  this.setIndicesPerPrimitive(1);

  return this;
};

inherit(vglModule.points, vglModule.primitive);

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class vertexDataP3f
 *
 * @returns {vglModule.vertexDataP3f}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.vertexDataP3f = function() {
  'use strict';

  if (!(this instanceof vglModule.vertexDataP3f)) {
    return new vglModule.vertexDataP3f();
  }

  /** @private */
  this.m_position = [];

  return this;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class vertexDataP3N3f
 *
 * @class
 * @returns {vglModule.vertexDataP3N3f}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.vertexDataP3N3f = function() {
  'use strict';

  if (!(this instanceof vglModule.vertexDataP3N3f)) {
    return new vglModule.vertexDataP3N3f();
  }

  this.m_position = [];
  this.m_normal = [];

  return this;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class vertexDataP3T3f
 *
 * @class
 * @returns {vglModule.vertexDataP3T3f}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.vertexDataP3T3f = function() {
  'use strict';

  if (!(this instanceof vglModule.vertexDataP3T3f)) {
    return new vglModule.vertexDataP3T3f();
  }

  this.m_position = [];
  this.m_texCoordinate = [];

  return this;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class sourceData
 * @class
 * @returns {vglModule.sourceData}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.sourceData = function() {
  'use strict';

  if (!(this instanceof vglModule.sourceData)) {
    return new vglModule.sourceData();
  }

  var m_attributesMap = {},
      m_data = [],

      ////////////////////////////////////////////////////////////////////////////
      /**
       * Attribute data for the source
       */
      ////////////////////////////////////////////////////////////////////////////
      vglAttributeData = function() {
        // Number of components per group
        // Type of data type (GL_FLOAT etc)
        this.m_numberOfComponents = 0;
            // Size of data type
        this.m_dataType = 0;
        this.m_dataTypeSize = 0;
        // Specifies whether fixed-point data values should be normalized
        // (true) or converted directly as fixed-point values (false)
        // when they are accessed.
        this.m_normalized = false;
        // Strides for each attribute.
        this.m_stride = 0;
        // Offset
        this.m_offset = 0;
      };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return raw data for this source
   *
   * @returns {Array}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.data = function() {
    return m_data;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Add new attribute data to the source
   */
  ////////////////////////////////////////////////////////////////////////////
  this.addAttribute = function(key, dataType, sizeOfDataType, offset, stride,
                               noOfComponents, normalized) {

    if (!m_attributesMap.hasOwnProperty(key)) {
      var newAttr = new vglAttributeData();
      newAttr.m_dataType = dataType;
      newAttr.m_dataTypeSize = sizeOfDataType;
      newAttr.m_offset = offset;
      newAttr.m_stride = stride;
      newAttr.m_numberOfComponents = noOfComponents;
      newAttr.m_normalized = normalized;
      m_attributesMap[key] = newAttr;
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return size of the source data
   */
  ////////////////////////////////////////////////////////////////////////////
  this.sizeOfArray = function() {
    return Object.size(m_data);
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return length of array
   */
  ////////////////////////////////////////////////////////////////////////////
  this.lengthOfArray = function() {
    return m_data.length;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return size of the source data in bytes
   */
  ////////////////////////////////////////////////////////////////////////////
  /*
    * TODO: code below is probably wrong.
    *   Example:
    *            format P3N3f
    *            m_data = [ 1, 2, 3, 4, 5, 6 ]; // contains one vertex, one normal, m_data.length == 6
    *
    *       The inner loop computes:
    *             sizeInBytes += 3 * 4; // for position
    *             sizeInBytes += 3 * 4; // for normal
    *
    *        Then sizeInBytes *= 6; // m_data.length == 6
    *        which gives sizeInBytes == 144 bytes when it should have been 4*6 = 24
    */
  this.sizeInBytes = function() {
    var sizeInBytes = 0,
        keys = this.keys(), i;

    for (i = 0; i < keys.length(); ++i) {
      sizeInBytes += this.numberOfComponents(keys[i])
                     * this.sizeOfAttributeDataType(keys[i]);
    }

    sizeInBytes *= this.sizeOfArray();

    return sizeInBytes;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Check if there is attribute exists of a given key type
   */
  ////////////////////////////////////////////////////////////////////////////
  this.hasKey = function(key) {
    return m_attributesMap.hasOwnProperty(key);
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return keys of all attributes
   */
  ////////////////////////////////////////////////////////////////////////////
  this.keys = function() {
    return Object.keys(m_attributesMap);
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return number of attributes of source data
   */
  ////////////////////////////////////////////////////////////////////////////
  this.numberOfAttributes = function() {
    return Object.size(m_attributesMap);
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return number of components of the attribute data
   */
  ////////////////////////////////////////////////////////////////////////////
  this.attributeNumberOfComponents = function(key) {
    if (m_attributesMap.hasOwnProperty(key)) {
      return m_attributesMap[key].m_numberOfComponents;
    }

    return 0;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return if the attribute data is normalized
   */
  ////////////////////////////////////////////////////////////////////////////
  this.normalized = function(key) {
    if (m_attributesMap.hasOwnProperty(key)) {
      return m_attributesMap[key].m_normalized;
    }

    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return size of the attribute data type
   */
  ////////////////////////////////////////////////////////////////////////////
  this.sizeOfAttributeDataType = function(key) {
    if (m_attributesMap.hasOwnProperty(key)) {
      return m_attributesMap[key].m_dataTypeSize;
    }

    return 0;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return attribute data type
   */
  ////////////////////////////////////////////////////////////////////////////
  this.attributeDataType = function(key) {
    if (m_attributesMap.hasOwnProperty(key)) {
      return m_attributesMap[key].m_dataType;
    }

    return undefined;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return attribute offset
   */
  ////////////////////////////////////////////////////////////////////////////
  this.attributeOffset = function(key) {
    if (m_attributesMap.hasOwnProperty(key)) {
      return m_attributesMap[key].m_offset;
    }

    return 0;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return attribute stride
   */
  ////////////////////////////////////////////////////////////////////////////
  this.attributeStride = function(key) {
    if (m_attributesMap.hasOwnProperty(key)) {
      return m_attributesMap[key].m_stride;
    }

    return 0;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Virtual function to insert new vertex data at the end
   */
  ////////////////////////////////////////////////////////////////////////////
  this.pushBack = function(vertexData) {
    // Should be implemented by the base class
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Insert new data block to the raw data
   */
  ////////////////////////////////////////////////////////////////////////////
  this.insert = function(data) {
    m_data = m_data.concat(data);
  };

  this.insertAt = function(index, data) {
    var i;

    if (!data.length) {
      m_data[index] = data;
    } else {
      for (i = 0; i < data.length; i++) {
        m_data[index*data.length+i] = data[i];
      }
    }
  };

  return this;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class sourceDataP3T3f
 *
 * @returns {vglModule.sourceDataP3T3f}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.sourceDataP3T3f = function() {
  'use strict';

  if (!(this instanceof vglModule.sourceDataP3T3f)) {
    return new vglModule.sourceDataP3T3f();
  }
  vglModule.sourceData.call(this);

  this.addAttribute(vglModule.vertexAttributeKeys.Position, gl.FLOAT, 4, 0, 6 * 4, 3,
                    false);
  this.addAttribute(vglModule.vertexAttributeKeys.TextureCoordinate, gl.FLOAT, 4, 12,
                    6 * 4, 3, false);

  this.pushBack = function(value) {
    this.insert(value.m_position);
    this.insert(value.m_texCoordinate);
  };

  return this;
};

inherit(vglModule.sourceDataP3T3f, vglModule.sourceData);

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class sourceDataP3N3f
 *
 * @returns {vglModule.sourceDataP3N3f}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.sourceDataP3N3f = function() {
  'use strict';

  if (!(this instanceof vglModule.sourceDataP3N3f)) {
    return new vglModule.sourceDataP3N3f();
  }

  vglModule.sourceData.call(this);

  this.addAttribute(vglModule.vertexAttributeKeys.Position, gl.FLOAT, 4, 0, 6 * 4, 3,
                    false);
  this.addAttribute(vglModule.vertexAttributeKeys.Normal, gl.FLOAT, 4, 12, 6 * 4, 3,
                    false);

  this.pushBack = function(value) {
    this.insert(value.m_position);
    this.insert(value.m_normal);
  };

  return this;
};

inherit(vglModule.sourceDataP3N3f, vglModule.sourceData);

/////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class sourceDataP3fv
 *
 * @returns {vglModule.sourceDataP3fv}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.sourceDataP3fv = function() {
  'use strict';

  if (!(this instanceof vglModule.sourceDataP3fv)) {
    return new vglModule.sourceDataP3fv();
  }

  vglModule.sourceData.call(this);

  this.addAttribute(vglModule.vertexAttributeKeys.Position, gl.FLOAT, 4, 0, 3 * 4, 3,
                    false);

  this.pushBack = function(value) {
    this.insert(value);
  };

  return this;
};

inherit(vglModule.sourceDataP3fv, vglModule.sourceData);

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class sourceDataT2fv
 *
 * @returns {vglModule.sourceDataT2fv}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.sourceDataT2fv = function() {
  'use strict';

  if (!(this instanceof vglModule.sourceDataT2fv)) {
    return new vglModule.sourceDataT2fv();
  }

  vglModule.sourceData.call(this);

  this.addAttribute(vglModule.vertexAttributeKeys.TextureCoordinate, gl.FLOAT, 4, 0,
                    2 * 4, 2, false);

  this.pushBack = function(value) {
    this.insert(value);
  };

  return this;
};

inherit(vglModule.sourceDataT2fv, vglModule.sourceData);

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class sourceDataC3fv
 *
 * @returns {vglModule.sourceDataC3fv}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.sourceDataC3fv = function() {
  'use strict';

  if (!(this instanceof vglModule.sourceDataC3fv)) {
    return new vglModule.sourceDataC3fv();
  }

  vglModule.sourceData.call(this);

  this.addAttribute(vglModule.vertexAttributeKeys.Color, gl.FLOAT, 4, 0, 3 * 4, 3, false);

  this.pushBack = function(value) {
    this.insert(value);
  };

  return this;
};

inherit(vglModule.sourceDataC3fv, vglModule.sourceData);

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class sourceDataSf meant to hold scalar float values
 *
 * @class
 * @returns {vglModule.sourceDataSf}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.sourceDataSf = function() {
  'use strict';

  if (!(this instanceof vglModule.sourceDataSf)) {
    return new vglModule.sourceDataSf();
  }

  var m_min = null,
      m_max = null,
      m_fixedmin = null,
      m_fixedmax = null;

  vglModule.sourceData.call(this);

  this.addAttribute(vglModule.vertexAttributeKeys.Scalar, gl.FLOAT, 4, 0, 4, 1, false);

  this.pushBack = function(value) {
    if (m_max === null || value > m_max) {
      m_max = value;
    }
    if (m_min === null || value < m_min) {
      m_min = value;
    }
    this.insert(value);
  };

  this.insertAt = function(index, value) {
    if (m_max === null || value > m_max) {
      m_max = value;
    }
    if (m_min === null || value < m_min) {
      m_min = value;
    }
    //call superclass ??
    //vglModule.sourceData.insertAt.call(this, index, value);
    this.data()[index] = value;
  };

  this.scalarRange = function() {
    if (m_fixedmin === null || m_fixedmax === null) {
      return [m_min, m_max];
    }

    return [m_fixedmin, m_fixedmax];
  };

  this.setScalarRange = function(min, max) {
    m_fixedmin = min;
    m_fixedmax = max;
  };

  return this;
};

inherit(vglModule.sourceDataSf, vglModule.sourceData);

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class geometryData
 *
 * @class
 * @returns {vglModule.geometryData}
 */
 /////////////////////////////////////////////////////////////////////////////
vglModule.geometryData = function() {
  'use strict';

  if (!(this instanceof vglModule.geometryData)) {
    return vglModule.geometryData();
  }
  vglModule.data.call(this);

  /** @private */
  var m_name = "",
      m_primitives = [],
      m_sources = [],
      m_bounds = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
      m_computeBoundsTimestamp = vglModule.timestamp(),
      m_boundsDirtyTimestamp = vglModule.timestamp();

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return type
   */
  ////////////////////////////////////////////////////////////////////////////
  this.type = function() {
    return vglModule.data.geometry;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return ID of the geometry data
   */
  ////////////////////////////////////////////////////////////////////////////
  this.name = function() {
    return m_name;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set name of the geometry data
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setName = function(name) {
    m_name = name;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Add new source
   */
  ////////////////////////////////////////////////////////////////////////////
  this.addSource = function(source) {
    // @todo Check if the incoming source has duplicate keys

    // NOTE This might not work on IE8 or lower
    if (m_sources.indexOf(source) === -1) {
      m_sources.push(source);

      if (source.hasKey(vglModule.vertexAttributeKeys.Position)) {
        m_boundsDirtyTimestamp.modified();
      }
      return true;
    }

    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return source for a given index. Returns 0 if not found.
   */
  ////////////////////////////////////////////////////////////////////////////
  this.source = function(index) {
    if (index < m_sources.length) {
      return m_sources[index];
    }

    return 0;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return number of sources
   */
  ////////////////////////////////////////////////////////////////////////////
  this.numberOfSources = function() {
    return m_sources.length;
  };

  /**
   * Return source data given a key
   */
  this.sourceData = function(key) {
    var i;

    for (i = 0; i < m_sources.length; ++i) {
      if (m_sources[i].hasKey(key)) {
        return m_sources[i];
      }
    }

    return null;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Add new primitive
   */
  ////////////////////////////////////////////////////////////////////////////
  this.addPrimitive = function(primitive) {
    m_primitives.push(primitive);
    return true;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return primitive for a given index. Returns null if not found.
   */
  ////////////////////////////////////////////////////////////////////////////
  this.primitive = function(index) {
    if (index < m_primitives.length) {
      return m_primitives[index];
    }

    return null;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return number of primitives
   */
  ////////////////////////////////////////////////////////////////////////////
  this.numberOfPrimitives = function() {
    return m_primitives.length;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return bounds [minX, maxX, minY, maxY, minZ, maxZ]
   */
  ////////////////////////////////////////////////////////////////////////////
  this.bounds = function() {
    if (m_boundsDirtyTimestamp.getMTime() > m_computeBoundsTimestamp.getMTime()) {
      this.computeBounds();
    }
    return m_bounds;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Reset bounds
   */
  ////////////////////////////////////////////////////////////////////////////
  this.resetBounds = function() {
    m_bounds[0] = 0.0;
    m_bounds[1] = 0.0;
    m_bounds[2] = 0.0;
    m_bounds[3] = 0.0;
    m_bounds[4] = 0.0;
    m_bounds[5] = 0.0;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set bounds
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setBounds = function(minX, maxX, minY, maxY, minZ, maxZ) {
    m_bounds[0] = minX;
    m_bounds[1] = maxX;
    m_bounds[2] = minY;
    m_bounds[3] = maxY;
    m_bounds[4] = minZ;
    m_bounds[5] = maxZ;

    m_computeBoundsTimestamp.modified();

    return true;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Compute bounds
   */
  ////////////////////////////////////////////////////////////////////////////
  this.computeBounds = function() {
    if (m_boundsDirtyTimestamp.getMTime() > m_computeBoundsTimestamp.getMTime()) {
      var attr = vglModule.vertexAttributeKeys.Position,
          sourceData = this.sourceData(attr),
          data = sourceData.data(),
          numberOfComponents = sourceData.attributeNumberOfComponents(attr),
          stride = sourceData.attributeStride(attr),
          offset = sourceData.attributeOffset(attr),
          sizeOfDataType = sourceData.sizeOfAttributeDataType(attr),
          count = data.length,
          ib = 0,
          jb = 0,
          value = null,
          vertexIndex,
          j;

      // We advance by index, not by byte
      stride /= sizeOfDataType;
      offset /= sizeOfDataType;

      this.resetBounds();

      for (vertexIndex = offset; vertexIndex < count; vertexIndex += stride) {
        for (j = 0; j < numberOfComponents; ++j) {
          value = data[vertexIndex + j];
          ib = j * 2;
          jb = j * 2 + 1;

          if (vertexIndex === offset) {
            m_bounds[ib] = value;
            m_bounds[jb] = value;
          } else {
            if (value > m_bounds[jb]) {
              m_bounds[jb] = value;
            }
            if (value < m_bounds[ib]) {
              m_bounds[ib] = value;
            }
          }
        }
      }

      m_computeBoundsTimestamp.modified();
    }
  };


  ////////////////////////////////////////////////////////////////////////////
  /**
   * Returns the vertex closest to a given position
   */
  ////////////////////////////////////////////////////////////////////////////
  this.findClosestVertex = function(point) {
    var attr = vglModule.vertexAttributeKeys.Position,
        sourceData = this.sourceData(attr),
        sizeOfDataType = sourceData.sizeOfAttributeDataType(attr),
        numberOfComponents = sourceData.attributeNumberOfComponents(attr),
        data = sourceData.data(),
        stride = sourceData.attributeStride(attr) / sizeOfDataType,
        offset = sourceData.attributeOffset(attr) / sizeOfDataType,
        minDist = Number.MAX_VALUE,
        minIndex = null,
        vi, vPos, dx, dy, dz, dist, i;

    // assume positions are always triplets
    if (numberOfComponents !== 3) {
      console.log('[warning] Find closest vertex assumes three' +
        'component vertex ');
    }

    if (!point.z) {
      point = {x:point.x, y:point.y, z:0};
    }

    for (vi = offset, i = 0; vi < data.length; vi += stride, i++) {
      vPos = [data[vi],
              data[vi + 1],
              data[vi + 2]];

      dx = vPos[0] - point.x;
      dy = vPos[1] - point.y;
      dz = vPos[2] - point.z;
      dist = Math.sqrt(dx*dx + dy*dy + dz*dz);
      if (dist < minDist) {
        minDist = dist;
        minIndex = i;
      }
    }
    return minIndex;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Returns the requested vertex position
   */
  ////////////////////////////////////////////////////////////////////////////
  this.getPosition = function(index) {
    var attr = vglModule.vertexAttributeKeys.Position,
        sourceData = this.sourceData(attr),
        sizeOfDataType = sourceData.sizeOfAttributeDataType(attr),
        numberOfComponents = sourceData.attributeNumberOfComponents(attr),
        data = sourceData.data(),
        stride = sourceData.attributeStride(attr) / sizeOfDataType,
        offset = sourceData.attributeOffset(attr) / sizeOfDataType;

    // assume positions are always triplets
    if (numberOfComponents !== 3) {
      console.log("[warning] getPosition assumes three component data");
    }

    return [ data[offset + index*stride],
             data[offset + index*stride + 1],
             data[offset + index*stride + 2] ];
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Returns the scalar corresponding to a given vertex index
   */
  ////////////////////////////////////////////////////////////////////////////
  this.getScalar = function(index) {
    var attr = vglModule.vertexAttributeKeys.Scalar,
        sourceData = this.sourceData(attr),
        numberOfComponents, sizeOfDataType, data, stride, offset;

    if (!sourceData) {
      return null;
    }

    numberOfComponents = sourceData.attributeNumberOfComponents(attr);
    sizeOfDataType = sourceData.sizeOfAttributeDataType(attr);
    data = sourceData.data();
    stride = sourceData.attributeStride(attr) / sizeOfDataType;
    offset = sourceData.attributeOffset(attr) / sizeOfDataType;

    //console.log("index for scalar is " + index);
    //console.log("offset for scalar is " + offset);
    //console.log("stride for scalar is " + stride);

    //console.log("have " + data.length + " scalars");

    if (index * stride + offset >= data.length) {
      console.log("access out of bounds in getScalar");
    }

    return data[index * stride + offset];
  };

  return this;
};

inherit(ogs.vgl.geometryData, ogs.vgl.data);//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, gl, ogs, vec4, Float32Array, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class mapper
 *
 * @class
 * @returns {vglModule.mapper}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.mapper = function() {
  'use strict';

  if (!(this instanceof vglModule.mapper)) {
    return new vglModule.mapper();
  }
  vglModule.boundingObject.call(this);

  /** @private */
  var m_dirty = true,
      m_color = [ 0.0, 1.0, 1.0 ],
      m_geomData = null,
      m_buffers = [],
      m_bufferVertexAttributeMap = {},
      m_glCompileTimestamp = vglModule.timestamp();

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Delete cached VBO if any
   *
   * @private
   */
  ////////////////////////////////////////////////////////////////////////////
  function deleteVertexBufferObjects() {
    var i;
    for (i = 0; i < m_buffers.length; ++i) {
      gl.deleteBuffer(m_buffers[i]);
    }
  }

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Create new VBO for all its geometryData sources and primitives
   *
   * @private
   */
  ////////////////////////////////////////////////////////////////////////////
  function createVertexBufferObjects() {
    if (m_geomData) {
      var numberOfSources = m_geomData.numberOfSources(),
          i, j, k, bufferId = null, keys, ks, numberOfPrimitives;

      for (i = 0; i < numberOfSources; ++i) {
        bufferId = gl.createBuffer();
        gl.bindBuffer(gl.ARRAY_BUFFER, bufferId);
        gl.bufferData(gl.ARRAY_BUFFER,
          new Float32Array(m_geomData.source(i).data()), gl.STATIC_DRAW);

        keys = m_geomData.source(i).keys();
        ks = [];

        for (j = 0; j < keys.length; ++j) {
          ks.push(keys[j]);
        }

        m_bufferVertexAttributeMap[i] = ks;
        m_buffers[i] = bufferId;
      }

      numberOfPrimitives = m_geomData.numberOfPrimitives();
      for (k = 0; k < numberOfPrimitives; ++k) {
        bufferId = gl.createBuffer();
        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, bufferId);
        gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, m_geomData.primitive(k)
            .indices(), gl.STATIC_DRAW);
        m_buffers[i++] = bufferId;
      }

      m_glCompileTimestamp.modified();
    }
  }

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Clear cache related to buffers
   *
   * @private
   */
  ////////////////////////////////////////////////////////////////////////////
  function cleanUpDrawObjects() {
    m_bufferVertexAttributeMap = {};
    m_buffers = [];
  }

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Setup draw objects; Delete old ones and create new ones
   *
   * @private
   */
  ////////////////////////////////////////////////////////////////////////////
  function setupDrawObjects() {
    // Delete buffer objects from past if any.
    deleteVertexBufferObjects();

    // Clear any cache related to buffers
    cleanUpDrawObjects();

    // Now construct the new ones.
    createVertexBufferObjects();

    m_dirty = false;
  }

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Compute bounds of the data
   */
  ////////////////////////////////////////////////////////////////////////////
  this.computeBounds = function() {
    if (m_geomData === null || typeof m_geomData === 'undefined') {
      this.resetBounds();
      return;
    }

    var computeBoundsTimestamp = this.computeBoundsTimestamp(),
        boundsDirtyTimestamp = this.boundsDirtyTimestamp(),
        geomBounds = null;

    if (boundsDirtyTimestamp.getMTime() > computeBoundsTimestamp.getMTime()) {
      geomBounds = m_geomData.bounds();

      this.setBounds(geomBounds[0], geomBounds[1], geomBounds[2],
        geomBounds[3], geomBounds[4], geomBounds[5]) ;

      computeBoundsTimestamp.modified();
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get solid color of the geometry
   */
  ////////////////////////////////////////////////////////////////////////////
  this.color = function() {
    return m_color;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set solid color of the geometry. Default is teal [1.0, 1.0, 1.0]
   *
   * @param r Red component of the color [0.0 - 1.0]
   * @param g Green component of the color [0.0 - 1.0]
   * @param b Blue component of the color [0.0 - 1.0]
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setColor = function(r, g, b) {
    m_color[0] = r;
    m_color[1] = g;
    m_color[2] = b;

    this.modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return stored geometry data if any
   */
  ////////////////////////////////////////////////////////////////////////////
  this.geometryData = function() {
    return m_geomData;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Connect mapper to its geometry data
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setGeometryData = function(geom) {
    if (m_geomData !== geom) {
      m_geomData = geom;

      this.modified();
      this.boundsDirtyTimestamp().modified();
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Render the mapper
   */
  ////////////////////////////////////////////////////////////////////////////
  this.render = function(renderState) {
    if (this.getMTime() > m_glCompileTimestamp.getMTime()) {
      setupDrawObjects(renderState);
    }

    // Fixed vertex color
    gl.vertexAttrib3fv(vglModule.vertexAttributeKeys.Color, this.color());

    // TODO Use renderState
    var bufferIndex = 0,
        j = 0, i, noOfPrimitives = null, primitive = null;

    for (i in m_bufferVertexAttributeMap) {
      if (m_bufferVertexAttributeMap.hasOwnProperty(i)) {
        gl.bindBuffer(gl.ARRAY_BUFFER, m_buffers[bufferIndex]);
        for (j = 0; j < m_bufferVertexAttributeMap[i].length; ++j) {
          renderState.m_material
              .bindVertexData(renderState, m_bufferVertexAttributeMap[i][j]);
        }
        ++bufferIndex;
      }
    }

    noOfPrimitives = m_geomData.numberOfPrimitives();
    for (j = 0; j < noOfPrimitives; ++j) {
      gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, m_buffers[bufferIndex++]);
      primitive = m_geomData.primitive(j);//
      gl.drawElements(primitive.primitiveType(), primitive.numberOfIndices(),
                      primitive.indicesValueType(), 0);
    }
  };

  return this;
};

inherit(vglModule.mapper, vglModule.boundingObject);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

vglModule.groupMapper = function() {
  'use strict';

  if (!(this instanceof vglModule.groupMapper)) {
    return new vglModule.groupMapper();
  }
  vglModule.mapper.call(this);

  /** @private */
  var m_createMappersTimestamp = vglModule.timestamp(),
      m_mappers = [],
      m_geomDataArray = [];

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return stored geometry data if any
   *
   * @param index optional
   */
  ////////////////////////////////////////////////////////////////////////////
  this.geometryData = function(index) {
    if (index !== undefined && index < m_geomDataArray.length ) {
      return m_geomDataArray[index];
    }

    if (m_geomDataArray.length > 0) {
      return m_geomDataArray[0];
    }

    return null;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Connect mapper to its geometry data
   *
   * @param geom {vglModule.geomData}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setGeometryData = function(geom) {
    if (m_geomDataArray.length === 1) {
      if (m_geomDataArray[0] === geom) {
        return;
      }
    }
    m_geomDataArray = [];
    m_geomDataArray.push(geom);
    this.modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return stored geometry data array if any
   */
  ////////////////////////////////////////////////////////////////////////////
  this.geometryDataArray = function() {
    return m_geomDataArray;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Connect mapper to its geometry data
   *
   * @param geoms {Array}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setGeometryDataArray = function(geoms) {
    if (geoms instanceof Array) {
      if (m_geomDataArray !== geoms) {
        m_geomDataArray = [];
        m_geomDataArray = geoms;
        this.modified();
        return true;
      }
    } else {
      console.log('[error] Requies array of geometry data');
    }

    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Compute bounds of the data
   */
  ////////////////////////////////////////////////////////////////////////////
  this.computeBounds = function() {
    if (m_geomDataArray === null ||
        m_geomDataArray === undefined) {
      this.resetBounds();
      return;
    }

    var computeBoundsTimestamp = this.computeBoundsTimestamp(),
        boundsDirtyTimestamp = this.boundsDirtyTimestamp(),
        m_bounds = this.bounds(),
        geomBounds = null,
        i = null;

    if (boundsDirtyTimestamp.getMTime() >
        computeBoundsTimestamp.getMTime()) {

      for (i = 0; i < m_geomDataArray.length; ++i) {
        geomBounds = m_geomDataArray[i].bounds();

        if (m_bounds[0] > geomBounds[0]) {
          m_bounds[0] = geomBounds[0];
        }
        if (m_bounds[1] < geomBounds[1]) {
          m_bounds[1] = geomBounds[1];
        }
        if (m_bounds[2] > geomBounds[2]) {
          m_bounds[2] = geomBounds[2];
        }
        if (m_bounds[3] < geomBounds[3]) {
          m_bounds[3] = geomBounds[3];
        }
        if (m_bounds[4] > geomBounds[4]) {
          m_bounds[4] = geomBounds[4];
        }
        if (m_bounds[5] < geomBounds[5]) {
          m_bounds[5] = geomBounds[5];
        }
      }

      this.modified();
      computeBoundsTimestamp.modified();
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Render the mapper
   */
  ////////////////////////////////////////////////////////////////////////////
  this.render = function(renderState) {
    var i = null;

    if (this.getMTime() > m_createMappersTimestamp.getMTime()) {
      // NOTE Hoping that it will release the graphics resources
      for (i = 0; i < m_geomDataArray.length; ++i) {
        m_mappers.push(vglModule.mapper());
        m_mappers[i].setGeometryData(m_geomDataArray[i]);
      }
        m_createMappersTimestamp.modified();
    }

    for (i = 0; i < m_mappers.length; ++i) {
      m_mappers[i].render(renderState);
    }
  };

  return this;
};

inherit(vglModule.groupMapper, vglModule.mapper);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

vglModule.materialAttributeType = {
  "Undefined" : 0x0,
  "ShaderProgram" : 0x1,
  "Texture" : 0x2,
  "Blend" : 0x3,
  "Depth" : 0x4
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class materialAttribute
 *
 * @class
 * @param type
 * @returns {vglModule.materialAttribute}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.materialAttribute = function(type) {
  'use strict';

  if (!(this instanceof vglModule.materialAttribute)) {
    return new vglModule.materialAttribute();
  }
  vglModule.object.call(this);

  /** @private */
  var m_type = type,
      m_enabled = true;

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return tyep of the material attribute
   *
   * @returns {*}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.type = function() {
    return m_type;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return if material attribute is enabled or not
   *
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.enabled = function() {
    return m_enabled;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Setup (initialize) the material attribute
   *
   * @param renderState
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setup = function(renderState) {
    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Bind and activate the material attribute
   *
   * @param renderState
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.bind = function(renderState) {
    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Undo bind and deactivate the material
   *
   * @param renderState
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.undoBind = function(renderState) {
    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Initialize vertex data for the material attribute
   *
   * @param renderState
   * @param key
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setupVertexData = function(renderState, key) {
    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Bind and activate vertex specific data
   *
   * @param renderState
   * @param key
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.bindVertexData = function(renderState, key) {
    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Undo bind and deactivate vertex specific data
   *
   * @param renderState
   * @param key
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.undoBindVertexData = function(renderState, key) {
    return false;
  };

  return this;
};

inherit(vglModule.materialAttribute, vglModule.object);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, gl, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of clas blendFunction
 *
 * @class
 * @param source
 * @param destination
 * @returns {vglModule.blendFunction}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.blendFunction = function(source, destination) {
  'use strict';

  if (!(this instanceof vglModule.blendFunction)) {
    return new vglModule.blendFunction(source, destination);
  }

  /** @private */
  var m_source = source,
      m_destination = destination;

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Apply blend function to the current state
   *
   * @param {vglModule.renderState}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.apply = function(renderState) {
    gl.blendFuncSeparate(m_source, m_destination, gl.ONE, gl.ONE_MINUS_SRC_ALPHA);
  };

  return this;
};

////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class blend
 *
 * @returns {vglModule.blend}
 */
////////////////////////////////////////////////////////////////////////////
vglModule.blend = function() {
  'use strict';

  if (!(this instanceof vglModule.blend)) {
    return new vglModule.blend();
  }
  vglModule.materialAttribute.call(
    this, vglModule.materialAttributeType.Blend);

  /** @private */
  var m_wasEnabled = false,
      m_blendFunction = vglModule.blendFunction(gl.SRC_ALPHA,
                                                gl.ONE_MINUS_SRC_ALPHA);

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Bind blend attribute
   *
   * @param {vglModule.renderState}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.bind = function(renderState) {
    m_wasEnabled = gl.isEnabled(gl.BLEND);

    if (this.enabled()) {
      gl.enable(gl.BLEND);
      m_blendFunction.apply(renderState);
    }
    else {
      gl.disable(gl.BLEND);
    }

    return true;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Undo bind blend attribute
   *
   * @param {vglModule.renderState}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.undoBind = function(renderState) {
    if (m_wasEnabled) {
      gl.enable(gl.BLEND);
    }
    else {
      gl.disable(gl.BLEND);
    }

    return true;
  };

  return this;
};

inherit(vglModule.blend, vglModule.materialAttribute);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class material
 *
 * @class
 * @returns {vglModule.material}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.material = function() {
  'use strict';

  if (!(this instanceof vglModule.material)) {
    return new vglModule.material();
  }
  vglModule.object.call(this);

  // / Private member variables
  var m_shaderProgram = new vglModule.shaderProgram(),
      m_binNumber = 100,
      m_textureAttributes = {},
      m_attributes = {};

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return bin number for the material
   *
   * @default 100
   * @returns {number}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.binNumber = function() {
    return m_binNumber;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set bin number for the material
   *
   * @param binNo
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setBinNumber = function(binNo) {
    m_binNumber = binNo;
    this.modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Check if incoming attribute already exists in the material
   *
   * @param attr
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.exists = function(attr) {
    if (attr.type() === vglModule.materialAttribute.Texture) {
      return m_textureAttributes.hasOwnProperty(attr);
    }

    return m_attributes.hasOwnProperty(attr);
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set a new attribute for the material
   *
   * This method replace any existing attribute except for textures as
   * materials can have multiple textures.
   *
   * @param attr
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setAttribute = function(attr) {
    if (attr.type() === vglModule.materialAttributeType.Texture &&
        m_textureAttributes[attr.textureUnit()] !== attr) {
      m_textureAttributes[attr.textureUnit()] = attr;
      this.modified();
      return true;
    }

    if (m_attributes[attr.type()] === attr) {
      return false;
    }

    // Shader is a very special attribute
    if (attr.type() === vglModule.materialAttributeType.ShaderProgram) {
      m_shaderProgram = attr;
    }

    m_attributes[attr.type()] = attr;
    this.modified();
    return true;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Add a new attribute to the material.
   *
   * @param attr
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.addAttribute = function(attr) {
    if (this.exists(attr)) {
      return false;
    }

    if (attr.type() === vglModule.materialAttributeType.Texture) {
      // TODO Currently we don't check if we are replacing or not.
      // It would be nice to have a flag for it.
      m_textureAttributes[attr.textureUnit()] = attr;
      this.modified();
      return true;
    }

    // Shader is a very special attribute
    if (attr.type() === vglModule.materialAttributeType.ShaderProgram) {
      m_shaderProgram = attr;
    }

    m_attributes[attr.type()] = attr;
    this.modified();
    return true;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return shader program used by the material
   *
   * @returns {vglModule.shaderProgram}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.shaderProgram = function() {
    return m_shaderProgram;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Activate the material
   *
   * @param renderState
   */
  ////////////////////////////////////////////////////////////////////////////
  this.render = function(renderState) {
    this.bind(renderState);
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Deactivate the material
   *
   * @param renderState
   */
  ////////////////////////////////////////////////////////////////////////////
  this.remove = function(renderState) {
    this.undoBind(renderState);
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Bind and activate material states
   *
   * @param renderState
   */
  ////////////////////////////////////////////////////////////////////////////
  this.bind = function(renderState) {
    var key = null;

    for (key in m_attributes) {
      if (m_attributes.hasOwnProperty(key)) {
        m_attributes[key].bind(renderState);
      }
    }

    for (key in m_textureAttributes) {
      if (m_textureAttributes.hasOwnProperty(key)) {
        m_textureAttributes[key].bind(renderState);
      }
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Undo-bind and de-activate material states
   *
   * @param renderState
   */
  ////////////////////////////////////////////////////////////////////////////
  this.undoBind = function(renderState) {
    var key = null;
    for (key in m_attributes) {
      if (m_attributes.hasOwnProperty(key)) {
        m_attributes[key].undoBind(renderState);
      }
    }

    for (key in m_textureAttributes) {
      if (m_textureAttributes.hasOwnProperty(key)) {
        m_textureAttributes[key].undoBind(renderState);
      }
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Bind vertex data
   *
   * @param renderState
   * @param key
   */
  ////////////////////////////////////////////////////////////////////////////
  this.bindVertexData = function(renderState, key) {
    var i = null;

    for (i in m_attributes) {
      if (m_attributes.hasOwnProperty(i)) {
        m_attributes[i].bindVertexData(renderState, key);
      }
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Undo bind vertex data
   *
   * @param renderState
   * @param key
   */
  ////////////////////////////////////////////////////////////////////////////
  this.undoBindVertexData = function(renderState, key) {
    var i = null;

    for (i in m_attributes) {
      if (m_attributes.hasOwnProperty(i)) {
        m_attributes.undoBindVertexData(renderState, key);
      }
    }
  };

  return this;
};

vglModule.material.RenderBin = {
  "Base" : 0,
  "Default" : 100,
  "Opaque" : 100,
  "Transparent" : 1000,
  "Overlay" : 10000
};

inherit(vglModule.material, vglModule.object);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, bitwise:true, indent: 2*/

/*global vglModule, gl, ogs, vec2, vec3, vec4, mat4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class renderState
 *
 * @returns {vglModule.renderState}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.renderState = function() {
  'use strict';

  this.m_modelViewMatrix = mat4.create();
  this.m_normalMatrix = mat4.create();
  this.m_projectionMatrix = null;
  this.m_material = null;
  this.m_mapper = null;
};

////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class renderer *
 *
 * @returns {vglModule.renderer}
 */
////////////////////////////////////////////////////////////////////////////
vglModule.renderer = function() {
  'use strict';

  if (!(this instanceof vglModule.renderer)) {
    return new vglModule.renderer();
  }
  vglModule.object.call(this);

  /** @private */
  var m_backgroundColor = [0.0, 0.0, 0.0, 1.0],
      m_sceneRoot = new vglModule.groupNode(),
      m_camera = new vglModule.camera(),
      m_nearClippingPlaneTolerance = null,
      m_x = 0,
      m_y = 0,
      m_width = 0,
      m_height = 0;

  m_camera.addChild(m_sceneRoot);

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get width of the renderer
   */
  ////////////////////////////////////////////////////////////////////////////
  this.width = function() {
    return m_width;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get height of the renderer
   */
  ////////////////////////////////////////////////////////////////////////////
  this.height = function() {
    return m_height;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get background color
   */
  ////////////////////////////////////////////////////////////////////////////
  this.backgroundColor = function() {
    return m_backgroundColor;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set background color of the renderer
   *
   * @param r
   * @param g
   * @param b
   * @param a
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setBackgroundColor = function(r, g, b, a) {
    m_backgroundColor[0] = r;
    m_backgroundColor[1] = g;
    m_backgroundColor[2] = b;
    m_backgroundColor[3] = a;

    this.modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get scene root
   *
   * @returns {vglModule.groupNode}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.sceneRoot = function() {
    return m_sceneRoot;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get main camera of the renderer
   *
   * @returns {vglModule.camera}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.camera = function() {
    return m_camera;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Render the scene
   */
  ////////////////////////////////////////////////////////////////////////////
  this.render = function() {
    var i, renSt, children, actor = null, sortedActors = [],
        mvMatrixInv = mat4.create();

    gl.clearColor(m_backgroundColor[0], m_backgroundColor[1],
      m_backgroundColor[2], m_backgroundColor[3]);
    gl.enable(gl.DEPTH_TEST);
    gl.depthFunc(gl.LEQUAL);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    renSt = new vglModule.renderState();
    children = m_sceneRoot.children();

    for ( i = 0; i < children.length; ++i) {
      actor = children[i];
      actor.computeBounds();
      if (!actor.visible()) {
        continue;
      }

      sortedActors.push([actor.material().binNumber(), actor]);
    }

    // Now perform sorting
    sortedActors.sort(function(a, b) {return a[0] - b[0];});

    for ( i = 0; i < sortedActors.length; ++i) {
      actor = sortedActors[i][1];

      if (actor.referenceFrame() ===
          vglModule.boundingObject.ReferenceFrame.Relative) {
        mat4.multiply(renSt.m_modelViewMatrix, m_camera.viewMatrix(),
          actor.matrix());
        renSt.m_projectionMatrix = m_camera.projectionMatrix();
      } else {
        renSt.m_modelViewMatrix = actor.matrix();
        renSt.m_projectionMatrix = mat4.create();
        mat4.ortho(renSt.m_projectionMatrix, 0, m_width, 0, m_height, -1, 1);
      }

      mat4.invert(mvMatrixInv, renSt.m_modelViewMatrix);
      mat4.transpose(renSt.m_normalMatrix, mvMatrixInv);
      renSt.m_material = actor.material();
      renSt.m_mapper = actor.mapper();

      // TODO Fix this shortcut
      renSt.m_material.render(renSt);
      renSt.m_mapper.render(renSt);
      renSt.m_material.remove(renSt);
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Automatically set up the camera based on visible actors
   */
  ////////////////////////////////////////////////////////////////////////////
  this.resetCamera = function() {
    m_camera.computeBounds();

    var vn = m_camera.directionOfProjection(),
        visibleBounds = m_camera.bounds(),
        center = [
          (visibleBounds[0] + visibleBounds[1]) / 2.0,
          (visibleBounds[2] + visibleBounds[3]) / 2.0,
          (visibleBounds[4] + visibleBounds[5]) / 2.0
        ],
        diagonals = [
          visibleBounds[1] - visibleBounds[0],
          visibleBounds[3] - visibleBounds[2],
          visibleBounds[5] - visibleBounds[4]
        ],
        radius = 0.0,
        aspect = m_camera.viewAspect(),
        angle = m_camera.viewAngle(),
        distance = null,
        vup = null;

    if (diagonals[0] > diagonals[1]) {
      if (diagonals[0] > diagonals[2]) {
        radius = diagonals[0] / 2.0;
      } else {
        radius = diagonals[2] / 2.0;
      }
    } else {
      if (diagonals[1] > diagonals[2]) {
        radius = diagonals[1] / 2.0;
      } else {
        radius = diagonals[2] / 2.0;
      }
    }

    // @todo Need to figure out what's happening here
    if (aspect >= 1.0) {
      angle = 2.0 * Math.atan(Math.tan(angle * 0.5) / aspect);
    } else {
      angle = 2.0 * Math.atan(Math.tan(angle * 0.5) * aspect);
    }

    distance = radius / Math.sin(angle * 0.5);
    vup = m_camera.viewUpDirection();

    if (Math.abs(vec3.dot(vup, vn)) > 0.999) {
      m_camera.setViewUpDirection(-vup[2], vup[0], vup[1]);
    }

    m_camera.setFocalPoint(center[0], center[1], center[2]);
    m_camera.setPosition(center[0] + distance * -vn[0],
      center[1] + distance * -vn[1], center[2] + distance * -vn[2]);

    this.resetCameraClippingRange(visibleBounds);
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Recalculate camera's clipping range
   */
  ////////////////////////////////////////////////////////////////////////////
  this.resetCameraClippingRange = function(bounds) {
    var vn = m_camera.viewPlaneNormal(),
        position = m_camera.position(),
        a = -vn[0],
        b = -vn[1],
        c = -vn[2],
        d = -(a*position[0] + b*position[1] + c*position[2]),
        range = vec2.create(),
        dist = null,
        i = null,
        j = null,
        k = null;

    if (typeof bounds === 'undefined') {
      m_camera.computeBounds();
      bounds = m_camera.bounds();
    }

    // Set the max near clipping plane and the min far clipping plane
    range[0] = a * bounds[0] + b * bounds[2] + c * bounds[4] + d;
    range[1] = 1e-18;

    // Find the closest / farthest bounding box vertex
    for (k = 0; k < 2; k++ ) {
      for (j = 0; j < 2; j++) {
        for (i = 0; i < 2; i++) {
          dist = a * bounds[i] + b * bounds[2 + j] + c * bounds[4 + k] + d;
          range[0] = (dist < range[0]) ? (dist) : (range[0]);
          range[1] = (dist > range[1]) ? (dist) : (range[1]);
        }
      }
    }

    // Do not let the range behind the camera throw off the calculation.
    if (range[0] < 0.0) {
      range[0] = 0.0;
    }

    // Give ourselves a little breathing room
    range[0] = 0.99 * range[0] - (range[1] - range[0]) * 0.5;
    range[1] = 1.01 * range[1] + (range[1] - range[0]) * 0.5;

    // Make sure near is not bigger than far
    range[0] = (range[0] >= range[1]) ? (0.01 * range[1]) : (range[0]);

    // Make sure near is at least some fraction of far - this prevents near
    // from being behind the camera or too close in front. How close is too
    // close depends on the resolution of the depth buffer.
    if (!m_nearClippingPlaneTolerance) {
      m_nearClippingPlaneTolerance = 0.01;

      if (gl !== null && gl.getParameter(gl.DEPTH_BITS) > 16) {
        m_nearClippingPlaneTolerance = 0.001;
      }
    }

    // make sure the front clipping range is not too far from the far clippnig
    // range, this is to make sure that the zbuffer resolution is effectively
    // used.
    if (range[0] < m_nearClippingPlaneTolerance*range[1]) {
       range[0] = m_nearClippingPlaneTolerance*range[1];
    }

    m_camera.setClippingRange(range[0], range[1]);
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Resize viewport given a width and height
   */
  ////////////////////////////////////////////////////////////////////////////
  this.resize = function(width, height) {
    // @note: where do m_x and m_y come from?
    this.positionAndResize(m_x, m_y, width, height);
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Resize viewport given a position, width and height
   */
  ////////////////////////////////////////////////////////////////////////////
  this.positionAndResize = function(x, y, width, height) {
    // TODO move this code to camera
    if (x < 0 || y < 0 || width < 0 || height < 0) {
      console.log('[error] Invalid position and resize values',
        x, y, width, height);
    }

    m_width = width;
    m_height = height;

    gl.viewport(x, y, m_width, m_height);
    m_camera.setViewAspect(m_width / m_height);
    this.modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Add new actor to the collection
   *
   * @param actor
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.addActor = function(actor) {
    if (actor instanceof vglModule.actor) {
      m_sceneRoot.addChild(actor);
      this.modified();
      return true;
    }

    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Add an array of actors to the collection
   */
  ////////////////////////////////////////////////////////////////////////////
  this.addActors = function(actors) {
    var i = null;
    if (actors instanceof Array) {
      for (i = 0; i < actors.length; ++i) {
        m_sceneRoot.addChild(actors[i]);
      }
      this.modified();
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Remove the actor from the collection
   *
   * @param actor
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.removeActor = function(actor) {
    if (m_sceneRoot.children().indexOf(actor) !== -1) {
      m_sceneRoot.removeChild(actor);
      this.modified();
      return true;
    }

    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Remove actors from the collection
   *
   * @param actors
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.removeActors = function(actors) {
    if (!(actors instanceof Array)) {
      return false;
    }

    var i;
    for (i = 0; i < actors.length; ++i) {
      m_sceneRoot.removeChild(actors[i]);
    }
    this.modified();
    return true;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Remove all actors for a renderer
   *
   * @returns {*}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.removeAllActors = function() {
    return m_sceneRoot.removeChildren();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Transform a point in the world space to display space
   */
  ////////////////////////////////////////////////////////////////////////////
  this.worldToDisplay = function(worldPt, viewMatrix, projectionMatrix, width,
                                 height) {
    var viewProjectionMatrix = mat4.create(),
        winX = null,
        winY = null,
        winZ = null,
        winW = null,
        clipPt = null;


    mat4.multiply(viewProjectionMatrix, projectionMatrix, viewMatrix);

    // Transform world to clipping coordinates
    clipPt = vec4.create();
    vec4.transformMat4(clipPt, worldPt, viewProjectionMatrix);

    if (clipPt[3] !== 0.0) {
      clipPt[0] = clipPt[0] / clipPt[3];
      clipPt[1] = clipPt[1] / clipPt[3];
      clipPt[2] = clipPt[2] / clipPt[3];
      clipPt[3] = 1.0;
    }

    winX = Math.round((((clipPt[0]) + 1) / 2.0) * width);
    // We calculate -point3D.getY() because the screen Y axis is
    // oriented top->down
    winY = Math.round(((1 - clipPt[1]) / 2.0) * height);
    winZ = clipPt[2];
    winW = clipPt[3];

    return vec4.fromValues(winX, winY, winZ, winW);
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Transform a point in display space to world space
   * @param displayPt
   * @param viewMatrix
   * @param projectionMatrix
   * @param width
   * @param height
   * @returns {vec4}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.displayToWorld = function(displayPt, viewMatrix, projectionMatrix,
                                 width, height) {
    var x = (2.0 * displayPt[0] / width) - 1,
        y = -(2.0 * displayPt[1] / height) + 1,
        z = displayPt[2],
        viewProjectionInverse = mat4.create(),
        worldPt = null;

    mat4.multiply(viewProjectionInverse, projectionMatrix, viewMatrix);
    mat4.invert(viewProjectionInverse, viewProjectionInverse);

    worldPt = vec4.fromValues(x, y, z, 1);
    vec4.transformMat4(worldPt, worldPt, viewProjectionInverse);
    if (worldPt[3] !== 0.0) {
      worldPt[0] = worldPt[0] / worldPt[3];
      worldPt[1] = worldPt[1] / worldPt[3];
      worldPt[2] = worldPt[2] / worldPt[3];
      worldPt[3] = 1.0;
    }

    return worldPt;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get the focusDisplayPoint
   * @returns {vec4}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.focusDisplayPoint = function() {
    var focalPoint = m_camera.focalPoint(),
      focusWorldPt = vec4.fromValues(
        focalPoint[0], focalPoint[1], focalPoint[2], 1);

    return this.worldToDisplay(
      focusWorldPt, m_camera.viewMatrix(),
      m_camera.projectionMatrix(), m_width, m_height);
  };

  return this;
};

inherit(vglModule.renderer, vglModule.object);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

// TODO Current we support only one context
var gl = 0;

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class renderWindow
 *
 * @class
 * @returns {vglModule.renderWindow}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.renderWindow = function(canvas) {
  'use strict';

  if (!(this instanceof vglModule.renderWindow)) {
    return new vglModule.renderWindow(canvas);
  }
  vglModule.object.call(this);

  /** @private */
  var m_x = 0,
      m_y = 0,
      m_width = 400,
      m_height = 400,
      m_canvas = canvas,
      m_activeRender = null,
      m_renderers = [];

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get size of the render window
   *
   * @returns {Array}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.windowSize = function() {
    return [ m_width, m_height ];
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set size of the render window
   *
   * @param width
   * @param height
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setWindowSize = function(width, height) {

    if (m_width !== width || m_height !== height) {
      m_width = width;
      m_height = height;

      this.modified();

      return true;
    }

    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get window position (top left coordinates)
   *
   * @returns {Array}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.windowPosition = function() {
    return [ m_x, m_y ];
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set window position (top left coordinates)
   *
   * @param x
   * @param y
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setWindowPosition = function(x, y) {
    if ((m_x !== x) || (m_y !== y)) {
      m_x = x;
      m_y = y;
      this.modified();
      return true;
    }
    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get active renderer of the the render window
   *
   * @returns vglModule.renderer
   */
  ////////////////////////////////////////////////////////////////////////////
  this.activeRenderer = function() {
    return m_activeRender;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Add renderer to the render window
   *
   * @param ren
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.addRenderer = function(ren) {
    if (this.hasRenderer(ren) === false) {
      m_renderers.push(ren);
      if (m_activeRender === null) {
        m_activeRender = ren;
      }
      this.modified();
      return true;
    }
    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Remove renderer from the render window
   *
   * @param ren
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.removeRenderer = function(ren) {
    var index = m_renderers.indexOf(ren);
    if (index !== -1) {
      if (m_activeRender === ren) {
        m_activeRender = null;
      }
      m_renderers = m_renderers.splice(index, 1);
      this.modified();
      return true;
    }
    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return a renderer at a given index
   *
   * @param index
   * @returns {vglModule.renderer}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.getRenderer = function(index) {
    if (index < m_renderers.length) {
      return m_renderers[index];
    }

    console.log("[WARNING] Out of index array");
    return null;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Check if the renderer exists
   *
   * @param ren
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.hasRenderer = function(ren) {
    var i;
    for (i = 0; i < m_renderers.length; ++i) {
      if (ren === m_renderers[i]) {
        return true;
      }
    }

    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Resize window
   *
   * @param width
   * @param height
   */
  ////////////////////////////////////////////////////////////////////////////
  this.resize = function(width, height) {
    this.positionAndResize(m_x, m_y, width, height);
    this.modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Resize and reposition the window
   *
   * @param x
   * @param y
   * @param width
   * @param height
   */
  ////////////////////////////////////////////////////////////////////////////
  this.positionAndResize = function(x, y, width, height) {
    m_x = x;
    m_y = y;
    m_width = width;
    m_height = height;
    var i;
    for (i = 0; i < m_renderers.length; ++i) {
      m_renderers[i].positionAndResize(m_x, m_y, m_width, m_height);
    }
    this.modified();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Create the window
   *
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.createWindow = function() {
    // Initialize the global variable gl to null.
    gl = null;

    try {
      // Try to grab the standard context. If it fails, fallback to
      // experimental.
      gl = m_canvas.getContext("webgl")
           || m_canvas.getContext("experimental-webgl");

      // Set width and height of renderers if not set already
      var i;
      for (i = 0; i < m_renderers.length; ++i) {
        if ((m_renderers[i].width() > m_width) || m_renderers[i].width() === 0
            || (m_renderers[i].height() > m_height)
            || m_renderers[i].height() === 0) {
          m_renderers[i].resize(m_x, m_y, m_width, m_height);
        }
      }

      return true;
    }
    catch (e) {
    }

    // If we don't have a GL context, give up now
    if (!gl) {
      console("[ERROR] Unable to initialize WebGL. Your browser may not support it.");
    }

    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Delete this window and release any graphics resources
   */
  ////////////////////////////////////////////////////////////////////////////
  this.deleteWindow = function() {
    // TODO
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Render the scene
   */
  ////////////////////////////////////////////////////////////////////////////
  this.render = function() {
    var i;
    for (i = 0; i < m_renderers.length; ++i) {
      m_renderers[i].render();
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get the focusDisplayPoint from the activeRenderer
   * @returns {vec4}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.focusDisplayPoint = function() {
    return m_activeRender.focusDisplayPoint();
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Transform a point in display space to world space
   * @param {Number} x
   * @param {Number} y
   * @param {vec4} focusDisplayPoint
   * @returns {vec4}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.displayToWorld = function(x, y, focusDisplayPoint) {
    var camera = m_activeRender.camera();
    if(!focusDisplayPoint) {
      focusDisplayPoint = m_activeRender.focusDisplayPoint();
    }

    return m_activeRender.displayToWorld(
      vec4.fromValues(x, y, focusDisplayPoint[2], 1.0), camera.viewMatrix(),
      camera.projectionMatrix(), m_width, m_height);
  };

  return this;
};

inherit(vglModule.renderWindow, vglModule.object);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class interactorStyle
 *
 * @class vglModule.interactorStyle
 * interactorStyle is a base class for all interactor styles
 * @returns {vglModule.interactorStyle}
 */
////////////////////////////////////////////////////////////////////////////
vglModule.interactorStyle = function() {
  'use strict';

  if (!(this instanceof vglModule.interactorStyle)) {
    return new vglModule.interactorStyle();
  }
  vglModule.object.call(this);

  // Private member variables
  var m_that = this,
      m_viewer = null;

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return viewer referenced by the interactor style
   *
   * @returns {null}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.viewer = function() {
    return m_viewer;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set viewer for the interactor style
   *
   * @param viewer
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setViewer = function(viewer) {
    if (viewer !== m_viewer) {
      m_viewer = viewer;
      $(m_viewer).on(vglModule.command.mousePressEvent, m_that.handleMouseDown);
      $(m_viewer).on(vglModule.command.mouseReleaseEvent, m_that.handleMouseUp);
      $(m_viewer).on(vglModule.command.mouseMoveEvent, m_that.handleMouseMove);
      $(m_viewer).on(vglModule.command.keyPressEvent, m_that.handleKeyPress);
      $(m_viewer).on(vglModule.command.mouseContextMenuEvent,
                     m_that.handleContextMenu);
      this.modified();
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Handle mouse down event
   *
   * @param event
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.handleMouseDown = function(event) {
    return true;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Handle mouse up event
   *
   * @param event
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.handleMouseUp = function(event) {
    return true;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Handle mouse move event
   *
   * @param event
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.handleMouseMove = function(event) {
    return true;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Handle key press event
   *
   * @param event
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.handleKeyPress = function(event) {
    return true;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Handle context menu event
   *
   * @param event
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.handleContextMenu = function(event) {
    return true;
  };

  return this;
};

inherit(vglModule.interactorStyle, vglModule.object);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of trackballInteractorStyle
 *
 * @class vglModule.trackballInteractorStyle
 * @returns {vglModule.trackballInteractorStyle}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.trackballInteractorStyle = function() {
  'use strict';

  if (!(this instanceof vglModule.trackballInteractorStyle)) {
    return new vglModule.trackballInteractorStyle();
  }
  vglModule.interactorStyle.call(this);
  var m_that = this,
      m_leftMouseButtonDown = false,
      m_rightMouseButtonDown = false,
      m_middleMouseButtonDown = false,
      m_width,
      m_height,
      m_renderer,
      m_camera,
      m_outsideCanvas,
      m_coords,
      m_currentMousePos,
      m_focalPoint,
      m_focusWorldPt,
      m_focusDisplayPt,
      m_displayPt1,
      m_displayPt2,
      m_worldPt1,
      m_worldPt2,
      m_dx,
      m_dy,
      m_dz,
      m_zTrans,
      m_mouseLastPos = {
        x: 0,
        y: 0
      };


  /////////////////////////////////////////////////////////////////////////////
  /**
   * Handle mouse move event
   *
   * @param event
   * @returns {boolean}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.handleMouseMove = function(event) {
    var canvas = m_that.viewer().canvas();
    m_width = m_that.viewer().renderWindow().windowSize()[0];
    m_height = m_that.viewer().renderWindow().windowSize()[1];
    m_renderer = m_that.viewer().renderWindow().activeRenderer();
    m_camera = m_renderer.camera();
    m_outsideCanvas = false;
    m_coords = canvas.relMouseCoords(event);
    m_currentMousePos = {
      x: 0,
      y: 0
    };
    if ((m_coords.x < 0) || (m_coords.x > m_width)) {
      m_currentMousePos.x = 0;
      m_outsideCanvas = true;
    } else {
      m_currentMousePos.x = m_coords.x;
    }
    if ((m_coords.y < 0) || (m_coords.y > m_height)) {
      m_currentMousePos.y = 0;
      m_outsideCanvas = true;
    } else {
      m_currentMousePos.y = m_coords.y;
    }
    if (m_outsideCanvas === true) {
      return;
    }
    m_focalPoint = m_camera.focalPoint();
    m_focusWorldPt = vec4.fromValues(m_focalPoint[0], m_focalPoint[1], m_focalPoint[2], 1);
    m_focusDisplayPt = m_renderer.worldToDisplay(m_focusWorldPt, m_camera.viewMatrix(),
      m_camera.projectionMatrix(), m_width, m_height);
    m_displayPt1 = vec4.fromValues(
      m_currentMousePos.x, m_currentMousePos.y, m_focusDisplayPt[2], 1.0);
    m_displayPt2 = vec4.fromValues(
      m_mouseLastPos.x, m_mouseLastPos.y, m_focusDisplayPt[2], 1.0);
    m_worldPt1 = m_renderer.displayToWorld(
      m_displayPt1, m_camera.viewMatrix(), m_camera.projectionMatrix(), m_width, m_height);
    m_worldPt2 = m_renderer.displayToWorld(
      m_displayPt2, m_camera.viewMatrix(), m_camera.projectionMatrix(), m_width, m_height);

    m_dx = m_worldPt1[0] - m_worldPt2[0];
    m_dy = m_worldPt1[1] - m_worldPt2[1];
    m_dz = m_worldPt1[2] - m_worldPt2[2];

    if (m_middleMouseButtonDown) {
      m_camera.pan(-m_dx, -m_dy, -m_dz);
      $(m_that).trigger(vglModule.command.middleButtonPressEvent);
    }
    if (m_leftMouseButtonDown) {
      m_camera.rotate((m_mouseLastPos.x - m_currentMousePos.x),
      (m_mouseLastPos.y - m_currentMousePos.y));
      m_renderer.resetCameraClippingRange();
      $(m_that).trigger(vglModule.command.leftButtonPressEvent);
    }
    if (m_rightMouseButtonDown) {
      m_zTrans = (m_currentMousePos.y - m_mouseLastPos.y) / m_height;

      // Calculate zoom scale here
      if (m_zTrans > 0) {
        m_camera.zoom(1 - Math.abs(m_zTrans));
      } else {
        m_camera.zoom(1 + Math.abs(m_zTrans));
      }
      m_renderer.resetCameraClippingRange();
      $(m_that).trigger(vglModule.command.rightButtonPressEvent);
    }
    m_mouseLastPos.x = m_currentMousePos.x;
    m_mouseLastPos.y = m_currentMousePos.y;
    return false;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Handle mouse down event
   *
   * @param event
   * @returns {boolean}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.handleMouseDown = function(event) {
    var canvas = m_that.viewer().canvas();
    if (event.button === 0) {
      m_leftMouseButtonDown = true;
    }
    if (event.button === 1) {
      m_middleMouseButtonDown = true;
    }
    if (event.button === 2) {
      m_rightMouseButtonDown = true;
    }
    m_coords = canvas.relMouseCoords(event);
    if (m_coords.x < 0) {
      m_mouseLastPos.x = 0;
    } else {
      m_mouseLastPos.x = m_coords.x;
    }
    if (m_coords.y < 0) {
      m_mouseLastPos.y = 0;
    } else {
      m_mouseLastPos.y = m_coords.y;
    }
    return false;
  };

  // @note We never get mouse up from scroll bar: See the bug report here
  // http://bugs.jquery.com/ticket/8184
  /////////////////////////////////////////////////////////////////////////////
  /**
   * Handle mouse up event
   *
   * @param event
   * @returns {boolean}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.handleMouseUp = function(event) {
    var canvas = m_that.viewer().canvas();
    if (event.button === 0) {
      m_leftMouseButtonDown = false;
    }
    if (event.button === 1) {
      m_middleMouseButtonDown = false;
    }
    if (event.button === 2) {
      m_rightMouseButtonDown = false;
    }
    return false;
  };

  return this;
};
inherit(vglModule.trackballInteractorStyle, vglModule.interactorStyle);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global window, vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class viewer
 *
 * @param canvas
 * @returns {vglModule.viewer}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.viewer = function(canvas) {
  'use strict';

  if (!(this instanceof vglModule.viewer)) {
    return new vglModule.viewer(canvas);
  }

  vglModule.object.call(this);

  var m_that = this,
      m_canvas = canvas,
      m_ready = false,
      m_interactorStyle = null,
      m_renderer = vglModule.renderer(),
      m_renderWindow = vglModule.renderWindow(m_canvas);

  m_renderWindow.addRenderer(m_renderer);

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get canvas of the viewer
   *
   * @returns {*}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.canvas = function() {
    return m_canvas;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Return render window of the viewer
   *
   * @returns {vglModule.renderWindow}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.renderWindow = function() {
    return m_renderWindow;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Initialize the viewer
   *
   * This is a must call or otherwise render context may not initialized
   * properly.
   */
  ////////////////////////////////////////////////////////////////////////////
  this.init = function() {
    if (m_renderWindow !== null) {
      m_renderWindow.createWindow();
      m_ready = true;
    }
    else {
      console.log("[ERROR] No render window attached");
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get interactor style of the viewer
   *
   * @returns {vglModule.interactorStyle}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.interactorStyle = function() {
    return m_interactorStyle;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set interactor style to be used by the viewer
   *
   * @param {vglModule.interactorStyle} style
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setInteractorStyle = function(style) {
    if (style !== m_interactorStyle) {
      m_interactorStyle = style;
      m_interactorStyle.setViewer(this);
      this.modified();
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Handle mouse down event
   *
   * @param event
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.handleMouseDown = function(event) {
    if (m_ready === true) {
      var fixedEvent = $.event.fix(event || window.event);
      // Only prevent default action for right mouse button
      if (event.button === 2) {
        fixedEvent.preventDefault();
      }
      fixedEvent.state = 'down';
      fixedEvent.type = vglModule.command.mousePressEvent;
      $(m_that).trigger(fixedEvent);
    }

    return true;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Handle mouse up event
   *
   * @param event
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.handleMouseUp = function(event) {
    if (m_ready === true) {
      var fixedEvent = $.event.fix(event || window.event);
      fixedEvent.preventDefault();
      fixedEvent.state = 'up';
      fixedEvent.type = vglModule.command.mouseReleaseEvent;
      $(m_that).trigger(fixedEvent);
    }

    return true;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Handle mouse move event
   *
   * @param event
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.handleMouseMove = function(event) {
    if (m_ready === true) {
      var fixedEvent = $.event.fix(event || window.event);
      fixedEvent.preventDefault();
      fixedEvent.type = vglModule.command.mouseMoveEvent;
      $(m_that).trigger(fixedEvent);
    }

    return true;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Handle key press event
   *
   * @param event
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.handleKeyPress = function(event) {
    if (m_ready === true) {
      var fixedEvent = $.event.fix(event || window.event);
      fixedEvent.preventDefault();
      fixedEvent.type = vglModule.command.keyPressEvent;
      $(m_that).trigger(fixedEvent);
    }

    return true;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Handle context menu event
   *
   * @param event
   * @returns {boolean}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.handleContextMenu = function(event) {
    if (m_ready === true) {
      var fixedEvent = $.event.fix(event || window.event);
      fixedEvent.preventDefault();
      fixedEvent.type = vglModule.command.contextMenuEvent;
      $(m_that).trigger(fixedEvent);
    }

    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get mouse coodinates related to canvas
   *
   * @param event
   * @returns {{x: number, y: number}}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.relMouseCoords = function(event) {
    var totalOffsetX = 0,
        totalOffsetY = 0,
        canvasX = 0,
        canvasY = 0,
        currentElement = m_canvas;

    do {
      totalOffsetX += currentElement.offsetLeft - currentElement.scrollLeft;
      totalOffsetY += currentElement.offsetTop - currentElement.scrollTop;
    } while (currentElement === currentElement.offsetParent);

    canvasX = event.pageX - totalOffsetX;
    canvasY = event.pageY - totalOffsetY;

    return {
      x: canvasX,
      y: canvasY
    };
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Render
   */
  ////////////////////////////////////////////////////////////////////////////
  this.render = function() {
    m_renderWindow.render();
  };

  return this;
};

inherit(vglModule.viewer, vglModule.object);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global gl, vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class shader
 *
 * @param type
 * @returns {vglModule.shader}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.shader = function(type) {
  'use strict';

  if (!(this instanceof vglModule.shader)) {
    return new vglModule.shader(type);
  }
  vglModule.object.call(this);

  var m_shaderHandle = null,
      m_compileTimestamp = vglModule.timestamp(),
      m_shaderType = type,
      m_shaderSource = "";

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Get shader handle
   */
  /////////////////////////////////////////////////////////////////////////////
  this.shaderHandle = function() {
    return m_shaderHandle;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Get type of the shader
   *
   * @returns {*}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.shaderType = function() {
    return m_shaderType;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Get shader source
   *
   * @returns {string}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.shaderSource = function() {
    return m_shaderSource;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Set shader source
   *
   * @param {string} source
   */
  /////////////////////////////////////////////////////////////////////////////
  this.setShaderSource = function(source) {
    m_shaderSource = source;
    this.modified();
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Compile the shader
   *
   * @returns {null}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.compile = function() {
    if (this.getMTime() < m_compileTimestamp.getMTime()) {
      return m_shaderHandle;
    }

    gl.deleteShader(m_shaderHandle);
    m_shaderHandle = gl.createShader(m_shaderType);
    gl.shaderSource(m_shaderHandle, m_shaderSource);
    gl.compileShader(m_shaderHandle);

    // See if it compiled successfully
    if (!gl.getShaderParameter(m_shaderHandle, gl.COMPILE_STATUS)) {
      console.log("[ERROR] An error occurred compiling the shaders: "
                  + gl.getShaderInfoLog(m_shaderHandle));
      console.log(m_shaderSource);
      gl.deleteShader(m_shaderHandle);
      return null;
    }

    m_compileTimestamp.modified();

    return m_shaderHandle;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Attach shader to the program
   *
   * @param programHandle
   */
  /////////////////////////////////////////////////////////////////////////////
  this.attachShader = function(programHandle) {
    gl.attachShader(programHandle, m_shaderHandle);
  };
};

inherit(vglModule.shader, vglModule.object);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, gl, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instace of class shaderProgram
 *
 * @class
 * @returns {vglModule.shaderProgram}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.shaderProgram = function() {
  'use strict';

  if (!(this instanceof vglModule.shaderProgram)) {
    return new vglModule.shaderProgram();
  }
  vglModule.materialAttribute.call(
    this, vglModule.materialAttributeType.ShaderProgram);

  /** @private */
  var m_programHandle = 0,
      m_compileTimestamp = vglModule.timestamp(),
      m_shaders = [],
      m_uniforms = [],
      m_vertexAttributes = {},
      m_uniformNameToLocation = {},
      m_vertexAttributeNameToLocation = {};

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Query uniform location in the program
   *
   * @param name
   * @returns {*}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.queryUniformLocation = function(name) {
    return gl.getUniformLocation(m_programHandle, name);
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Query attribute location in the program
   *
   * @param name
   * @returns {*}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.queryAttributeLocation = function(name) {
    return gl.getAttribLocation(m_programHandle, name);
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Add a new shader to the program
   *
   * @param shader
   * @returns {boolean}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.addShader = function(shader) {
    if (m_shaders.indexOf(shader) > -1) {
      return false;
    }

    var i;
    for (i = 0; i < m_shaders.length; ++i) {
      if (m_shaders[i].shaderType() === shader.shaderType()) {
        m_shaders.splice(m_shaders.indexOf(shader), 1);
      }
    }

    m_shaders.push(shader);

    this.modified();
    return true;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Add a new uniform to the program
   *
   * @param uniform
   * @returns {boolean}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.addUniform = function(uniform) {
    if (m_uniforms.indexOf(uniform) > -1) {
      return false;
    }

    m_uniforms.push(uniform);
    this.modified();
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Add a new vertex attribute to the program
   *
   * @param attr
   * @param key
   */
  /////////////////////////////////////////////////////////////////////////////
  this.addVertexAttribute = function(attr, key) {
    m_vertexAttributes[key] = attr;

    this.modified();
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Get uniform location
   *
   * This method does not perform any query into the program but relies on
   * the fact that it depends on a call to queryUniformLocation earlier.
   *
   * @param name
   * @returns {number}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.uniformLocation = function(name) {
    return m_uniformNameToLocation[name];
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Get attribute location
   *
   * This method does not perform any query into the program but relies on the
   * fact that it depends on a call to queryUniformLocation earlier.
   *
   * @param name
   * @returns {number}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.attributeLocation = function(name) {
    return m_vertexAttributeNameToLocation[name];
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Get uniform object using name as the key
   *
   * @param name
   * @returns {*}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.uniform = function(name) {
    var i;
    for (i = 0; i < m_uniforms.length; ++i) {
      if (m_uniforms[i].name() === name) {
        return m_uniforms[i];
      }
    }

    return null;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Update all uniforms
   *
   * This method should be used directly unless required
   */
  /////////////////////////////////////////////////////////////////////////////
  this.updateUniforms = function() {
    var i;

    for (i = 0; i < m_uniforms.length; ++i) {
      m_uniforms[i].callGL(m_uniformNameToLocation[m_uniforms[i].name()]);
    }
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Link shader program
   *
   * @returns {boolean}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.link = function() {
    gl.linkProgram(m_programHandle);

    // If creating the shader program failed, alert
    if (!gl.getProgramParameter(m_programHandle, gl.LINK_STATUS)) {
      console.log("[ERROR] Unable to initialize the shader program.");
      return false;
    }

    return true;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Use the shader program
   */
  /////////////////////////////////////////////////////////////////////////////
  this.use = function() {
    gl.useProgram(m_programHandle);
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Peform any clean up required when the program gets deleted
   */
  /////////////////////////////////////////////////////////////////////////////
  this.cleanUp = function() {
    this.deleteVertexAndFragment();
    this.deleteProgram();
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Delete the shader program
   */
  /////////////////////////////////////////////////////////////////////////////
  this.deleteProgram = function() {
    gl.deleteProgram(m_programHandle);
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Delete vertex and fragment shaders
   */
  /////////////////////////////////////////////////////////////////////////////
  this.deleteVertexAndFragment = function() {
    var i;
    for (i = 0; i < m_shaders.length; ++i) {
      gl.deleteShader(m_shaders[i].shaderHandle());
    }
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Bind the program with its shaders
   *
   * @param renderState
   * @returns {boolean}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.bind = function(renderState) {
    var i = 0;

    if (m_programHandle === 0
        || (m_compileTimestamp.getMTime() < this.getMTime())) {
      m_programHandle = gl.createProgram();

      if (m_programHandle === 0) {
        console.log("[ERROR] Cannot create Program Object");
        return false;
      }

      // Compile shaders
      for (i = 0; i < m_shaders.length; ++i) {
        m_shaders[i].compile();
        m_shaders[i].attachShader(m_programHandle);
      }

      this.bindAttributes();

      // link program
      if (!this.link()) {
        console.log("[ERROR] Failed to link Program");
        this.cleanUp();
      }

      this.use();
      this.bindUniforms();
      m_compileTimestamp.modified();
    }
    else {
      this.use();
    }

    // Call update callback.
    for (i = 0; i < m_uniforms.length; ++i) {
      m_uniforms[i].update(renderState, this);
    }

    // Now update values to GL.
    this.updateUniforms();
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Undo binding of the shader program
   *
   * @param renderState
   */
  /////////////////////////////////////////////////////////////////////////////
  this.undoBind = function(renderState) {
    // Do nothing
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Bind vertex data
   *
   * @param renderState
   * @param key
   */
  /////////////////////////////////////////////////////////////////////////////
  this.bindVertexData = function(renderState, key) {
    if (m_vertexAttributes.hasOwnProperty(key)) {
      m_vertexAttributes[key].bindVertexData(renderState, key);
    }
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Undo bind vetex data
   *
   * @param renderState
   * @param key
   */
  /////////////////////////////////////////////////////////////////////////////
  this.undoBindVertexData = function(renderState, key) {
    if (m_vertexAttributes.hasOwnProperty(key)) {
      m_vertexAttributes[key].undoBindVertexData(renderState, key);
    }
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Bind uniforms
   */
  /////////////////////////////////////////////////////////////////////////////
  this.bindUniforms = function() {
    var i;
    for (i = 0; i < m_uniforms.length; ++i) {
      m_uniformNameToLocation[m_uniforms[i].name()] = this
          .queryUniformLocation(m_uniforms[i].name());
    }
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Bind vertex attributes
   */
  /////////////////////////////////////////////////////////////////////////////
  this.bindAttributes = function() {
    var key, name;
    for (key in m_vertexAttributes) {
      name = m_vertexAttributes[key].name();
      gl.bindAttribLocation(m_programHandle, key, name);
      m_vertexAttributeNameToLocation[name] = key;
    }
  };

  return this;
};

inherit(vglModule.shaderProgram, vglModule.materialAttribute);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global Uint8Array, vglModule, gl, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class texture
 *
 * @class
 * @returns {vglModule.texture}
 */
///////////////////////////////////////////////////////////////////////////////
vglModule.texture = function() {
  'use strict';

  if (!(this instanceof vglModule.texture)) {
    return new vglModule.texture();
  }
  vglModule.materialAttribute.call(
    this, vglModule.materialAttributeType.Texture);

  this.m_width = 0;
  this.m_height = 0;
  this.m_depth = 0;

  this.m_textureHandle = null;
  this.m_textureUnit = 0;

  this.m_pixelFormat = null;
  this.m_pixelDataType = null;

  this.m_internalFormat = null;

  this.m_image = null;

  var m_setupTimestamp = vglModule.timestamp();

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Create texture, update parameters, and bind data
   *
   * @param renderState
   */
  /////////////////////////////////////////////////////////////////////////////
  this.setup = function(renderState) {
    gl.deleteTexture(this.m_textureHandle);
    this.m_textureHandle = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, this.m_textureHandle);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.pixelStorei(gl.UNPACK_ALIGNMENT, 1);
    gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);

    if (this.m_image !== null) {
      this.updateDimensions();
      this.computeInternalFormatUsingImage();

      // console.log("m_internalFormat " + this.m_internalFormat);
      // console.log("m_pixelFormat " + this.m_pixelFormat);
      // console.log("m_pixelDataType " + this.m_pixelDataType);

      // FOR now support only 2D textures
      gl.texImage2D(gl.TEXTURE_2D, 0, this.m_internalFormat,
                    this.m_pixelFormat, this.m_pixelDataType, this.m_image);
    }
    else {
      gl.texImage2D(gl.TEXTURE_2D, 0, this.m_internalFormat,
                    this.m_pixelFormat, this.m_pixelDataType, null);
    }

    gl.bindTexture(gl.TEXTURE_2D, null);
    m_setupTimestamp.modified();
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Create texture and if already created use it
   *
   * @param renderState
   */
  /////////////////////////////////////////////////////////////////////////////
  this.bind = function(renderState) {
    // TODO Call setup via material setup
    if (this.getMTime() > m_setupTimestamp.getMTime()) {
      this.setup(renderState);
    }

    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, this.m_textureHandle);
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Turn off the use of this texture
   *
   * @param renderState
   */
  /////////////////////////////////////////////////////////////////////////////
  this.undoBind = function(renderState) {
    gl.bindTexture(gl.TEXTURE_2D, null);
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Get image used by the texture
   *
   * @returns {vglModule.image}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.image = function() {
    return this.m_image;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Set image for the texture
   *
   * @param {vglModule.image} image
   * @returns {boolean}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.setImage = function(image) {
    if (image !== null) {
      this.m_image = image;
      this.updateDimensions();
      this.modified();
      return true;
    }

    return false;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Get texture unit of the texture
   *
   * @returns {number}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.textureUnit = function() {
    return this.m_textureUnit;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Set texture unit of the texture. Default is 0.
   *
   * @param {number} unit
   * @returns {boolean}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.setTextureUnit = function(unit) {
    if (this.m_textureUnit === unit) {
      return false;
    }

    this.m_textureUnit = unit;
    this.modified();
    return true;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Get width of the texture
   *
   * @returns {*}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.width = function() {
    return this.m_width;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Set width of the texture
   *
   * @param {number} width
   * @returns {boolean}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.setWidth = function(width) {
    if (this.m_image === null) {
      return false;
    }

    this.m_width = width;
    this.modified();

    return true;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Get depth of the texture
   *
   * @returns {number}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.depth = function() {
    return this.m_depth;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Set depth of the texture
   *
   * @param {number} depth
   * @returns {boolean}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.setDepth = function(depth) {
    if (this.m_image === null) {
      return false;
    }

    this.m_depth = depth;
    this.modified();
    return true;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Get the texture handle (id) of the texture
   *
   * @returns {*}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.textureHandle = function() {
    return this.m_textureHandle;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Get internal format of the texture
   *
   * @returns {*}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.internalFormat = function() {
    return this.m_internalFormat;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Set internal format of the texture
   *
   * @param internalFormat
   * @returns {boolean}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.setInternalFormat = function(internalFormat) {
    if (this.m_internalFormat !== internalFormat) {
      this.m_internalFormat = internalFormat;
      this.modified();
      return true;
    }

    return false;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Get pixel format of the texture
   *
   * @returns {*}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.pixelFormat = function() {
    return this.m_pixelFormat;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Set pixel format of the texture
   *
   * @param pixelFormat
   * @returns {boolean}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.setPixelFormat = function(pixelFormat) {
    if (this.m_image === null) {
      return false;
    }

    this.m_pixelFormat = pixelFormat;
    this.modified();
    return true;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Get pixel data type
   *
   * @returns {*}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.pixelDataType = function() {
    return this.m_pixelDataType;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Set pixel data type
   *
   * @param pixelDataType
   * @returns {boolean}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.setPixelDataType = function(pixelDataType) {
    if (this.m_image === null) {
      return false;
    }

    this.m_pixelDataType = pixelDataType;

    this.modified();

    return true;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Compute internal format of the texture
   */
  /////////////////////////////////////////////////////////////////////////////
  this.computeInternalFormatUsingImage = function() {
    // Currently image does not define internal format
    // and hence it's pixel format is the only way to query
    // information on how color has been stored.
    // switch (this.m_image.pixelFormat()) {
    // case gl.RGB:
    // this.m_internalFormat = gl.RGB;
    // break;
    // case gl.RGBA:
    // this.m_internalFormat = gl.RGBA;
    // break;
    // case gl.Luminance:
    // this.m_internalFormat = gl.Luminance;
    // break;
    // case gl.LuminanceAlpha:
    // this.m_internalFormat = gl.LuminanceAlpha;
    // break;
    // // Do nothing when image pixel format is none or undefined.
    // default:
    // break;
    // };

    // TODO Fix this
    this.m_internalFormat = gl.RGBA;
    this.m_pixelFormat = gl.RGBA;
    this.m_pixelDataType = gl.UNSIGNED_BYTE;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Update texture dimensions
   */
  /////////////////////////////////////////////////////////////////////////////
  this.updateDimensions = function() {
    if (this.m_image !== null) {
      this.m_width = this.m_image.width;
      this.m_height = this.m_image.height;
      this.m_depth = 0; // Only 2D images are supported now
    }
  };

  return this;
};

inherit(vglModule.texture, vglModule.materialAttribute);

///////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class lookupTable
 *
 * @class
 * @returns {vglModule.lookupTable}
 */
///////////////////////////////////////////////////////////////////////////////
vglModule.lookupTable = function() {
  'use strict';

  if (!(this instanceof vglModule.lookupTable)) {
    return new vglModule.lookupTable();
  }
  vglModule.texture.call(this);

  var m_setupTimestamp = vglModule.timestamp(),
      m_range = [0,0];

  this.m_colorTable = //paraview bwr colortable
	  [0.07514311,0.468049805,1,1,
	   0.247872569,0.498782363,1,1,
	   0.339526309,0.528909511,1,1,
	   0.409505078,0.558608486,1,1,
	   0.468487184,0.588057293,1,1,
	   0.520796675,0.617435078,1,1,
	   0.568724526,0.646924167,1,1,
	   0.613686735,0.676713218,1,1,
	   0.656658579,0.707001303,1,1,
	   0.698372844,0.738002964,1,1,
	   0.739424025,0.769954435,1,1,
	   0.780330104,0.803121429,1,1,
	   0.821573924,0.837809045,1,1,
	   0.863634967,0.874374691,1,1,
	   0.907017747,0.913245283,1,1,
	   0.936129275,0.938743558,0.983038586,1,
	   0.943467973,0.943498599,0.943398095,1,
	   0.990146732,0.928791426,0.917447482,1,
	   1,0.88332677,0.861943246,1,
	   1,0.833985467,0.803839606,1,
	   1,0.788626485,0.750707739,1,
	   1,0.746206642,0.701389973,1,
	   1,0.70590052,0.654994046,1,
	   1,0.667019783,0.610806959,1,
	   1,0.6289553,0.568237474,1,
	   1,0.591130233,0.526775617,1,
	   1,0.552955184,0.485962266,1,
	   1,0.513776083,0.445364274,1,
	   1,0.472800903,0.404551679,1,
	   1,0.428977855,0.363073592,1,
	   1,0.380759558,0.320428137,1,
	   0.961891484,0.313155629,0.265499262,1,
	   0.916482116,0.236630659,0.209939162,1].map(
             function(x) {return x*255;});

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Create lookup table, initialize parameters, and bind data to it
   *
   * @param {vglModule.renderState} renderState
   */
  /////////////////////////////////////////////////////////////////////////////
  this.setup = function(renderState) {
    gl.deleteTexture(this.m_textureHandle);
    this.m_textureHandle = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, this.m_textureHandle);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.pixelStorei(gl.UNPACK_ALIGNMENT, 1);

    this.m_width = this.m_colorTable.length/4;
    this.m_height = 1;
    this.m_depth = 0;
    gl.texImage2D(gl.TEXTURE_2D,
                  0, gl.RGBA, this.m_width, this.m_height, this.m_depth,
                  gl.RGBA, gl.UNSIGNED_BYTE, new Uint8Array(this.m_colorTable));

    gl.bindTexture(gl.TEXTURE_2D, null);
    m_setupTimestamp.modified();
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Get color table used by the lookup table
   *
   * @returns {*}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.colorTable = function() {
    return this.m_colorTable;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Set color table used by the lookup table
   *
   * @param colors
   * @returns {boolean}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.setColorTable = function(colors) {
    if (this.m_colorTable === colors) {
      return false;
    }

    this.m_colorTable = colors;
    this.modified();
    return true;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Get scalar range
   *
   * @returns {Array}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.range = function() {
    return m_range;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Set scalar range for the lookup table
   *
   * @param range
   * @returns {boolean}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.setRange = function(range) {
    if (m_range === range) {
      return false;
    }
    m_range = range;
    this.modified();
    return true;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Given a [min,max] range update the lookup table range
   *
   * @param range
   */
  /////////////////////////////////////////////////////////////////////////////
  this.updateRange = function(range) {
    if (!(range instanceof Array)) {
      console.log('[error] Invalid data type for range. Requires array [min,max]');
    }

    if (range[0] < m_range[0]) {
      m_range[0] = range[0];
      this.modified();
    }

    if (range[1] > m_range[1]) {
      m_range[1] = range[1];
      this.modified();
    }
  };

  return this;
};

inherit(vglModule.lookupTable, vglModule.texture);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, gl, ogs, vec2, vec3, vec4, mat3, mat4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class uniform
 *
 * @param type
 * @param name
 * @returns {vglModule.uniform} OpenGL uniform encapsulation
 */
///////////////////////////////////////////////////////////////////////////////
vglModule.uniform = function(type, name) {
  'use strict';

  if (!(this instanceof vglModule.uniform)) {
    return new vglModule.uniform();
  }

  this.getTypeNumberOfComponents = function(type) {
    switch (type) {
      case gl.FLOAT:
      case gl.INT:
      case gl.BOOL:
        return 1;

      case gl.FLOAT_VEC2:
      case gl.INT_VEC2:
      case gl.BOOL_VEC2:
        return 2;

      case gl.FLOAT_VEC3:
      case gl.INT_VEC3:
      case gl.BOOLT_VEC3:
        return 3;

      case gl.FLOAT_VEC4:
      case gl.INT_VEC4:
      case gl.BOOL_VEC4:
        return 4;

      case gl.FLOAT_MAT3:
        return 9;

      case gl.FLOAT_MAT4:
        return 16;

      default:
        return 0;
    }
  };

  var m_type = type,
      m_name = name,
      m_dataArray = [ this.getTypeNumberOfComponents(m_type) ],
      m_numberOfElements = 1;

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Get name of the uniform
   *
   * @returns {*}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.name = function() {
    return m_name;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Get type of the uniform
   *
   * @returns {*}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.type = function() {
    return m_type;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Get value of the uniform
   *
   * @returns {Array}
   */
  /////////////////////////////////////////////////////////////////////////////
  this.get = function() {
    return m_dataArray;
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Set value of the uniform
   *
   * @param value
   */
  /////////////////////////////////////////////////////////////////////////////
  this.set = function(value) {
    var i = 0;
    if (value instanceof mat4.constructor) {
      for (i = 0; i < 16; ++i) {
        m_dataArray[i] = value[i];
      }
    }
    else if (value instanceof mat3.constructor) {
      for (i = 0; i < 9; ++i) {
        m_dataArray[i] = value[i];
      }
    }
    else if (value instanceof vec4.constructor) {
      for (i = 0; i < 4; ++i) {
        m_dataArray[i] = value[i];
      }
    }
    else if (value instanceof vec3.constructor) {
      for (i = 0; i < 3; ++i) {
        m_dataArray[i] = value[i];
      }
    }
    else if (value instanceof vec2.constructor) {
      for (i = 0; i < 2; ++i) {
        m_dataArray[i] = value[i];
      }
    }
    else {
      m_dataArray[0] = value;
    }
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Call GL and pass updated values to the current shader
   *
   * @param location
   */
  /////////////////////////////////////////////////////////////////////////////
  this.callGL = function(location) {
    if (this.m_numberElements < 1) {
      return;
    }

    switch (m_type) {
      case gl.BOOL:
      case gl.INT:
        gl.uniform1iv(location, m_dataArray);
        break;
      case gl.FLOAT:
        gl.uniform1fv(location, m_dataArray);
        break;
      case gl.FLOAT_VEC2:
        gl.uniform2fv(location, m_dataArray);
        break;
      case gl.FLOAT_VEC3:
        gl.uniform3fv(location, m_dataArray);
        break;
      case gl.FLOAT_VEC4:
        gl.uniform4fv(location, m_dataArray);
        break;
      case gl.FLOAT_MAT3:
        gl.uniformMatrix3fv(location, gl.FALSE, m_dataArray);
        break;
      case gl.FLOAT_MAT4:
        gl.uniformMatrix4fv(location, gl.FALSE, m_dataArray);
        break;
      default:
        break;
    }
  };

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Virtual method to update the uniform
   *
   * Should be implemented by the derived class.
   *
   * @param renderState
   * @param program
   */
  /////////////////////////////////////////////////////////////////////////////
  this.update = function(renderState, program) {
    // Should be implemented by the derived class
  };

  return this;
};

///////////////////////////////////////////////////////////////////////////////
/**
 * Create new instance of class modelViewUniform
 *
 * @param name
 * @returns {vglModule.modelViewUniform}
 */
///////////////////////////////////////////////////////////////////////////////
vglModule.modelViewUniform = function(name) {
  'use strict';

  if (!(this instanceof vglModule.modelViewUniform)) {
    return new vglModule.modelViewUniform(name);
  }

  if (name.length === 0) {
    name = "modelViewMatrix";
  }

  vglModule.uniform.call(this, gl.FLOAT_MAT4, name);

  this.set(mat4.create());

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Update the uniform given a render state and shader program
   *
   * @param {vglModule.renderState} renderState
   * @param {vglModule.shaderProgram} program
   */
  /////////////////////////////////////////////////////////////////////////////
  this.update = function(renderState, program) {
    this.set(renderState.m_modelViewMatrix);
  };

  return this;
};

inherit(vglModule.modelViewUniform, vglModule.uniform);

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class projectionUniform
 *
 * @param name
 * @returns {vglModule.projectionUniform}
 */
///////////////////////////////////////////////////////////////////////////////
vglModule.projectionUniform = function(name) {
  'use strict';

  if (!(this instanceof vglModule.projectionUniform)) {
    return new vglModule.projectionUniform(name);
  }

  if (name.length === 0) {
    name = "projectionMatrix";
  }

  vglModule.uniform.call(this, gl.FLOAT_MAT4, name);

  this.set(mat4.create());

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Update the uniform given a render state and shader program
   *
   * @param renderState
   * @param program
   */
  /////////////////////////////////////////////////////////////////////////////
  this.update = function(renderState, program) {
    this.set(renderState.m_projectionMatrix);
  };

  return this;
};

inherit(vglModule.projectionUniform, vglModule.uniform);

///////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class floatUniform
 *
 * @param name
 * @param value
 * @returns {vglModule.floatUniform}
 */
///////////////////////////////////////////////////////////////////////////////
vglModule.floatUniform = function(name, value) {
  'use strict';

  if (!(this instanceof vglModule.floatUniform)) {
    return new vglModule.floatUniform(name, value);
  }

  if (name.length === 0) {
    name = "floatUniform";
  }

  if (!value) {
    value = 1.0;
  }

  vglModule.uniform.call(this, gl.FLOAT, name);

  this.set(value);
};

inherit(vglModule.floatUniform, vglModule.uniform);


///////////////////////////////////////////////////////////////////////////////
/**
 * Create new instance of class normalMatrixUniform
 *
 * @param name
 * @returns {vglModule.normalMatrixUniform}
 */
///////////////////////////////////////////////////////////////////////////////
vglModule.normalMatrixUniform = function(name) {
  'use strict';

  if (!(this instanceof vglModule.normalMatrixUniform)) {
    return new vglModule.normalMatrixUniform(name);
  }

  if (name.length === 0) {
    name = "normalMatrix";
  }

  vglModule.uniform.call(this, gl.FLOAT_MAT4, name);

  this.set(mat4.create());

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Update the uniform given a render state and shader program
   *
   * @param {vglModule.renderState} renderState
   * @param {vglModule.shaderProgram} program
   */
  /////////////////////////////////////////////////////////////////////////////
  this.update = function(renderState, program) {
    this.set(renderState.m_normalMatrix);
  };

  return this;
};

inherit(vglModule.normalMatrixUniform, vglModule.uniform);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, gl, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Keys to identify vertex attributes
 *
 * @type {{Position: number, Normal: number, TextureCoordinate: number,
 *         Color: number, Scalar: number, Scalar2: number, Scalar3: number,
 *         Scalar4: number, Scalar5: number, Scalar6: number, Scalar7: number,
 *         CountAttributeIndex: number}}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.vertexAttributeKeys = {
  "Position" : 0,
  "Normal" : 1,
  "TextureCoordinate" : 2,
  "Color" : 3,
  "Scalar" : 4,
  "Scalar2" : 5,
  "Scalar3" : 6,
  "Scalar4" : 7,
  "Scalar5" : 8,
  "Scalar6" : 9,
  "Scalar7" : 10,
  "CountAttributeIndex" : 11
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of vertexAttribute
 *
 * @param {string} name
 * @returns {vglModule.vertexAttribute}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.vertexAttribute = function(name) {
  'use strict';

  if (!(this instanceof vglModule.vertexAttribute)) {
    return new vglModule.vertexAttribute(name);
  }

  var m_name = name;

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get name of the vertex attribute
   *
   * @returns {string}
   */
  //////////////////////////////////////////////////////////////////////////////
  this.name = function() {
    return m_name;
  };

  //////////////////////////////////////////////////////////////////////////////
  /**
   * Bind vertex data to the given render state
   *
   * @param {vglModule.renderState} renderState
   * @param {vglModule.vertexAttributeKeys} key
   */
  //////////////////////////////////////////////////////////////////////////////
  this.bindVertexData = function(renderState, key) {
    var geometryData = renderState.m_mapper.geometryData(),
        sourceData = geometryData.sourceData(key),
        program = renderState.m_material.shaderProgram();

    gl.vertexAttribPointer(program.attributeLocation(m_name), sourceData
        .attributeNumberOfComponents(key), sourceData.attributeDataType(key),
                           sourceData.normalized(key), sourceData
                               .attributeStride(key), sourceData
                               .attributeOffset(key));

    gl.enableVertexAttribArray(program.attributeLocation(m_name));
  };

  //////////////////////////////////////////////////////////////////////////////
  /**
   * Undo bind vertex data for a given render state
   *
   * @param {vglModule.renderState} renderState
   * @param {vglModule.vertexAttributeKeys} key
   */
  //////////////////////////////////////////////////////////////////////////////
  this.undoBindVertexData = function(renderState, key) {
    var program = renderState.m_material.shaderProgram();

    gl.disableVertexAttribArray(program.attributeLocation(m_name));
  };
};
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class source
 *
 * @returns {vglModule.source}
 */
///////////////////////////////////////////////////////////////////////////////
vglModule.source = function() {
  'use strict';

  if (!(this instanceof vglModule.source)) {
    return new vglModule.source();
  }

  vglModule.object.call(this);

  /////////////////////////////////////////////////////////////////////////////
  /**
   * Virtual function to create a source instance
   */
  /////////////////////////////////////////////////////////////////////////////
  this.create = function() {
  };

  return this;
};

inherit(vglModule.source, vglModule.object);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class planeSource
 *
 * @class
 * @returns {vglModule.planeSource}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.planeSource = function() {
  'use strict';

  if (!(this instanceof vglModule.planeSource)) {
    return new vglModule.planeSource();
  }
  vglModule.source.call(this);

  var m_origin = [ 0.0, 0.0, 0.0 ],
      m_point1 = [ 1.0, 0.0, 0.0 ],
      m_point2 = [ 0.0, 1.0, 0.0 ],
      m_normal = [ 0.0, 0.0, 1.0 ],
      m_xresolution = 1,
      m_yresolution = 1,
      m_geom = null;

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set origin of the plane
   *
   * @param x
   * @param y
   * @param z
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setOrigin = function(x, y, z) {
    m_origin[0] = x;
    m_origin[1] = y;
    m_origin[2] = z;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set point that defines the first axis of the plane
   *
   * @param x
   * @param y
   * @param z
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setPoint1 = function(x, y, z) {
    m_point1[0] = x;
    m_point1[1] = y;
    m_point1[2] = z;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set point that defines the first axis of the plane
   *
   * @param x
   * @param y
   * @param z
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setPoint2 = function(x, y, z) {
    m_point2[0] = x;
    m_point2[1] = y;
    m_point2[2] = z;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Create a plane geometry given input parameters
   *
   * @returns {null}
   */
  ////////////////////////////////////////////////////////////////////////////
  this.create = function() {
    m_geom = new vglModule.geometryData();

    var x = [], tc = [], v1 = [], v2 = [],
        pts = [], i, j, k, ii, numPts, numPolys,
        posIndex = 0, normIndex = 0, colorIndex = 0, texCoordIndex = 0,
        positions = [], normals = [], colors = [],
        texCoords = [], indices = [], tristrip = null,
        sourcePositions = null, sourceColors = null, sourceTexCoords;

    x.length = 3;
    tc.length = 2;
    v1.length = 3;
    v2.length = 3;
    pts.length = 3;

    // Check input
    for (i = 0; i < 3; i++) {
      v1[i] = m_point1[i] - m_origin[i];
      v2[i] = m_point2[i] - m_origin[i];
    }

    // TODO Compute center and normal
    // Set things up; allocate memory
    numPts = (m_xresolution + 1) * (m_yresolution + 1);
    numPolys = m_xresolution * m_yresolution * 2;
    positions.length = 3 * numPts;
    normals.length = 3 * numPts;
    texCoords.length = 2 * numPts;
    indices.length = numPts;

    for (k = 0, i = 0; i < (m_yresolution + 1); i++) {
      tc[1] = i / m_yresolution;

      for (j = 0; j < (m_xresolution + 1); j++) {
        tc[0] = j / m_xresolution;

        for (ii = 0; ii < 3; ii++) {
          x[ii] = m_origin[ii] + tc[0] * v1[ii] + tc[1] * v2[ii];
        }

        positions[posIndex++] = x[0];
        positions[posIndex++] = x[1];
        positions[posIndex++] = x[2];

        colors[colorIndex++] = 1.0;
        colors[colorIndex++] = 1.0;
        colors[colorIndex++] = 1.0;

        normals[normIndex++] = m_normal[0];
        normals[normIndex++] = m_normal[1];
        normals[normIndex++] = m_normal[2];

        texCoords[texCoordIndex++] = tc[0];
        texCoords[texCoordIndex++] = tc[1];
      }
    }

    /// Generate polygon connectivity
    for (i = 0; i < m_yresolution; i++) {
      for (j = 0; j < m_xresolution; j++) {
        pts[0] = j + i * (m_xresolution + 1);
        pts[1] = pts[0] + 1;
        pts[2] = pts[0] + m_xresolution + 2;
        pts[3] = pts[0] + m_xresolution + 1;
      }
    }

    for (i = 0; i < numPts; ++i) {
      indices[i] = i;
    }

    tristrip = new vglModule.triangleStrip();
    tristrip.setIndices(indices);

    sourcePositions = vglModule.sourceDataP3fv();
    sourcePositions.pushBack(positions);

    sourceColors = vglModule.sourceDataC3fv();
    sourceColors.pushBack(colors);

    sourceTexCoords = vglModule.sourceDataT2fv();
    sourceTexCoords.pushBack(texCoords);

    m_geom.addSource(sourcePositions);
    m_geom.addSource(sourceColors);
    m_geom.addSource(sourceTexCoords);
    m_geom.addPrimitive(tristrip);

    return m_geom;
  };
};

inherit(vglModule.planeSource, vglModule.source);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class pointSource
 *
 * @class
 * @returns {vglModule.pointSource}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.pointSource = function() {
  'use strict';

  if (!(this instanceof vglModule.pointSource)) {
    return new vglModule.pointSource();
  }
  vglModule.source.call(this);

  var m_positions = [],
      m_colors = [],
      m_textureCoords = [],
      m_geom = null;

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set positions for the source
   *
   * @param positions
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setPositions = function(positions) {
    if (positions instanceof Array) {
      m_positions = positions;
    }
    else {
      console
          .log("[ERROR] Invalid data type for positions. Array is required.");
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set colors for the points
   *
   * @param colors
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setColors = function(colors) {
    if (colors instanceof Array) {
      m_colors = colors;
    }
    else {
      console.log("[ERROR] Invalid data type for colors. Array is required.");
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set texture coordinates for the points
   *
   * @param texcoords
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setTextureCoordinates = function(texcoords) {
    if (texcoords instanceof Array) {
      m_textureCoords = texcoords;
    }
    else {
      console.log("[ERROR] Invalid data type for "
                  + "texture coordinates. Array is required.");
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Create a point geometry given input parameters
   */
  ////////////////////////////////////////////////////////////////////////////
  this.create = function() {
    m_geom = new vglModule.geometryData();

    if (m_positions.length % 3 !== 0) {
      console.log("[ERROR] Invalid length of the points array");
      return;
    }

    var numPts = m_positions.length / 3,
        i = 0,
        indices = [],
        pointsPrimitive,
        sourcePositions,
        sourceColors,
        sourceTexCoords;

    indices.length = numPts;
    for (i = 0; i < numPts; ++i) {
      indices[i] = i;
    }

    pointsPrimitive = new vglModule.points();
    pointsPrimitive.setIndices(indices);

    sourcePositions = vglModule.sourceDataP3fv();
    sourcePositions.pushBack(m_positions);
    m_geom.addSource(sourcePositions);

    if ((m_colors.length > 0) && m_colors.length === m_positions.length) {
      sourceColors = vglModule.sourceDataC3fv();
      sourceColors.pushBack(m_colors);
      m_geom.addSource(sourceColors);
    }
    else if ((m_colors.length > 0) && m_colors.length !== m_positions.length) {
      console
          .log("[ERROR] Number of colors are different than number of points");
    }

    if ((m_textureCoords.length > 0)
        && m_textureCoords.length === m_positions.length) {
      sourceTexCoords = vglModule.sourceDataT2fv();
      sourceTexCoords.pushBack(m_textureCoords);
      m_geom.addSource(sourceTexCoords);
    }
    else if ((m_textureCoords.length > 0)
             && (m_textureCoords.length / 2) !== (m_positions.length / 3)) {
      console
          .log("[ERROR] Number of texture coordinates are different than number of points");
    }

    m_geom.addPrimitive(pointsPrimitive);

    return m_geom;
  };
};

inherit(vglModule.pointSource, vglModule.source);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class lineSource
 *
 * @class
 * @returns {vglModule.lineSource}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.lineSource = function(positions, colors) {
  'use strict';

  if (!(this instanceof vglModule.lineSource)) {
    return new vglModule.lineSource();
  }
  vglModule.source.call(this);

  var m_positions = positions,
      m_colors = colors,
      m_height = null,
      m_geom = null;

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set start positions for the lines
   *
   * @param positions
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setPositions = function(positions) {
    if (positions instanceof Array) {
      m_positions = positions;
      this.modified();
      return true;
    }

    console
      .log("[ERROR] Invalid data type for positions. Array is required.");
    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Set colors for the lines
   *
   * @param colors
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setColors = function(colors) {
    if (colors instanceof Array) {
      m_colors = colors;
      this.modified();
      return true;
    }

    console.log("[ERROR] Invalid data type for colors. Array is required.");
    return false;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Create a point geometry given input parameters
   */
  ////////////////////////////////////////////////////////////////////////////
  this.create = function() {
    if (!m_positions) {
      console.log("[error] Invalid positions");
      return;
    }

    if (m_positions.length % 3 !== 0) {
      console.log("[error] Line source requires 3d points");
      return;
    }

    if (m_positions.length % 3 !== 0) {
      console.log("[ERROR] Invalid length of the points array");
      return;
    }

    var m_geom = new vglModule.geometryData(),
        numPts = m_positions.length / 3,
        i,
        indices = [],
        linesPrimitive,
        sourcePositions,
        sourceColors;

    indices.length = numPts;

    for (i = 0; i < numPts; ++i) {
      indices[i] = i;
    }

    linesPrimitive = new vglModule.lines();
    linesPrimitive.setIndices(indices);

    sourcePositions = vglModule.sourceDataP3fv();
    sourcePositions.pushBack(m_positions);
    m_geom.addSource(sourcePositions);

    if ( m_colors && (m_colors.length > 0) &&
         m_colors.length === m_positions.length) {
      sourceColors = vglModule.sourceDataC3fv();
      sourceColors.pushBack(m_colors);
      m_geom.addSource(sourceColors);
    }
    else if (m_colors && (m_colors.length > 0) &&
             m_colors.length !== m_positions.length) {
      console
        .log("[error] Number of colors are different than number of points");
    }

    m_geom.addPrimitive(linesPrimitive);

    return m_geom;
  };
};

inherit(vglModule.lineSource, vglModule.source);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global document, vglModule, gl, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class utils
 *
 * Utility class provides helper functions such as functions to create
 * shaders, geometry etc.
 *
 * @returns {vglModule.utils}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils = function() {
  'use strict';

  if (!(this instanceof vglModule.utils)) {
    return new vglModule.utils();
  }
  vglModule.object.call(this);

  return this;
};

inherit(vglModule.utils, vglModule.object);

//////////////////////////////////////////////////////////////////////////////
/**
 * Helper function to compute power of 2 number
 *
 * @param value
 * @param pow
 *
 * @returns {number}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.computePowerOfTwo = function(value, pow) {
  'use strict';
  pow = pow || 1;
  while (pow < value) {
    pow *= 2;
  }
  return pow;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of default vertex shader that uses a texture
 *
 * Helper function to create default vertex shader
 *
 * @param context
 * @returns {vglModule.shader}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createTextureVertexShader = function(context) {
  'use strict';
  var vertexShaderSource = [
        'attribute vec3 vertexPosition;',
        'attribute vec3 textureCoord;',
        'uniform mediump float pointSize;',
        'uniform mat4 modelViewMatrix;',
        'uniform mat4 projectionMatrix;',
        'varying highp vec3 iTextureCoord;',
        'void main(void)',
        '{',
        'gl_PointSize = pointSize;',
        'gl_Position = projectionMatrix * modelViewMatrix * vec4(vertexPosition, 1.0);',
        ' iTextureCoord = textureCoord;', '}' ].join('\n'),
      shader = new vglModule.shader(gl.VERTEX_SHADER);
  shader.setShaderSource(vertexShaderSource);
  return shader;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of default fragment shader that uses a texture
 *
 * Helper function to create default fragment shader with sampler
 *
 * @param context
 * @returns {vglModule.shader}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createTextureFragmentShader = function(context) {
  'use strict';
  var fragmentShaderSource = [
        'varying highp vec3 iTextureCoord;',
        'uniform sampler2D sampler2d;',
        'uniform mediump float opacity;',
        'void main(void) {',
        'gl_FragColor = vec4(texture2D(sampler2d, vec2(iTextureCoord.s, iTextureCoord.t)).xyz, opacity);',
        '}' ].join('\n'),
      shader = new vglModule.shader(gl.FRAGMENT_SHADER);

  shader.setShaderSource(fragmentShaderSource);
  return shader;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create variation of createTextureFragmentShader which uses texture alpha
 *
 * Helper function to create default fragment shader with sampler
 *
 * @param context
 * @returns {vglModule.shader}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createRgbaTextureFragmentShader = function(context) {
  'use strict';
  var fragmentShaderSource = [
        'varying highp vec3 iTextureCoord;',
        'uniform sampler2D sampler2d;',
        'void main(void) {',
        'gl_FragColor = vec4(texture2D(sampler2d, vec2(iTextureCoord.s, iTextureCoord.t)).xyzw);',
        '}' ].join('\n'),
      shader = new vglModule.shader(gl.FRAGMENT_SHADER);

  shader.setShaderSource(fragmentShaderSource);
  return shader;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of default vertex shader
 *
 * Helper function to create default vertex shader
 *
 * @param context
 * @returns {vglModule.shader}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createVertexShader = function(context) {
  'use strict';
  var vertexShaderSource = [
        'attribute vec3 vertexPosition;',
        'attribute vec3 vertexColor;',
        'uniform mediump float pointSize;',
        'uniform mat4 modelViewMatrix;',
        'uniform mat4 projectionMatrix;',
        'varying mediump vec3 iVertexColor;',
        'varying highp vec3 iTextureCoord;',
        'void main(void)',
        '{',
        'gl_PointSize = pointSize;',
        'gl_Position = projectionMatrix * modelViewMatrix * vec4(vertexPosition, 1.0);',
        ' iVertexColor = vertexColor;', '}' ].join('\n'),
      shader = new vglModule.shader(gl.VERTEX_SHADER);

  shader.setShaderSource(vertexShaderSource);
  return shader;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of vertex shader with a solid color
 *
 * Helper function to create default vertex shader
 *
 * @param context
 * @returns {vglModule.shader}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createVertexShaderSolidColor = function(context) {
  'use strict';
  var vertexShaderSource = [
        'attribute vec3 vertexPosition;',
        'uniform mediump float pointSize;',
        'uniform mat4 modelViewMatrix;',
        'uniform mat4 projectionMatrix;',
        'void main(void)',
        '{',
        'gl_PointSize = pointSize;',
        'gl_Position = projectionMatrix * modelViewMatrix * vec4(vertexPosition, 1.0);',
        '}' ].join('\n'),
    shader = new vglModule.shader(gl.VERTEX_SHADER);

  shader.setShaderSource(vertexShaderSource);
  return shader;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of vertex shader that passes values through
 * for color mapping
 *
 * Helper function to create default vertex shader
 *
 * @param context
 * @returns {vglModule.shader}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createVertexShaderColorMap = function(context, min, max) {
  'use strict';
  var vertexShaderSource = [
        'attribute vec3 vertexPosition;',
        'attribute float vertexScalar;',
        'uniform mediump float pointSize;',
        'uniform mat4 modelViewMatrix;',
        'uniform mat4 projectionMatrix;',
        'uniform float lutMin;',
        'uniform float lutMax;',
        'varying mediump float iVertexScalar;',
        'void main(void)',
        '{',
        'gl_PointSize = pointSize;',
        'gl_Position = projectionMatrix * modelViewMatrix * vec4(vertexPosition, 1.0);',
        'iVertexScalar = (vertexScalar-lutMin)/(lutMax-lutMin);',
        '}' ].join('\n'),
      shader = new vglModule.shader(gl.VERTEX_SHADER);

  shader.setShaderSource(vertexShaderSource);
  return shader;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of default fragment shader
 *
 * Helper function to create default fragment shader
 *
 * @param context
 * @returns {vglModule.shader}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createFragmentShader = function(context) {
  'use strict';
  var fragmentShaderSource = [ 'varying mediump vec3 iVertexColor;',
                              'uniform mediump float opacity;',
                              'void main(void) {',
                              'gl_FragColor = vec4(iVertexColor, opacity);',
                              '}' ].join('\n'),
      shader = new vglModule.shader(gl.FRAGMENT_SHADER);

  shader.setShaderSource(fragmentShaderSource);
  return shader;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a Phong vertex shader
 *
 * Helper function to create Phong vertex shader
 *
 * @param context
 * @returns {vglModule.shader}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createPhongVertexShader = function(context) {
  'use strict';

  var vertexShaderSource = [
      'attribute highp vec3 vertexPosition;',
      'attribute mediump vec3 vertexNormal;',
      'attribute mediump vec3 vertexColor;',

      'uniform highp mat4 projectionMatrix;',
      'uniform mat4 modelViewMatrix;',
      'uniform mat4 normalMatrix;',

      'varying highp vec4 varPosition;',
      'varying mediump vec3 varNormal;',
      'varying mediump vec3 iVertexColor;',

      'void main(void)',
      '{',
      'varPosition = modelViewMatrix * vec4(vertexPosition, 1.0);',
      'gl_Position = projectionMatrix * varPosition;',
      'varNormal = vec3(normalMatrix * vec4(vertexNormal, 0.0));',
      'iVertexColor = vertexColor;',
      '}' ].join('\n'),

      shader = new vglModule.shader(gl.VERTEX_SHADER);

  shader.setShaderSource(vertexShaderSource);

  return shader;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of Phong fragment shader
 *
 * Helper function to create Phong fragment shader
 *
 * NOTE: Shader assumes directional light
 *
 * @param context
 * @returns {vglModule.shader}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createPhongFragmentShader = function(context) {
  'use strict';
  var fragmentShaderSource = [
    'precision mediump float;',
    'varying vec3 varNormal;',
    'varying vec4 varPosition;',
    'varying mediump vec3 iVertexColor;',
    'const vec3 lightPos = vec3(0.0, 0.0,10000.0);',
    'const vec3 ambientColor = vec3(0.01, 0.01, 0.01);',
    'const vec3 specColor = vec3(1.0, 1.0, 1.0);',

    'void main() {',
    'vec3 normal = normalize(varNormal);',
    'vec3 lightDir = normalize(lightPos);',
    'vec3 reflectDir = -reflect(lightDir, normal);',
    'vec3 viewDir = normalize(-varPosition.xyz);',

    'float lambertian = max(dot(lightDir,normal), 0.0);',
    'float specular = 0.0;',

    'if(lambertian > 0.0) {',
    'float specAngle = max(dot(reflectDir, viewDir), 0.0);',
    'specular = pow(specAngle, 64.0);',
    '}',
    'gl_FragColor = vec4(ambientColor +',
    'lambertian*iVertexColor +',
    'specular*specColor, 1.0);',
    '}' ].join('\n'),
    shader = new vglModule.shader(gl.FRAGMENT_SHADER);

  shader.setShaderSource(fragmentShaderSource);
  return shader;
};


//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of fragment shader with an assigned constant color.
 *
 * Helper function to create default fragment shader
 *
 * @param context
 * @returns {vglModule.shader}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createFragmentShaderSolidColor = function(context, color) {
  'use strict';
  var fragmentShaderSource = ['uniform mediump float opacity;',
                              'void main(void) {',
                              'gl_FragColor = vec4(' + color[0] + ',' + color[1] + ',' + color[2] + ', opacity);',
                              '}' ].join('\n'),
      shader = new vglModule.shader(gl.FRAGMENT_SHADER);

  shader.setShaderSource(fragmentShaderSource);
  return shader;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of fragment shader that maps values into colors bia lookup table
 *
 * Helper function to create default fragment shader
 *
 * @param context
 * @returns {vglModule.shader}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createFragmentShaderColorMap = function(context) {
  'use strict';
  var fragmentShaderSource = [
        'varying mediump float iVertexScalar;',
        'uniform sampler2D sampler2d;',
        'uniform mediump float opacity;',
        'void main(void) {',
        'gl_FragColor = vec4(texture2D(sampler2d, vec2(iVertexScalar, 0.0)).xyz, opacity);',
        '}' ].join('\n'),
      shader = new vglModule.shader(gl.FRAGMENT_SHADER);

  shader.setShaderSource(fragmentShaderSource);
  return shader;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of vertex shader for point sprites
 *
 * Helper function to create default point sprites vertex shader
 *
 * @param context
 * @returns {vglModule.shader}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createPointSpritesVertexShader = function(context) {
  'use strict';
  var vertexShaderSource = [
        'attribute vec3 vertexPosition;',
        'attribute vec3 vertexColor;',
        'uniform mediump float pointSize;',
        'uniform mat4 modelViewMatrix;',
        'uniform mat4 projectionMatrix;',
        'varying mediump vec3 iVertexColor;',
        'void main(void)',
        '{',
        'gl_PointSize = pointSize;',
        'gl_Position = projectionMatrix * modelViewMatrix * vec4(vertexPosition, 1.0);',
        ' iVertexColor = vertexColor;', '}' ].join('\n'),
      shader = new vglModule.shader(gl.VERTEX_SHADER);
  shader.setShaderSource(vertexShaderSource);
  return shader;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of fragment shader for point sprites
 *
 * Helper function to create default point sprites fragment shader
 *
 * @param context
 * @returns {vglModule.shader}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createPointSpritesFragmentShader = function(context) {
  'use strict';
  var fragmentShaderSource = [
        'varying mediump vec3 iVertexColor;',
        'uniform sampler2D sampler2d;',
        'uniform mediump float opacity;',
        'uniform mediump float vertexColorWeight;',
        'void main(void) {',
        'highp vec4 texColor = texture2D(sampler2d, gl_PointCoord);',
        'highp vec3 finalColor = iVertexColor * vertexColorWeight + (1.0 - vertexColorWeight) * texColor.xyz;',
        'gl_FragColor = vec4(finalColor, opacity * texColor.w);',
        '}' ].join('\n'),
    shader = new vglModule.shader(gl.FRAGMENT_SHADER);

  shader.setShaderSource(fragmentShaderSource);
  return shader;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of texture material
 *
 * Helper function to create a texture material
 *
 * @returns {vglModule.material}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createTextureMaterial = function(isRgba) {
  'use strict';
  var mat = new vglModule.material(),
    blend = new vglModule.blend(),
    prog = new vglModule.shaderProgram(),
    vertexShader = vglModule.utils.createTextureVertexShader(gl),
    fragmentShader = null,
    posVertAttr = new vglModule.vertexAttribute("vertexPosition"),
    texCoordVertAttr = new vglModule.vertexAttribute("textureCoord"),
    pointsizeUniform = new vglModule.floatUniform("pointSize", 5.0),
    modelViewUniform = new vglModule.modelViewUniform("modelViewMatrix"),
    projectionUniform = new vglModule.projectionUniform("projectionMatrix"),
    samplerUniform = new vglModule.uniform(gl.INT, "sampler2d"),
    opacityUniform = null;

  samplerUniform.set(0);

  prog.addVertexAttribute(posVertAttr, vglModule.vertexAttributeKeys.Position);
  prog.addVertexAttribute(texCoordVertAttr,
                          vglModule.vertexAttributeKeys.TextureCoordinate);
  prog.addUniform(pointsizeUniform);
  prog.addUniform(modelViewUniform);
  prog.addUniform(projectionUniform);

  if (isRgba) {
    fragmentShader = vglModule.utils.createRgbaTextureFragmentShader(gl);
  } else {
    fragmentShader = vglModule.utils.createTextureFragmentShader(gl);
    opacityUniform = new vglModule.floatUniform("opacity", 1.0);
    prog.addUniform(opacityUniform);
  }

  prog.addShader(fragmentShader);
  prog.addShader(vertexShader);
  mat.addAttribute(prog);
  mat.addAttribute(blend);

  return mat;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of geometry material
 *
 * Helper function to create geometry material
 *
 * @returns {vglModule.material}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createGeometryMaterial = function() {
  'use strict';
   var mat = new vglModule.material(),
       blend = new vglModule.blend(),
       prog = new vglModule.shaderProgram(),
       vertexShader = vglModule.utils.createVertexShader(gl),
       fragmentShader = vglModule.utils.createFragmentShader(gl),
       posVertAttr = new vglModule.vertexAttribute("vertexPosition"),
       colorVertAttr = new vglModule.vertexAttribute("vertexColor"),
       pointsizeUniform = new vglModule.floatUniform("pointSize", 5.0),
       opacityUniform = new vglModule.floatUniform("opacity", 1.0),
       modelViewUniform = new vglModule.modelViewUniform("modelViewMatrix"),
       projectionUniform = new vglModule.projectionUniform("projectionMatrix");

  prog.addVertexAttribute(posVertAttr, vglModule.vertexAttributeKeys.Position);
  prog.addVertexAttribute(colorVertAttr, vglModule.vertexAttributeKeys.Color);
  prog.addUniform(pointsizeUniform);
  prog.addUniform(opacityUniform);
  prog.addUniform(modelViewUniform);
  prog.addUniform(projectionUniform);
  prog.addShader(fragmentShader);
  prog.addShader(vertexShader);
  mat.addAttribute(prog);
  mat.addAttribute(blend);

  return mat;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of geometry material with the phong shader
 *
 * Helper function to create color phong shaded geometry material
 *
 * @returns {vglModule.material}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createPhongMaterial = function() {
  'use strict';
   var mat = new vglModule.material(),
       blend = new vglModule.blend(),
       prog = new vglModule.shaderProgram(),
       vertexShader = vglModule.utils.createPhongVertexShader(gl),
       fragmentShader = vglModule.utils.createPhongFragmentShader(gl),
       posVertAttr = new vglModule.vertexAttribute("vertexPosition"),
       normalVertAttr = new vglModule.vertexAttribute("vertexNormal"),
       colorVertAttr = new vglModule.vertexAttribute("vertexColor"),
       opacityUniform = new vglModule.floatUniform("opacity", 1.0),
       modelViewUniform = new vglModule.modelViewUniform("modelViewMatrix"),
       normalUniform = new vglModule.normalMatrixUniform("normalMatrix"),
       projectionUniform = new vglModule.projectionUniform("projectionMatrix");

  prog.addVertexAttribute(posVertAttr, vglModule.vertexAttributeKeys.Position);
  prog.addVertexAttribute(normalVertAttr, vglModule.vertexAttributeKeys.Normal);
  prog.addVertexAttribute(colorVertAttr, vglModule.vertexAttributeKeys.Color);
  prog.addUniform(opacityUniform);
  prog.addUniform(modelViewUniform);
  prog.addUniform(projectionUniform);
  prog.addUniform(normalUniform);
  prog.addShader(fragmentShader);
  prog.addShader(vertexShader);
  mat.addAttribute(prog);
  mat.addAttribute(blend);

  return mat;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of colored geometry material
 *
 * Helper function to create color geometry material
 *
 * @returns {vglModule.material}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createColorMaterial = function() {
  'use strict';
  var mat = new vglModule.material(),
      blend = new vglModule.blend(),
      prog = new vglModule.shaderProgram(),
      vertexShader = vglModule.utils.createVertexShader(gl),
      fragmentShader = vglModule.utils.createFragmentShader(gl),
      posVertAttr = new vglModule.vertexAttribute("vertexPosition"),
      texCoordVertAttr = new vglModule.vertexAttribute("textureCoord"),
      colorVertAttr = new vglModule.vertexAttribute("vertexColor"),
      pointsizeUniform = new vglModule.floatUniform("pointSize", 5.0),
      opacityUniform = new vglModule.floatUniform("opacity", 0.5),
      modelViewUniform = new vglModule.modelViewUniform("modelViewMatrix"),
      projectionUniform = new vglModule.projectionUniform("projectionMatrix");

  prog.addVertexAttribute(posVertAttr, vglModule.vertexAttributeKeys.Position);
  prog.addVertexAttribute(colorVertAttr, vglModule.vertexAttributeKeys.Color);
  prog.addVertexAttribute(texCoordVertAttr,
                          vglModule.vertexAttributeKeys.TextureCoordinate);
  prog.addUniform(pointsizeUniform);
  prog.addUniform(opacityUniform);
  prog.addUniform(modelViewUniform);
  prog.addUniform(projectionUniform);
  prog.addShader(fragmentShader);
  prog.addShader(vertexShader);
  mat.addAttribute(prog);
  mat.addAttribute(blend);

  return mat;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of geometry material
 *
 * Helper function to create geometry material
 *
 * @returns {vglModule.material}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createColorMappedMaterial = function(lut) {
  'use strict';
  if (!lut) {
    lut = new vglModule.lookupTable();
  }

  var scalarRange = lut.range(),
      mat = new vglModule.material(),
      blend = new vglModule.blend(),
      prog = new vglModule.shaderProgram(),
      vertexShader = vglModule.utils.createVertexShaderColorMap(
        gl,scalarRange[0],scalarRange[1]),
      fragmentShader = vglModule.utils.createFragmentShaderColorMap(gl),
      posVertAttr = new vglModule.vertexAttribute("vertexPosition"),
      scalarVertAttr = new vglModule.vertexAttribute("vertexScalar"),
      pointsizeUniform = new vglModule.floatUniform("pointSize", 5.0),
      opacityUniform = new vglModule.floatUniform("opacity", 0.5),
      lutMinUniform = new vglModule.floatUniform("lutMin", scalarRange[0]),
      lutMaxUniform = new vglModule.floatUniform("lutMax", scalarRange[1]),
      modelViewUniform = new vglModule.modelViewUniform("modelViewMatrix"),
      projectionUniform = new vglModule.projectionUniform("projectionMatrix"),
      samplerUniform = new vglModule.uniform(gl.FLOAT, "sampler2d"),
      lookupTable = lut;

  samplerUniform.set(0);

  prog.addVertexAttribute(posVertAttr, vglModule.vertexAttributeKeys.Position);
  prog.addVertexAttribute(scalarVertAttr, vglModule.vertexAttributeKeys.Scalar);
  prog.addUniform(pointsizeUniform);
  prog.addUniform(opacityUniform);
  prog.addUniform(lutMinUniform);
  prog.addUniform(lutMaxUniform);
  prog.addUniform(modelViewUniform);
  prog.addUniform(projectionUniform);
  prog.addShader(fragmentShader);
  prog.addShader(vertexShader);
  mat.addAttribute(prog);
  mat.addAttribute(blend);
  mat.addAttribute(lookupTable);

  return mat;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Update color mapped material
 *
 * @param mat
 * @param scalarRange
 * @param lut
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.updateColorMappedMaterial = function(mat, lut) {
  'use strict';
  if (!mat) {
    console.log('[warning] Invalid material. Nothing to update.');
    return;
  }

  if (!lut) {
    console.log('[warning] Invalid lookup table. Nothing to update.');
    return;
  }


  var lutMin = mat.shaderProgram().uniform('lutMin'),
      lutMax = mat.shaderProgram().uniform('lutMax');

  lutMin.set(lut.range()[0]);
  lutMax.set(lut.range()[1]);

  // This will replace the existing lookup table
  mat.setAttribute(lut);
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of solid color material
 *
 * Helper function to create geometry material
 *
 * @returns {vglModule.material}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createSolidColorMaterial = function(color) {
  'use strict';
  if (!color) {
    color = [1.0,1.0,1.0];
  }

  var mat = new vglModule.material(),
      blend = new vglModule.blend(),
      prog = new vglModule.shaderProgram(),
      vertexShader = vglModule.utils.createVertexShaderSolidColor(gl),
      fragmentShader = vglModule.utils.createFragmentShaderSolidColor(gl, color),
      posVertAttr = new vglModule.vertexAttribute("vertexPosition"),
      pointsizeUniform = new vglModule.floatUniform("pointSize", 5.0),
      opacityUniform = new vglModule.floatUniform("opacity", 1.0),
      modelViewUniform = new vglModule.modelViewUniform("modelViewMatrix"),
      projectionUniform = new vglModule.projectionUniform("projectionMatrix");

  prog.addVertexAttribute(posVertAttr, vglModule.vertexAttributeKeys.Position);
  prog.addUniform(pointsizeUniform);
  prog.addUniform(opacityUniform);
  prog.addUniform(modelViewUniform);
  prog.addUniform(projectionUniform);
  prog.addShader(fragmentShader);
  prog.addShader(vertexShader);
  mat.addAttribute(prog);
  mat.addAttribute(blend);

  return mat;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of point sprites material
 *
 * Helper function to create point sprites material
 *
 * @returns {vglModule.material}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createPointSpritesMaterial = function(image) {
  'use strict';
  var mat = new vglModule.material(),
      blend = new vglModule.blend(),
      prog = new vglModule.shaderProgram(),
      vertexShader = vglModule.utils.createPointSpritesVertexShader(gl),
      fragmentShader = vglModule.utils.createPointSpritesFragmentShader(gl),
      posVertAttr = new vglModule.vertexAttribute("vertexPosition"),
      colorVertAttr = new vglModule.vertexAttribute("vertexColor"),
      pointsizeUniform = new vglModule.floatUniform("pointSize", 5.0),
      opacityUniform = new vglModule.floatUniform("opacity", 1.0),
      vertexColorWeightUniform =
        new vglModule.floatUniform("vertexColorWeight", 0.0),
      modelViewUniform = new vglModule.modelViewUniform("modelViewMatrix"),
      projectionUniform = new vglModule.projectionUniform("projectionMatrix"),
      samplerUniform = new vglModule.uniform(gl.INT, "sampler2d"),
      texture = new vglModule.texture();

  samplerUniform.set(0);
  prog.addVertexAttribute(posVertAttr, vglModule.vertexAttributeKeys.Position);
  prog.addVertexAttribute(colorVertAttr, vglModule.vertexAttributeKeys.Color);
  prog.addUniform(pointsizeUniform);
  prog.addUniform(opacityUniform);
  prog.addUniform(vertexColorWeightUniform);
  prog.addUniform(modelViewUniform);
  prog.addUniform(projectionUniform);
  prog.addShader(fragmentShader);
  prog.addShader(vertexShader);
  mat.addAttribute(prog);
  mat.addAttribute(blend);

  texture.setImage(image);
  mat.addAttribute(texture);
  return mat;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of an actor that contains a plane geometry
 *
 * Function to create a plane node This method will create a plane actor
 * with texture coordinates, eventually normal, and plane material.
 *
 * @returns {vglModule.actor}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createPlane = function(originX, originY, originZ,
                                       point1X, point1Y, point1Z,
                                       point2X, point2Y, point2Z) {
  'use strict';
  var mapper = new vglModule.mapper(),
      planeSource = new vglModule.planeSource(),
      mat = vglModule.utils.createGeometryMaterial(),
      actor = new vglModule.actor();

  planeSource.setOrigin(originX, originY, originZ);
  planeSource.setPoint1(point1X, point1Y, point1Z);
  planeSource.setPoint2(point2X, point2Y, point2Z);

  mapper.setGeometryData(planeSource.create());
  actor.setMapper(mapper);
  actor.setMaterial(mat);

  return actor;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of an actor that contains a texture plane geometry
 *
 * Helper function to create a plane textured node This method will create
 * a plane actor with texture coordinates, eventually normal, and plane
 * material.
 *
 * @returns {vglModule.actor}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createTexturePlane = function(originX, originY, originZ,
                                              point1X, point1Y, point1Z,
                                              point2X, point2Y, point2Z,
                                              isRgba) {
  'use strict';
  var mapper = new vglModule.mapper(),
      planeSource = new vglModule.planeSource(),
      mat = vglModule.utils.createTextureMaterial(isRgba),
      actor = new vglModule.actor();

  planeSource.setOrigin(originX, originY, originZ);
  planeSource.setPoint1(point1X, point1Y, point1Z);
  planeSource.setPoint2(point2X, point2Y, point2Z);
  mapper.setGeometryData(planeSource.create());

  actor.setMapper(mapper);
  actor.setMaterial(mat);

  return actor;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of an actor that contains points
 *
 * Helper function to create a point node This method will create a point
 * actor with texture coordinates, eventually normal, and plane material.
 *
 * @returns {vglModule.actor}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createPoints = function(positions, colors, texcoords) {
  'use strict';
  if (!positions) {
    console.log("[ERROR] Cannot create points without positions");
    return null;
  }

  var mapper = new vglModule.mapper(),
      pointSource = new vglModule.pointSource(),
      mat = vglModule.utils.createGeometryMaterial(),
      actor = new vglModule.actor();

  pointSource.setPositions(positions);
  if (colors) {
    pointSource.setColors(colors);
  }

  if (texcoords) {
    pointSource.setTextureCoordinates(texcoords);
  }

  mapper.setGeometryData(pointSource.create());
  actor.setMapper(mapper);
  actor.setMaterial(mat);

  return actor;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of an actor that contains point sprites
 *
 * Helper function to create a point sprites node This method will create
 * a point sprites actor with texture coordinates, normals, and a point sprites
 * material.
 *
 * @returns {vglModule.actor}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createPointSprites = function(image, positions, colors,
                                              texcoords) {
  'use strict';
  if (!image) {
    console.log("[ERROR] Point sprites requires an image");
    return null;
  }

  if (!positions) {
    console.log("[ERROR] Cannot create points without positions");
    return null;
  }

  var mapper = new vglModule.mapper(),
      pointSource = new vglModule.pointSource(),
      mat = vglModule.utils.createPointSpritesMaterial(image),
      actor = new vglModule.actor();

  pointSource.setPositions(positions);
  if (colors) {
    pointSource.setColors(colors);
  }

  if (texcoords) {
    pointSource.setTextureCoordinates(texcoords);
  }

  mapper.setGeometryData(pointSource.create());
  actor.setMapper(mapper);
  actor.setMaterial(mat);

  return actor;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create lines given positions, colors, and desired length
 *
 * @param positions
 * @param colors
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createLines = function(positions, colors) {
  'use strict';
  if (!positions) {
    console.log("[ERROR] Cannot create points without positions");
    return null;
  }

  var mapper = new vglModule.mapper(),
      lineSource = new vglModule.lineSource(),
      mat = vglModule.utils.createGeometryMaterial(),
      actor = new vglModule.actor();

  lineSource.setPositions(positions);
  if (colors) {
    lineSource.setColors(colors);
  }

  mapper.setGeometryData(lineSource.create());
  actor.setMapper(mapper);
  actor.setMaterial(mat);

  return actor;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create color legend
 *
 * @param lookupTable
 * @param width
 * @param height
 * @param origin
 * @param divs
 * @returns {Array}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.createColorLegend = function(varname, lookupTable, origin,
                                             width, height, countMajor,
                                             countMinor) {
  'use strict';

  if (!lookupTable) {
    console.log('[error] Invalid lookup table');
    return [];
  }

  //////////////////////////////////////////////////////////////////////////////
  /**
   * Create labels for the legend
   *
   * @param ticks
   * @param range
   * @param divs
   */
  //////////////////////////////////////////////////////////////////////////////
  function createLabels(varname, positions, range) {
    if (!positions) {
      console.log('[error] Create labels requires positions (x,y,z) array');
      return;
    }

    if (positions.length % 3 !== 0) {
      console.log('[error] Create labels require positions array contain 3d points');
      return;
    }

    if (!range) {
      console.log('[error] Create labels requires Valid range');
      return;
    }

    var actor = null,
        size = vglModule.utils.computePowerOfTwo(48),
        index = 0,
        actors = [],
        origin = [],
        pt1 = [],
        pt2 = [],
        delta = (positions[6] - positions[0]),
        axisLabelOffset = 4, i;

    origin.length = 3;
    pt1.length = 3;
    pt2.length = 3;

    // For now just create labels for end points
    for (i = 0; i < 2; ++i) {
      index = i * (positions.length - 3);

      origin[0] = positions[index] - delta;
      origin[1] = positions[index + 1] - 2 * delta;
      origin[2] = positions[index + 2];

      pt1[0] = positions[index] + delta;
      pt1[1] = origin[1];
      pt1[2] = origin[2];

      pt2[0] = origin[0];
      pt2[1] = positions[1];
      pt2[2] = origin[2];

      actor = vglModule.utils.createTexturePlane(
        origin[0], origin[1], origin[2],
        pt1[0], pt1[1], pt1[2],
        pt2[0], pt2[1], pt2[2], true);

      actor.setReferenceFrame(vglModule.boundingObject.ReferenceFrame.Absolute);
      actor.material().setBinNumber(vglModule.material.RenderBin.Overlay);
      actor.material().addAttribute(vglModule.utils.create2DTexture(
        range[i].toFixed(2).toString(), 12, null));
      actors.push(actor);
    }

    // Create axis label
    origin[0] = (positions[0] + positions[positions.length - 3]  - size) * 0.5;
    origin[1] = positions[1] + axisLabelOffset;
    origin[2] = positions[2];

    pt1[0] = origin[0] + size;
    pt1[1] = origin[1];
    pt1[2] = origin[2];

    pt2[0] = origin[0];
    pt2[1] = origin[1] + size;
    pt2[2] = origin[2];

    actor = vglModule.utils.createTexturePlane(
      origin[0], origin[1], origin[2],
      pt1[0], pt1[1], pt1[2],
      pt2[0], pt2[1], pt2[2], true);
    actor.setReferenceFrame(vglModule.boundingObject.ReferenceFrame.Absolute);
    actor.material().setBinNumber(vglModule.material.RenderBin.Overlay);
    actor.material().addAttribute(vglModule.utils.create2DTexture(
      varname, 24, null));
    actors.push(actor);

    return actors;
  }

  //////////////////////////////////////////////////////////////////////////////
  // TODO Currently we assume that the ticks are laid on x-axis
  // and this is on a 2D plane (ignoring Z axis. For now lets
  // not draw minor ticks.
  /**
   * Create ticks and labels
   *
   * @param originX
   * @param originY
   * @param originZ
   * @param pt1X
   * @param pt1Y
   * @param pt1Z
   * @param pt2X
   * @param pt2Y
   * @param pt2Z
   * @param divs
   * @param heightMajor
   * @param heightMinor
   * @returns {Array} Returns array of vglModule.actor
   */
  //////////////////////////////////////////////////////////////////////////////
  function createTicksAndLabels(varname, lut,
                        originX, originY, originZ,
                        pt1X, pt1Y, pt1Z,
                        pt2X, pt2Y, pt2Z,
                        countMajor, countMinor,
                        heightMajor, heightMinor) {
    var width = pt2X - pt1X,
        index = null,
        delta = width / countMajor,
        positions = [],
        actor = null,
        actors = [];

    for (index = 0; index <= countMajor; ++index) {
      positions.push(pt1X + delta * index);
      positions.push(pt1Y);
      positions.push(pt1Z);

      positions.push(pt1X + delta * index);
      positions.push(pt1Y + heightMajor);
      positions.push(pt1Z);
    }

    actor = vglModule.utils.createLines(positions, null);
    actor.setReferenceFrame(vglModule.boundingObject.ReferenceFrame.Absolute);
    actor.material().setBinNumber(vglModule.material.RenderBin.Overlay);
    actors.push(actor);

    actors = actors.concat(createLabels(varname, positions, lut.range()));
    return actors;
  }

  // TODO Currently we create only one type of legend
  var pt1X = origin[0] + width,
      pt1Y = origin[1],
      pt1Z = 0.0,
      pt2X = origin[0],
      pt2Y = origin[1] + height,
      pt2Z = 0.0,
      actors = [],
      actor = null,
      mapper = null,
      mat = null,
      group = vglModule.groupNode();

  actor = vglModule.utils.createTexturePlane(
    origin[0], origin[1], origin[2],
    pt1X, pt1Y, pt1Z,
    pt2X, pt2Y, pt2Z
  );

  mat = actor.material();
  mat.addAttribute(lookupTable);
  actor.setMaterial(mat);
  group.addChild(actor);
  actor.setReferenceFrame(vglModule.boundingObject.ReferenceFrame.Absolute);
  actors.push(actor);
  actors = actors.concat(createTicksAndLabels(
                          varname,
                          lookupTable,
                          origin[0], origin[1], origin[1],
                          pt2X, pt1Y, pt1Z,
                          pt1X, pt1Y, pt1Z,
                          countMajor, countMinor, 5, 3));

  // TODO This needs to change so that we can return a group node
  // which should get appended to the scene graph
  return actors;
};

//////////////////////////////////////////////////////////////////////////////
/**
 * Create 2D texture by rendering text using canvas2D context
 *
 * @param textToWrite
 * @param textSize
 * @param color
 * @returns {vglModule.texture}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.utils.create2DTexture = function(textToWrite, textSize,
  color, font, alignment, baseline, bold) {
  'use strict';

  var canvas = document.getElementById('textRendering'),
      ctx = null,
      texture = vglModule.texture();

  font = font || 'sans-serif';
  alignment = alignment || 'center';
  baseline = baseline || 'bottom';

  if (typeof bold === 'undefined') {
    bold = true;
  }

  if (!canvas) {
    canvas = document.createElement('canvas');
  }
  ctx = canvas.getContext('2d');

  canvas.setAttribute('id', 'textRendering');
  canvas.style.display = 'none';

  // Make width and height equal so that we get pretty looking text.
  canvas.height = vglModule.utils.computePowerOfTwo(8 * textSize);
  canvas.width = canvas.height;

  ctx.fillStyle = 'rgba(0, 0, 0, 0)';
  ctx.fillRect(0, 0, ctx.canvas.width, ctx.canvas.height);

  // This determines the text colour, it can take a hex value or rgba value (e.g. rgba(255,0,0,0.5))
  ctx.fillStyle = 'rgba(200, 85, 10, 1.0)';

  // This determines the alignment of text, e.g. left, center, right
  ctx.textAlign = alignment;

  // This determines the baseline of the text, e.g. top, middle, bottom
  ctx.textBaseline = baseline;

  // This determines the size of the text and the font family used
  ctx.font = 4 * textSize + "px " + font;
  if (bold) {
    ctx.font = "bold " + ctx.font;
  }

  ctx.fillText(textToWrite, canvas.width/2, canvas.height/2, canvas.width);

  texture.setImage(canvas);
  texture.updateDimensions();

  return texture;
};
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of class picker
 *
 * @class vglModule.picker
 * @returns {vglModule.picker}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.picker = function() {
  'use strict';

  if (!(this instanceof vglModule.picker)) {
    return new vglModule.picker();
  }
  vglModule.object.call(this);

  /** @private */
  var m_that = this,
      m_tolerance = 0.025,
      m_actors = [];

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Get actors intersected
   */
  ////////////////////////////////////////////////////////////////////////////
  this.getActors = function() {
    return m_actors;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * Perform pick operation
   */
  ////////////////////////////////////////////////////////////////////////////
  this.pick = function(selectionX, selectionY, renderer) {
    // Check if variables are acceptable
    if (typeof(selectionX) === "undefined"){
      return 0;
    }
    if (typeof(selectionY) === "undefined"){
      return 0;
    }
    if (typeof(renderer) === "undefined"){
      return 0;
    }

    // Clean list of actors intersected previously
    m_actors = [];

    //
    var camera = renderer.camera(),
        width = renderer.width(),
        height = renderer.height(),
        fpoint = camera.focalPoint(),
        focusWorldPt = vec4.fromValues(fpoint[0], fpoint[1], fpoint[2], 1.0),
        focusDisplayPt = renderer.worldToDisplay(
          focusWorldPt, camera.viewMatrix(),
        camera.projectionMatrix(), width, height),
        displayPt = vec4.fromValues(selectionX,
                      selectionY, focusDisplayPt[2], 1.0),
        // Convert selection point into world coordinates
        worldPt = renderer.displayToWorld(displayPt, camera.viewMatrix(),
                    camera.projectionMatrix(), width, height),
        cameraPos = camera.position(), ray = [], actors, count, i, bb,
        tmin, tmax, tymin, tymax, tzmin, tzmax, actor;

    for (i = 0; i < 3; ++i){
      ray[i] = worldPt[i] - cameraPos[i];
    }

    // Go through all actors and check if intersects
    actors = renderer.sceneRoot().children();
    count = 0;

    for (i = 0; i < actors.length; ++i) {
      actor = actors[i];
      if (actor.visible() === true) {
        bb = actor.bounds();
        // Ray-aabb intersection - Smits' method
        if (ray[0] >= 0){
          tmin = (bb[0] - cameraPos[0])/ray[0];
          tmax = (bb[1] - cameraPos[0])/ray[0];
        } else {
          tmin = (bb[1] - cameraPos[0])/ray[0];
          tmax = (bb[0] - cameraPos[0])/ray[0];
        }
        if (ray[1] >= 0){
          tymin = (bb[2] - cameraPos[1])/ray[1];
          tymax = (bb[3] - cameraPos[1])/ray[1];
        } else {
          tymin = (bb[3] - cameraPos[1])/ray[1];
          tymax = (bb[2] - cameraPos[1])/ray[1];
        }
        if ((tmin > tymax) || (tymin > tmax)) {
          continue;
        }


        if (tymin > tmin) {
          tmin = tymin;
        }
        if (tymax < tmax) {
          tmax = tymax;
        }
        if (ray[2] >= 0) {
          tzmin = (bb[4] - cameraPos[2])/ray[2];
          tzmax = (bb[5] - cameraPos[2])/ray[2];
        } else {
          tzmin = (bb[5] - cameraPos[2])/ray[2];
          tzmax = (bb[4] - cameraPos[2])/ray[2];
        }
        if ((tmin > tzmax) || (tzmin > tmax)) {
          continue;
        }
        if (tzmin > tmin) {
          tmin = tzmin;
        }
        if (tzmax < tmax) {
          tmax = tzmax;
        }

        m_actors[count++] = actor;
      }
    }
    return count;
  };

  return this;
};

inherit(vglModule.picker, vglModule.object);
//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $, Uint16Array*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/**
 * Create a new instance of shapefile reader
 *
 * This contains code that reads a shapefile and produces vgl geometries
 *
 * @class
 * @returns {vglModule.shapefileReader}
 */
//////////////////////////////////////////////////////////////////////////////
vglModule.shapefileReader = function() {
  'use strict';

  if (!(this instanceof vglModule.shapefileReader)) {
    return new vglModule.shapefileReader();
  }

  var m_that = this;
  var SHP_HEADER_LEN = 8;
  var SHP_NULL = 0;
  var SHP_POINT = 1;
  var SHP_POLYGON = 5;
  var SHP_POLYLINE = 3;

  this.int8 = function (data, offset) {
      return data.charCodeAt (offset);
  };

  this.bint32 = function (data, offset) {
    return (
      ((data.charCodeAt (offset) & 0xff) << 24) +
        ((data.charCodeAt (offset + 1) & 0xff) << 16) +
        ((data.charCodeAt (offset + 2) & 0xff) << 8) +
        (data.charCodeAt (offset + 3) & 0xff)
    );
  };

  this.lint32 = function (data, offset) {
    return (
      ((data.charCodeAt (offset + 3) & 0xff) << 24) +
        ((data.charCodeAt (offset + 2) & 0xff) << 16) +
        ((data.charCodeAt (offset + 1) & 0xff) << 8) +
        (data.charCodeAt (offset) & 0xff)
    );
  };

  this.bint16 = function (data, offset) {
    return (
      ((data.charCodeAt (offset) & 0xff) << 8) +
        (data.charCodeAt (offset + 1) & 0xff)
    );
  };

  this.lint16 = function (data, offset) {
    return (
      ((data.charCodeAt (offset + 1) & 0xff) << 8) +
        (data.charCodeAt (offset) & 0xff)
    );
  };

  this.ldbl64 = function (data, offset) {
    var b0 = data.charCodeAt (offset) & 0xff;
    var b1 = data.charCodeAt (offset + 1) & 0xff;
    var b2 = data.charCodeAt (offset + 2) & 0xff;
    var b3 = data.charCodeAt (offset + 3) & 0xff;
    var b4 = data.charCodeAt (offset + 4) & 0xff;
    var b5 = data.charCodeAt (offset + 5) & 0xff;
    var b6 = data.charCodeAt (offset + 6) & 0xff;
    var b7 = data.charCodeAt (offset + 7) & 0xff;

    var sign = 1 - 2 * (b7 >> 7);
    var exp = (((b7 & 0x7f) << 4) + ((b6 & 0xf0) >> 4)) - 1023;
    //var frac = (b6 & 0x0f) * Math.pow (2, -4) + b5 * Math.pow (2, -12) + b4 *
    // Math.pow (2, -20) + b3 * Math.pow (2, -28) + b2 * Math.pow (2, -36) + b1 *
    // Math.pow (2, -44) + b0 * Math.pow (2, -52);

    //return sign * (1 + frac) * Math.pow (2, exp);
    var frac = (b6 & 0x0f) * Math.pow (2, 48) + b5 * Math.pow (2, 40) + b4 *
                 Math.pow (2, 32) + b3 * Math.pow (2, 24) + b2 *
                 Math.pow (2, 16) + b1 * Math.pow (2, 8) + b0;

    return sign * (1 + frac * Math.pow (2, -52)) * Math.pow (2, exp);
  };

  this.lfloat32 = function (data, offset) {
    var b0 = data.charCodeAt (offset) & 0xff;
    var b1 = data.charCodeAt (offset + 1) & 0xff;
    var b2 = data.charCodeAt (offset + 2) & 0xff;
    var b3 = data.charCodeAt (offset + 3) & 0xff;

    var sign = 1 - 2 * (b3 >> 7);
    var exp = (((b3 & 0x7f) << 1) + ((b2 & 0xfe) >> 7)) - 127;
    var frac = (b2 & 0x7f) * Math.pow (2, 16) + b1 * Math.pow (2, 8) + b0;

    return sign * (1 + frac * Math.pow (2, -23)) * Math.pow (2, exp);
  };

  this.str = function (data, offset, length) {
    var chars = [];
    var index = offset;
    while (index < offset + length) {
      var c = data[index];
      if (c.charCodeAt (0) !== 0)
        chars.push (c);
      else {
        break;
      }
      index ++;
    }
    return chars.join ('');
  };

  this.readHeader = function (data) {
    var code = this.bint32(data, 0);
    var length = this.bint32(data, 24);
    var version = this.lint32(data, 28);
    var shapetype = this.lint32(data, 32);

    var xmin = this.ldbl64(data, 36);
    var ymin = this.ldbl64(data, 44);
    var xmax = this.ldbl64(data, 52);
    var ymax = this.ldbl64(data, 60);
    return {
      code: code,
      length: length,
      version: version,
      shapetype: shapetype,
      bounds: new Box (vect (xmin, ymin), vect (xmax, ymax))
    };
  };

  this.loadShx = function (data) {
    var indices = [];
    var appendIndex = function (offset) {
      indices.push (2 * m_that.bint32(data, offset));
      return offset + 8;
    };
    var offset = 100;
    while (offset < data.length) {
      offset = appendIndex (offset);
    }
    return indices;
  };

  this.Shapefile = function (options) {
    var path = options.path;
    $.ajax ({
      url: path + '.shx',
      mimeType: 'text/plain; charset=x-user-defined',
      success: function (data) {
        var indices = this.loadShx(data);
        $.ajax ({
          url: path + '.shp',
          mimeType: 'text/plain; charset=x-user-defined',
          success: function (data) {
            $.ajax ({
              url: path + '.dbf',
              mimeType: 'text/plain; charset=x-user-defined',
              success: function (dbf_data) {
                var layer = this.loadShp (data, dbf_data, indices, options);
                options.success (layer);
              }
            });
          }
        });
      }
    });
  };

  this.localShapefile = function(options) {
    var shxFile = options.shx;
    var shpFile = options.shp;
    var dbfFile = options.dbf;
    var shxReader = new FileReader();
    shxReader.onloadend = function() {
      var indices = m_that.loadShx(shxReader.result);
      var shpReader = new FileReader();

      shpReader.onloadend = function() {
        var shpData = shpReader.result;

        var dbfReader = new FileReader();
        dbfReader.onloadend = function() {
          var dbfData = dbfReader.result;
          var layer = m_that.loadShp(shpData, dbfData, indices, options);
          options.success(layer);
        };
        dbfReader.readAsBinaryString(dbfFile);
      };
      shpReader.readAsBinaryString(shpFile);
    };
    shxReader.readAsBinaryString(shxFile);
  };

  this.loadDBF = function (data) {
    var readHeader = function (offset) {
      var name = m_that.str(data, offset, 10);
      var type = m_that.str(data, offset + 11, 1);
      var length = m_that.int8(data, offset + 16);
      return {
        name: name,
        type: type,
        length: length
      };
    };

    // Level of the dBASE file
    var level = m_that.int8(data, 0);
    if (level == 4) {
      throw "Level 7 dBASE not supported";
    }

    // Date of last update
    var year = m_that.int8(data, 1);
    var month = m_that.int8(data, 2);
    var day = m_that.int8(data, 3);

    var num_entries = m_that.lint32(data, 4);
    var header_size = m_that.lint16(data, 8);
    var record_size = m_that.lint16(data, 10);

    var FIELDS_START = 32;
    var HEADER_LENGTH = 32;

    var header_offset = FIELDS_START;
    var headers = [];
    while (header_offset < header_size - 1) {
      headers.push (readHeader(header_offset));
      header_offset += HEADER_LENGTH;
    }

    var records = [];
    var record_offset = header_size;
    while (record_offset < header_size + num_entries * record_size) {
      var declare = m_that.str(data, record_offset, 1);
      if (declare == '*') {
        // Record size in the header include the size of the delete indicator
        record_offset += record_size;
      }
      else {
        // Move offset to the start of the actual data
        record_offset ++;
        var record = {};
        for (var i = 0; i < headers.length; i ++) {
          var header = headers[i];
          var value;
          if (header.type == 'C') {
              value = m_that.str(data, record_offset, header.length).trim ();
          }
          else if (header.type == 'N') {
              value = parseFloat (m_that.str (data, record_offset, header.length));
          }
          record_offset += header.length;
          record[header.name] = value;
        }
        records.push(record);
      }
    }
    return records;
  };

  this.loadShp = function (data, dbf_data, indices, options) {
    var features = [];
    var readRing = function (offset, start, end) {
      var ring = [];
      for (var i = end - 1; i >= start; i --) {
        var x = m_that.ldbl64(data, offset + 16 * i);
        var y = m_that.ldbl64(data, offset + 16 * i + 8);
        ring.push ([x, y]);
      }
      //if (ring.length <= 3)
      // return [];
      return ring;
    };

    var readRecord = function (offset) {
      var index = m_that.bint32(data, offset);
      var record_length = m_that.bint32(data, offset + 4);
      var record_offset = offset + 8;
      var geom_type = m_that.lint32(data, record_offset);

      if (geom_type == SHP_NULL) {
        console.log ("NULL Shape");
        //return offset + 12;
      }
      else if (geom_type == SHP_POINT) {
        var x = m_that.ldbl64(data, record_offset + 4);
        var y = m_that.ldbl64(data, record_offset + 12);

        features.push ({
          type: 'Point',
          attr: {},
          geom: [[x, y]]
        });
      }
      else if (geom_type == SHP_POLYGON) {
        var num_parts = m_that.lint32(data, record_offset + 36);
        var num_points = m_that.lint32(data, record_offset + 40);

        var parts_start = offset + 52;
        var points_start = offset + 52 + 4 * num_parts;

        var rings = [];
        for (var i = 0; i < num_parts; i ++) {
          var start = m_that.lint32(data, parts_start + i * 4);
          var end;
          if (i + 1 < num_parts) {
            end = m_that.lint32(data, parts_start + (i + 1) * 4);
          }
          else {
            end = num_points;
          }
          var ring = readRing (points_start, start, end);
          rings.push (ring);
        }
        features.push ({
          type: 'Polygon',
          attr: {},
          geom: [rings]
        });
      }
      else if (geom_type == SHP_POLYLINE) {
        var num_parts = m_that.lint32(data, record_offset + 36);
        var num_points = m_that.lint32(data, record_offset + 40);

        var parts_start = offset + 52;
        var points_start = offset + 52 + 4 * num_parts;

        var rings = [];
        for (var i = 0; i < num_parts; i ++) {
          var start = m_that.lint32(data, parts_start + i * 4);
          var end;
          if (i + 1 < num_parts) {
              end = m_that.lint32(data, parts_start + (i + 1) * 4);
          }
          else {
              end = num_points;
          }
          var ring = readRing (points_start, start, end);
          rings.push (ring);
        }
        features.push ({
          type: 'Polyline',
          attr: {},
          geom: [rings]
        });
      }
      else {
        throw "Not Implemented: " + geom_type;
      }
      //return offset + 2 * record_length + SHP_HEADER_LEN;
    };

    var attr = this.loadDBF(dbf_data);

    //var offset = 100;
    //while (offset < length * 2) {
    // offset = readRecord (offset);
    //}
    for (var i = 0; i < indices.length; i ++) {
      var offset = indices[i];
      readRecord (offset);
    }

    var layer = []; //new Layer ();

    for (var i = 0; i < features.length; i ++) {
      var feature = features[i];
      feature.attr = attr[i];
      layer.push (feature);
    }
    return layer;
  };

  return this;
};//////////////////////////////////////////////////////////////////////////////
/**
 * @module ogs.vgl
 */

/*jslint devel: true, forin: true, newcap: true, plusplus: true*/
/*jslint white: true, continue:true, indent: 2*/

/*global vglModule, ogs, vec4, inherit, $*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
// vbgModule.vtkReader class
// This contains code that unpack a json base64 encoded vtkdataset,
// such as those produced by ParaView's webGL exporter (where much
// of the code originated from) and convert it to VGL representation.
//
//////////////////////////////////////////////////////////////////////////////

vglModule.vtkReader = function() {
  'use strict';

  if (!(this instanceof vglModule.vtkReader)) {
    return new vglModule.vtkReader();
  }

  var m_base64Chars =
    ['A','B','C','D','E','F','G','H','I','J','K','L','M',
     'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
     'a','b','c','d','e','f','g','h','i','j','k','l','m',
     'n','o','p','q','r','s','t','u','v','w','x','y','z',
     '0','1','2','3','4','5','6','7','8','9','+','/'],
  m_reverseBase64Chars = [],
  m_vtkObjectList = [],
  m_vtkObjectCount = 0,
  m_vtkScene = null,
  END_OF_INPUT = -1,
  m_base64Str = "",
  m_base64Count = 0,
  m_pos = 0,
  m_viewer = null,
  i = null;

  //initialize the array here if not already done.
  if (m_reverseBase64Chars.length === 0) {
    for ( i = 0; i < m_base64Chars.length; i++) {
      m_reverseBase64Chars[m_base64Chars[i]] = i;
    }
  }



  ////////////////////////////////////////////////////////////////////////////
  /**
   * ntos
   *
   * @param n
   * @returns unescaped n
   */
  ////////////////////////////////////////////////////////////////////////////
  this.ntos = function (n) {
    var unN,
    uri;

    n = n.toString(16);
    if (n.length === 1) {
      n = '0' + n;
    }
    n = '%' + n;

    /*global unescape*/
    return unescape(n);
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * readReverseBase64
   *
   * @returns
   */
  ////////////////////////////////////////////////////////////////////////////
  this.readReverseBase64 = function () {
    var nextCharacter;

    if (!m_base64Str) {
      return END_OF_INPUT;
    }

    while (true) {
      if (m_base64Count >= m_base64Str.length) {
        return END_OF_INPUT;
      }
      nextCharacter = m_base64Str.charAt(m_base64Count);
      m_base64Count++;

      if (m_reverseBase64Chars[nextCharacter]) {
        return m_reverseBase64Chars[nextCharacter];
      }
      if (nextCharacter === 'A') {
        return 0;
      }
    }

    return END_OF_INPUT;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * decode64
   *
   * @param str
   * @returns result
   */
  ////////////////////////////////////////////////////////////////////////////
  this.decode64 = function(str) {
    var result = '',
    inBuffer = new Array(4),
    done = false;

    m_base64Str = str;
    m_base64Count = 0;

    /*jslint bitwise: true*/
    while (!done &&
           (inBuffer[0] = this.readReverseBase64()) !== END_OF_INPUT &&
           (inBuffer[1] = this.readReverseBase64()) !== END_OF_INPUT) {
      inBuffer[2] = this.readReverseBase64();
      inBuffer[3] = this.readReverseBase64();
      result += this.ntos((((inBuffer[0] << 2) & 0xff) | inBuffer[1] >> 4));
      if (inBuffer[2] !== END_OF_INPUT) {
        result +=  this.ntos((((inBuffer[1] << 4) & 0xff) | inBuffer[2] >> 2));
        if (inBuffer[3] !== END_OF_INPUT) {
          result +=  this.ntos((((inBuffer[2] << 6) & 0xff) | inBuffer[3]));
        } else {
          done = true;
        }
      } else {
        done = true;
      }
    }

    return result;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * readNumber
   *
   * @param ss
   * @returns v
   */
  ////////////////////////////////////////////////////////////////////////////
  this.readNumber = function(ss) {
    /*jslint bitwise: true*/
    var v = ((ss[m_pos++]) +
             (ss[m_pos++] << 8) +
             (ss[m_pos++] << 16) +
             (ss[m_pos++] << 24));
    return v;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * readF3Array
   *
   * @param numberOfPoints
   * @param ss
   * @returns points
   */
  ////////////////////////////////////////////////////////////////////////////
  this.readF3Array = function(numberOfPoints, ss) {
    /*global Int8Array*/
    var test = new Int8Array(numberOfPoints*4*3),
    points = null, i;

    for(i = 0; i < numberOfPoints*4*3; i++) {
      test[i] = ss[m_pos++];
    }

    /*global Float32Array*/
    points = new Float32Array(test.buffer);

    return points;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * readColorArray
   *
   * @param numberOfPoints
   * @param ss
   * @param vglcolors
   * @returns points
   */
  ////////////////////////////////////////////////////////////////////////////
  this.readColorArray = function (numberOfPoints, ss, vglcolors) {
    var i,r,g,b;
    for(i = 0; i < numberOfPoints; i++) {
      r = ss[m_pos++]/255.0;
      g = ss[m_pos++]/255.0;
      b = ss[m_pos++]/255.0;
      m_pos++;
      vglcolors.pushBack([r,g,b]);
    }
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * parseObject
   *
   * @param buffer
   * @returns geom
   */
  ////////////////////////////////////////////////////////////////////////////
  this.parseObject = function(buffer) {
    var geom = new vglModule.geometryData(), numberOfPoints,
    numberOfIndex, points, normals, colors, index,
    ss = [], test, i, size, type = null, data = null,
    vglpoints = null, vglVertexes = null, vglcolors = null,
    vgllines = null, indices = null, v1 = null,
    vgltriangles = null, tcoord, matrix;

    //dehexlify
    data = this.decode64(buffer);
    for(i = 0; i < data.length; i++) {
      /*jslint bitwise: true*/
      ss[i] = data.charCodeAt(i) & 0xff;
    }

    m_pos = 0;
    size = this.readNumber(ss);
    type = String.fromCharCode(ss[m_pos++]);
    geom.setName(type);

    //-=-=-=-=-=[ LINES ]=-=-=-=-=-
    if (type === 'L') {
      numberOfPoints = this.readNumber(ss);

      //Getting Points
      vglpoints = new vglModule.sourceDataP3fv();
      points = this.readF3Array(numberOfPoints, ss);
      for(i = 0; i < numberOfPoints; i++) {
        vglpoints.pushBack([points[i*3/*+0*/], points[i*3+1], points[i*3+2]]);
      }
      geom.addSource(vglpoints);

      //Getting Colors
      vglcolors = new vglModule.sourceDataC3fv();
      this.readColorArray(numberOfPoints, ss, vglcolors);
      geom.addSource(vglcolors);

      //Getting connectivity
      vgllines = new vglModule.lines();
      geom.addPrimitive(vgllines);
      numberOfIndex = this.readNumber(ss);

      /*global Int8Array*/
      test = new Int8Array(numberOfIndex*2);
      for(i = 0; i < numberOfIndex*2; i++) {
        test[i] = ss[m_pos++];
      }

      /*global Uint16Array*/
      index = new Uint16Array(test.buffer);
      vgllines.setIndices(index);

      /*global gl*/
      vgllines.setPrimitiveType(gl.LINES);

      /*
      //Getting Matrix
      //TODO: renderer is not doing anything with this yet
      test = new Int8Array(16*4);
      for(i=0; i<16*4; i++)
      test[i] = ss[m_pos++];
      matrix = new Float32Array(test.buffer);
      */
    }

    //-=-=-=-=-=[ MESH ]=-=-=-=-=-
    else if (type === 'M') {

      numberOfPoints = this.readNumber(ss);
      //console.log("MESH " + numberOfPoints)

      //Getting Points
      vglpoints = new vglModule.sourceDataP3N3f();
      points = this.readF3Array(numberOfPoints, ss);

      //Getting Normals
      normals = this.readF3Array(numberOfPoints, ss);

      for(i = 0; i < numberOfPoints; i++) {
        v1 = new vglModule.vertexDataP3N3f();
        v1.m_position = [points[i*3/*+0*/], points[i*3+1], points[i*3+2]];
        v1.m_normal = [normals[i*3/*+0*/], normals[i*3+1], normals[i*3+2]];
        vglpoints.pushBack(v1);
      }
      geom.addSource(vglpoints);

      //Getting Colors
      vglcolors = new vglModule.sourceDataC3fv();
      this.readColorArray(numberOfPoints, ss, vglcolors);
      geom.addSource(vglcolors);

      //Getting connectivity
      test = [];
      vgltriangles = new vglModule.triangles();
      geom.addPrimitive(vgltriangles);
      numberOfIndex = this.readNumber(ss);

      /*global Int8Array*/
      test = new Int8Array(numberOfIndex*2);
      for(i = 0; i < numberOfIndex*2; i++) {
        test[i] = ss[m_pos++];
      }

      /*global Uint16Array*/
      index = new Uint16Array(test.buffer);
      vgltriangles.setIndices(index);


      //Getting Matrix
      size = 16*4;
      test = new Int8Array(size);
      for(i = 0; i < size; i++) {
        test[i] = ss[m_pos++];
      }

      matrix = new Float32Array(test.buffer);

      //Getting TCoord
      //TODO: renderer is not doing anything with this yet
      tcoord = null;

    }

    // Points
    else if (type === 'P'){
      numberOfPoints = this.readNumber(ss);
      //console.log("POINTS " + numberOfPoints);

      //Getting Points and creating 1:1 connectivity
      vglpoints = new vglModule.sourceDataP3fv();
      points = this.readF3Array(numberOfPoints, ss);

      /*global Uint16Array*/
      indices = new Uint16Array(numberOfPoints);
      for (i = 0; i < numberOfPoints; i++) {
        indices[i] = i;
        vglpoints.pushBack([points[i*3/*+0*/],points[i*3+1],points[i*3+2]]);
      }
      geom.addSource(vglpoints);

      //Getting Colors
      vglcolors = new vglModule.sourceDataC3fv();
      this.readColorArray(numberOfPoints, ss, vglcolors);
      geom.addSource(vglcolors);

      //Getting connectivity
      vglVertexes = new vglModule.points();
      vglVertexes.setIndices(indices);
      geom.addPrimitive(vglVertexes);

      /*
      //Getting Matrix
      //TODO: not used yet
      test = new Int8Array(16*4);
      for(i=0; i<16*4; i++)
      test[i] = ss[m_pos++];
      matrix = new Float32Array(test.buffer);
      */
    }
    //ColorMap
    else if (type === 'C') {
      console.log('Skipping parsing color map');
    }
    // Unknown
    else {
      console.log("Ignoring unrecognized encoded data type " + type);
    }

    return geom;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * parseSceneMetadata
   *
   * @param data
   * @returns renderer
   */
  ////////////////////////////////////////////////////////////////////////////
  this.parseSceneMetadata = function(renderer, sceneJSON) {

    var sceneRenderer = sceneJSON.Renderers[0],
        camera = renderer.camera(), bgc;

    camera.setCenterOfRotation(sceneJSON.Center);
    camera.setViewAngleDegrees(sceneRenderer.LookAt[0]);
    camera.setPosition(
      sceneRenderer.LookAt[7], sceneRenderer.LookAt[8],
      sceneRenderer.LookAt[9]);
    camera.setFocalPoint(
      sceneRenderer.LookAt[1], sceneRenderer.LookAt[2],
      sceneRenderer.LookAt[3]);
    camera.setViewUpDirection(
      sceneRenderer.LookAt[4], sceneRenderer.LookAt[5],
      sceneRenderer.LookAt[6]);

    bgc = sceneRenderer.Background1;
    renderer.setBackgroundColor(bgc[0], bgc[1], bgc[2], 1);
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * createViewer
   *
   * @param data
   * @returns renderer
   */
  ////////////////////////////////////////////////////////////////////////////
  this.updateViewer = function(node) {
    var renderer, mapper, material, objIdx = 0,
    actor, interactorStyle, bgc, geom, rawGeom, vtkObject,
    shaderProg, opacityUniform, geomType, layer, layerList, tmpList;

    if (m_vtkObjectCount === 0) {
      return null;
    }

    if(m_viewer === null) {
      m_viewer = ogs.vgl.viewer(node);
      m_viewer.init();
      m_viewer.renderWindow().resize(node.width, node.height);
    }

    renderer = m_viewer.renderWindow().activeRenderer();
    tmpList = this.clearVtkObjectData();
    for(layer = m_vtkScene.Renderers.length - 1; layer >= 0; layer--) {
      layerList = tmpList[layer];
      if (layerList == null || typeof layerList === 'undefined') {
        continue;
      }

      for(objIdx; objIdx < layerList.length; objIdx++) {
        mapper = ogs.vgl.mapper();
        vtkObject = layerList[objIdx];
        geom = this.parseObject(vtkObject.data);
        geomType = geom.name();
        mapper.setGeometryData(geom);

        //create material based on type.
        if (geomType === "M") {
          material = ogs.vgl.utils.createPhongMaterial();
        }
        else if(geomType === "L") {
          material = ogs.vgl.utils.createGeometryMaterial();
        }
        else if("P") {
          material = ogs.vgl.utils.createGeometryMaterial();
        }
        else if("C") {
          material = ogs.vgl.utils.createGeometryMaterial();
        }


        //default opacity === solid. If were transparent, set it lower.
        if (vtkObject.hasTransparency) {
          shaderProg = material.shaderProgram();
          opacityUniform = shaderProg.uniform("opacity");
          opacityUniform = new ogs.vgl.floatUniform("opacity", 0.5);
          shaderProg.addUniform(opacityUniform);
        }

        actor = ogs.vgl.actor();
        actor.setMapper(mapper);
        actor.setMaterial(material);
        renderer.addActor(actor);
      }
    }
    this.parseSceneMetadata(renderer, m_vtkScene);
    interactorStyle = ogs.vgl.trackballInteractorStyle();
    m_viewer.setInteractorStyle(interactorStyle);

    return m_viewer;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * addVtkObjectData - Adds binary VTK geometry data to the list for parsing.
   *
   * @param vtkObject
   *
   *        vtkObject = {
   *                      data:,
   *                      hasTransparency:,
   *                      layer:
   *                    };
   *
   *
   * @returns void
   */
  ////////////////////////////////////////////////////////////////////////////
  this.addVtkObjectData = function(vtkObject) {
    var layerList, i = 0, md5;
    if (typeof m_vtkObjectList[vtkObject.layer] === 'undefined') {
      m_vtkObjectList[vtkObject.layer] = [];
    }

    layerList = m_vtkObjectList[vtkObject.layer];
    for (i; i < layerList.length; ++i) {
      md5 = layerList[i].md5;
      if (vtkObject.md5 === md5) {
        return;
      }
    }

    m_vtkObjectList[vtkObject.layer].push(vtkObject);
    m_vtkObjectCount++;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * clearVtkObjectData - Clear out the list of VTK geometry data.
   *
   * @param void
   * @returns void
   */
  ////////////////////////////////////////////////////////////////////////////
  this.clearVtkObjectData = function() {
    var tmpList = m_vtkObjectList;
    m_vtkObjectList = [[]];
    m_vtkObjectCount = 0;
    return tmpList;
  };

  ////////////////////////////////////////////////////////////////////////////
  /**
   * setVtkScene - Set the VTK scene data for camera initialization.
   *
   * @param scene
   * @returns void
   */
  ////////////////////////////////////////////////////////////////////////////
  this.setVtkScene = function(scene) {
    m_vtkScene = scene;
  };

  return this;
};
