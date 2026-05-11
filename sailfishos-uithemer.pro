TEMPLATE = subdirs

# Build the GUI app and the bus-activated privileged daemon as one
# project. Each child .pro lives next to its sources under src/<role>/,
# so qmake creates per-target Makefiles in matching build subdirectories.
# %qtc_qmake5 in the RPM spec drives both (gui, daemon + the implicit
# ops.pri include) in a single invocation.
#
# Naming note: src/daemon/ produces the privileged D-Bus
# /usr/libexec/sailfishos-uithemer-helperd. (2.7.0 retired the headless
# /usr/bin/sailfishos-uithemer-icond binary along with OptionsPage's
# autoupdate/systemupgrade/boot-reassert units.)
SUBDIRS = gui daemon

gui.subdir     = src/gui
daemon.subdir  = src/daemon

# Both binaries pull in src/ops/ops.pri at compile-time, so there is no
# inter-subdir build-order dependency. Listing the depends here anyway
# makes the topological order explicit if a future ops/ change does
# require a rebuild ordering.
gui.depends     =
daemon.depends  =
