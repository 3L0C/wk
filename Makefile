# wk - Which-key via X11/Wayland
# See LICENSE file for copyright and license details.

# Package info
NAME        := wk
VERSION     := 0.0.1

# Tools
PKG_CONFIG  ?= pkg-config

# Install locations
PREFIX      := /usr/local
MANPREFIX   := $(PREFIX)/share/man

# Project directories
BUILD_DIR   := ./build
CONF_DIR    := ./config
LIB_DIR     := ./lib
MAN_DIR     := ./man
SOURCE_DIR  := ./src

# Backend directories
X11_DIR     := $(LIB_DIR)/x11
WAY_DIR     := $(LIB_DIR)/wayland

# Common files
HEADERS     := $(wildcard $(SOURCE_DIR)/*.h) $(CONF_DIR)/config.h $(CONF_DIR)/chords.h
SOURCES     := $(wildcard $(SOURCE_DIR)/*.c)
OBJECTS     := $(addprefix $(BUILD_DIR)/, $(notdir $(SOURCES:.c=.o)))
MANFILES    := $(wildcard $(MAN_DIR)/*.1)
LIB_HDRS    := $(wildcard $(LIB_DIR)/*.h)
LIB_SRCS    := $(wildcard $(LIB_DIR)/*.c)
LIB_OBJS    := $(addprefix $(BUILD_DIR)/lib/, $(notdir $(LIB_SRCS:.c=.o)))

# Dependencies
DEPS        := $(SOURCES:.c=.d) $(LIB_SRCS:.c=.d)
X11_DEPS    := $(X11_SRCS:.c=.d)
WAY_DEPS    := $(WAY_SRCS:.c=.d)

# X11 files
X11_HDRS    := $(wildcard $(X11_DIR)/*.h)
X11_SRCS    := $(wildcard $(X11_DIR)/*.c)
X11_OBJS    := $(addprefix $(BUILD_DIR)/lib/x11/, $(notdir $(X11_SRCS:.c=.o)))

# Wayland files
WAY_HDRS    := $(wildcard $(WAY_DIR)/*.h) $(WAY_DIR)/wlr-layer-shell-unstable-v1.h
WAY_SRCS    := $(wildcard $(WAY_DIR)/*.c) $(WAY_DIR)/wlr-layer-shell-unstable-v1.c $(WAY_DIR)/xdg-shell.c
WAY_OBJS    := $(addprefix $(BUILD_DIR)/lib/wayland/, $(notdir $(WAY_SRCS:.c=.o)))
# WAY_XMLS    :=

# Standard flags
CFLAGS      := -Wall -Wextra -Werror -Wno-unused-parameter -DVERSION=\"$(VERSION)\" -MMD -MP -iquote.
CFLAGS      += $(shell $(PKG_CONFIG) --cflags-only-I cairo pango pangocairo)
LDFLAGS     += $(shell $(PKG_CONFIG) --libs cairo pango pangocairo)

# X11 flags
X11_CFLAGS  += -DWK_X11_BACKEND $(shell $(PKG_CONFIG) --cflags-only-I x11 xinerama)
X11_LDFLAGS += $(shell $(PKG_CONFIG) --libs x11 xinerama)

# Wayland flags
WAY_CFLAGS  += -DWK_WAYLAND_BACKEND $(shell $(PKG_CONFIG) --cflags-only-I wayland-client xkbcommon)
WAY_LDFLAGS += $(shell $(PKG_CONFIG) --libs wayland-client xkbcommon)

# Check for debug
ifneq (0,$(words $(filter debug,$(MAKECMDGOALS))))
CFLAGS      += -ggdb
endif

# Making X11
ifeq (1,$(words $(filter x11,$(MAKECMDGOALS))))
TARGET_OBJS += $(X11_OBJS)
CFLAGS      += $(X11_CFLAGS)
LDFLAGS     += $(X11_LDFLAGS)
endif

# Making Wayland
ifeq (1,$(words $(filter wayland,$(MAKECMDGOALS))))
TARGET_OBJS += $(WAY_OBJS)
CFLAGS      += $(WAY_CFLAGS)
LDFLAGS     += $(WAY_LDFLAGS)
endif

# Making all
ifeq (0,$(words $(MAKECMDGOALS)))
TARGET_OBJS += $(X11_OBJS) $(WAY_OBJS)
CFLAGS      += $(X11_CFLAGS) $(WAY_CFLAGS)
LDFLAGS     += $(X11_LDFLAGS) $(WAY_LDFLAGS)
endif

ifeq (1,$(words $(filter all,$(MAKECMDGOALS))))
TARGET_OBJS += $(X11_OBJS) $(WAY_OBJS)
CFLAGS      += $(X11_CFLAGS) $(WAY_CFLAGS)
LDFLAGS     += $(X11_LDFLAGS) $(WAY_LDFLAGS)
endif

# Targets
all: $(BUILD_DIR)/$(NAME)

x11: all

wayland: $(WAY_HDRS) $(WAY_SRCS) all

debug: options

options:
	@ printf "%-11s = %s\n" "CFLAGS"  "$(CFLAGS)"
	@ printf "%-11s = %s\n" "HEADERS" "$(HEADERS)"
	@ printf "%-11s = %s\n" "SOURCES" "$(SOURCES)"
	@ printf "%-11s = %s\n" "OBJECTS" "$(OBJECTS)"
	@ printf "%-11s = %s\n" "LIB_HDRS" "$(LIB_HDRS)"
	@ printf "%-11s = %s\n" "LIB_SRCS" "$(LIB_SRCS)"
	@ printf "%-11s = %s\n" "LIB_OBJS" "$(LIB_OBJS)"
	@ printf "%-11s = %s\n" "TARGET_OBJS" "$(TARGET_OBJS)"

# Wayland-scanner files
$(WAY_DIR)/xdg-shell.c:
	wayland-scanner private-code < "$$($(PKG_CONFIG) --variable=pkgdatadir wayland-protocols)/stable/xdg-shell/xdg-shell.xml" > $@

$(WAY_DIR)/wlr-layer-shell-unstable-v1.h: $(WAY_DIR)/wlr-layer-shell-unstable-v1.xml
	wayland-scanner client-header < $^ > $@

$(WAY_DIR)/wlr-layer-shell-unstable-v1.c: $(WAY_DIR)/wlr-layer-shell-unstable-v1.xml
	wayland-scanner private-code < $^ > $@

# Main package
$(BUILD_DIR)/$(NAME): $(OBJECTS) $(LIB_OBJS) $(TARGET_OBJS)
	@ printf "%s %s %s\n" $(CC) "$@ $^" "$(CFLAGS) $(LDFLAGS)"
	@ mkdir -p $(BUILD_DIR)
	@ $(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@
	@ cp build/$(NAME) $(NAME)

# Libs
$(BUILD_DIR)/lib/%.o: $(LIB_DIR)/%.c $(LIB_HDRS)
	@ printf "%s %s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)/lib
	@ $(CC) -c $(C_LANG) $(CFLAGS) -iquote$(LIB_DIR) -o $@ $<

# X11 targets
$(BUILD_DIR)/lib/x11/%.o: $(X11_DIR)/%.c $(X11_HDRS) $(LIB_HDRS)
	@ printf "%s %s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)/lib/x11
	@ $(CC) -c $(C_LANG) $(CFLAGS) -iquote$(X11_DIR) -o $@ $<

# Wayland targets
$(BUILD_DIR)/lib/wayland/%.o: $(WAY_DIR)/%.c $(WAY_HDRS) $(LIB_HDRS)
	@ printf "%s %s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)/lib/wayland
	@ $(CC) -c $(C_LANG) $(CFLAGS) -iquote$(WAY_DIR) -o $@ $<

# Main source targets
$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c $(HEADERS) $(LIB_HDRS)
	@ printf "%s %s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)
	@ $(CC) -c $(C_LANG) $(CFLAGS) -o $@ $<

# Config files
$(CONF_DIR)/config.h:
	cp $(CONF_DIR)/config.def.h $@

$(CONF_DIR)/chords.h:
	cp $(CONF_DIR)/chords.def.h $@

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

-include $(DEPS) $(X11_DEPS) $(WAY_DEPS)
.PHONY: all debug x11 wayland options clean dist install uninstall
