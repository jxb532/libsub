#
#    Copyright (C) 2012-2018 Carl Hetherington <cth@carlh.net>
#
#    This file is part of libsub.
#
#    libsub is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    libsub is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with libsub.  If not, see <http://www.gnu.org/licenses/>.

import subprocess
import os
import shlex
from waflib import Context

APPNAME = 'libsub'

this_version = subprocess.Popen(shlex.split('git tag -l --points-at HEAD'), stdout=subprocess.PIPE).communicate()[0]
last_version = subprocess.Popen(shlex.split('git describe --tags --abbrev=0'), stdout=subprocess.PIPE).communicate()[0]

if this_version == '':
    VERSION = '%sdevel' % last_version[1:].strip()
else:
    VERSION = this_version[1:].strip()

API_VERSION = '-1.0'

try:
    from subprocess import STDOUT, check_output, CalledProcessError
except ImportError:
    # python 2.6 (in Centos 6) doesn't include check_output
    # monkey patch it in!
    import subprocess
    STDOUT = subprocess.STDOUT

    def check_output(*popenargs, **kwargs):
        if 'stdout' in kwargs:  # pragma: no cover
            raise ValueError('stdout argument not allowed, '
                             'it will be overridden.')
        process = subprocess.Popen(stdout=subprocess.PIPE,
                                   *popenargs, **kwargs)
        output, _ = process.communicate()
        retcode = process.poll()
        if retcode:
            cmd = kwargs.get("args")
            if cmd is None:
                cmd = popenargs[0]
            raise subprocess.CalledProcessError(retcode, cmd,
                                                output=output)
        return output
    subprocess.check_output = check_output

    # overwrite CalledProcessError due to `output`
    # keyword not being available (in 2.6)
    class CalledProcessError(Exception):

        def __init__(self, returncode, cmd, output=None):
            self.returncode = returncode
            self.cmd = cmd
            self.output = output

        def __str__(self):
            return "Command '%s' returned non-zero exit status %d" % (
                self.cmd, self.returncode)
    subprocess.CalledProcessError = CalledProcessError

def options(opt):
    opt.load('compiler_cxx')
    opt.add_option('--enable-debug', action='store_true', default=False, help='build with debugging information and without optimisation')
    opt.add_option('--static', action='store_true', default=False, help='build libsub statically and link statically to cxml and dcp')
    opt.add_option('--target-windows', action='store_true', default=False, help='set up to do a cross-compile to make a Windows package')
    opt.add_option('--disable-tests', action='store_true', default=False, help='disable building of tests')
    opt.add_option('--force-cpp11', action='store_true', default=False, help='force use of C++11')

def configure(conf):
    conf.load('compiler_cxx')
    conf.env.append_value('CXXFLAGS', ['-Wall', '-Wextra', '-D_FILE_OFFSET_BITS=64', '-D__STDC_FORMAT_MACROS'])
    if conf.options.force_cpp11:
        conf.env.append_value('CXXFLAGS', ['-std=c++11', '-DBOOST_NO_CXX11_SCOPED_ENUMS'])
    conf.env.append_value('CXXFLAGS', ['-DLIBSUB_VERSION="%s"' % VERSION])

    conf.env.ENABLE_DEBUG = conf.options.enable_debug
    conf.env.STATIC = conf.options.static
    conf.env.TARGET_WINDOWS = conf.options.target_windows
    conf.env.DISABLE_TESTS = conf.options.disable_tests
    conf.env.API_VERSION = API_VERSION

    if conf.options.target_windows:
        conf.env.append_value('CXXFLAGS', '-DLIBSUB_WINDOWS')
    else:
        conf.env.append_value('CXXFLAGS', '-DLIBSUB_POSIX')

    if conf.options.enable_debug:
        conf.env.append_value('CXXFLAGS', '-g')
    else:
        conf.env.append_value('CXXFLAGS', '-O3')

    # Disable libxml++ deprecation warnings for now
    conf.env.append_value('CXXFLAGS', ['-Wno-deprecated-declarations'])

    conf.check_cfg(package='openssl', args='--cflags --libs', uselib_store='OPENSSL', mandatory=True)

    if conf.options.static:
        conf.check_cfg(package='libcxml', atleast_version='0.16.0', args='--cflags', uselib_store='CXML', mandatory=True)
        conf.env.HAVE_CXML = 1
        conf.env.LIB_CXML = ['glibmm-2.4', 'glib-2.0', 'pcre', 'sigc-2.0', 'rt', 'xml++-2.6', 'xml2', 'pthread', 'lzma', 'dl', 'z']
        conf.env.STLIB_CXML = ['cxml']
        conf.check_cfg(package='libdcp-1.0', atleast_version='1.6.2', args='--cflags', uselib_store='DCP', mandatory=True)
        conf.env.HAVE_DCP = 1
        conf.env.STLIB_DCP = ['dcp-1.0', 'asdcp-cth', 'kumu-cth', 'openjp2']
        conf.env.LIB_DCP = ['ssl', 'crypto', 'xmlsec1-openssl', 'xmlsec1']
    else:
        conf.check_cfg(package='libcxml', atleast_version='0.16.0', args='--cflags --libs', uselib_store='CXML', mandatory=True)
        conf.check_cfg(package='libdcp-1.0', atleast_version='1.6.2', args='--cflags --libs', uselib_store='DCP', mandatory=True)

    conf.env.DEFINES_DCP = [f.replace('\\', '') for f in conf.env.DEFINES_DCP]

    boost_lib_suffix = ''
    if conf.env.TARGET_WINDOWS:
        boost_lib_suffix = '-mt'

    conf.check_cxx(fragment="""
                            #include <boost/version.hpp>\n
                            #if BOOST_VERSION < 104500\n
                            #error boost too old\n
                            #endif\n
                            int main(void) { return 0; }\n
                            """,
                   mandatory=True,
                   msg='Checking for boost library >= 1.45',
                   okmsg='yes',
                   errmsg='too old\nPlease install boost version 1.45 or higher.')

    conf.check_cxx(fragment="""
    			    #include <boost/filesystem.hpp>\n
    			    int main() { boost::filesystem::copy_file ("a", "b"); }\n
			    """,
                   msg='Checking for boost filesystem library',
                   libpath='/usr/local/lib',
                   lib=['boost_filesystem%s' % boost_lib_suffix, 'boost_system%s' % boost_lib_suffix],
                   uselib_store='BOOST_FILESYSTEM')

    # Find the icu- libraries on the system as we need to link to them when we look for boost locale.
    locale_libs = ['boost_locale%s' % boost_lib_suffix, 'boost_system%s' % boost_lib_suffix]
    for pkg in subprocess.check_output(['pkg-config', '--list-all']).splitlines():
        pkg = pkg.decode('utf-8')
        if pkg.startswith("icu"):
            for lib in subprocess.check_output(['pkg-config', '--libs-only-l', pkg.split()[0]]).split():
                name = lib[2:]
                if not name in locale_libs:
                    locale_libs.append(name.decode('utf-8'))

    conf.check_cxx(fragment="""
    			    #include <boost/locale.hpp>\n
    			    int main() { boost::locale::conv::to_utf<char> ("a", "cp850"); }\n
			    """,
                   msg='Checking for boost locale library',
                   libpath='/usr/local/lib',
                   lib=locale_libs,
                   uselib_store='BOOST_LOCALE')

    conf.check_cxx(fragment="""
    			    #include <boost/regex.hpp>\n
    			    int main() { boost::regex re ("foo"); }\n
			    """,
                   msg='Checking for boost regex library',
                   libpath='/usr/local/lib',
                   lib=['boost_regex%s' % boost_lib_suffix, 'boost_system%s' % boost_lib_suffix],
                   uselib_store='BOOST_REGEX')

    if not conf.env.DISABLE_TESTS:
        conf.recurse('test')

def build(bld):
    create_version_cc(bld, VERSION)

    if bld.env.TARGET_WINDOWS:
        boost_lib_suffix = '-mt'
    else:
        boost_lib_suffix = ''

    bld(source='libsub%s.pc.in' % bld.env.API_VERSION,
        version=VERSION,
        includedir='%s/include/libsub%s' % (bld.env.PREFIX, bld.env.API_VERSION),
        libs="-L${libdir} -lsub%s -lboost_system%s" % (bld.env.API_VERSION, boost_lib_suffix),
        install_path='${LIBDIR}/pkgconfig')

    bld.recurse('src')
    if not bld.env.DISABLE_TESTS:
        bld.recurse('test')
    bld.recurse('tools')

    bld.add_post_fun(post)

def dist(ctx):
    ctx.excl = 'TODO core *~ .git build .waf* .lock* doc/*~ src/*~ test/ref/*~ __pycache__ GPATH GRTAGS GSYMS GTAGS'

def create_version_cc(bld, version):
    if os.path.exists('.git'):
        cmd = "LANG= git log --abbrev HEAD^..HEAD ."
        output = subprocess.Popen(cmd, shell=True, stderr=subprocess.STDOUT, stdout=subprocess.PIPE).communicate()[0].splitlines()
        o = output[0].decode('utf-8')
        commit = o.replace ("commit ", "")[0:10]
    else:
        commit = "release"

    try:
        text =  '#include "version.h"\n'
        text += 'char const * sub::git_commit = \"%s\";\n' % commit
        text += 'char const * sub::version = \"%s\";\n' % version
        if bld.env.ENABLE_DEBUG:
            debug_string = 'true'
        else:
            debug_string = 'false'
        text += 'bool const built_with_debug = %s;\n' % debug_string
        print('Writing version information to src/version.cc')
        o = open('src/version.cc', 'w')
        o.write(text)
        o.close()
    except IOError:
        print('Could not open src/version.cc for writing\n')
        sys.exit(-1)

def post(ctx):
    if ctx.cmd == 'install':
        ctx.exec_command('/sbin/ldconfig')

def tags(bld):
    os.system('etags src/*.cc src/*.h')
