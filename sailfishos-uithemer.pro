TEMPLATE = subdirs

# Build the GUI app and the headless reassert helper as one project.
# Each subdir entry uses subdir.file (no per-target subdirectory required),
# so qmake creates per-target Makefiles in the project root and the wrapper
# orchestrates them. This lets %qtc_qmake5 in the RPM spec build everything
# in a single invocation, instead of cd'ing into a build subdirectory and
# fighting whatever %_builddir / %qtc_source_path injection the macro does.
SUBDIRS = main helper

main.file   = app.pro
helper.file = sailfishos-uithemer-reassert.pro

# The helper reuses headers from the main app's src/ tree, but does not link
# against the GUI binary, so no build-order dependency is required between
# the two SUBDIRS entries.
