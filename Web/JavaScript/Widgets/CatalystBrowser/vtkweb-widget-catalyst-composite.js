(function ($, GLOBAL) {
    var CHECKBOX_TEMPLATE = "<span><input type='checkbox' name='IDX' CHECKED/><img src='URL' alt='TOOLTIP' title='TOOLTIP' width='WIDTH'/></span>",
    CODE_MAP = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789/",
    indexMap = {};

    // Fill indexMap
    for(var i = 0; i < CODE_MAP.length; ++i) {
        indexMap[CODE_MAP[i]] = i + 1;
    }

    function repaint(container) {
        var layers = container.data('layer_toggle'),
        buffers = container.data('image_buffer'),
        bgCtx = container.data('bgCtx'),
        frontCtx = container.data('frontCtx'),
        bgCanvas = container.data('bgCanvas'),
        size = container.data('size'),
        count = layers.length,
        pixelOrder = container.data('pixel_order'),
        nbPixTotal = size[0] * size[1];

        // Draw BG
        bgCtx.clearRect(0, 0, size[0], size[1]);
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

        // Draw to front buffer with scaling
        // This will stretch origin image if not same ratio FIXME !!!
        frontCtx.drawImage(bgCanvas, 0, 0, size[0], size[1], 0, 0, container.width(), container.height());
    }

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
            html: buffer.join('')
        }).appendTo(container);
    }

    function initializeListeners(container) {
        var layersVisibility = container.data('layer_toggle');

        $('input', container).change(function(){
            var me = $(this);
            layersVisibility[Number(me.attr('name'))] = me.is(':checked');
            repaint(container);
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
            var me = $(this).unbind().empty().addClass('vtkweb-catalyst-composite');

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

                    // Store metadata
                    me.data('pixel_order', pixelOrdering);
                    me.data('path', dataBasePath);
                    me.data('layer_toggle', layerEnabled);
                    me.data('image_buffer', bufferStack);
                    me.data('bgCtx', bgCtx);
                    me.data('bgCanvas', bgCanvas[0]);
                    me.data('frontCtx', frontCtx);
                    me.data('size', [width, height]);

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
                        frontCtx.fillStyle="#70f3ff";
                        frontCtx.fillRect(0,0,me.width() * (1 - (imageLoadedCountDown/nbImages)), me.height());

                        if(!--imageLoadedCountDown) {
                            repaint(me);
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