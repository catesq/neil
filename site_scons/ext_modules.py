#

import os, typing



    # used by build_cmake_module() to check if the library is has been built 
def check_lib_exists(lib_name, lib_path):
    suffixes = ['.a', '.lib']
    return any([os.path.exists(os.path.join(lib_path, 'lib' + lib_name + suffix)) for suffix in suffixes])



# base class
class cpp_module:
    def get_src_path(self):
        return '${SRC_PATH}/libneil/src/' + self.name

    def get_src_path_key(self):
        return self.name.upper() + '_SRC_PATH'

    def build(self, env):
        pass

    def get_env_flags(self) -> {}:
        return {}

    def get_cpp_defines(self):
        return {}




class cmake_module(cpp_module):
    def __init__(self, name, lib_name = None, src_subdir = False, always_link = False, lib_path = False, cpp_defines = {}, cmake_opts = '', cmakelists_dir = '../..'):
        #, cmakelists_subdir = ''):
        """
        name: matches a subdirectory ogf libneil - and as used in scons build
        lib_name: used internally - only needed when it doesn't match 'name'
        src_subdir: gist library has an unexpected subdirectory 
        """
        self.name = name
        self.lib_name = lib_name if lib_name else name
        self.src_subdir = src_subdir         #
        self.always_link = always_link       #
        self.cpp_defines = cpp_defines       #
        self.extra_opts = cmake_opts         #
        self.lib_path = lib_path             # the location of the library relative to '<proj_dir>/build/release' - usually blank
        self.cmakelists_dir = cmakelists_dir # the location of CMakeLists.txt relative to '<proj_dir>/build/release' - usually '../..'

    def get_env_flags(self) -> typing.Dict[str, str]:
        return {
            self.name.upper() + '_LIB_NAME': self.lib_name,
        }
    
    def get_src_path(self):
        src_path = super().get_src_path()

        if self.src_subdir:
            return os.path.join(src_path, self.src_subdir)
        else:
            return src_path 

    def build(self, env):
        module_path = os.path.join(env['SRC_PATH'], 'libneil', 'src', self.name)

        if not os.path.exists(module_path):
            os.makedirs(module_path)

        cmake_build_type = 'Debug' if env['DEBUG'] else 'Release'
        cmake_build_path = os.path.join(module_path, 'build', cmake_build_type.lower())

        if self.lib_path:
            cmake_lib_path = os.path.join(cmake_build_path, self.lib_path)
        else:
            cmake_lib_path = os.path.join(cmake_build_path, self.src_subdir) if self.src_subdir else cmake_build_path

        cmake_lib_path = os.path.abspath(cmake_lib_path )

        if not os.path.exists(cmake_build_path):
            os.makedirs(cmake_build_path)

        if check_lib_exists(self.lib_name, cmake_lib_path):
            return
        
        cwd = os.getcwd()
        os.chdir(cmake_build_path)
        
        cmake_opts = self.extra_opts + ' -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=off -DCMAKE_BUILD_TYPE=' + cmake_build_type
        env.Execute('cmake ' + cmake_opts  + ' ' + self.cmakelists_dir)
        env.Execute('cmake --build .')
        os.chdir(cwd)


        if check_lib_exists(self.lib_name, cmake_lib_path):
            if self.always_link:
                env.Append(LINKFLAGS=['-L' + cmake_build_path], LIBS=[self.lib_name])
            else:
                env.Append(LINKFLAGS=['-L' + cmake_build_path])
        else:
            raise Exception("%s library not built" % self.lib_name)

    def get_cpp_defines(self):
        return self.cpp_defines





class scons_module(cpp_module):
    def __init__(self, name, env_flags = {}):
        self.name = name
        self.env_flags = env_flags

    def get_env_flags(self) -> {}:
        return self.env_flags

    def build(self, env):
        src_path = '${%s}/SConscript' % self.get_src_path_key()
        module_build_path = '${BUILD_PATH}/libneil/src/' + self.name
        
        env.SConscript(
            src_path, 
            variant_dir=module_build_path, 
            duplicate=0
        )





class cpp_path(cpp_module):
    def __init__(self, name, path):
        self.name = name
        self.path = path
    
    def get_src_path(self):
        return self.path

    def get_src_path_key(self):
        return self.name.upper() + '_SRC_PATH'




