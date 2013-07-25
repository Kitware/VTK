/**
 * Create a renderer object working fully in WebGL
 * Here is a sample set of command to illustrate how to use this renderer
 *
 * var renderer = new WebGLRenderer('rendererId','http://localhost:8080/ParaViewWebService')
 * renderer.init(sessionId, viewId);
 * renderer.bindToElementId('containerID'); // => Add a WebGL canvas inside a div tag id 'containerID'
 * renderer.start();
 *
 * renderer.init(otherSessionId, otherViewId);
 * renderer.view.width = '100';
 * renderer.view.height = '400';
 * renderer.setSize('100', '400');
 *
 * renderer.unbindToElementId('containerID');
 */

// Global object to keep track of WebGL renderers
var webglRenderers = new Object();

window.requestAnimFrame = (function(){
  return  window.requestAnimationFrame       ||
          window.webkitRequestAnimationFrame ||
          window.mozRequestAnimationFrame    ||
          window.oRequestAnimationFrame      ||
          window.msRequestAnimationFrame     ||
          function(/* function */ callback, /* DOMElement */ element){
            window.setTimeout(callback, 1000 / 60);
          };
})();

function WebGLRenderer(rendererId, coreServiceURL) {
    this.baseURL = coreServiceURL + "/WebGL";
    this.rendererId = rendererId;
    this.sessionId = "";
    this.viewId = "";
    this.nbError = 0;
    this.localTimeStamp = 0;
    this.offlineMode = false;
    this.setServerMode(false);
    this.forceSquareSize = false;

    this.view = new Object();
    this.view.width = 100;
    this.view.height = 100;
    this.view.id = rendererId;
    this.view.alt = "ParaView Renderer";

    //Default Shaders
    this.view.shaderfs = document.createElement("script");
    this.view.shaderfs.id = "shader-fs";
    this.view.shaderfs.type = "x-shader/x-fragment";
    this.view.shaderfs.innerHTML = "\
    #ifdef GL_ES\n\
    precision highp float;\n\
    #endif\n\
    uniform bool uIsLine;\
    varying vec4 vColor;\
    varying vec4 vTransformedNormal;\
    varying vec4 vPosition;\
    void main(void) {\
        float directionalLightWeighting1 = max(dot(normalize(vTransformedNormal.xyz), vec3(0.0, 0.0, 1.0)), 0.0); \
        float directionalLightWeighting2 = max(dot(normalize(vTransformedNormal.xyz), vec3(0.0, 0.0, -1.0)), 0.0);\
        vec3 lightWeighting = max(vec3(1.0, 1.0, 1.0) * directionalLightWeighting1, vec3(1.0, 1.0, 1.0) * directionalLightWeighting2);\
        if (uIsLine == false){\
          gl_FragColor = vec4(vColor.rgb * lightWeighting, vColor.a);\
        } else {\
          gl_FragColor = vColor*vec4(1.0, 1.0, 1.0, 1.0);\
        }\
    }";
    this.view.shadervs = document.createElement("script");
    this.view.shadervs.id = "shader-vs";
    this.view.shadervs.type = "x-shader/x-vertex";
    this.view.shadervs.innerHTML = "\
    attribute vec3 aVertexPosition;\
    attribute vec4 aVertexColor;\
    attribute vec3 aVertexNormal;\
    uniform mat4 uMVMatrix;\
    uniform mat4 uPMatrix;\
    uniform mat4 uNMatrix;\
    varying vec4 vColor;\
    varying vec4 vPosition;\
    varying vec4 vTransformedNormal;\
    void main(void) {\
        vPosition = uMVMatrix * vec4(aVertexPosition, 1.0);\
        gl_Position = uPMatrix * vPosition;\
        vTransformedNormal = uNMatrix * vec4(aVertexNormal, 1.0);\
        vColor = aVertexColor;\
    }";

	// Point Shaders
    this.view.shaderfsPoint = document.createElement("script");
    this.view.shaderfsPoint.id = "shader-fs-Point";
    this.view.shaderfsPoint.type = "x-shader/x-fragment";
    this.view.shaderfsPoint.innerHTML = "\
    #ifdef GL_ES\n\
    precision highp float;\n\
    #endif\n\
    varying vec4 vColor;\
    void main(void) {\
        gl_FragColor = vColor;\
    }";
    this.view.shadervsPoint = document.createElement("script");
    this.view.shadervsPoint.id = "shader-vs-Point";
    this.view.shadervsPoint.type = "x-shader/x-vertex";
    this.view.shadervsPoint.innerHTML = "\
    attribute vec3 aVertexPosition;\
    attribute vec4 aVertexColor;\
    uniform mat4 uMVMatrix;\
    uniform mat4 uPMatrix;\
    uniform mat4 uNMatrix;\
    uniform float uPointSize;\
    varying vec4 vColor;\
    void main(void) {\
        vec4 pos = uMVMatrix * vec4(aVertexPosition, 1.0);\
        gl_Position = uPMatrix * pos;\
        vColor = aVertexColor*vec4(1.0, 1.0, 1.0, 1.0);\
        gl_PointSize = uPointSize;\
    }";

	//
    this.canvasName = "glcanvas" + rendererId;
    this.view.html = '<div><canvas id="' + this.canvasName + '" style="border: none; overflow: hidden;';
    if (this.forceSquareSize == true) this.view.html += ' position: absolute;';
    this.view.html += ' left:-1px; top:-1px; right:0px; z-index=0;" width="' + this.view.width
    + '" height="' + this.view.height + '"onmousedown="handleMouseDown(event,\''+rendererId+'\')"\
     onmousemove="handleMouseMove(event,\''+rendererId+'\')" onmouseup="handleMouseUp(event,\'' + rendererId
     + '\')" oncontextmenu="consumeEvent(event)"> Your browser doesn\'t appear to support the HTML5 <code>\
     &lt;canvas&gt;</code> element.</canvas>';

     this.view.html += '<canvas id="' + this.canvasName + 'Widget" style="position: absolute; left:-1px; top:-1px; z-index:1;';
     if(this.forceSquareSize == true) this.view.html += 'position: absolute;';
     this.view.html += '" width="' + this.view.width + '" height="' + this.view.height +
     '"onmousedown="handleMouseDown(event,\''+rendererId+'\')" onmousemove="handleMouseMove(event,\''+rendererId+'\')"\
     onmouseup="handleMouseUp(event,\'' + rendererId + '\')" oncontextmenu="consumeEvent(event)"></canvas></div>';
    this.fps = 0;

    // Register in global var
    webglRenderers[rendererId] = this;
}

WebGLRenderer.prototype.bindToElementId = function (elementId) {
    this.oldInnerHTML = document.getElementById(elementId).innerHTML;
    document.getElementById(elementId).innerHTML = this.view.html;

    document.getElementById(elementId).appendChild(this.view.shaderfs);
    document.getElementById(elementId).appendChild(this.view.shadervs);
    document.getElementById(elementId).appendChild(this.view.shaderfsPoint);
    document.getElementById(elementId).appendChild(this.view.shadervsPoint);
}

WebGLRenderer.prototype.unbindToElementId = function (elementId) {
  document.getElementById(elementId).innerHTML = this.oldInnerHTML;
  clearTimeout(this.drawInterval);
  if (typeof(paraview) != "undefined") paraview.updateConfiguration(true, "JPEG", "NO");
}

WebGLRenderer.prototype.setOfflineMode = function (mode) {
  this.offlineMode = mode;
  this.requestMetaData();
}

WebGLRenderer.prototype.bindToElement = function (element) {
    this.oldInnerHTML = element.innerHTML;
    element.innerHTML = this.view.html;

    element.appendChild(this.view.shaderfs);
    element.appendChild(this.view.shadervs);
    element.appendChild(this.view.shaderfsPoint);
    element.appendChild(this.view.shadervsPoint);
}

WebGLRenderer.prototype.unbindToElement = function (element) {
  element.innerHTML = this.oldInnerHTML;
  clearTimeout(this.drawInterval);
  if (typeof(paraview) != "undefined") paraview.updateConfiguration(true, "JPEG", "NO");
}

WebGLRenderer.prototype.init = function (sessionId, viewId) {
    this.sessionId = sessionId;
    this.viewId = viewId;
}

WebGLRenderer.prototype.start = function(metadata, objects) {
    if (typeof(renderers) == "undefined"){
      renderers = Object();
      renderers.current = this;
    }
    if (typeof(paraview) != "undefined") paraview.updateConfiguration(true, "JPEG", "WebGL");
    canvas = document.getElementById(this.canvasName);
    canvas.width = this.view.width;
    canvas.height = this.view.height;

    this.hasSceneChanged = true;        //Scene Graph Has Changed
    this.oldCamPos = null;              //Last Known Camera Position
    this.sceneJSON = null;              //Current Scene Graph
    this.up = [];
    this.right = [];
    this.z_dir = [];
    this.objects = [];                  //List of objects
    this.nbErrors = 0;                  //Number of Errors
    this.background = null;             //Background object: mesh, normals, colors, render
    this.interactionRatio = 2;
    this.requestInterval = 250;         //Frequency it request new data from the server
    this.requestOldInterval = 250;      //
    this.updateInterval = 100;          //Frequency the server will be updated
    this.fps = 0;
    this.frames = 0;
    this.lastTime = new Date().getTime();
    this.view.aspectRatio = 1;
    this.lookAt = [0,0,0,0,1,0,0,0,1];
    this.offlineMode = !(typeof(metadata)=="undefined" || typeof(objects)=="undefined");

    this.cachedObjects = [];            //List of Cached Objects
    this.isCaching = false;             //Is Caching or Not

    this.processQueue = [];             //List of process to be executed

    this.objScale = 1.0;                //Scale applied locally in the scene
    this.translation = [0.0, 0.0, 0.0]; //Translation
    this.rotMatrix = mat4.create();     //Rotation Matrix
    mat4.identity(this.rotMatrix);
    this.rotMatrix2 = mat4.create(this.rotMatrix);

    this.mouseDown = false;
    this.lastMouseX = 0;
    this.lastMouseY = 0;

    this.mvMatrix = mat4.create(this.rotMatrix);
    this.pMatrix = mat4.create(this.rotMatrix);

    // Initialize the GL context
    this.gl = null;
    try {
      this.gl = canvas.getContext("experimental-webgl");
      this.gl.viewportWidth = this.view.width;
      this.gl.viewportHeight = this.view.height;
    } catch(e) {}

    if (this.gl) {
      this.gl.clearColor(0.0, 0.0, 0.0, 1.0);
      this.gl.clearDepth(1.0);
      this.gl.enable(this.gl.DEPTH_TEST);
      this.gl.depthFunc(this.gl.LEQUAL);

      this.gl.blendFunc(this.gl.SRC_ALPHA, this.gl.ONE_MINUS_SRC_ALPHA);

      this.initShaders();

      this.ctx2d = document.getElementById(this.canvasName + "Widget").getContext('2d');
      // Set up to draw the scene periodically.
      this.drawInterval = requestAnimFrame(new Function("webglRenderers['" + this.view.id + "'].drawScene();"));

      if (!this.offlineMode){
        this.requestMetaData();
        this.updateCamera();
      } else {
        this.sceneJSON = JSON.parse(metadata);

        for(aw=0; aw<objects.length-1; aw++){
          obj = new Object();
          obj.data = objects[aw];
          obj.hasTransparency = this.sceneJSON.Objects[aw].transparency;
          obj.layer = this.sceneJSON.Objects[aw].layer;
          obj.render = function(){};
          this.processQueue[aw] = obj;
          this.objects[this.objects.length] = obj;
        }
      }
    } else {
      canvas.parentNode.innerHTML = "<table width=100% height=100%><tr><td align=center>\
      Sorry, your browser do not support WebGL.<br> For more information visit the website\
      <a href='http://get.webgl.org/' target='_blank'>http://get.webgl.org/</a></td></tr></table>";
    }
}

WebGLRenderer.prototype.setForceSquareSize = function(b){
this.forceSquareSize = b;
}

WebGLRenderer.prototype.getPageX = function(){
    var location = 0;
    var node = document.getElementById(this.canvasName);
    while(node) {
        location += node.offsetLeft;
        node = node.offsetParent;
    }
    return location;
}

WebGLRenderer.prototype.getPageY = function(){
    var location = 0;
    var node = document.getElementById(this.canvasName);
    while(node) {
        location += node.offsetTop;
        node = node.offsetParent;
    }
    return location;
}

WebGLRenderer.prototype.setServerMode = function(mode){
  if (typeof(this.interaction) == "undefined"){
    this.interaction = new Object();
    this.interaction.lastRealEvent = 0;
    this.interaction.needUp = false;
    this.interaction.lastEvent = 0;
    this.interaction.isDragging = false;
    this.interaction.action = " ";
    this.interaction.keys = " ";
    this.interaction.button = 0;
    this.interaction.x = 0;
    this.interaction.y = 0;
    this.interaction.xOrigin = 0;
    this.interaction.yOrigin = 0;
    this.serverMode = false;
  }
  if (this.serverMode == mode) return;
  this.serverMode = mode;
  if (!this.serverMode){
    this.updateId = setTimeout("webglRenderers[\'" + this.view.id + "\'].updateCamera()", this.updateInterval);
  }
  canvas = document.getElementById(this.canvasName);
  canvasWidget = document.getElementById(this.canvasName + "Widget");
  if (this.serverMode){
    this.requestOldInterval = this.requestInterval;
    this.requestInterval = 50;
    canvas.setAttribute("onmousedown", "mouseServerInt('"+this.view.id+"','"+this.sessionId+"','"+this.viewId+"','down',event)");
    canvas.setAttribute("onmousemove", "mouseServerInt('"+this.view.id+"','"+this.sessionId+"','"+this.viewId+"','move',event)");
    canvas.setAttribute("onmouseup"  , "mouseServerInt('"+this.view.id+"','"+this.sessionId+"','"+this.viewId+"','up',event)");
    canvasWidget.setAttribute("onmousedown", "mouseServerInt('"+this.view.id+"','"+this.sessionId+"','"+this.viewId+"','down',event)");
    canvasWidget.setAttribute("onmousemove", "mouseServerInt('"+this.view.id+"','"+this.sessionId+"','"+this.viewId+"','move',event)");
    canvasWidget.setAttribute("onmouseup"  , "mouseServerInt('"+this.view.id+"','"+this.sessionId+"','"+this.viewId+"','up',event)");
  } else {
    this.requestInterval = this.requestOldInterval;
    canvas.setAttribute("onmousedown", "handleMouseDown(event,'" + this.rendererId + "')");
    canvas.setAttribute("onmousemove", "handleMouseMove(event,'" + this.rendererId + "')");
    canvas.setAttribute("onmouseup"  , "handleMouseUp(event,'" + this.rendererId + "')");
    canvasWidget.setAttribute("onmousedown", "handleMouseDown(event,'" + this.rendererId + "')");
    canvasWidget.setAttribute("onmousemove", "handleMouseMove(event,'" + this.rendererId + "')");
    canvasWidget.setAttribute("onmouseup"  , "handleMouseUp(event,'" + this.rendererId + "')");
  }
  canvas.setAttribute("oncontextmenu", "consumeEvent(event)");
}

WebGLRenderer.prototype.setSize = function(width, height) {
	width = parseFloat(width);
	height = parseFloat(height);
	w = width;
	h = height;
    this.view.aspectRatio = width/height;
    if(this.forceSquareSize){
      if (width > height) height = width;
      else width = height;
    }
    this.view.width = width;
    this.view.height = height;
    canvas = document.getElementById(this.canvasName);
    canvasWidget = document.getElementById(this.canvasName + "Widget");
    if (canvas){
	    canvas.width = this.view.width;
	    canvas.height = this.view.height;
	    canvasWidget.width = this.view.width;
	    canvasWidget.height = this.view.height;
	    if (typeof(this.gl) != "undefined" && this.gl != null){
	        if (!this.offlineMode) updateRendererSize(this.sessionId, this.viewId, width, height);
		    this.gl.viewportWidth = this.view.width;
		    this.gl.viewportHeight = this.view.height;
	    }
	    left = 0; tt = 0;
	    if (this.forceSquareSize){
	      left = Math.round((w-this.view.width)/2);
	      tt = Math.round((h-this.view.height)/2);
	    }
	    this.view.left = left;
	    this.view.top = top;
	    if(this.forceSquareSize == true){
	      canvas.setAttribute("style", "position: absolute; overflow: hidden; left: " + left + "px; top: " + tt + "px; right: 0px; z-index:0;");
	      canvasWidget.setAttribute("style", "position: absolute; overflow: hidden; left: " + left + "px; top: " + tt + "px; right: 0px; z-index:1;");
	    } else {
	      canvas.setAttribute("style", "overflow: hidden; left: " + left + "px; top: " + tt + "px; right: 0px; z-index:0;");
	      canvasWidget.setAttribute("style", "position: absolute; overflow: hidden; left: " + left + "px; top: " + tt + "px; right: 0px; z-index:1;");
	    }
    }
}

WebGLRenderer.prototype.requestMetaData = function() {
  if (this.mouseDown || renderers.current != this) return;
  if (this.offlineMode) return;

  interval = this.requestInterval;
  if (this.serverMode) interval = interval/2;
  this.timer = setTimeout("webglRenderers[\'" + this.view.id + "\'].requestMetaData()", interval);
  var request = new XMLHttpRequest();
  request.requester = this;
  filename = this.baseURL + "?sid=" + this.sessionId + "&vid=" + this.viewId + "&q=meta";
  try {
    request.open("GET", filename, false);
    request.overrideMimeType('text/plain; charset=x-user-defined');
    request.onreadystatechange = function() {
      if(this.requester.mouseDown) return;
      if (request.status != 200) this.requester.nbErrors++
      else if (request.readyState == 4) {
        aux = JSON.parse(request.responseText);
        this.requester.hasSceneChanged = JSON.stringify(aux)!=JSON.stringify(this.requester.sceneJSON);
        this.requester.sceneJSON = JSON.parse(request.responseText);
        if (this.requester.hasSceneChanged) this.requester.updateScene();
      }
    }
  request.send();
  } catch (e) {
    this.nbErrors++;
  }
}

WebGLRenderer.prototype.updateScene = function(){
  if (typeof(this.sceneJSON) == "undefined" || this.sceneJSON == null) return;
  c1 = [0,0,0];
  c2 = [0,0,0];
  for(l=0; l<this.sceneJSON.Renderers.length; l++){
    if(this.sceneJSON.Renderers[l].layer==0){
      this.lookAt = this.sceneJSON.Renderers[l].LookAt;
      c1 = this.sceneJSON.Renderers[l].Background1;
      if (typeof(this.sceneJSON.Renderers[l].Background2) != "undefined") c2 = this.sceneJSON.Renderers[l].Background2;
    }
  }
  this.initBackground(c1, c2);
  if (JSON.stringify(this.oldCamPos)!=JSON.stringify(this.lookAt)){
    this.translation = [0.0, 0.0, 0.0];
    this.objScale = 1.0;
    mat4.identity(this.rotMatrix);

	this.up = [this.lookAt[4], this.lookAt[5], this.lookAt[6]];
	this.z_dir = [this.lookAt[1]-this.lookAt[7],
                  this.lookAt[2]-this.lookAt[8],
                  this.lookAt[3]-this.lookAt[9]];
	vec3.normalize(this.z_dir, this.z_dir);
	vec3.cross(this.z_dir, this.up, this.right);
  }
  this.oldCamPos = this.lookAt;
  var aux = [];
  intAtServer = false;
  if (!this.offlineMode){
    for(w=0; w<this.objects.length; w++){
      for(j=0; j<this.sceneJSON.Objects.length; j++){
        if (this.objects[w].md5 == this.sceneJSON.Objects[j].md5 && this.objects[w].id == this.sceneJSON.Objects[j].id){
          aux[aux.length] = this.objects[w];
        }
      }
    }
    this.objects = aux;

    for(w=0; w<this.sceneJSON.Objects.length; w++){
      foundit = false;

      if (this.isCaching){
        for(j=0; j<this.cachedObjects.length; j++)
          if (this.cachedObjects[j].md5==this.sceneJSON.Objects[w].md5 &&
              this.cachedObjects[j].id==this.sceneJSON.Objects[w].id){
            this.objects[this.objects.length] = this.cachedObjects[j];
            foundit = true;
          }
      }
      if (!foundit){
        for(k=0; k<this.sceneJSON.Objects[w].parts; k++){
          foundit = false;
	      for(j=0; j<this.objects.length; j++){
	        if (this.objects[j].md5==this.sceneJSON.Objects[w].md5 &&
              this.objects[j].id==this.sceneJSON.Objects[w].id && this.objects[j].part==k+1 )
	          foundit=true;
	      }
          if(!foundit) this.requestObject(this.sessionId, this.sceneJSON.id, this.sceneJSON.Objects[w].md5,
                                      k+1, this.sceneJSON.Objects[w].id, this.sceneJSON.Objects[w].transparency, this.sceneJSON.Objects[w].layer);
          }
      }
      if (this.sceneJSON.Objects[w].interactAtServer==1) intAtServer = true;
    }
  }
  this.hasSceneChanged = false;
  this.setServerMode(intAtServer);
}

WebGLRenderer.prototype.requestObject = function(sid, vid, md5, part, id, hastransparency, layer){
  if (this.offlineMode) return;
  var request = new XMLHttpRequest();
  request.requester = this;
  filename = this.baseURL + "?sid=" + sid + "&vid=" + vid + "&hash=" + md5 + "&part=" + part + "&q=mesh&id=" + id;
  try {
    request.open("GET", filename, false);
    request.overrideMimeType('text/plain; charset=x-user-defined');
    request.onreadystatechange = function() {
      if (request.status != 200) this.requester.nbErrors++
      else if (request.readyState == 4) {
        foundit = -1;
        for (i=0; i<this.requester.objects.length; i++)
          if (this.requester.objects[i].md5 == md5 && this.requester.objects[i].part == part
              && this.requester.objects[i].id == id) foundit = i;
        if (foundit == -1){
          foundit = this.requester.objects.length;
          this.requester.objects.length++;
        }
        this.requester.objects[foundit] = new Object();
        this.requester.objects[foundit].md5 = md5;    //hash
        this.requester.objects[foundit].part = part;  //part
        this.requester.objects[foundit].sid = sid;    //scene id
        this.requester.objects[foundit].vid = vid;    //view id
        this.requester.objects[foundit].id = id;      //object id
        this.requester.objects[foundit].data = request.responseText;
        this.requester.objects[foundit].hasTransparency = hastransparency;
        this.requester.objects[foundit].layer = layer;
        this.requester.objects[foundit].render = function(){};
        this.requester.processQueue[this.requester.processQueue.length] = this.requester.objects[foundit];
        this.requester.cachedObjects[this.requester.cachedObjects.length] = this.requester.objects[foundit];
      }
    }
    request.send();
  } catch (e){
    this.nbErrors++;
  }
}

WebGLRenderer.prototype.parseObject = function(obj){
  var ss = []; pos = 0;
  for(i=0; i<obj.data.length; i++) ss[i] = obj.data.charCodeAt(i) & 0xff;

  size = (ss[pos++]) + (ss[pos++] << 8) + (ss[pos++] << 16) + (ss[pos++] << 24);
  type = String.fromCharCode(ss[pos++]);
  obj.type = type;
  obj.father = this;

  if (type == 'L'){
    obj.numberOfPoints = (ss[pos++]) + (ss[pos++] << 8) + (ss[pos++] << 16) + (ss[pos++] << 24);
    //Getting Points
    test = new Int8Array(obj.numberOfPoints*4*3); for(i=0; i<obj.numberOfPoints*4*3; i++) test[i] = ss[pos++];
    obj.points = new Float32Array(test.buffer);
    //Generating Normals
    test = new Array(obj.numberOfPoints*3); for(i=0; i<obj.numberOfPoints*3; i++) test[i] = 0.0;
    obj.normals = new Float32Array(test);
    //Getting Colors
    test = []; for(i=0; i<obj.numberOfPoints*4; i++) test[i] = ss[pos++]/255.0;
    obj.colors = new Float32Array(test);

    obj.numberOfIndex = (ss[pos++]) + (ss[pos++] << 8) + (ss[pos++] << 16) + (ss[pos++] << 24);
    //Getting Index
    test = new Int8Array(obj.numberOfIndex*2); for(i=0; i<obj.numberOfIndex*2; i++) test[i] = ss[pos++];
    obj.index = new Uint16Array(test.buffer);
    //Getting Matrix
    test = new Int8Array(16*4); for(i=0; i<16*4; i++) test[i] = ss[pos++];
    obj.matrix = new Float32Array(test.buffer);

    //Creating Buffers
    obj.lbuff = this.gl.createBuffer(); this.gl.bindBuffer(this.gl.ARRAY_BUFFER, obj.lbuff);
    this.gl.bufferData(this.gl.ARRAY_BUFFER, obj.points, this.gl.STATIC_DRAW); obj.lbuff.itemSize = 3;

    obj.nbuff = this.gl.createBuffer(); this.gl.bindBuffer(this.gl.ARRAY_BUFFER, obj.nbuff);
    this.gl.bufferData(this.gl.ARRAY_BUFFER, obj.normals, this.gl.STATIC_DRAW);  obj.nbuff.itemSize = 3;

    obj.cbuff = this.gl.createBuffer(); this.gl.bindBuffer(this.gl.ARRAY_BUFFER, obj.cbuff);
    this.gl.bufferData(this.gl.ARRAY_BUFFER, obj.colors, this.gl.STATIC_DRAW);   obj.cbuff.itemSize = 4;

    obj.ibuff = this.gl.createBuffer(); this.gl.bindBuffer(this.gl.ELEMENT_ARRAY_BUFFER, obj.ibuff);
    this.gl.bufferData(this.gl.ELEMENT_ARRAY_BUFFER, obj.index, this.gl.STREAM_DRAW);

    obj.render = this.renderLine;
  }

  //-=-=-=-=-=[ MESH ]=-=-=-=-=-
  else if (type == 'M'){
    obj.numberOfVertices = (ss[pos++]) + (ss[pos++] << 8) + (ss[pos++] << 16) + (ss[pos++] << 24);
    //Getting Vertices
    test = new Int8Array(obj.numberOfVertices*4*3); for(i=0; i<obj.numberOfVertices*4*3; i++) test[i] = ss[pos++];
    obj.vertices = new Float32Array(test.buffer);
    //Getting Normals
    test = new Int8Array(obj.numberOfVertices*4*3); for(i=0; i<obj.numberOfVertices*4*3; i++) test[i] = ss[pos++];
    obj.normals = new Float32Array(test.buffer);
    //Getting Colors
    test = []; for(i=0; i<obj.numberOfVertices*4; i++) test[i] = ss[pos++]/255.0;
    obj.colors = new Float32Array(test);

    obj.numberOfIndex = (ss[pos++]) + (ss[pos++] << 8) + (ss[pos++] << 16) + (ss[pos++] << 24);
    //Getting Index
    test = new Int8Array(obj.numberOfIndex*2); for(i=0; i<obj.numberOfIndex*2; i++) test[i] = ss[pos++];
    obj.index = new Uint16Array(test.buffer);
    //Getting Matrix
    test = new Int8Array(16*4); for(i=0; i<16*4; i++) test[i] = ss[pos++];
    obj.matrix = new Float32Array(test.buffer);
    //Getting TCoord
    obj.tcoord = null;

    //Create Buffers
    obj.vbuff = this.gl.createBuffer(); this.gl.bindBuffer(this.gl.ARRAY_BUFFER, obj.vbuff);
    this.gl.bufferData(this.gl.ARRAY_BUFFER, obj.vertices, this.gl.STATIC_DRAW); obj.vbuff.itemSize = 3;

    obj.nbuff = this.gl.createBuffer(); this.gl.bindBuffer(this.gl.ARRAY_BUFFER, obj.nbuff);
    this.gl.bufferData(this.gl.ARRAY_BUFFER, obj.normals, this.gl.STATIC_DRAW);  obj.nbuff.itemSize = 3;

    obj.cbuff = this.gl.createBuffer(); this.gl.bindBuffer(this.gl.ARRAY_BUFFER, obj.cbuff);
    this.gl.bufferData(this.gl.ARRAY_BUFFER, obj.colors, this.gl.STATIC_DRAW);   obj.cbuff.itemSize = 4;

    obj.ibuff = this.gl.createBuffer(); this.gl.bindBuffer(this.gl.ELEMENT_ARRAY_BUFFER, obj.ibuff);
    this.gl.bufferData(this.gl.ELEMENT_ARRAY_BUFFER, obj.index, this.gl.STREAM_DRAW);

    obj.render = this.renderMesh;
  }

  // ColorMap Widget
  else if (type == 'C'){
    obj.numOfColors = size;

    //Getting Position
    test = new Int8Array(2*4); for(i=0; i<2*4; i++) test[i] = ss[pos++];
    obj.position = new Float32Array(test.buffer);

    //Getting Size
    test = new Int8Array(2*4); for(i=0; i<2*4; i++) test[i] = ss[pos++];
    obj.size = new Float32Array(test.buffer);

    //Getting Colors
    obj.colors = [];
	for(c=0; c<obj.numOfColors; c++){
	  test = new Int8Array(4); for(i=0; i<4; i++) test[i] = ss[pos++];
	  v = new Float32Array(test.buffer);
	  xrgb = [v[0], ss[pos++], ss[pos++], ss[pos++]];
	  obj.colors[c] = xrgb;
	}

	obj.orientation = ss[pos++];
	obj.numOfLabels = ss[pos++];
	tt = "";
	for(jj=0; jj<(ss.length-pos); jj++) tt = tt + String.fromCharCode(ss[pos+jj]);
	obj.title = tt;

    obj.render = this.renderColorMap;
  }

  // Points
  else if (type == 'P'){
    obj.numberOfPoints = (ss[pos++]) + (ss[pos++] << 8) + (ss[pos++] << 16) + (ss[pos++] << 24);
    //Getting Points
    test = new Int8Array(obj.numberOfPoints*4*3); for(i=0; i<obj.numberOfPoints*4*3; i++) test[i] = ss[pos++];
    obj.points = new Float32Array(test.buffer);

    //Getting Colors
    test = []; for(i=0; i<obj.numberOfPoints*4; i++) test[i] = ss[pos++]/255.0;
    obj.colors = new Float32Array(test);

    //Getting Matrix //Wendel
    test = new Int8Array(16*4); for(i=0; i<16*4; i++) test[i] = ss[pos++];
    obj.matrix = new Float32Array(test.buffer);

    //Creating Buffers
    obj.pbuff = this.gl.createBuffer(); this.gl.bindBuffer(this.gl.ARRAY_BUFFER, obj.pbuff);
    this.gl.bufferData(this.gl.ARRAY_BUFFER, obj.points, this.gl.STATIC_DRAW); obj.pbuff.itemSize = 3;

    obj.cbuff = this.gl.createBuffer(); this.gl.bindBuffer(this.gl.ARRAY_BUFFER, obj.cbuff);
    this.gl.bufferData(this.gl.ARRAY_BUFFER, obj.colors, this.gl.STATIC_DRAW);   obj.cbuff.itemSize = 4;

    obj.render = this.renderPoints;
  }
}

WebGLRenderer.prototype.renderColorMap = function(){
  obj = this;
  render = this.father;

  range = [obj.colors[0][0], obj.colors[obj.colors.length-1][0]];
  size = [obj.size[0]*render.view.width, obj.size[1]*render.view.height];
  pos = [obj.position[0]*render.view.width, (1-obj.position[1])*render.view.height];
  pos[1] = pos[1]-size[1];
  dx = size[0]/size[1];
  dy = size[1]/size[0];
  realSize = size;

  textSizeX = Math.round(render.view.height/35);
  textSizeY = Math.round(render.view.height/23);
  if (obj.orientation == 1){
    size[0] = size[0]*dy/25;
    size[1] = size[1]-(2*textSizeY);
  } else {
    size[0] = size[0];
    size[1] = size[1]*dx/25;
  }

  // Draw Gradient
  ctx = this.father.ctx2d;
  if(obj.orientation == 1){
    pos[1] += 2*textSizeY;
    grad = ctx.createLinearGradient(pos[0], pos[1], pos[0], pos[1]+size[1]);
  } else {
    pos[1] += 2*textSizeY;
    grad = ctx.createLinearGradient(pos[0], pos[1], pos[0]+size[0], pos[1]);
  }
  if ((range[1]-range[0]) == 0){
      color = 'rgba(' + obj.colors[0][1] + ',' + obj.colors[0][2] + ',' + obj.colors[0][3] + ',1)';
      grad.addColorStop(0, color);
      grad.addColorStop(1, color);
  } else {
    for(c=0; c<obj.colors.length; c++){
      v = ((obj.colors[c][0]-range[0])/(range[1]-range[0]));
      if (obj.orientation == 1) v=1-v;
      color = 'rgba(' + obj.colors[c][1] + ',' + obj.colors[c][2] + ',' + obj.colors[c][3] + ',1)';
      grad.addColorStop(v, color);
    }
  }
  ctx.fillStyle = grad;
  ctx.fillRect(pos[0], pos[1], size[0], size[1]);
  // Draw Range Labels
  range[0] = Math.round(range[0]*1000)/1000;
  range[1] = Math.round(range[1]*1000)/1000;
  ctx.fillStyle = 'white';
  ctx.font = textSizeY + 'px sans-serif';
  ctx.txtBaseline = 'ideographic';
  if (obj.orientation == 1){
    ctx.fillText(range[1], pos[0], pos[1]-5);
    ctx.fillText(range[0], pos[0], pos[1]+size[1]+textSizeY);
  } else {
    ctx.fillText(range[0], pos[0], pos[1]+size[1]+textSizeY);
    txt = range[1].toString();
    ctx.fillText(range[1], pos[0]+size[0]-((txt.length-1)*textSizeX), pos[1]+size[1]+textSizeY);
  }
  // Draw Title
  ctx.fillStyle = 'white';
  ctx.font = textSizeY + 'px sans-serif';
  ctx.txtBaseline = 'ideographic';
  if (obj.orientation == 1) ctx.fillText(obj.title, pos[0]+(obj.size[0]*render.view.width)/2-(obj.title.length*textSizeX/2), pos[1]-textSizeY-5);
  else ctx.fillText(obj.title, pos[0]+size[0]/2-(obj.title.length*textSizeX/2), pos[1]-textSizeY-5);
  // Draw Intervals' line
  //Draw Interval make the render process slow
  /*
  interval = obj.numOfLabels-1;
  if (obj.orientation == 1){
    diff = size[1]/(interval-1);
    y = pos[1]+size[1];
    x = size[0]/2;
    for(ii=0; ii<interval; ii++){
      y = Math.floor(y) + 0.5;
      if (ii%5) ctx.moveTo(pos[0]+2*x, y);
      else ctx.moveTo(pos[0]+x, y);
      ctx.lineTo(pos[0]+x*3, y);
      ctx.lineWidth = 1;
      ctx.strokeStyle = "white";
      ctx.stroke();
      y -= diff;
    }
  } else {
    diff = size[0]/(interval-1);
    y = size[1]/2;
    x = pos[0];
    for(ii=0; ii<interval; ii++){
      x = Math.floor(x) + 0.5;
      if (ii%5) ctx.moveTo(x, pos[1]);
      else ctx.moveTo(x, pos[1]+y);
      ctx.lineTo(x, pos[1]-y);
      ctx.lineWidth = 1;
      ctx.strokeStyle = "white";
      ctx.stroke();
      x += diff;
    }
  }/**/
}

WebGLRenderer.prototype.initBackground = function(c1, c2){
  if (typeof(this.gl) == "undefined") return;
  if (typeof(this.sceneJSON) == "undefined") return;

  this.background = new Object();
  this.background.vertices = new Float32Array([-1.0, -1.0, 0.0, 1.0, -1.0, 0.0, 1.0, 1.0, 0.0, -1.0, 1.0, 0.0]);
  this.background.colors = new Float32Array([c1[0], c1[1], c1[2], 1.0,
                                             c1[0], c1[1], c1[2], 1.0,
                                             c2[0], c2[1], c2[2], 1.0,
                                             c2[0], c2[1], c2[2], 1.0]);
  this.background.index = new Uint16Array([0, 1, 2, 0, 2, 3]);
  this.background.normals = new Float32Array([0.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 0.0, -1.0, 0.0, 0.0, -1.0]);

  this.background.numberOfIndex = 6;

  //Create Buffers
  this.background.vbuff = this.gl.createBuffer(); this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.background.vbuff);
  this.gl.bufferData(this.gl.ARRAY_BUFFER, this.background.vertices, this.gl.STATIC_DRAW); this.background.vbuff.itemSize = 3;
  this.background.nbuff = this.gl.createBuffer(); this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.background.nbuff);
  this.gl.bufferData(this.gl.ARRAY_BUFFER, this.background.normals, this.gl.STATIC_DRAW);  this.background.nbuff.itemSize = 3;
  this.background.cbuff = this.gl.createBuffer(); this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.background.cbuff);
  this.gl.bufferData(this.gl.ARRAY_BUFFER, this.background.colors, this.gl.STATIC_DRAW);   this.background.cbuff.itemSize = 4;
  this.background.ibuff = this.gl.createBuffer(); this.gl.bindBuffer(this.gl.ELEMENT_ARRAY_BUFFER, this.background.ibuff);
  this.gl.bufferData(this.gl.ELEMENT_ARRAY_BUFFER, this.background.index, this.gl.STREAM_DRAW);
}

WebGLRenderer.prototype.renderBackground = function(){
  if (this.background == null) return;

  this.gl.useProgram(this.shaderProgram);
  this.gl.uniform1i(this.shaderProgram.uIsLine, false);

  mat4.translate(this.mvMatrix, [0.0, 0.0, -1.0]);
  this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.background.vbuff);
  this.gl.vertexAttribPointer(this.shaderProgram.vertexPositionAttribute, this.background.vbuff.itemSize, this.gl.FLOAT, false, 0, 0);
  this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.background.nbuff);
  this.gl.vertexAttribPointer(this.shaderProgram.vertexNormalAttribute, this.background.nbuff.itemSize, this.gl.FLOAT, false, 0, 0);
  this.gl.bindBuffer(this.gl.ARRAY_BUFFER, this.background.cbuff);
  this.gl.vertexAttribPointer(this.shaderProgram.vertexColorAttribute, this.background.cbuff.itemSize, this.gl.FLOAT, false, 0, 0);
  this.gl.bindBuffer(this.gl.ELEMENT_ARRAY_BUFFER, this.background.ibuff);
  this.setMatrixUniforms(this.shaderProgram);
  this.gl.drawElements(this.gl.TRIANGLES, this.background.numberOfIndex, this.gl.UNSIGNED_SHORT, 0);
}

WebGLRenderer.prototype.renderMesh = function(){
  obj = this;
  render = this.father;
  render.gl.useProgram(render.shaderProgram);

  render.gl.uniform1i(render.shaderProgram.uIsLine, false);

  cameraRot = mat4.toRotationMat(render.mvMatrix);
  mat4.transpose(cameraRot);
  inverse = mat4.create(); mat4.inverse(cameraRot, inverse);
  test = mat4.create(obj.matrix);
  mat4.transpose(test);

  icenter = [-render.sceneJSON.Center[0], -render.sceneJSON.Center[1], -render.sceneJSON.Center[2]];

  mvPushMatrix(render.mvMatrix);
  mat4.multiply(render.mvMatrix, cameraRot, render.mvMatrix);
  if(obj.layer == 0) mat4.translate(render.mvMatrix, render.translation);
  mat4.multiply(render.mvMatrix, inverse, render.mvMatrix);

  if(obj.layer == 0) mat4.translate(render.mvMatrix, render.sceneJSON.Center);
  mat4.multiply(render.mvMatrix, cameraRot, render.mvMatrix);
  if(obj.layer == 0) mat4.scale(render.mvMatrix, [render.objScale, render.objScale, render.objScale], render.mvMatrix);
  mat4.multiply(render.mvMatrix, render.rotMatrix, render.mvMatrix);
  mat4.multiply(render.mvMatrix, inverse, render.mvMatrix);
  if(obj.layer == 0) mat4.translate(render.mvMatrix, icenter);

  render.rotMatrix2 = render.mvMatrix;

  mat4.multiply(render.mvMatrix, test, render.mvMatrix);

  render.gl.bindBuffer(render.gl.ARRAY_BUFFER, obj.vbuff);
  render.gl.vertexAttribPointer(render.shaderProgram.vertexPositionAttribute, obj.vbuff.itemSize, render.gl.FLOAT, false, 0, 0);
  render.gl.bindBuffer(render.gl.ARRAY_BUFFER, obj.nbuff);
  render.gl.vertexAttribPointer(render.shaderProgram.vertexNormalAttribute, obj.nbuff.itemSize, render.gl.FLOAT, false, 0, 0);
  render.gl.bindBuffer(render.gl.ARRAY_BUFFER, obj.cbuff);
  render.gl.vertexAttribPointer(render.shaderProgram.vertexColorAttribute, obj.cbuff.itemSize, render.gl.FLOAT, false, 0, 0);
  render.gl.bindBuffer(render.gl.ELEMENT_ARRAY_BUFFER, obj.ibuff);
  render.setMatrixUniforms(render.shaderProgram);
  render.gl.drawElements(render.gl.TRIANGLES, obj.numberOfIndex, render.gl.UNSIGNED_SHORT, 0);
  render.mvMatrix = mvPopMatrix();
}

WebGLRenderer.prototype.renderLine = function(){
  obj = this;
  render = this.father;
  render.gl.useProgram(render.shaderProgram);

  render.gl.enable(render.gl.POLYGON_OFFSET_FILL);  //Avoid zfighting
  render.gl.polygonOffset(-1.0, -1.0);

  render.gl.uniform1i(render.shaderProgram.uIsLine, true);

  cameraRot = mat4.toRotationMat(render.mvMatrix);
  mat4.transpose(cameraRot);
  inverse = mat4.create(); mat4.inverse(cameraRot, inverse);
  test = mat4.create(obj.matrix);
  mat4.transpose(test);

  icenter = [-render.sceneJSON.Center[0], -render.sceneJSON.Center[1], -render.sceneJSON.Center[2]];

  mvPushMatrix(render.mvMatrix);
  mat4.multiply(render.mvMatrix, cameraRot, render.mvMatrix);
  if(obj.layer == 0) mat4.translate(render.mvMatrix, render.translation);
  mat4.multiply(render.mvMatrix, inverse, render.mvMatrix);

  if(obj.layer == 0) mat4.translate(render.mvMatrix, render.sceneJSON.Center);
  mat4.multiply(render.mvMatrix, cameraRot, render.mvMatrix);
  if(obj.layer == 0) mat4.scale(render.mvMatrix, [render.objScale, render.objScale, render.objScale], render.mvMatrix);
  mat4.multiply(render.mvMatrix, render.rotMatrix, render.mvMatrix);
  mat4.multiply(render.mvMatrix, inverse, render.mvMatrix);
  if(obj.layer == 0) mat4.translate(render.mvMatrix, icenter);

  render.rotMatrix2 = render.mvMatrix;

  mat4.multiply(render.mvMatrix, test, render.mvMatrix);

  render.gl.bindBuffer(render.gl.ARRAY_BUFFER, obj.lbuff);
  render.gl.vertexAttribPointer(render.shaderProgram.vertexPositionAttribute, obj.lbuff.itemSize, render.gl.FLOAT, false, 0, 0);
  render.gl.bindBuffer(render.gl.ARRAY_BUFFER, obj.nbuff);
  render.gl.vertexAttribPointer(render.shaderProgram.vertexNormalAttribute, obj.nbuff.itemSize, render.gl.FLOAT, false, 0, 0);
  render.gl.bindBuffer(render.gl.ARRAY_BUFFER, obj.cbuff);
  render.gl.vertexAttribPointer(render.shaderProgram.vertexColorAttribute, obj.cbuff.itemSize, render.gl.FLOAT, false, 0, 0);
  render.gl.bindBuffer(render.gl.ELEMENT_ARRAY_BUFFER, obj.ibuff);
  render.setMatrixUniforms(render.shaderProgram);
  render.gl.drawElements(render.gl.LINES, obj.numberOfIndex, render.gl.UNSIGNED_SHORT, 0);
  render.mvMatrix = mvPopMatrix();

  render.gl.disable(render.gl.POLYGON_OFFSET_FILL);
}

WebGLRenderer.prototype.renderPoints = function(){
  obj = this;
  render = this.father;
  render.gl.useProgram(render.pointShaderProgram);

  render.gl.enable(render.gl.POLYGON_OFFSET_FILL);  //Avoid zfighting
  render.gl.polygonOffset(-1.0, -1.0);

  render.gl.uniform1f(render.pointShaderProgram.uPointSize, 2.0);//Wendel

  cameraRot = mat4.toRotationMat(render.mvMatrix);
  mat4.transpose(cameraRot);
  inverse = mat4.create(); mat4.inverse(cameraRot, inverse);
  test = mat4.create(obj.matrix);
  mat4.transpose(test);

  icenter = [-render.sceneJSON.Center[0], -render.sceneJSON.Center[1], -render.sceneJSON.Center[2]];

  mvPushMatrix(render.mvMatrix);
  mat4.multiply(render.mvMatrix, cameraRot, render.mvMatrix);
  if(obj.layer == 0) mat4.translate(render.mvMatrix, render.translation);
  mat4.multiply(render.mvMatrix, inverse, render.mvMatrix);

  if(obj.layer == 0) mat4.translate(render.mvMatrix, render.sceneJSON.Center);
  mat4.multiply(render.mvMatrix, cameraRot, render.mvMatrix);
  if(obj.layer == 0) mat4.scale(render.mvMatrix, [render.objScale, render.objScale, render.objScale], render.mvMatrix);
  mat4.multiply(render.mvMatrix, render.rotMatrix, render.mvMatrix);
  mat4.multiply(render.mvMatrix, inverse, render.mvMatrix);
  if(obj.layer == 0) mat4.translate(render.mvMatrix, icenter);

  render.rotMatrix2 = render.mvMatrix;

  mat4.multiply(render.mvMatrix, test, render.mvMatrix);

  render.gl.bindBuffer(render.gl.ARRAY_BUFFER, obj.pbuff);
  render.gl.vertexAttribPointer(render.pointShaderProgram.vertexPositionAttribute, obj.pbuff.itemSize, render.gl.FLOAT, false, 0, 0);
  render.gl.bindBuffer(render.gl.ARRAY_BUFFER, obj.cbuff);
  render.gl.vertexAttribPointer(render.pointShaderProgram.vertexColorAttribute, obj.cbuff.itemSize, render.gl.FLOAT, false, 0, 0);
  render.setMatrixUniforms(render.pointShaderProgram);
  render.gl.drawArrays(render.gl.POINTS, 0, obj.numberOfPoints);//Wendel
  render.mvMatrix = mvPopMatrix();

  render.gl.disable(render.gl.POLYGON_OFFSET_FILL);
}

WebGLRenderer.prototype.setMatrixUniforms = function(s) {
  mvMatrixInv = mat4.create();
  normal = mat4.create();
  mat4.inverse(this.mvMatrix, mvMatrixInv);
  mat4.transpose(mvMatrixInv, normal);

  this.gl.uniformMatrix4fv(s.pMatrixUniform, false, this.pMatrix);
  this.gl.uniformMatrix4fv(s.mvMatrixUniform, false, this.mvMatrix);
  if(s.nMatrixUniform != null) this.gl.uniformMatrix4fv(s.nMatrixUniform, false, normal);
}

WebGLRenderer.prototype.processObject = function() {
  if (this.processQueue.length != 0){
    obj = this.processQueue[this.processQueue.length-1];
    this.processQueue.length -= 1;
    this.parseObject(obj);
  }
}

WebGLRenderer.prototype.drawScene = function() {
  this.drawInterval = requestAnimFrame(new Function("webglRenderers['" + this.view.id + "'].drawScene();"));
  if (this.hasSceneChanged){
    this.updateScene();
  }
  if (this.sceneJSON == null){
    return;
  }
  this.frames++;

  if(this.frames >= 50 && this.nbErrors < 5){
    this.frames = 0;
    ko = new Date();
    currTime = ko.getTime();
    diff = currTime - this.lastTime;
    this.lastTime = currTime;
    this.fps = 50000/diff;
  }
  this.processObject();

  this.gl.viewport(0, 0, this.gl.viewportWidth, this.gl.viewportHeight);
  this.gl.clear(this.gl.COLOR_BUFFER_BIT | this.gl.DEPTH_BUFFER_BIT);

  mat4.ortho(-1.0, 1.0, -1.0, 1.0, 	1.0, 1000000.0, this.pMatrix);
  mat4.identity(this.mvMatrix);
  this.gl.disable(this.gl.DEPTH_TEST);
  this.renderBackground();
  this.gl.enable(this.gl.DEPTH_TEST);

  this.ctx2d.clearRect(0, 0, this.view.width, this.view.height);
  for(rr=this.sceneJSON.Renderers.length-1; rr>=0 ; rr--){
    renderer = this.sceneJSON.Renderers[rr];
    width  = renderer.size[0]-renderer.origin[0];
    height = renderer.size[1]-renderer.origin[1];
    width = width*this.view.width;
    height = height*this.view.height;
    x = renderer.origin[0]*this.view.width;
    y = renderer.origin[1]*this.view.height;
    if (y < 0) y = 0;
    this.gl.viewport(x, y, width, height);
    //this.gl.clear(this.gl.DEPTH_BUFFER_BIT);
    mat4.perspective(renderer.LookAt[0], width/height, 0.1, 1000000.0, this.pMatrix);
    mat4.identity(this.mvMatrix);
    mat4.lookAt([renderer.LookAt[7], renderer.LookAt[8], renderer.LookAt[9]],
                [renderer.LookAt[1], renderer.LookAt[2], renderer.LookAt[3]],
                [renderer.LookAt[4], renderer.LookAt[5], renderer.LookAt[6]],
                this.mvMatrix);

    for(r=0; r<this.objects.length; r++){
      if (!this.objects[r].hasTransparency && this.objects[r].layer == rr) this.objects[r].render();
    }
    //Render Objects with Transparency
    this.gl.enable(this.gl.BLEND);                //Enable transparency
    this.gl.enable(this.gl.POLYGON_OFFSET_FILL);  //Avoid zfighting
    this.gl.polygonOffset(-1.0, -1.0);
    for(r=0; r<this.objects.length; r++){
      if (this.objects[r].hasTransparency && this.objects[r].layer == rr) this.objects[r].render();
    }
    this.gl.disable(this.gl.POLYGON_OFFSET_FILL);
    this.gl.disable(this.gl.BLEND);
  }
}

WebGLRenderer.prototype.initShaders = function() {
  this.shaderProgram;
  this.pointShaderProgram;

  var fragmentShader = this.getShader("shader-fs");
  var vertexShader = this.getShader("shader-vs");
  var pointFragShader = this.getShader("shader-fs-Point");
  var pointVertShader = this.getShader("shader-vs-Point");

  this.shaderProgram = this.gl.createProgram();
  this.gl.attachShader(this.shaderProgram, vertexShader);
  this.gl.attachShader(this.shaderProgram, fragmentShader);
  this.gl.linkProgram(this.shaderProgram);
  if (!this.gl.getProgramParameter(this.shaderProgram, this.gl.LINK_STATUS)) {
      alert("Could not initialise shaders");
  }
  this.pointShaderProgram = this.gl.createProgram();
  this.gl.attachShader(this.pointShaderProgram, pointVertShader);
  this.gl.attachShader(this.pointShaderProgram, pointFragShader);
  this.gl.linkProgram(this.pointShaderProgram);
  if (!this.gl.getProgramParameter(this.pointShaderProgram, this.gl.LINK_STATUS)) {
      alert("Could not initialise the point shaders");
  }

  this.gl.useProgram(this.pointShaderProgram);
  this.pointShaderProgram.vertexPositionAttribute = this.gl.getAttribLocation(this.pointShaderProgram, "aVertexPosition");
  this.gl.enableVertexAttribArray(this.pointShaderProgram.vertexPositionAttribute);
  this.pointShaderProgram.vertexColorAttribute = this.gl.getAttribLocation(this.pointShaderProgram, "aVertexColor");
  this.gl.enableVertexAttribArray(this.pointShaderProgram.vertexColorAttribute);
  this.pointShaderProgram.pMatrixUniform = this.gl.getUniformLocation(this.pointShaderProgram, "uPMatrix");
  this.pointShaderProgram.mvMatrixUniform = this.gl.getUniformLocation(this.pointShaderProgram, "uMVMatrix");
  this.pointShaderProgram.nMatrixUniform = this.gl.getUniformLocation(this.pointShaderProgram, "uNMatrix");
  this.pointShaderProgram.uPointSize = this.gl.getUniformLocation(this.pointShaderProgram, "uPointSize");

  this.gl.useProgram(this.shaderProgram);
  this.shaderProgram.vertexPositionAttribute = this.gl.getAttribLocation(this.shaderProgram, "aVertexPosition");
  this.gl.enableVertexAttribArray(this.shaderProgram.vertexPositionAttribute);
  this.shaderProgram.vertexColorAttribute = this.gl.getAttribLocation(this.shaderProgram, "aVertexColor");
  this.gl.enableVertexAttribArray(this.shaderProgram.vertexColorAttribute);
  this.shaderProgram.vertexNormalAttribute = this.gl.getAttribLocation(this.shaderProgram, "aVertexNormal");
  this.gl.enableVertexAttribArray(this.shaderProgram.vertexNormalAttribute);
  this.shaderProgram.pMatrixUniform = this.gl.getUniformLocation(this.shaderProgram, "uPMatrix");
  this.shaderProgram.mvMatrixUniform = this.gl.getUniformLocation(this.shaderProgram, "uMVMatrix");
  this.shaderProgram.nMatrixUniform = this.gl.getUniformLocation(this.shaderProgram, "uNMatrix");
  this.shaderProgram.uIsLine = this.gl.getUniformLocation(this.shaderProgram, "uIsLine");
}

WebGLRenderer.prototype.getShader = function(id) {
    var shaderScript = document.getElementById(id);
    if (!shaderScript) {
        return null;
    }

    var str = "";
    var k = shaderScript.firstChild;
    while (k) {
        if (k.nodeType == 3) {
            str += k.textContent;
        }
        k = k.nextSibling;
    }

    var shader;
    if (shaderScript.type == "x-shader/x-fragment") {
        shader = this.gl.createShader(this.gl.FRAGMENT_SHADER);
    } else if (shaderScript.type == "x-shader/x-vertex") {
        shader = this.gl.createShader(this.gl.VERTEX_SHADER);
    } else {
        return null;
    }

    this.gl.shaderSource(shader, str);
    this.gl.compileShader(shader);

    if (!this.gl.getShaderParameter(shader, this.gl.COMPILE_STATUS)) {
        alert(this.gl.getShaderInfoLog(shader));
        return null;
    }

    return shader;
}

WebGLRenderer.prototype.forceUpdateCamera = function(){
  if (typeof(this.gl) == "undefined" || this.gl == null || renderers.current != this) return;
  if (this.offlineMode) return;
  render = this;

  pos = [render.lookAt[7], render.lookAt[8], render.lookAt[9]];
  up  = [render.lookAt[4], render.lookAt[5], render.lookAt[6]];
  fp  = [render.lookAt[1], render.lookAt[2], render.lookAt[3]];
  tt  = [render.translation[0], render.translation[1], 0.0];
  center = [render.sceneJSON.Center[0], render.sceneJSON.Center[1], render.sceneJSON.Center[2]];

  cameraRot = mat4.toRotationMat(render.mvMatrix);
  mat4.transpose(cameraRot);
  inverse = mat4.create(); mat4.inverse(cameraRot, inverse);

  inv = mat4.create();
  mat4.identity(inv);
  mat4.multiply(inv, cameraRot, inv);
  mat4.scale(inv, [render.objScale, render.objScale, render.objScale], inv);
  mat4.multiply(inv, render.rotMatrix, inv);
  mat4.multiply(inv, inverse, inv);

  mat4.inverse(inv, inv);
  fp = vec3.subtract(fp, center, fp);
  pos = vec3.subtract(pos, center, pos);
  mat4.multiplyVec3(inv, fp, fp);
  mat4.multiplyVec3(inv, pos, pos);
  mat4.multiplyVec3(inv, up, up);
  fp = vec3.add(fp, center, fp);
  pos = vec3.add(pos, center, pos);
  vec3.normalize(up, up);

  tt2 = [0, 0, 0];
  tt2[0] += tt[0]*render.right[0];
  tt2[1] += tt[0]*render.right[1];
  tt2[2] += tt[0]*render.right[2];
  tt2[0] += tt[1]*render.up[0];
  tt2[1] += tt[1]*render.up[1];
  tt2[2] += tt[1]*render.up[2];

  vec3.subtract(pos, tt2, pos);
  vec3.subtract(fp , tt2, fp);

  paraviewObjects[parseInt(render.sessionId)].sendEvent("UpdateCamera", render.viewId + " " + fp[0] + " " + fp[1] + " " + fp[2]
  + " " + up[0] + " " + up[1] + " " + up[2] + " " + pos[0] + " " + pos[1] + " " + pos[2]);
  clearTimeout(this.updateId);
  this.updateId = setTimeout("webglRenderers[\'" + this.view.id + "\'].updateCamera()", this.updateInterval);
}

WebGLRenderer.prototype.updateCamera = function(){
  if (!this.mouseDown){
    this.updateId = setTimeout("webglRenderers[\'" + this.view.id + "\'].updateCamera()", this.updateInterval);
    return;
  }
  if (this.serverMode) return;
  this.forceUpdateCamera();
}

/**********************************************************************************/

var mvMatrixStack = [];
function mvPushMatrix(m) {
    var copy = mat4.create();
    mat4.set(m, copy);
    mvMatrixStack.push(copy);
}

function mvPopMatrix() {
  if (mvMatrixStack.length == 0) {
    throw "Invalid popMatrix!";
  }
  return mvMatrixStack.pop();
}


function handleMouseDown(event, id) {
  render = webglRenderers[id];
  render.mouseDown = true;
  render.lastMouseX = event.clientX;
  render.lastMouseY = event.clientY;
  if (!render.offlineMode){
    paraviewObjects[render.sessionId].sendEvent('MouseEvent', render.viewId + ' 0 0');
    updateRendererSize(render.sessionId, render.viewId, render.view.width/render.interactionRatio, render.view.height/render.interactionRatio);
  }
  event.preventDefault();
  return false;
}

function handleMouseUp(event, id) {
  render = webglRenderers[id];
  render.mouseDown = false;
  if (!render.offlineMode){
    paraviewObjects[render.sessionId].sendEvent('MouseEvent', render.viewId + ' 2 0');
    updateRendererSize(render.sessionId, render.viewId, render.view.width, render.view.height);
    render.forceUpdateCamera();
    render.requestMetaData();
  }
  event.preventDefault();
}

function handleMouseMove(event, id) {
  render = webglRenderers[id];
  if (!render.mouseDown) {
    return;
  }
  var newX = event.clientX;
  var newY = event.clientY;
  var deltaX = newX - render.lastMouseX;
  var deltaY = newY - render.lastMouseY;

  if (event.button == 0){
    var rX = deltaX/50.0;
    var rY = deltaY/50.0;
    var mx = mat4.create(); mat4.identity(mx); mat4.rotate(mx, rX, [0, 1, 0]);
    var my = mat4.create(); mat4.identity(my); mat4.rotate(my, rY, [1, 0, 0]);
    mat4.multiply(mx, my, mx);
    mat4.multiply(mx, render.rotMatrix, render.rotMatrix);
  } else if (event.button == 1){
    z = Math.abs(render.sceneJSON.Renderers[0].LookAt[9]-render.sceneJSON.Renderers[0].LookAt[3]);
    aux = z/render.objScale;
    render.translation[0] += aux*deltaX/1500.0;
    render.translation[1] -= aux*deltaY/1500.0;
  } else if (event.button == 2){
    render.objScale += render.objScale*(deltaY)/200.0;
  } else {
    render.objScale += render.objScale*(deltaY)/200.0;
  }

  render.lastMouseX = newX;
  render.lastMouseY = newY;

  event.preventDefault();
}

function mouseServerInt(rendererId, sessionId, viewId, action, event){
    consumeEvent(event);
	render = webglRenderers[rendererId];
    render.interaction.lastRealEvent = event;
    var width = render.view.width;
    var height = render.view.height;

    if(action == 'down') {
        if(render.interaction.needUp) {
            paraviewObjects[sessionId].sendEvent('MouseEvent', viewId + ' 2 ' + render.interaction.lastEvent);
        }
        render.interaction.isDragging = true;
        render.interaction.needUp = true;
        switch(event.button){
            case 1:
                render.interaction.button =  '0 ';
                break;
            case 4:
                render.interaction.button =  '1 ';
                break;
            case 2:
                render.interaction.button =  '2 ';
                break;
        }
        render.interaction.action = " 0 ";
        render.interaction.keys = "";
        if(event.ctrlKey) {
            render.interaction.keys += "1";
        } else {
            render.interaction.keys += "0";
        }
        if(event.shiftKey) {
            render.interaction.keys += " 1";
        } else {
            render.interaction.keys += " 0";
        }
        render.interaction.x = event.screenX;
        render.interaction.y = event.screenY;

        // Keep relative origin
        var docX = event.pageX;
        var docY = event.pageY;
        render.interaction.xOrigin = docX - render.getPageX();
        render.interaction.yOrigin = docY - render.getPageY();
    } else if (action == 'move') {
        render.interaction.action = " 1 ";
    } else if ( action == 'up' || action == 'click') {
        render.interaction.isDragging = false;
        render.interaction.needUp = false;
        render.interaction.action = " 2 ";
        //
        var mouseInfo = ((event.screenX-render.interaction.x + render.interaction.xOrigin)/height) + " " + (1-(event.screenY-render.interaction.y + render.interaction.yOrigin)/height) + " " + render.interaction.keys ;
        render.interaction.lastEvent = render.interaction.button + mouseInfo;
        paraviewObjects[sessionId].sendEvent('MouseEvent', viewId + render.interaction.action + render.interaction.lastEvent);
        render.interaction.scale = 1;
        render.interaction.button = event.button + ' ';
        render.interaction.keys = "0 0"
    }
    if(render.interaction.isDragging ){
        var mouseInfoDrag = ((event.screenX-render.interaction.x + render.interaction.xOrigin)/height) + " " + (1-(event.screenY-render.interaction.y + render.interaction.yOrigin)/height) + " " + render.interaction.keys ;
        var mouseAction = 'MouseEvent';
        if(action == 'move') {
            mouseAction = 'MouseMove';
        }
        render.interaction.lastEvent = render.interaction.button + mouseInfoDrag;
        paraviewObjects[sessionId].sendEvent(mouseAction, viewId + render.interaction.action + render.interaction.lastEvent);
    }
    return false;
}
