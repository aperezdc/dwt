project('dwt', 'c',
	version: '0.6.1',
	default_options: [
		'buildtype=debugoptimized',
		'c_std=c99',
		'b_lto=true',
	]
)

gnome = import('gnome')

dwt_resources = gnome.compile_resources('dwt.gresources', 'dwt.gresources.xml')

dep_vte = dependency('vte-2.91')
dwt_sources = [
	'dwt.c',
	'dwt-settings.c',
	'dg-settings.c',
]

executable('dwt',
	install: true,
	sources: dwt_sources + dwt_resources,
	dependencies: dep_vte,
)

install_man('dwt.1')

install_data('dwt.desktop',
	install_dir: join_paths(get_option('datadir'), 'applications'))
