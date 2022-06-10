# Neil
# Modular Sequencer
# Copyright (C) 2006 The Neil Development Team
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

VERSION = "0.13"

import os, glob, sys, time
import distutils.sysconfig

posix = os.name == 'posix'
win32 = os.name == 'nt'
mac = os.name == 'mac'



######################################
#
# init environment and define options
#
######################################

def tools_converter(value):
    return value.split(',')

def bool_converter(value):
    if value == 'True':
        return True
    elif value == 'False':
        return False
    return bool(value)

opts = Variables('options.conf', ARGUMENTS )
opts.Add("PREFIX", 'Set the install "prefix" ( /path/to/PREFIX )', "/usr/local")
opts.Add("DESTDIR", 'Set the root directory to install into ( /path/to/DESTDIR )', "")
opts.Add("ETCDIR", 'Set the configuration dir "prefix" ( /path/to/ETC )', "/etc")

if posix:
    opts.Add("COMPILER", "Either clang or gcc", "clang")

env = Environment(ENV = os.environ, options=opts)

if posix and env['COMPILER'] == 'clang':
    env["CC"] = (os.getenv("CC") or env["CC"])
    env["CXX"] = (os.getenv("CXX") or env["CXX"])
    env["ENV"].update(x for x in os.environ.items() if x[0].startswith("CCC_"))

env.SConsignFile()

######################################
# build settings
######################################

def get_settings_dir():
    if win32:
        return os.path.expanduser('~/neil')
    elif posix:
        return os.path.expanduser('~/.neil')
    else:
        return None

distutils_prefix = "%s%s" % (env['DESTDIR'], env['PREFIX'])

env['ROOTPATH'] = os.getcwd()
env['SITE_PACKAGE_PATH'] = \
    distutils.sysconfig.get_python_lib(prefix=distutils_prefix)
env['APPLICATIONS_PATH'] = '${DESTDIR}${PREFIX}/share/applications'
env['BIN_PATH'] = '${DESTDIR}${PREFIX}/bin'
env['SHARE_PATH'] = '${DESTDIR}${PREFIX}/share/neil'
env['DOC_PATH'] = '${DESTDIR}${PREFIX}/share/doc/neil'
env['ETC_PATH'] = '${DESTDIR}${ETCDIR}/neil'
env['ICONS_NEIL_PATH'] = '${DESTDIR}${PREFIX}/share/icons/neil'
env['ICONS_HICOLOR_PATH'] = '${DESTDIR}${PREFIX}/share/icons/hicolor'
env['PIXMAPS_PATH'] = '${DESTDIR}${PREFIX}/share/pixmaps/neil'
env['SETTINGS_PATH'] = get_settings_dir()

CONFIG_PATHS = dict(
    site_packages = 'SITE_PACKAGE_PATH',
    applications = 'APPLICATIONS_PATH',
    bin = 'BIN_PATH',
    share = 'SHARE_PATH',
    doc = 'DOC_PATH',
    icons_neil = 'ICONS_NEIL_PATH',
    icons_hicolor = 'ICONS_HICOLOR_PATH',
    pixmaps = 'PIXMAPS_PATH',
    etc = 'ETC_PATH',
    settings = 'SETTINGS_PATH'
)

######################################
# save config
######################################

opts.Save('options.conf', env)
Help( opts.GenerateHelpText( env ) )

######################################
# install paths
######################################

import SCons
from SCons.Script.SConscript import SConsEnvironment

SConsEnvironment.Chmod = SCons.Action.ActionFactory(os.chmod, lambda dest, mode: 'os.chmod(dest, mode)')

def InstallPerm(env, dir, source, perm):
    obj = env.Install(dir, source)
    for i in obj:
        print(i)
        env.AddPostAction(i, env.Chmod(str(i), perm))
    return dir

SConsEnvironment.InstallPerm = InstallPerm

def install(target, source, perm=None):
    if not perm:
        env.Install(dir=env.Dir(target), source=source)
    else:
        env.InstallPerm(dir=env.Dir(target), source=source, perm=perm)

env.Alias(target='install', source="${DESTDIR}${PREFIX}")
env.Alias(target='install', source="${DESTDIR}${ETCDIR}")
env.Alias(target='install', source=get_settings_dir())

def install_recursive(target, path, mask):
    for f in glob.glob(os.path.join(path, mask)):
        install(target, f)
    for filename in os.listdir(path):
        fullpath = os.path.join(path, filename)
        if os.path.isdir(fullpath):
            install_recursive(os.path.join(target, filename), fullpath, mask)

def build_path_config(target, source, env):
    import os, sys
    
    from io import StringIO
    from configparser import ConfigParser

    outpath = str(target[0])
    import os
    s = StringIO()
    cfg = ConfigParser()
    cfg.add_section('Paths')
    remove_prefix = '${DESTDIR}'
    for key, value in CONFIG_PATHS.items():
        value = env[value]
        if value.startswith(remove_prefix) == '':
            value = value[len(remove_prefix):]
        cfg.set('Paths', key, os.path.abspath(str(env.Dir(value))))
    cfg.write(s)
    open(outpath, 'w').write(s.getvalue())


def pip(pkg):
    os.system('pip show %s >> /dev/null && if [ $? -ne 0 ]; then echo "Installing python rdflib with pip..."; pip install -qq --target %s %s; fi' % (pkg, env['SITE_PACKAGE_PATH'], pkg))

builders = dict(
    BuildPathConfig = Builder(action=build_path_config),
)

env['BUILDERS'].update(builders)

Export(
    'env',
    'install',
    'install_recursive',
    'get_settings_dir',
    'pip',
    'win32', 'mac', 'posix', 
)

env.SConscript('applications/SConscript')
env.SConscript('bin/SConscript')
env.SConscript('doc/SConscript')
env.SConscript('demosongs/SConscript')
env.SConscript('etc/SConscript')
env.SConscript('icons/SConscript')
env.SConscript('pixmaps/SConscript')
env.SConscript('presets/SConscript')
env.SConscript('themes/SConscript')
env.SConscript('src/SConscript')

# libneil
env.SConscript('libneil/SConscript')
