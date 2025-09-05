
# Neil
# Modular Sequencer
# Copyright (C) 2006,2007,2008 The Neil Development Team
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

"""
Organizes finding Neils resources across the system.
Basically a wrapper around the path.cfg file in <install_dir>/share/neil or <local_settings_dir>
"""

from configparser import ConfigParser
import sys, os

from neil.utils.base import settingspath



if 'NEIL_BASE_PATH' in os.environ:
    BASE_PATH = os.environ['NEIL_BASE_PATH']
else:
    BASE_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), '../..'))



class PathConfig(ConfigParser):
    def __init__(self):
        ConfigParser.__init__(self)
        self.settings_dir = settingspath()

        try_paths = [
            os.path.join(settingspath(), 'path.cfg'), 
            os.environ['NEIL_PATH_CFG'] if 'NEIL_PATH_CFG' in os.environ else os.path.join(BASE_PATH, 'share/neil/path.cfg')
        ]

        path = self.get_cfg_path(try_paths)
        
        assert path, "Unable to find path.cfg"

        print("Using path config: " + path)

        self.read([path])

        site_packages = self.get_path('site_packages')
        if not site_packages in sys.path:
            print(site_packages + "  missing in sys.path, prepending")
            sys.exit(0)
            # sys.path = [site_packages] + sys.path


    def get_cfg_path(self, paths):
        for path in paths:
            path = os.path.expanduser(path)
            # if not os.path.isabs(path):
            #     path = os.path.normpath(os.path.join(BASE_PATH,path))
            # print("searching " + path)
            if os.path.isfile(path):
                return path


    def get_paths(self, pathid, append=''):
        paths = []
        default_path = self.get_path(pathid, append)
        if default_path:
            paths.append(default_path)
        paths.append(os.path.expanduser(os.path.join(self.settings_dir, pathid)))
        return paths
    

    def get_path(self, pathid, append=''):
        if not self.has_option('Paths', pathid):
            return ''
        value = os.path.expanduser(self.get('Paths', pathid))
        if not os.path.isabs(value):
            value = os.path.normpath(os.path.join(BASE_PATH, value))
        if append:
            value = os.path.join(value, append)
        return value


path_cfg = PathConfig()

