
import gi
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk

import zzub

from neil import components
from neil.utils import ui

from .actions import on_popup_disconnect, on_popup_disconnect_all, \
                     on_popup_edit_cv_connector, on_popup_remove_cv_connector

from .submenus import machine_tree_submenu



# in the plugin router view, when right mouse button clicked on one of the connections
class ConnectionMenu(ui.EasyMenu):
    __neil__ = dict(
        id = 'neil.core.contextmenu.connection',
        singleton = False,
        categories = [],
    )


    # connections is a list of 3 item tuples (metaplugin, connection_index, connection_type)
    # the list is often one item long 
    def __init__(self, connections):
        ui.EasyMenu.__init__(self)

        if len(connections) == 1:
            self.build_connections_submenu(self, connections)
        else:
            for (target, source), connections in self.group_by_plugin(connections):
                submenu = self.build_connections_submenu(ui.MenuWrapper(), connections)
                self.add_submenu("Connections from %s to %s" % (source.get_name(), target.get_name()), submenu)

            self.add_item("Disconnect all", on_popup_disconnect_all, connections)


    def group_by_plugin(self, connections):
        """
        @param connections list of connection given to init
        @return a list where each item is a list of connections for one plugin
        """
        groups = {}
        for target, index, conntype in connections:
            source = target.get_input_connection_plugin(index)
            if (target, source) not in groups:
                groups[(target, source)] = []
            groups[(target, source)].append((target, index, conntype))

        # sort so audio connnections are before cv connections
        for pkey, conns in groups.items():
            groups[pkey] = sorted(conns, key=lambda x: x[2])

        return groups.items()
    

    def build_connections_submenu(self, menu: ui.EasyMenu, connections):
        """
        @param a menu to append menu items for when a connection is right clicked
        @connections a list of 3 item tuples (target_plugin_id, connection_id, subconnector_id)
        @return the menu with new items added
        """

        for target, conn_index, conntype in connections:
            if conntype == zzub.zzub_connection_type_audio:
                menu.add_submenu("Audio: insert effect", machine_tree_submenu(connection=(target, conn_index)))
                menu.add_item("Audio: disconnect", on_popup_disconnect, target, conn_index)
            elif conntype == zzub.zzub_connection_type_cv:
                edits = []
                cuts = []

                source = target.get_input_connection_plugin(conn_index)
                cv_connection = target.get_input_connection(conn_index).as_cv_connection()

                for subconn_index, connector in enumerate(cv_connection.get_connectors()):
                    desc = self.describe_cv_link(source, target, connector)
                    edits.append(ui.quick_menu_item("CV: edit %s" % desc, on_popup_edit_cv_connector, target, conn_index, subconn_index))
                    cuts.append(ui.quick_menu_item("CV: remove %s" % desc, on_popup_remove_cv_connector, target, conn_index, subconn_index))
                
                for edit_item in edits:
                    menu.append(edit_item)
                
                for cut_item in cuts:
                    menu.append(cut_item)

                if cv_connection.get_connector_count() > 1:
                    menu.add_item("CV: remove all", on_popup_disconnect, target, conn_index)

        if len(connections) > 1:
            menu.add_item("Disconnect this plugin", on_popup_disconnect_all, connections)

        return menu


    def describe_cv_link(self, from_plugin, to_plugin, connector):
        """
        build a label like: "cv audio/cv paramater data from 'port name' of 'plugin_name'"
        """
        node_from = self.describe_connector_node(from_plugin, connector.get_source())
        node_to = self.describe_connector_node(to_plugin, connector.get_target())

        return f"{node_from} -> {node_to}"


    def describe_connector_node(self, plugin:zzub.Plugin, node:zzub.CvNode):
        if node.port_type == zzub.zzub_port_type_audio:
            return "audio %s" % ( self.describe_cv_audio_node(node) )
        elif node.port_type == zzub.zzub_port_type_track:
            return "param %s" % (self.describe_cv_parameter_node(plugin, node, zzub.zzub_parameter_group_track) )
        elif node.port_type == zzub.zzub_port_type_parameter:
            return "param %s" % (self.describe_cv_parameter_node(plugin, node, zzub.zzub_parameter_group_global), )
        elif node.port_type == zzub.zzub_port_type_cv:
            return "port  %d" % node.value
        # elif node.type == zzub.zzub_cv_node_type_midi:
        #     return "midi  %d" % node.value
        else:
            return "unknown connector"


    def describe_cv_parameter_node(self, plugin, node, parameter_group):
        return plugin.get_pluginloader().get_parameter(parameter_group, node.value).get_name()
    

    def describe_cv_audio_node(self, node):
        if node.value == 1:
            return "left channel"
        elif node.value == 2:
            return "right channel"
        elif node.value == 3:
            return "stereo"
        else:
            return "channels %d" % node.value