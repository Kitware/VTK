/**
 * VTK-Web Widget Library.
 *
 * This module extend jQuery object to add support for graphical components
 * related to open tree structure.
 *
 * @class jQuery.vtk.ui.Tree
 */
(function ($) {

    // =======================================================================
    // ==== Defaults constant values =========================================
    // =======================================================================
    var directives = {
            'li' : {
                'child <- children': {
                    '@node_id' : 'child.id',
                    '@type' : 'child.type',
                    '@class+' : function(arg) {
                        hasChild = arg.child.item.children ? ' Open' : '';
                        lastChild = (arg.pos == arg.child.items.length - 1) ? ' lastChild' : '';
                        return lastChild + hasChild;
                    },
                    '.label' : 'child.name',
                    '.tail' : function(arg) {
                        if(!arg.child.item.hasOwnProperty('fields')) {
                            return "";
                        }
                        var fields = arg.child.item.fields;
                        var result = [];
                        for(var key in fields) {
                            result.push(fieldHandler(key, fields[key]));
                        }
                        return result.join('');
                    },
                    'div.children' : function(ctxt) {
                        if(ctxt.child.item.hasOwnProperty('children')) {
                            return treeGenerator(ctxt.child.item);
                        }
                        return '';
                    }
                }
            }
    },
    treeGenerator = null,
    fieldHandler = function(key, value) {
        var buffer = [ '<div class="action" type="', key, '" '];
        if (typeof value === "object") {
            for(var innerKey in value) {
                buffer.push(innerKey);
                buffer.push('="')
                buffer.push(value[innerKey]);
                buffer.push('" ');
            }

        } else if (typeof value === "string") {
            buffer.push('data="')
            buffer.push(value);
            buffer.push('"');
        }
        buffer.push('></div>')
        return buffer.join('');
    };

    $.fn.vtkTree = function(options) {
        // Handle data with default values
        var opts = $.extend({},$.fn.vtkTree.defaults, options);

        // Compile template only once
        if(treeGenerator === null) {
            template = $(opts.template);
            treeGenerator = template.compile(directives);
        }

        return this.each(function() {
            var me = $(this).empty().addClass('vtk-tree'),
            container = $('<div/>'),
            data = { children: [opts.data] };
            me.append(container);
            me.data('tree', data);

            // Generate HTML
            container.render(data, treeGenerator);

            // Initialize pipelineBrowser (Visibility + listeners)
            initializeListener(me);
        });
    };

    $.fn.vtkTree.defaults = {
        template: "#vtk-templates > .vtkweb-widget-tree > ul",
        data: {}
    };

    // =======================================================================

    function initializeListener(container, activePath) {
        $('.action', container).click(function(e) {
            var me = $(this),
            node = me.closest('li'),
            id = node.attr('node_id'),
            type = me.attr('type');

            $('.node-line', container).removeClass('selected');
            $('.node-line:eq(0)', node).addClass('selected');

            container.trigger({
                'type': type,
                'node': id,
                'origin': me
            });
        });
        $('.node-line', container).click(function() {
            var me = $(this),
            node = me.closest('li'),
            id = node.attr('node_id');

            $('.node-line', container).removeClass('selected');
            $('.node-line:eq(0)', node).addClass('selected');

            container.trigger({
                'type': 'select',
                'node': id,
                'origin': me
            });
        });
    }

}(jQuery));