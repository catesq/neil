#

import os, typing



    # used by build_cmake_module() to check if the library is has been built 
def check_lib_exists(lib_name, lib_path, is_shared):
    suffixes = ['.so'] if is_shared else ['.a', '.lib']
    return any([os.path.exists(os.path.join(lib_path, 'lib' + lib_name + suffix)) for suffix in suffixes])



# base class
class cpp_module:
    def get_src_path(self):
        return '${SRC_PATH}/libneil/src/' + self.name

    def get_src_path_key(self):
        return self.name.upper() + '_SRC_PATH'

    def build(self, env):
        pass

    def install(self, env):
        pass

    def get_env_flags(self) -> {}:
        return {}

    def get_cpp_defines(self):
        return {}




class cmake_module(cpp_module):
    def __init__(
                self, 
                name, 
                lib_name = None, 
                src_subdir = False, 
                always_link = False, 
                lib_path = False, 
                cpp_defines = {}, 
                cmake_opts = '', 
                cmake_env='',
                cmakelists_dir = '../..', 
                shared_libs=False
            ):
        #, cmakelists_subdir = ''):
        """
        name: should be matches a subdirectory of libneil - and as used in scons build
        lib_name: assigned to a scons env_flag - <name>_LIB_NAME=<lib_name> - when the library to link doesn't match name eg loguru builds logurud when building a debug lib
        src_subdir: gist library has an unexpected subdirectory, usually blank
        always_link: loguru and faust are always linked, others are optional
        cpp_defines: Sconscript uses env.Append(CPPDEFINES=self.cppdefines)
        cmake_opts: exrta opts for initial cmake call  
        lib_path: he location of the library relative to '<proj_dir>/build/release' - usually blank - faust is weird
        shared_libs: default to static libs to build a one big libneil which is easy to install, a static faust lib needed static llvm libs which was overkill
        """
        self.name = name
        self.lib_name = lib_name.removeprefix("lib") if lib_name else name
        self.src_subdir = src_subdir         #  gist library has an 'src' subdirectory, faust has many includes in 'architecture'
        self.always_link = always_link       #
        self.cpp_defines = cpp_defines       #
        self.extra_opts = cmake_opts         #
        self.cmake_env_vars = cmake_env      # has to use 'LLVM_CONFIG=/path/llvm-config cmake ...' instead of 'cmake -DLLVM_CONFIG=/path/llvm-config' 
                                             # because the faust build didn't pass LLVM_CONFIG to it's external Make.llvm command
        self.rel_lib_path = lib_path         # the location of the library relative to '<proj_dir>/build/release' - usually blank
        self.cmakelists_dir = cmakelists_dir # the location of CMakeLists.txt relative to '<proj_dir>/build/release' - usually '../..'
        self.shared_libs=shared_libs         # 


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


    def prepare(self, env):
        pass


    def build(self, env):
        module_path = os.path.join(env['SRC_PATH'], 'libneil', 'src', self.name)

        if not os.path.exists(module_path):
            os.makedirs(module_path)

        cmake_build_type = 'Debug' if env['DEBUG'] else 'Release'
        cmake_build_path = os.path.join(module_path, 'build', cmake_build_type.lower())

        if self.rel_lib_path:
            cmake_lib_path = os.path.join(cmake_build_path, self.rel_lib_path)
        else:
            cmake_lib_path = os.path.join(cmake_build_path, self.src_subdir) if self.src_subdir else cmake_build_path

        cmake_lib_path = os.path.abspath(cmake_lib_path )

        if not os.path.exists(cmake_build_path):
            os.makedirs(cmake_build_path)

        if self.lib_exists(cmake_lib_path):
            self.link_env(cmake_lib_path, env)
            return 
        
        cwd = os.getcwd()
        os.chdir(cmake_build_path)
        
        shared_libs_opt = " -DBUILD_SHARED_LIBS={} ".format("on" if self.shared_libs else "off")

        cmake_opts = self.extra_opts + shared_libs_opt + ' -DCMAKE_POLICY_DEFAULT_CMP0177=OLD -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=' + cmake_build_type
        env.Execute(self.cmake_env_vars + ' cmake ' + cmake_opts  + ' ' + self.cmakelists_dir)
        env.Execute(self.cmake_env_vars + ' cmake --build .')
        os.chdir(cwd)

        if not self.lib_exists(cmake_lib_path):
            raise Exception("%s library not built" % self.lib_name)

        self.link_env(cmake_lib_path, env)


    def lib_exists(self, lib_path):
        return check_lib_exists(self.lib_name, lib_path, self.shared_libs)


    def link_env(self, lib_path, env):
        if self.always_link:
            env.Append(LINKFLAGS=['-L' + lib_path], LIBS=[self.lib_name])
        else:
            env.Append(LINKFLAGS=['-L' + lib_path])


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




