3# Neil
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
    value = value.lower()
    if value in ('true','enabled','on','yes','1'):
        return True
    elif value in ('false','disabled','off','no','0'):
        return False
    return bool(value)


# test if build dir exists

root_build_dir = os.path.join(os.getcwd(), 'build')
if not os.path.exists(root_build_dir):
    os.mkdir(root_build_dir)

opts_file = os.path.join(root_build_dir, 'options.conf')

opts = Variables(opts_file, ARGUMENTS)
opts.Add("PREFIX", 'Set the install "prefix" ( /path/to/PREFIX )', "/usr/local")
opts.Add("DESTDIR", 'Set the root directory to install into ( /path/to/DESTDIR )', "")
# opts.Add("ETCDIR", 'Set the configuration dir "prefix" ( /path/to/ETC )', "/etc")
opts.Add("DEBUG", "Compile everything in debug mode if true", False, None, bool_converter)

if posix:
    opts.Add("COMPILER", "Either clang or gcc", "gcc")

env = Environment(ENV = os.environ, options=opts)

# if posix and env['COMPILER'] == 'clang':
#     env["CC"] = (os.getenv("CC") or env["CC"])
#     env["CXX"] = (os.getenv("CXX") or env["CXX"])
#     env["ENV"].update(x for x in os.environ.items() if x[0].startswith("CCC_"))

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

is_debug = 'DEBUG' in env and env['DEBUG']

env['SRC_PATH'] = os.getcwd()
env['BUILD_PATH'] = os.path.join(root_build_dir, 'debug' if is_debug else 'release')

env['SITE_PACKAGE_PATH'] = '${DESTDIR}${PREFIX}/lib/python' 
env['APPLICATIONS_PATH'] = '${DESTDIR}${PREFIX}/share/applications'
env['BIN_PATH'] = '${DESTDIR}${PREFIX}/bin'
env['SHARE_PATH'] = '${DESTDIR}${PREFIX}/share/neil'
env['DOC_PATH'] = '${DESTDIR}${PREFIX}/share/doc/neil'
# env['ETC_PATH'] = '${DESTDIR}${ETCDIR}/neil'
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
    # etc = 'ETC_PATH',
    settings = 'SETTINGS_PATH',
)

######################################
# save config
######################################
print("save ", env['BUILD_PATH'], "to:", opts_file)
opts.Save(opts_file, env)
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

main_env = env.Clone()

Export(
    'main_env',
    'install',
    'install_recursive',
    'get_settings_dir',
    'pip',
    'win32', 'mac', 'posix', 
    'tools_converter', 'bool_converter'
)

env.SConscript('applications/SConscript', variant_dir = os.path.join(env['BUILD_PATH'], 'applications'), duplicate = 0)
env.SConscript('bin/SConscript', variant_dir = os.path.join(env['BUILD_PATH'], 'bin'), duplicate = 0)
env.SConscript('doc/SConscript')
env.SConscript('demosongs/SConscript', variant_dir = os.path.join(env['BUILD_PATH'], 'demosongs'), duplicate = 0)
env.SConscript('etc/SConscript', variant_dir = os.path.join(env['BUILD_PATH'], 'etc'), duplicate = 0)
env.SConscript('icons/SConscript', variant_dir = os.path.join(env['BUILD_PATH'], 'icons'), duplicate = 0)
env.SConscript('pixmaps/SConscript', variant_dir = os.path.join(env['BUILD_PATH'], 'pixmaps'), duplicate = 0)
env.SConscript('presets/SConscript', variant_dir = os.path.join(env['BUILD_PATH'], 'presets'), duplicate = 0)
env.SConscript('themes/SConscript', variant_dir = os.path.join(env['BUILD_PATH'], 'themes'), duplicate = 0)
env.SConscript('src/SConscript', variant_dir = os.path.join(env['BUILD_PATH'], 'src'), duplicate = 0)


# libneil
env.SConscript('libneil/SConscript', variant_dir = os.path.join(env['BUILD_PATH'], 'libneil'), duplicate = 1)
