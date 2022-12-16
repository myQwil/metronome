project('metronome', 'cpp', default_options : ['cpp_std=c++11'])

# Documentation: https://mesonbuild.com/Qt5-module.html
qt5 = import('qt5')
qt5dep = dependency('qt5', modules : ['Core', 'Widgets'])
sdl2_dep = dependency('sdl2')

libpd_dir = '/home/mike/z/libpd'
libpd_include = [
	  libpd_dir + '/libpd_wrapper'
	, libpd_dir + '/libpd_wrapper/util'
	, libpd_dir + '/pure-data/src'
	, libpd_dir + '/cpp'
]
libpd_dep = declare_dependency(
	  link_args : [libpd_dir + '/libs/libpd.a']
	, include_directories : include_directories(libpd_include))

generated_files = qt5.preprocess(
	  moc_headers : 'mainwindow.h'
	, ui_files : 'mainwindow.ui'
	, dependencies : [qt5dep])

executable('metronome'
	, 'main.cpp'
	, 'mainwindow.cpp'
	, generated_files
	, dependencies : [qt5dep, sdl2_dep, libpd_dep]
	, install : true)
