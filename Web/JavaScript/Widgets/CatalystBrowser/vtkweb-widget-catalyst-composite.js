(function ($, GLOBAL) {
    var CHECKBOX_TEMPLATE = "<span><input type='checkbox' name='IDX' CHECKED/><img src='URL' alt='TOOLTIP' title='TOOLTIP' width='WIDTH'/></span>",
    CODE_MAP = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789/",
    indexMap = {};

    // Fill indexMap
    for(var i = 0; i < CODE_MAP.length; ++i) {
        indexMap[CODE_MAP[i]] = i + 1;
    }

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

    // ------------------------------------------------------------------------

    function createZoomableCanvasObject(container, bgCanvas, frontCanvas, pixelZoomRatio) {
        // First set up some variables we will need
        var modePan = 1,        // when dragging, it's a pan
        modeZoom = 2,           // when dragging, it's a zoom
        modeNone = 0,           // No mouse move handling
        mouseMode = modeNone,   // Current mode

        dzScale = 0.005,  // scaling factor to control how fast we zoom in and out
        wheelZoom = 0.05, // amount to change zoom with each wheel event

        drawingCenter = [0,0],  // Drawing parameters
        zoomLevel = 1.0,        //

        maxZoom = pixelZoomRatio, // limit how far we can zoom in
        minZoom = 1 / maxZoom,    // limit how far we can zoom out

        lastLocation = [0,0];  // Last place mouse event happened

        /*
         * Adds mouse event handlers so that we can pan and zoom the image
         */
        function setupEvents() {
            var element = frontCanvas;

            // Needed this to override context menu behavior
            element.bind('contextmenu', function(evt) { evt.preventDefault(); });

            // Wheel should zoom across browsers
            element.bind('DOMMouseScroll mousewheel', function (evt) {
                var x = (-evt.originalEvent.wheelDeltaY || evt.originalEvent.detail);

                lastLocation = getRelativeLocation(frontCanvas, evt);
                handleZoom((x > 0 ? wheelZoom : x < 0 ? -wheelZoom : 0));
                evt.preventDefault();

                // Redraw the image in the canvas
                redrawImage();
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
                        mouseMode = modePan;
                        break;
                }

                // Store mouse location
                lastLocation = getRelativeLocation(frontCanvas, evt);

                evt.preventDefault();
            });

            // Send mouse movement event to the forwarding function
            element.bind('mousemove', function(e) {
                if(mouseMode != modeNone) {
                    var loc = getRelativeLocation(frontCanvas, e);

                    if (mouseMode === modeZoom) {
                        var deltaY = loc[1] - lastLocation[1];
                        handleZoom(deltaY * dzScale);

                        // Update mouse location
                        lastLocation = loc;
                    } else {
                       handlePan(loc);
                    }

                    // Redraw the image in the frontCanvas
                    redrawImage();
                }
            });

            // Stop any zoom or pan events
            element.bind('mouseup', function(evt) {
                mouseMode = modeNone;
                evt.preventDefault();
            });
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
            var frontCtx = frontCanvas[0].getContext("2d"),
            w = container.width(),
            h = container.height(),
            iw = bgCanvas[0].width,
            ih = bgCanvas[0].height;

            if(iw === 0) {
                setTimeout(redrawImage, 100);
            } else {
                frontCanvas.attr("width", w);
                frontCanvas.attr("height", h);
                frontCtx.clearRect(0, 0, w, h);

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

                frontCtx.drawImage(bgCanvas[0],
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
            iw = bgCanvas[0].width,
            ih = bgCanvas[0].height;

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
            'paint': redrawImage
        };
    }

    // ------------------------------------------------------------------------

    function computeComposition(container) {
        var layers = container.data('layer_toggle'),
        buffers = container.data('image_buffer'),
        bgCtx = container.data('bgCtx'),
        frontCtx = container.data('frontCtx'),
        bgCanvas = container.data('bgCanvas'),
        frontCanvas = container.data('frontCanvas'),
        view = container.data('interactive-view'),
        size = container.data('size'),
        count = layers.length,
        pixelOrder = container.data('pixel_order'),
        nbPixTotal = size[0] * size[1],
        w = frontCanvas.width,
        h = frontCanvas.height,
        iw = size[0],
        ih = size[1],
        zoomLevel = Math.min( w / iw, h / ih),
        tw = Math.floor(iw*zoomLevel),
        th = Math.floor(ih*zoomLevel),
        tx = (container.width()/2) - (tw/2),
        ty = (container.height()/2) - (th/2);

        // Draw BG
        bgCtx.clearRect(0, 0, iw, ih);
        if(layers[0]) {
            bgCtx.putImageData(buffers[0], 0, 0);
        }
        // Update buffers to be transparent
        for(var i = 1; i < count; ++i) {
            var pix = buffers[i].data,
            pixCount = pix.length;
            for(var j = 3; j < pixCount; j += 4 ) {
                pix[j] = 0;
            }
        }
        // Update pixel that should be opaque
        var orderLayer = 0, found = false;
        for(var idx = 0; idx < nbPixTotal; ++idx) {
            if(pixelOrder[idx].length > 0) {
                found = false;
                for(orderLayer = 0; orderLayer < pixelOrder[idx].length && !found; ++orderLayer) {
                    found = layers[indexMap[pixelOrder[idx][orderLayer]]];
                }
                if(found) {
                    --orderLayer;
                    buffers[indexMap[pixelOrder[idx][orderLayer]]].data[idx*4+3] = 255;
                }
            }
        }
        // Draw pixels in BG
        var imgData = bgCtx.getImageData(0, 0, size[0], size[1]);
        srcPix = imgData.data;
        for(var idx = 0; idx < nbPixTotal; ++idx) {
            found = false;
            for(var layerIdx = 1; !found && layerIdx < count; ++layerIdx) {
                if(buffers[layerIdx].data[idx*4+3] === 255) {
                    found = true;
                    srcPix[idx*4] = buffers[layerIdx].data[idx*4];
                    srcPix[idx*4+1] = buffers[layerIdx].data[idx*4+1];
                    srcPix[idx*4+2] = buffers[layerIdx].data[idx*4+2];
                    srcPix[idx*4+3] = 255;
                }
            }
        }
        bgCtx.putImageData(imgData,0,0);

        view.paint();
    }

    // ------------------------------------------------------------------------

    function createControlPanel(container) {
        var layersVisibility = container.data('layer_toggle'),
        count = layersVisibility.length,
        path = container.data('path'),
        buffer = [];

        while(count--) {
            buffer.push(CHECKBOX_TEMPLATE.replace(/IDX/g, count).replace(/TOOLTIP/g, "Layer " + count).replace(/URL/g, path + '/' + count + '.jpg').replace(/WIDTH/g, "150"));
        }

        $('<div/>', {
            class: 'control',
            html: '<div class="header"><span class="vtk-icon-tools toggle"/><span class="vtk-icon-resize-full-2 reset"/><span class="vtk-icon-play play"/><span class="vtk-icon-stop stop"/></div><div class="parameters"><div class="toggle-container">TOGGLES</div><div class="extended-control">All <span class="vtk-icon-check-1 checkall action"/><span class="vtk-icon-cancel uncheckall action"/><div class="animation-type">Animation&nbsp;type:<select class="animation-type"><option value="-1">Incremental</option><option value="2">2 Layer at a time</option><option value="3">3 Layer at a time</option><option value="4">4 Layer at a time</option><option value="5">5 Layer at a time</option></select></div></div></div>'.replace('TOGGLES', buffer.join(''))
        }).appendTo(container);
    }

    // ------------------------------------------------------------------------

    function initializeListeners(container) {
        var layersVisibility = container.data('layer_toggle'),
        animationWorkIndex = 0,
        play = $('.play', container),
        stop = $('.stop', container),
        checkAll = $('.checkall', container),
        uncheckAll = $('.uncheckall', container),
        view = container.data('interactive-view'),
        keepAnimation = false;

        function animate() {
            var animeType = Number($('select.animation-type', container).val()),
            count = layersVisibility.length;
            while(count--) {
                if(animeType === -1) {
                    layersVisibility[count] = (animationWorkIndex === count) ? !layersVisibility[count] : layersVisibility[count];
                } else {
                    layersVisibility[count] = (count >= animationWorkIndex) && (count < (animationWorkIndex + animeType));
                }
                $('input[type="checkbox"]', container).eq(count).prop('checked', layersVisibility[count]);
            }
            animationWorkIndex = (animationWorkIndex + 1) % layersVisibility.length;
            container.trigger('composite-invalid');

            keepAnimation = keepAnimation && container.is(":visible");
            if(keepAnimation) {
                setTimeout(animate, 150);
            }
        }

        $('input[type="checkbox"]', container).change(function(){
            var me = $(this);
            layersVisibility[Number(me.attr('name'))] = me.is(':checked');
            container.trigger('composite-invalid');
        });

        $('.toggle', container).click(function(){
            container.toggleClass('small');
        });

        $('.reset', container).click(function(){
            view.resetCamera();
        });

        container.bind('composite-invalid', function(){
            computeComposition(container);
        });

        play.click(function(){
            animationWorkIndex = 0;
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
        checkAll.click(function(){
            $('input[type="checkbox"]', container).prop('checked', true);
            var count = layersVisibility.length;
            while(count--) {
                layersVisibility[count] = true;
            }
            container.trigger('composite-invalid');
        });
        uncheckAll.click(function(){
            $('input[type="checkbox"]', container).prop('checked', false);
            var count = layersVisibility.length;
            while(count--) {
                layersVisibility[count] = false;
            }
            container.trigger('composite-invalid');
        });
    }

    /**
     * jQuery catalyst view constructor.
     *
     * @member jQuery.vtkCatalystViewer
     * @param basePath
     * Root directory for data to visualize
     */

    $.fn.vtkCatalystCompositeViewer = function(dataBasePath) {
        return this.each(function() {
            var me = $(this).unbind().empty().addClass('vtkweb-catalyst-composite small');

            // Get meta-data
            $.ajax({
                url: dataBasePath + '/info.json',
                dataType: 'json',
                success: function( data ) {
                    // Extract info to local vars
                    var width = data.dimensions[0],
                    height = data.dimensions[1],
                    nbImages = data["composite-size"],
                    pixelOrdering = data["pixel-order"].split('+'),
                    imageStack = [],
                    bufferStack = [],
                    layerEnabled = [],
                    imageLoadedCountDown = nbImages;

                    // Create canvas
                    var bgCanvas = $('<canvas/>', {
                        style: "display:none;"
                    }).attr('width', width).attr('height', height);
                    bgCanvas.appendTo(me);
                    var bgCtx = bgCanvas[0].getContext("2d");

                    var frontCanvas = $('<canvas/>').attr('width', me.width()).attr('height', me.height());
                    frontCanvas.appendTo(me);
                    var frontCtx = frontCanvas[0].getContext("2d");

                    // Drawing area
                    var manipMgr = createZoomableCanvasObject(me, bgCanvas, frontCanvas, 10);

                    // Store metadata
                    me.data('pixel_order', pixelOrdering);
                    me.data('path', dataBasePath);
                    me.data('layer_toggle', layerEnabled);
                    me.data('image_buffer', bufferStack);
                    me.data('bgCtx', bgCtx);
                    me.data('bgCanvas', bgCanvas[0]);
                    me.data('frontCanvas', frontCanvas[0]);
                    me.data('frontCtx', frontCtx);
                    me.data('size', [width, height]);
                    me.data('interactive-view', manipMgr);

                    function onLoad() {
                        var buffer = bufferStack[Number($(this).attr('alt'))].data;

                        bgCtx.drawImage(this,0,0);
                        // Copy buffer
                        var srcPix = bgCtx.getImageData(0, 0, width, height).data;
                        for (var i = 0, n = srcPix.length; i < n; i+=4) {
                            buffer[i] = srcPix[i];
                            buffer[i+1] = srcPix[i+1];
                            buffer[i+2] = srcPix[i+2];
                            buffer[i+3] = srcPix[i+3];
                        }

                        frontCtx.clearRect(0, 0, me.width(), me.height());
                        frontCtx.fillStyle="#cccccc";
                        frontCtx.fillRect(0, 0, me.width() * (1 - (imageLoadedCountDown/nbImages)), me.height());

                        if(!--imageLoadedCountDown) {
                            computeComposition(me);
                        }
                    }

                    // Create and fill image buffer for each layer
                    for(var i=0; i < nbImages; ++i) {
                        var img = new Image();
                        img.alt = i;
                        img.src = dataBasePath + '/' + i + '.jpg';
                        img.onload = onLoad;
                        imageStack.push(img);
                        bufferStack.push(bgCtx.createImageData(width, height));
                        layerEnabled.push(true);
                    }

                    // Add control UI
                    createControlPanel(me);

                    // Attach interaction listeners
                    initializeListeners(me);
                },
                error: function(error) {
                    console.log("error when trying to download " + dataBasePath + '/info.json');
                    console.log(error);
                }
            });
        });
    }

}(jQuery, window));