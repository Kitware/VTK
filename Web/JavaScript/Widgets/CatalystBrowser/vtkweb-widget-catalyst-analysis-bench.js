(function ($, GLOBAL) {
    var TOOLBAR_TEMPLATE = '<div class=sub-menu><ul class="menu left"><li class="vtk-icon-list-add sub action" data-type="composite-image-stack"><ul></ul></li><li class="vtk-icon-chart-line sub action" data-type="catalyst-resample-viewer"><ul></ul></li><li class="vtk-icon-loop-alt sub action" data-type="catalyst-viewer"><ul></ul></li><li class="vtk-icon-laptop sub action" data-type="catalyst-pvweb"><ul></ul></li></ul><ul class="menu right"><li class="layout-size" data-layout-size="2"><span class="layout-value">50 %</span><span class="vtk-icon-zoom-in zoom-action" data-delta=-1/><span class="vtk-icon-zoom-out zoom-action" data-delta=1/></li><li class="vtk-icon-magic action"/><li class="vtk-icon-trash"/></ul></div><div class="bench-viewers"></div>',
    ENTRY_TEMPLATE = '<li class="create-viewer" data-path="PATH" data-title="TITLE">TITLE<i class=help>DESCRIPTION</i></li>',
    VIEWER_FACTORY = {
        "catalyst-viewer": {
            class: "vtk-icon-loop-alt",
            factory: function(domToFill, path) {
                domToFill.vtkCatalystViewer(path, false);
            }
        },
        "catalyst-resample-viewer" : {
            class: "vtk-icon-chart-line",
            factory: function(domToFill, path) {
                domToFill.vtkCatalystResamplerViewer(path);
            }
        },
        "composite-image-stack" : {
            class: "vtk-icon-list-add",
            factory: function(domToFill, path) {
                domToFill.vtkCatalystCompositeViewer(path);
            }
        },
        "catalyst-pvweb" : {
            class: "vtk-icon-laptop",
            factory: function(domToFill, path) {
                domToFill.vtkCatalystPVWeb(path);
            }
        }
    };

    // ------------------------------------------------------------------------

    function initializeListeners(container) {
        $('.create-viewer', container).addClass('action').click(function(){
            var me = $(this),
            path = me.attr('data-path'),
            type = me.parent().parent().attr('data-type'),
            title = me.attr('data-title'),
            workspace = $('.bench-viewers', container),
            layoutSize = Number($('.layout-size', container).attr('data-layout-size')),
            size = $(window).width() / layoutSize - 15,
            viewer = $('<div/>', { class: 'viewer', 'data-type': type, html: "<div class='title-bar'><span class='title'>"+title+"</span><span class='right action close vtk-icon-cancel'/></div><div class='content'></div>"});

            $('li.sub', container).removeClass('active');

            // Attach close action to viewer
            $('.close', viewer).bind('click', function(){
                viewer.remove();
            });

            // Add viewer
            viewer.css('width', (size-2) + 'px').css('height', (20 + size - 2) + 'px').appendTo(workspace);

            // Provide content
            VIEWER_FACTORY[type].factory($('.content', viewer), path);
        });

        $('.vtk-icon-trash', container).addClass('action').click(function(){
            $('.close', container).trigger('click');
        });

        $('.zoom-action', container).addClass('action').click(function(){
            var me = $(this),
            size = Number(me.parent().attr('data-layout-size')),
            delta = Number(me.attr('data-delta'));
            size += delta;
            if(size < 1) {
                size = 1;
            } else if(size > 5) {
                size = 5;
            }
            me.parent().attr('data-layout-size', size);
            $('.layout-value', me.parent).html( Math.floor(100/size) + ' %');
        });
        $('.vtk-icon-magic').hide();
        $('.sub-menu li.sub', container).click(function(){
            var me = $(this), alreadyActive = me.hasClass('active');
            $('li.sub', me.parent()).removeClass('active');
            if(!alreadyActive) {
                me.addClass('active');
            }
        });
    }

    /**
     * jQuery catalyst view constructor.
     *
     * @member jQuery.vtkCatalystViewer
     * @param project
     * @param basePath
     * Root directory for data to visualize
     */

    $.fn.vtkCatalystAnalysisBench = function(project, dataBasePath) {
        return this.each(function() {
            var me = $(this).unbind().empty().html(TOOLBAR_TEMPLATE),
            menu = $('.menu.left', me),
            buffer = [],
            analysis = project.analysis,
            count = analysis.length,
            containers = {
                "composite-image-stack" : $('.menu.left > li[data-type="composite-image-stack"] > ul', me),
                "catalyst-resample-viewer" : $('.menu.left > li[data-type="catalyst-resample-viewer"] > ul', me),
                "catalyst-viewer" : $('.menu.left > li[data-type="catalyst-viewer"] > ul', me),
                "catalyst-pvweb" : $('.menu.left > li[data-type="catalyst-pvweb"] > ul', me)
            },
            buffers = { "composite-image-stack" : [], "catalyst-resample-viewer" : [], "catalyst-viewer" : [], "catalyst-pvweb" : [] };

            // Fill buffers
            while(count--) {
                var item = analysis[count];
                buffers[item.type].push(ENTRY_TEMPLATE.replace(/PATH/g, dataBasePath + '/' + item.id).replace(/TITLE/g, item.title).replace(/DESCRIPTION/g, item.description));
            }

            // Update UI
            for(var key in containers) {
                containers[key].html(buffers[key].join(''));
            }

            // Handle listeners
            initializeListeners(me);
        });
    }

}(jQuery, window));