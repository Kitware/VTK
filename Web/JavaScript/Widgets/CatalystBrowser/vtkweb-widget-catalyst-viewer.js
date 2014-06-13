(function ($, GLOBAL) {
    var SLIDER_TEMPLATE = '<div class="label"><span class="flag vtk-icon-flag"/>LABEL<span class="NAME-value">DEFAULT</span></div><input type="range" min="0" max="SIZE" value="INDEX" name="NAME" data-values="VALUES"/>',
    SELECT_TEMPLATE = ' <div class="label select"><span class="flag vtk-icon-flag"/>LABEL<select name="NAME">VALUES</select></div>',
    OPTION_TEMPLATE = '<option>VALUE</option>',
    EXCLUDE_ARGS = { "theta": true };

    // ========================================================================
    // Helper method
    // ========================================================================

    function getRelativeLocation(element, mouseEvent) {
        var parentOffset = element.offset(),
        x = mouseEvent.pageX || mouseEvent.originalEvent.pageX || mouseEvent.originalEvent.mozMovementX,
        y = mouseEvent.pageY || mouseEvent.originalEvent.pageY || mouseEvent.originalEvent.mozMovementY,
        relX = x - parentOffset.left,
        relY = y - parentOffset.top;
        return [ relX, relY ];
    }

    // ========================================================================
    // Download manager
    // ========================================================================

    function createDownloadManager(container, poolSize, basepath) {
        var idleImages = [], processingQueue = [], manager = {
            clearProcessingQueue: function() {
                processingQueue = [];
            },

            download: function(url) {
                processingQueue.push(url);
                download();
            },

            downloadFiles: function(filePattern, argName, argValues, args) {
                var baseFileName = filePattern, rStr = '{'+argName+'}';

                for(key in args) {
                    if(key !== argName) {
                        baseFileName = baseFileName.replace('{'+key+'}', args[key]);
                    }
                }

                for(idx in argValues) {
                    processingQueue.push(basepath + '/' + baseFileName.replace(rStr, argValues[idx]));
                }
            }
        };

        // Attach download manager to container
        container.data('download-manager', manager);
        container.bind('load-image', function(e) {
            manager.download(basepath + '/' + e.filename);
        });

        function download() {
            while(idleImages.length > 0 && processingQueue.length > 0) {
                var img = idleImages.pop(),
                url = processingQueue.pop();
                img.src = url;
            }
        }

        function onLoadCallback(arg) {
            var me = $(this), url = me.attr('src');
            idleImages.push(this);
            container.trigger({
                type: "image-loaded",
                url: url
            });
            download();
        }

        function onError() {
            idleImages.push(this);
            download();
        }

        for(var i = 0; i < poolSize; ++i) {
            var img = new Image();
            img.onload = onLoadCallback;
            img.onabort = onError;
            img.onerror = onError;
            idleImages.push(img);
        }

        return manager;
    }

    // ========================================================================
    // Events
    // ========================================================================

    function fireLoadImage(container) {
        // Extrat container info
        var filename = container.data('info')['name_pattern'],
        args = container.data('active-args');

        // Update filename
        for(key in args) {
            filename = filename.replace('{'+key+'}', args[key]);
        }

        // Trigger event
        container.trigger({
            type: 'load-image',
            arguments: args,
            filename: filename
        });
    }

    // ========================================================================
    // Listeners
    // ========================================================================

    function initializeListeners(container) {
        var play = $('.play', container),
        stop = $('.stop', container),
        currentArgs = container.data('active-args'),
        activeArgName = null,
        activeValues = [],
        activeValueIndex = 0,
        keepAnimation = false;

        function animate() {
            if(activeArgName !== null) {
                activeValueIndex++;
                activeValueIndex = activeValueIndex % activeValues.length;
                updateActiveArgument(container, activeArgName, activeValues[activeValueIndex]);

                if(keepAnimation) {
                    setTimeout(animate, 150);
                }
            }
        }

        // Update Control UI when camera position change
        container.bind('invalidate-viewport', function(){
            // Update phi
            var currentPhi = Number(currentArgs.phi),
            phiSlider = $('input[name="phi"]', container),
            values = phiSlider.attr('data-values').split(':'),
            newIdx = 0,
            count = values.length;

            // Find matching index
            while(count--) {
                if(Number(values[count]) === currentPhi) {
                    newIdx = count;
                    count = 0;
                }
            }

            // Update slider value
            phiSlider.val(newIdx).trigger('change');
            $('span.phi-value', container).html(currentArgs.phi);
        });

        // Attach slider listener
        $('input[type="range"]', container).bind('change keyup mousemove',function(){
            var slider = $(this),
            name = slider.attr('name'),
            values = slider.attr('data-values').split(":"),
            idx = slider.val();

            updateActiveArgument(container, name, values[idx]);
        });

        // Attach select listener
        $('select', container).change(function(){
            var select = $(this),
            name = select.attr('name'),
            value = select.val();

            updateActiveArgument(container, name, value);
        });

        $('.toggle', container).click(function(){
            container.toggleClass('small');
        });

        $('.reset', container).click(function(){
            container.trigger('invalidate-size');
        });

        $('.label', container).click(function(){
            var me = $(this),
            all = $('.label', container),
            selectObj = $('select', me.parent()),
            sliderObj = $('input', me.parent());

            // Handle flag visibility
            all.removeClass('active');
            me.addClass('active');

            // Extract active parameter
            if(selectObj.length) {
                activeArgName = selectObj.attr('name');
                activeValueIndex = 0;
                activeValues = [];
                $('option', selectObj).each(function(idx, elm) {
                   activeValues.push($(this).text());
                });
            }
            if(sliderObj.length) {
                activeArgName = sliderObj.attr('name');
                activeValueIndex = sliderObj.val();
                activeValues = sliderObj.attr('data-values').split(':');
            }
        });

        play.click(function(){
            play.hide();
            stop.show();
            keepAnimation = true;
            animate();
        });
        stop.click(function(){
            stop.hide();
            play.show();
            keepAnimation = false;
        });
    }

    // ------------------------------------------------------------------------

    function updateActiveArgument(container, name, value) {
        if(container.data('active-args')[name] !== value) {
            var downloadManager = container.data('download-manager'),
            info = container.data('info');
            container.data('active-args')[name] = value;
            $('span.'+name+'-value', container).html(value);
            downloadManager.clearProcessingQueue();
            fireLoadImage(container);

            // Try to cache all argument values
            if(container.data('preload')) {
                downloadManager.downloadFiles(info['name_pattern'], name, info['arguments'][name]['values'], container.data('active-args'));
            }
        }
    }

    // ========================================================================
    // UI
    // ========================================================================

    var WidgetFactory = {
        "range": function(name, label, values, defaultValue) {
            return templateReplace(SLIDER_TEMPLATE, name, label, values, defaultValue);
        },
        "list": function(name, label, values, defaultValue) {
            var options = [];
            for(var idx in values) {
                options.push(OPTION_TEMPLATE.replace('VALUE', values[idx]));
            }
            return templateReplace(SELECT_TEMPLATE, name, label, [ options.join('') ], defaultValue);
        }
    };

    // ------------------------------------------------------------------------

    function templateReplace( templateString, name, label, values, defaultValue) {
        return templateString.replace(/NAME/g, name).replace(/LABEL/g, label).replace(/VALUES/g, values.join(':')).replace(/SIZE/g, values.length - 1).replace(/DEFAULT/g, defaultValue).replace(/INDEX/g, values.indexOf(defaultValue));
    }

    // ------------------------------------------------------------------------

    function createControlPanel(container, args) {
        var htmlBuffer = [],
        controlContainer = $('<div/>', {
            class: 'control',
            html: '<div class="header"><span class="vtk-icon-tools toggle"/><span class="vtk-icon-resize-full-2 reset"/><span class="vtk-icon-play play"/><span class="vtk-icon-stop stop"/></div><div class="parameters"></div>'
        });

        // Loop over each option
        for (key in args) {
            var name = key,
            type = args[key].type,
            label = args[key].label,
            values = args[key].values,
            defaultValue = args[key]['default'];

            // Update default value
            updateActiveArgument(container, name, defaultValue);

            // Filter out from UI some pre-defined args
            if(EXCLUDE_ARGS.hasOwnProperty(key)) {
                continue;
            }

            // Build widget if needed
            if(values.length > 1) {
                 htmlBuffer.push(WidgetFactory[type](name, label, values, defaultValue));
            }
        }


        // Add control panel to UI
        htmlBuffer.sort();
        $('<ul/>', {
            html: '<li>' + htmlBuffer.join('</li><li>') + '</li>'
        }).appendTo($('.parameters', controlContainer));
        controlContainer.appendTo(container);

        // Attache listeners
        initializeListeners(container);
    }

    // ----------------------------------------------------------------------

    function attachTouchListener(container) {
        var current_button = null, posX, posY, defaultDragButton = 1,
        isZooming = false, isDragging = false, mouseAction = 'up', target;

        function mobileTouchInteraction(evt) {
            evt.gesture.preventDefault();
            switch(evt.type) {
                case 'drag':
                    if(isZooming) {
                        return;
                    }
                    current_button = defaultDragButton;
                    if(mouseAction === 'up') {
                        mouseAction = "down";

                        target = evt.gesture.target;
                        isDragging = true;
                    } else {
                        mouseAction = "move";
                    }

                    posX = evt.gesture.touches[0].pageX;
                    posY = evt.gesture.touches[0].pageY;
                    break;
                case 'hold':
                    if(defaultDragButton === 1) {
                        defaultDragButton = 2;
                        //container.html("Pan mode").css('color','#FFFFFF');
                    } else {
                        defaultDragButton = 1;
                        //container.html("Rotation mode").css('color','#FFFFFF');
                    }

                    break;
                case 'release':
                    //container.html('');
                    current_button = 0;
                    mouseAction = "up";
                    isZooming = false;
                    isDragging = false;
                    break;
                case 'doubletap':
                    container.trigger('resetCamera');
                    return;
                case 'pinch':
                    if(isDragging) {
                        return;
                    }
                    current_button = 3;
                    if(mouseAction === 'up') {
                        mouseAction = 'down';
                        posX = 0;
                        posY = container.height();
                        target = evt.gesture.target;
                        isZooming = true;
                    } else {
                        mouseAction = 'move';
                        posY = container.height() * (1+(evt.gesture.scale-1)/2);
                    }
                    break;
            }

            // Trigger event
            container.trigger({
                type: 'mouse',
                action: mouseAction,
                current_button: current_button,
                charCode: '',
                altKey: false,
                ctrlKey: false,
                shiftKey: false,
                metaKey: false,
                delegateTarget: target,
                pageX: posX,
                pageY: posY
            });
        }

        // Bind listener to UI container
        container.hammer({
            prevent_default : true,
            prevent_mouseevents : true,
            transform : true,
            transform_always_block : true,
            transform_min_scale : 0.03,
            transform_min_rotation : 2,
            drag : true,
            drag_max_touches : 1,
            drag_min_distance : 10,
            swipe : false,
            hold : true // To switch from rotation to pan
        }).on("doubletap pinch drag release hold", mobileTouchInteraction);
    }

    // ------------------------------------------------------------------------

    function createZoomableCanvasObject(container, img, canvas, pixelZoomRatio) {
        // First set up some variables we will need
        var modeRotation = 1,   // when dragging, it's a rotation
        modePan = 2,            // when dragging, it's a pan
        modeZoom = 3,           // when dragging, it's a zoom
        modeNone = 0,           // No mouse move handling
        mouseMode = modeNone,   // Current mode

        dzScale = 0.005,  // scaling factor to control how fast we zoom in and out
        wheelZoom = 0.05, // amount to change zoom with each wheel event

        drawingCenter = [0,0],  // Drawing parameters
        zoomLevel = 1.0,        //

        maxZoom = pixelZoomRatio, // limit how far we can zoom in
        minZoom = 1 / maxZoom,    // limit how far we can zoom out

        lastLocation = [0,0],  // Last place mouse event happened

        // Rotation management vars
        thetaValues, phiValues, stepPhi, stepTheta, currentArgs;

        /*
         * Adds mouse event handlers so that we can pan and zoom the image
         */
        function setupEvents() {
            var element = canvas;

            // Needed this to override context menu behavior
            element.bind('contextmenu', function(evt) { evt.preventDefault(); });

            // Wheel should zoom across browsers
            element.bind('DOMMouseScroll mousewheel', function (evt) {
                var x = (-evt.originalEvent.wheelDeltaY || evt.originalEvent.detail);

                lastLocation = getRelativeLocation(canvas, evt);
                handleZoom((x > 0 ? wheelZoom : x < 0 ? -wheelZoom : 0));
                evt.preventDefault();

                // Redraw the image in the canvas
                redrawImage();
            });

            // Handle mobile
            attachTouchListener(element);
            element.bind('mouse', function(e){
                // action: mouseAction,
                // current_button: current_button,
                // charCode: '',
                // altKey: false,
                // ctrlKey: false,
                // shiftKey: false,
                // metaKey: false,
                // delegateTarget: target,
                // pageX: posX,
                // pageY: posY
                var action = e.action,
                altKey = e.altKey,
                shiftKey = e.shiftKey,
                ctrlKey = e.ctrlKey,
                x = e.pageX,
                y = e.pageY,
                current_button = e.current_button;

                if(action === 'down') {
                    if (e.altKey) {
                        current_button = 2;
                        e.altKey = false;
                    } else if (e.shiftKey) {
                        current_button = 3;
                        e.shiftKey = false;
                    }
                    // Detect interaction mode
                    switch(current_button) {
                        case 2: // middle mouse down = pan
                            mouseMode = modePan;
                            break;
                        case 3: // right mouse down = zoom
                            mouseMode = modeZoom;
                            break;
                        default:
                            mouseMode = modeRotation;
                            break;
                    }

                    // Store mouse location
                    lastLocation = [x, y];

                    e.preventDefault();
                } else if(action === 'up') {
                    mouseMode = modeNone;
                    e.preventDefault();
                } else if(action === 'move') {
                    if(mouseMode != modeNone) {
                        var loc = [x,y];

                        // Can NOT use switch as (modeRotation == modePan) is
                        // possible when Pan should take over rotation as
                        // rotation is not possible
                        if(mouseMode === modePan) {
                            handlePan(loc);
                        } else if (mouseMode === modeZoom) {
                            var deltaY = loc[1] - lastLocation[1];
                            handleZoom(deltaY * dzScale);

                            // Update mouse location
                            lastLocation = loc;
                        } else {
                           handleRotation(loc);
                        }

                        // Redraw the image in the canvas
                        redrawImage();
                    }
                }
            });

            // Zoom and pan events with mouse buttons and drag
            element.bind('mousedown', function(evt) {
                var current_button = evt.which;

                // alt+click simulates center button, shift+click simulates right
                if (evt.altKey) {
                    current_button = 2;
                    evt.altKey = false;
                } else if (evt.shiftKey) {
                    current_button = 3;
                    evt.shiftKey = false;
                }

                // Detect interaction mode
                switch(current_button) {
                    case 2: // middle mouse down = pan
                        mouseMode = modePan;
                        break;
                    case 3: // right mouse down = zoom
                        mouseMode = modeZoom;
                        break;
                    default:
                        mouseMode = modeRotation;
                        break;
                }

                // Store mouse location
                lastLocation = getRelativeLocation(canvas, evt);

                evt.preventDefault();
            });

            // Send mouse movement event to the forwarding function
            element.bind('mousemove', function(e) {
                if(mouseMode != modeNone) {
                    var loc = getRelativeLocation(canvas, e);

                    // Can NOT use switch as (modeRotation == modePan) is
                    // possible when Pan should take over rotation as
                    // rotation is not possible
                    if(mouseMode === modePan) {
                        handlePan(loc);
                    } else if (mouseMode === modeZoom) {
                        var deltaY = loc[1] - lastLocation[1];
                        handleZoom(deltaY * dzScale);

                        // Update mouse location
                        lastLocation = loc;
                    } else {
                       handleRotation(loc);
                    }

                    // Redraw the image in the canvas
                    redrawImage();
                }
            });

            // Stop any zoom or pan events
            element.bind('mouseup', function(evt) {
                mouseMode = modeNone;
                evt.preventDefault();
            });

            // Update rotation handler if possible
            modeRotation = container.data('info').arguments.hasOwnProperty('phi') ? modeRotation : modePan;
            if(modeRotation != modePan) {
                thetaValues = container.data('info').arguments.theta.values;
                phiValues   = container.data('info').arguments.phi.values;
                stepPhi     = phiValues[1] - phiValues[0];
                stepTheta   = thetaValues[1] - thetaValues[0];
                currentArgs = container.data('active-args');
            }
        }

        /*
         * If the data can rotate
         */
        function handleRotation(loc) {
            var currentPhi = currentArgs.phi,
            currentTheta = currentArgs.theta,
            currentPhiIdx = phiValues.indexOf(currentPhi),
            currentThetaIdx = thetaValues.indexOf(currentTheta)
            deltaPhi = (loc[0] - lastLocation[0]),
            deltaTheta = (loc[1] - lastLocation[1]),
            changeDetected = false;

            if(Math.abs(deltaPhi) > stepPhi) {
                changeDetected = true;
                currentPhiIdx += (deltaPhi > 0) ? 1 : -1;
                if(currentPhiIdx >= phiValues.length) {
                    currentPhiIdx -= phiValues.length;
                } else if(currentPhiIdx < 0) {
                    currentPhiIdx += phiValues.length;
                }
                currentArgs['phi'] = phiValues[currentPhiIdx];
            }

            if(Math.abs(deltaTheta) > stepTheta) {
                currentThetaIdx += (deltaTheta > 0) ? 1 : -1;
                if(currentThetaIdx >= thetaValues.length) {
                    currentThetaIdx = thetaValues.length - 1;
                } else if(currentThetaIdx < 0) {
                    currentThetaIdx = 0;
                }
                if(currentArgs['theta'] !== thetaValues[currentThetaIdx]) {
                    currentArgs['theta'] = thetaValues[currentThetaIdx];
                    changeDetected = true;
                }
            }

            if(changeDetected) {
                fireLoadImage(container);
                container.trigger('invalidate-viewport');

                // Update mouse location
                lastLocation = loc;
            }
        }

        /*
         * Does the actual image panning.  Panning should not mess with the
         * source width or source height, those are fixed by the current zoom
         * level.  Panning should only update the source origin (the x and y
         * coordinates of the upper left corner of the source rectangle).
         */
        function handlePan(loc) {
            // Update the source rectangle origin, but afterwards, check to
            // make sure we're not trying to look outside the image bounds.
            drawingCenter[0] += (loc[0] - lastLocation[0]);
            drawingCenter[1] += (loc[1] - lastLocation[1]);

            // Update mouse location
            lastLocation = loc;
        }

        /*
         * Does the actual image zooming.  Zooming first sets what the source width
         * and height should be based on the zoom level, then adjusts the source
         * origin to try and maintain the source center point.  However, zooming
         * must also not try to view outside the image bounds, so the center point
         * may be changed as a result of this.
         */
        function handleZoom(inOutAmount) {
            var beforeZoom = zoomLevel,
            afterZoom = beforeZoom + inOutAmount;

            // Disallow zoomLevel outside allowable range
            if (afterZoom < minZoom) {
                afterZoom = minZoom;
            } else if (afterZoom > maxZoom) {
                afterZoom = maxZoom;
            }

            if(beforeZoom != afterZoom) {
                zoomLevel = afterZoom;
                // FIXME ----------------------------------------------------------------
                // zoom by keeping location of "lastLocation" in the same screen position
                // FIXME ----------------------------------------------------------------
            }
        }

        /*
         * Convenience function to draw the image.  As a reminder, we always fill
         * the entire viewport.  Also, we always use the source origin and source
         * dimensions that we have calculated and maintain internally.
         */
        function redrawImage() {
            var ctx = canvas[0].getContext("2d"),
            w = container.width(),
            h = container.height(),
            iw = img[0].naturalWidth,
            ih = img[0].naturalHeight;

            if(iw === 0) {
                setTimeout(redrawImage, 100);
            } else {
                canvas.attr("width", w);
                canvas.attr("height", h);
                ctx.clearRect(0, 0, w, h);

                var tw = Math.floor(iw*zoomLevel),
                th = Math.floor(ih*zoomLevel),
                tx = drawingCenter[0] - (tw/2),
                ty = drawingCenter[1] - (th/2),
                dx = (tw > w) ? (tw - w) : (w - tw),
                dy = (th > h) ? (th - h) : (h - th),
                centerBounds = [ (w-dx)/2 , (h-dy)/2, (w+dx)/2, (h+dy)/2 ];

                if( drawingCenter[0] < centerBounds[0] || drawingCenter[0] > centerBounds[2]
                    || drawingCenter[1] < centerBounds[1] || drawingCenter[1] > centerBounds[3] ) {
                    drawingCenter[0] = Math.min( Math.max(drawingCenter[0], centerBounds[0]), centerBounds[2] );
                    drawingCenter[1] = Math.min( Math.max(drawingCenter[1], centerBounds[1]), centerBounds[3] );
                    tx = drawingCenter[0] - (tw/2);
                    ty = drawingCenter[1] - (th/2);
                }

                ctx.drawImage(img[0],
                              0,   0, iw, ih,  // Source image   [Location,Size]
                              tx, ty, tw, th); // Traget drawing [Location,Size]
            }
        }

        /*
         * Make sure the image will fit inside container as ZoomOut
         */

        function resetCamera() {
            var w = container.width(),
            h = container.height(),
            iw = img[0].naturalWidth,
            ih = img[0].naturalHeight;

            if(iw === 0) {
                setTimeout(resetCamera, 100);
            } else {
                zoomLevel = minZoom = Math.min( w / iw, h / ih );
                drawingCenter[0] = w/2;
                drawingCenter[1] = h/2;
                redrawImage();
            }
        }

        // Now do some initialization
        setupEvents();
        resetCamera();

        // Just expose a couple of methods that need to be called from outside
        return {
            'resetCamera': resetCamera,
            'imageLoaded': redrawImage
        };
    }

    // ------------------------------------------------------------------------

    function createImageViewer(container, func) {
        var imageContainer = $('<img/>', { class: 'image-viewer' }),
        imageCanvas = $('<canvas/>', { class: 'image-canvas' }),
        currentFileToRender = null;
        imageContainer.appendTo(imageCanvas);
        imageCanvas.appendTo(container);

        // Add zoom manager
        var manipMgr = createZoomableCanvasObject(container, imageContainer, imageCanvas, 10);

        container.bind('invalidate-size', function() {
            manipMgr.resetCamera();
        });
        imageContainer.bind('onload load', function(){
            manipMgr.imageLoaded();
            container.trigger('image-render');
        });
        container.bind('image-loaded', function(event){
            if(currentFileToRender === null || event.url.indexOf(currentFileToRender) != -1) {
                imageContainer.attr('src', event.url);
            }
        });
        container.bind('load-image', function(event){
            currentFileToRender = event.filename;
        });

        return imageCanvas;
    }

    // ========================================================================
    // JQuery
    // ========================================================================

    /**
     * jQuery catalyst view constructor.
     *
     * @member jQuery.vtkCatalystViewer
     * @param basePath
     * Root directory for data to visualize
     */

    $.fn.vtkCatalystViewer = function(dataBasePath, preload) {
        return this.each(function() {
            var me = $(this).empty().addClass('vtk-catalyst-viewer small'); //.unbind();

            // Get meta-data
            $.ajax({
                url: dataBasePath + '/info.json',
                dataType: 'json',
                success: function( data ) {
                    // Store metadata
                    me.data('info', data);
                    me.data('active-args', {});
                    me.data('base-path', dataBasePath);
                    me.data('preload', (preload ? true : false));

                    // Create download manager
                    createDownloadManager(me, 5, dataBasePath);

                    // Create Control UI
                    createControlPanel(me, data.arguments);

                    // Create interactive viewer
                    createImageViewer(me);

                    // Load default image
                    fireLoadImage(me);
                },
                error: function(error) {
                    console.log("error");
                    console.log(error);
                }
            });
        });
    }

    }(jQuery, window));
