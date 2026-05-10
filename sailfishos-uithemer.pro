TEMPLATE = subdirs

# Build the GUI app, the bus-activated privileged daemon, and the
# headless icond helper as one project. Each child .pro lives next
# to its sources under src/<role>/, so qmake creates per-target
# Makefiles in matching build subdirectories. %qtc_qmake5 in the RPM
# spec drives all four (gui, daemon, icond + the implicit ops.pri
# include) in a single invocation.
#
# Naming note: src/daemon/ produces the privileged D-Bus
# /usr/libexec/sailfishos-uithemer-helperd; src/icond/ produces the
# one-shot CLI /usr/bin/sailfishos-uithemer-icond. Two different
# binaries, two different lifecycles -- only the former is actually
# a daemon despite the latter's -d suffix.
SUBDIRS = gui daemon icond

gui.subdir     = src/gui
daemon.subdir  = src/daemon
icond.subdir   = src/icond

# All three binaries pull in src/ops/ops.pri at compile-time, so there
# is no inter-subdir build-order dependency. Listing the depends here
# anyway makes the topological order explicit if a future ops/ change
# does require a rebuild ordering.
gui.depends     =
daemon.depends  =
icond.depends   =
