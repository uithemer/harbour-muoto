TEMPLATE = subdirs

# Build the GUI app and the bus-activated privileged daemon as one
# project. Each child .pro lives next to its sources under src/<role>/,
# so qmake creates per-target Makefiles in matching build subdirectories.
# %qtc_qmake5 in the RPM spec drives both (gui, daemon + the implicit
# ops.pri include) in a single invocation.
#
# Naming note: src/daemon/ produces the privileged D-Bus
# /usr/libexec/harbour-muoto-helperd. (2.7.0 retired the headless
# /usr/bin/harbour-muoto-icond binary along with OptionsPage's
# autoupdate/systemupgrade/boot-reassert units.)
SUBDIRS = launcher launcher-daemon launcher-dynamic gui daemon listener

launcher.subdir          = src/launcher
launcher-daemon.subdir     = src/launcher-daemon
launcher-dynamic.subdir    = src/launcher-dynamic
gui.subdir                 = src/gui
daemon.subdir              = src/daemon
listener.subdir            = src/listener

launcher-daemon.depends    = launcher
launcher-dynamic.depends   = launcher
gui.depends                = launcher
daemon.depends             =
listener.depends           =
