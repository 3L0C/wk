# wk - Which-key via X and Wayland
# See LICENSE file for copyright and license details.

NAME        := wk
VERSION     := 0.0.1
PREFIX      := /usr/local
MANPREFIX   := $(PREFIX)/share/man
BUILD_DIR   := ./build
SOURCE_DIR  := ./src
MAN_DIR     := ./man
LIB_DIR     := ./lib
X11_DIR     := $(LIB_DIR)/x11
WAY_DIR     := $(LIB_DIR)/wayland
PKG_CONFIG  ?= pkg-config

# flags
CFLAGS      := -Wall -Wextra -Werror -Wno-unused-parameter -DVERSION=\"$(VERSION)\" -MMD -MP -iquote.
LDFLAGS     += $(shell $(PKG_CONFIG) --libs x11 xinerama cairo pango pangocairo wayland-client xkbcommon)
CFLAGS      += $(shell $(PKG_CONFIG) --cflags-only-I x11 xinerama cairo pango pangocairo wayland-client xkbcommon)

# x11 files
XHEADERS    := $(wildcard $(X11_DIR)/*.h)
XSOURCES    := $(wildcard $(X11_DIR)/*.c)
XOBJECTS    := $(addprefix $(BUILD_DIR)/lib/x11/, $(notdir $(XSOURCES:.c=.o)))

# wayland files
WHEADERS    := $(wildcard $(WAY_DIR)/*.h) $(WAY_DIR)/wlr-layer-shell-unstable-v1.h
WSOURCES    := $(wildcard $(WAY_DIR)/*.c) $(WAY_DIR)/wlr-layer-shell-unstable-v1.c $(WAY_DIR)/xdg-shell.c
WOBJECTS    := $(addprefix $(BUILD_DIR)/lib/wayland/, $(notdir $(WSOURCES:.c=.o)))

# common files
HEADERS     := $(wildcard $(SOURCE_DIR)/*.h) ./config.h ./chords.h
SOURCES     := $(wildcard $(SOURCE_DIR)/*.c)
OBJECTS     := $(addprefix $(BUILD_DIR)/, $(notdir $(SOURCES:.c=.o)))
MANFILES    := $(wildcard $(MAN_DIR)/*.1)
LIBHDRS     := $(wildcard $(LIB_DIR)/*.h)
LIBSRCS     := $(wildcard $(LIB_DIR)/*.c)
LIBOBJS     := $(addprefix $(BUILD_DIR)/lib/, $(notdir $(LIBSRCS:.c=.o)))
DEPS        := $(SOURCES:.c=.d) $(LIBSRCS:.c=.d) $(XSOURCES:.c=.d) $(WSOURCES:.c=.d)

# check for debug
ifneq (0,$(words $(filter debug,$(MAKECMDGOALS))))
CFLAGS += -ggdb
endif

# check for x11
ifeq (1,$(words $(filter x11,$(MAKECMDGOALS))))
BOBJECTS += $(XOBJECTS)
CFLAGS   += -DWK_X11_BACKEND
endif

# check for wayland
ifeq (1,$(words $(filter wayland,$(MAKECMDGOALS))))
BOBJECTS += $(WOBJECTS)
CFLAGS   += -DWK_WAYLAND_BACKEND
endif

# making all
ifeq (0,$(words $(MAKECMDGOALS)))
BOBJECTS += $(XOBJECTS) $(WOBJECTS)
CFLAGS   += -DWK_X11_BACKEND -DWK_WAYLAND_BACKEND
endif

all: $(BUILD_DIR)/$(NAME)

debug: options

x11: all

$(WAY_DIR)/xdg-shell.c:
	wayland-scanner private-code < "$$($(PKG_CONFIG) --variable=pkgdatadir wayland-protocols)/stable/xdg-shell/xdg-shell.xml" > $@

$(WAY_DIR)/wlr-layer-shell-unstable-v1.h: $(WAY_DIR)/wlr-layer-shell-unstable-v1.xml
	wayland-scanner client-header < $^ > $@

$(WAY_DIR)/wlr-layer-shell-unstable-v1.c: $(WAY_DIR)/wlr-layer-shell-unstable-v1.xml
	wayland-scanner private-code < $^ > $@

wayland: $(WAY_DIR)/xdg-shell.c $(WAY_DIR)/wlr-layer-shell-unstable-v1.h $(WAY_DIR)/wlr-layer-shell-unstable-v1.c all

options:
	@ printf "%-8s = %s\n" "CFLAGS"  "$(CFLAGS)"
	@ printf "%-8s = %s\n" "HEADERS" "$(HEADERS)"
	@ printf "%-8s = %s\n" "SOURCES" "$(SOURCES)"
	@ printf "%-8s = %s\n" "OBJECTS" "$(OBJECTS)"
	@ printf "%-8s = %s\n" "LIBHDRS" "$(LIBHDRS)"
	@ printf "%-8s = %s\n" "LIBSRCS" "$(LIBSRCS)"
	@ printf "%-8s = %s\n" "LIBOBJS" "$(LIBOBJS)"
	@ printf "%-8s = %s\n" "BOBJECTS" "$(BOBJECTS)"

$(BUILD_DIR)/$(NAME): $(OBJECTS) $(LIBOBJS) $(BOBJECTS)
	@ printf "%s %s %s\n" $(CC) "$@ $^" "$(CFLAGS) $(LDFLAGS)"
	@ mkdir -p $(BUILD_DIR)
	@ $(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@
	@ cp build/$(NAME) $(NAME)

$(BUILD_DIR)/lib/%.o: $(LIB_DIR)/%.c $(LIBHDRS)
	@ printf "%s %s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)/lib
	@ $(CC) -c $(C_LANG) $(CFLAGS) -iquote$(LIB_DIR) -o $@ $<

$(BUILD_DIR)/lib/x11/%.o: $(X11_DIR)/%.c $(XHEADERS) $(LIBHDRS)
	@ printf "%s %s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)/lib/x11
	@ $(CC) -c $(C_LANG) $(CFLAGS) -iquote$(X11_DIR) -o $@ $<

$(BUILD_DIR)/lib/wayland/%.o: $(WAY_DIR)/%.c $(WHEADERS) $(LIBHDRS)
	@ printf "%s %s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)/lib/wayland
	@ $(CC) -c $(C_LANG) $(CFLAGS) -iquote$(WAY_DIR) -o $@ $<

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c $(HEADERS) $(LIBHDRS)
	@ printf "%s %s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)
	@ $(CC) -c $(C_LANG) $(CFLAGS) -o $@ $<

config.h:
	cp config.def.h $@

chords.h:
	cp chords.def.h $@

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(NAME)
# rm -rf config.h
# rm -rf chords.h

dist: clean
	mkdir -p $(NAME)-$(VERSION)/src
	cp -R ./src $(NAME)-$(VERSION)
	cp -R ./man $(NAME)-$(VERSION)
	cp -R LICENSE Makefile README.md $(NAME)-$(VERSION)
	tar -cf $(NAME)-$(VERSION).tar $(NAME)-$(VERSION)
	gzip $(NAME)-$(VERSION).tar
	rm -rf $(NAME)-$(VERSION)

install: $(BUILD_DIR)/$(NAME)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f $(NAME) $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/$(NAME)
# mkdir -p $(DESTDIR)$(MANPREFIX)/man1
# cp -R $(MANFILES) $(MAN_DIR)
# chmod 644 $(patsubst %,$(MAN_DIR)/%,$(notdir $(MAN)))

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(NAME)
	rm -f $(foreach F,$(patsubst %,$(MAN_DIR)/%,$(notdir $(MANFILES))),$(DESTDDIR)$(MANPREFIX)/$(F))

-include $(DEPS)
.PHONY: all debug x11 wayland options clean dist install uninstall
