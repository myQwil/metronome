project('metronome', 'cpp',
	version : '0.1',
	default_options : [
		'warning_level=3',
		'cpp_std=c++14',
	],
)
sdl2_dep = dependency('sdl2')
json_dep = dependency('jsoncpp')

libpd_dir = './libpd'
libpd_include = [
	libpd_dir + '/libpd_wrapper',
	libpd_dir + '/libpd_wrapper/util',
	libpd_dir + '/pure-data/src',
	libpd_dir + '/cpp',
]
libpd_dep = declare_dependency(
	link_args : [libpd_dir + '/libs/libpd.a'],
	include_directories : include_directories(libpd_include),
)

# Documentation: https://mesonbuild.com/Qt5-module.html
qt5 = import('qt5')
qt5_dep = dependency('qt5', modules : ['Core', 'Widgets'])
generated_files = qt5.preprocess(dependencies : [qt5_dep],
	moc_headers : 'mainwindow.h',
	ui_files : 'mainwindow.ui',
)

exe = executable('metronome',
	['main.cpp', 'mainwindow.cpp', generated_files],
	dependencies : [sdl2_dep, json_dep, libpd_dep, qt5_dep],
	install : true,
)

install_data('Metronome.desktop',
	install_dir: join_paths(get_option('datadir'), 'applications'))

test('basic', exe)
