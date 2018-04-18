project('dwt', 'c',
	version: '0.6.1',
	default_options: [
		'buildtype=debugoptimized',
		'c_std=c11',
		'b_lto=true',
	]
)

gnome = import('gnome')

executable('dwt',
	'dwt.c',
	'dwt-settings.c',
	'dg-settings.c',
    gnome.compile_resources('dwt.gresources', 'dwt.gresources.xml'),
	dependencies: dependency('vte-2.91'),
	install: true,
)

install_man('dwt.1')

install_data('dwt.desktop',
	install_dir: join_paths(get_option('datadir'), 'applications'))
