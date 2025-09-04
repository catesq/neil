
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


from .managers import *


# the main class is the ComponenManager
# service locator pattern - a system wide singleton which is created in neil/__init__.py
# it is used to register and get all neil classes in the components directories

if __name__ == '__main__':
    class MyClass:
        __neil__ = dict(
            id = 'neil.hub.myclass',
            categories = [
                'uselessclass',
                'uselessclass2',
            ]
        )

        def __repr__(self):
            return '<%s>' % repr(self.x)

        def __init__(self, y=0):
            import random
            self.x = y or random.random()


    class MyClass2(MyClass):
        __neil__ = dict(
            id = 'neil.hub.myclass.singleton',
            singleton = True,
            categories = [
                'uselessclass',
                'uselessclass2',
            ]
        )


    class CancelException(Exception):
        __neil__ = dict(
            id = 'neil.exception.cancel',
            exception = True,
            categories = [
            ]
        )


    pkginfo = dict(
        classes = [
            MyClass,
            MyClass2,
            CancelException,
        ],
    )

    from neil import components
    
    components.register(pkginfo)
    
    print(components.get('neil.hub.myclass').x)
    print(components.get('neil.hub.myclass').x)
    print(components.get('neil.hub.myclass').x)
    print(components.get('neil.hub.myclass.singleton').x)
    print(components.get('neil.hub.myclass.singleton').x)
    print(components.get('neil.hub.myclass.singleton').x)
    print(components.get_from_category('uselessclass'))
    try:
        components.throw('neil.exception.cancel', "argh.")
    except components.exception('neil.exception.cancel') as test:
        print("passed.", test)
