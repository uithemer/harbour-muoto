# Shared "ops" sources. Included by every binary that needs a subset of
# the privileged-or-pure code (GUI, helperd daemon). Each binary just
# `include('../ops/ops.pri')` and gets the full set of sources +
# headers + INCLUDEPATH onto its own search path. Linker garbage
# collection drops anything a given binary does not actually call
# (e.g. the daemon never invokes IconApplier::buildPreview, so
# IconPreviewCache is unused there but still cheap to compile in).
#
# The static-library route (TEMPLATE=lib + STATICLIB) was rejected to
# keep the qmake graph and SDK build simple: SailfishApp's qmake plugin
# does not love mixed lib+app templates inside a subdirs project, and
# recompiling these few files twice costs <1 s.

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/iconapplier.h \
    $$PWD/iconpaths.h \
    $$PWD/iconpipeline.h \
    $$PWD/iconstockbackup.h \
    $$PWD/iconpackrunner.h \
    $$PWD/iconoverlay.h \
    $$PWD/iconpreviewcache.h \
    $$PWD/imageutil.h \
    $$PWD/filelock.h \
    $$PWD/spawner.h \
    $$PWD/dconfsettings.h \
    $$PWD/dconfuser.h \
    $$PWD/densityenabler.h \
    $$PWD/lipstickrefresh.h

SOURCES += \
    $$PWD/iconapplier.cpp \
    $$PWD/iconpaths.cpp \
    $$PWD/iconpipeline.cpp \
    $$PWD/iconstockbackup.cpp \
    $$PWD/iconpackrunner.cpp \
    $$PWD/iconoverlay.cpp \
    $$PWD/iconpreviewcache.cpp \
    $$PWD/imageutil.cpp \
    $$PWD/filelock.cpp \
    $$PWD/spawner.cpp \
    $$PWD/dconfuser.cpp \
    $$PWD/densityenabler.cpp \
    $$PWD/lipstickrefresh.cpp
