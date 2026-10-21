TARGET = harbour-plump

CONFIG += sailfishapp c++17

VERSION = 0.1.0
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

# The card code shared with the other card games, a git submodule.
!exists($$PWD/sailfish-cards/cards.pri): error("sailfish-cards is missing: run git submodule update --init")
include($$PWD/sailfish-cards/cards.pri)
include($$PWD/sailfish-cards/online.pri)

SOURCES += src/harbour-plump.cpp \
    src/gamecontroller.cpp \
    src/core/bot.cpp \
    src/core/game.cpp

HEADERS += src/gamecontroller.h \
    src/core/bot.h \
    src/core/card.h \
    src/core/game.h

DISTFILES += qml/harbour-plump.qml \
    qml/components/PlayerScore.qml \
    qml/cover/CoverPage.qml \
    qml/pages/GamePage.qml \
    qml/pages/RulesPage.qml \
    qml/pages/SettingsPage.qml \
    rpm/harbour-plump.changes \
    rpm/harbour-plump.spec \
    translations/*.ts \
    harbour-plump.desktop \
    LICENSE \
    README.md \
    tests/Makefile \
    tests/test_rules.cpp

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

CONFIG += sailfishapp_i18n
TRANSLATIONS += translations/harbour-plump-sv.ts

# Rule tests: plain C++, no Qt. `make check` here, or `make -C tests` on any host.
check.commands = $(CXX) -std=c++17 -O2 -I$$CARDS_DIR/src -o test_rules $$PWD/tests/test_rules.cpp \
    $$PWD/src/core/bot.cpp $$PWD/src/core/game.cpp \
    $$CARDS_DIR/src/cards/cards.cpp && ./test_rules
QMAKE_EXTRA_TARGETS += check
