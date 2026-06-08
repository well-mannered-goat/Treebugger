project('treebugger', 'cpp',
    version: '0.1',
    default_options: ['warning_level=3', 'cpp_std=c++17'])

inc_dir = include_directories('src/include')

subdir('src/handlers')

sources = [
    'src/main.cpp',
    'src/debugger.cpp',
    'src/debuggee.cpp'
]

executable('trdbg',
    sources + handler_sources,
    include_directories: inc_dir,
    install: true)