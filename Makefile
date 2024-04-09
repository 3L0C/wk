# wk - Which-key via X11/Wayland
# See LICENSE file for copyright and license details.

# Package info
NAME         := wk
VERSION      := 0.0.1

# Tools
PKG_CONFIG   ?= pkg-config

# Install locations
PREFIX       := /usr/local
MANPREFIX    := $(PREFIX)/share/man

# Project directories
BUILD_DIR    := ./build
CONF_DIR     := ./config
MAN_DIR      := ./man
SOURCE_DIR   := ./src
COMMON_DIR   := $(SOURCE_DIR)/common
RUNTIME_DIR  := $(SOURCE_DIR)/runtime
COMPILER_DIR := $(SOURCE_DIR)/compiler

# Backend directories
X11_DIR      := $(RUNTIME_DIR)/x11
WAY_DIR      := $(RUNTIME_DIR)/wayland

# Main files
HEADERS      := $(wildcard $(SOURCE_DIR)/*.h) $(CONF_DIR)/config.h $(CONF_DIR)/key_chords.h
SOURCES      := $(wildcard $(SOURCE_DIR)/*.c)
OBJECTS      := $(addprefix $(BUILD_DIR)/, $(notdir $(SOURCES:.c=.o)))

# Common files
COMM_HDRS    := $(wildcard $(COMMON_DIR)/*.h)
COMM_SRCS    := $(wildcard $(COMMON_DIR)/*.c)
COMM_OBJS    := $(addprefix $(BUILD_DIR)/common/, $(notdir $(COMM_SRCS:.c=.o)))

# Compiler files
COMP_HDRS    := $(wildcard $(COMPILER_DIR)/*.h)
COMP_SRCS    := $(wildcard $(COMPILER_DIR)/*.c)
COMP_OBJS    := $(addprefix $(BUILD_DIR)/compiler/, $(notdir $(COMP_SRCS:.c=.o)))

# Runtime files
RUN_HDRS     := $(wildcard $(RUNTIME_DIR)/*.h)
RUN_SRCS     := $(wildcard $(RUNTIME_DIR)/*.c)
RUN_OBJS     := $(addprefix $(BUILD_DIR)/runtime/, $(notdir $(RUN_SRCS:.c=.o)))

# Man files
MANFILES     := $(wildcard $(MAN_DIR)/*.1)

# Dependencies
DEPS         := $(SOURCES:.c=.d) $(COMM_SRCS:.c=.d) $(COMP_SRCS:.c=.d) $(RUN_SRCS:.c=.d)
X11_DEPS     := $(X11_SRCS:.c=.d)
WAY_DEPS     := $(WAY_SRCS:.c=.d)

# X11 files
X11_HDRS     := $(wildcard $(X11_DIR)/*.h)
X11_SRCS     := $(wildcard $(X11_DIR)/*.c)
X11_OBJS     := $(addprefix $(BUILD_DIR)/runtime/x11/, $(notdir $(X11_SRCS:.c=.o)))

# Wayland files
WAY_HDRS     := $(wildcard $(WAY_DIR)/*.h) $(WAY_DIR)/wlr-layer-shell-unstable-v1.h
WAY_SRCS     := $(wildcard $(WAY_DIR)/*.c) $(WAY_DIR)/wlr-layer-shell-unstable-v1.c $(WAY_DIR)/xdg-shell.c
WAY_OBJS     := $(addprefix $(BUILD_DIR)/runtime/wayland/, $(notdir $(WAY_SRCS:.c=.o)))

# Standard flags
CFLAGS       := -Wall -Wextra -Werror -Wno-unused-parameter -DVERSION=\"$(VERSION)\" -MMD -MP -iquote.
CFLAGS       += $(shell $(PKG_CONFIG) --cflags-only-I cairo pango pangocairo)
LDFLAGS      += $(shell $(PKG_CONFIG) --libs cairo pango pangocairo)

# X11 flags
X11_CFLAGS   += -DWK_X11_BACKEND $(shell $(PKG_CONFIG) --cflags-only-I x11 xinerama)
X11_LDFLAGS  += $(shell $(PKG_CONFIG) --libs x11 xinerama)

# Wayland flags
WAY_CFLAGS   += -DWK_WAYLAND_BACKEND $(shell $(PKG_CONFIG) --cflags-only-I wayland-client xkbcommon)
WAY_LDFLAGS  += $(shell $(PKG_CONFIG) --libs wayland-client xkbcommon)

# Check for debug
ifneq (0,$(words $(filter debug,$(MAKECMDGOALS))))
CFLAGS       += -ggdb
endif

# Making X11
ifeq (1,$(words $(filter x11,$(MAKECMDGOALS))))
TARGET_OBJS  += $(X11_OBJS)
CFLAGS       += $(X11_CFLAGS)
LDFLAGS      += $(X11_LDFLAGS)
endif

# Making Wayland
ifeq (1,$(words $(filter wayland,$(MAKECMDGOALS))))
TARGET_OBJS  += $(WAY_OBJS)
CFLAGS       += $(WAY_CFLAGS)
LDFLAGS      += $(WAY_LDFLAGS)
endif

# Making all
ifeq (0,$(words $(MAKECMDGOALS)))
TARGET_OBJS  += $(X11_OBJS) $(WAY_OBJS)
CFLAGS       += $(X11_CFLAGS) $(WAY_CFLAGS)
LDFLAGS      += $(X11_LDFLAGS) $(WAY_LDFLAGS)
endif

ifeq (1,$(words $(filter all,$(MAKECMDGOALS))))
TARGET_OBJS  += $(X11_OBJS) $(WAY_OBJS)
CFLAGS       += $(X11_CFLAGS) $(WAY_CFLAGS)
LDFLAGS      += $(X11_LDFLAGS) $(WAY_LDFLAGS)
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
	@ printf "%-11s = %s\n" "COMM_HDRS" "$(COMM_HDRS)"
	@ printf "%-11s = %s\n" "COMM_SRCS" "$(COMM_SRCS)"
	@ printf "%-11s = %s\n" "COMM_OBJS" "$(COMM_OBJS)"
	@ printf "%-11s = %s\n" "COMP_HDRS" "$(COMP_HDRS)"
	@ printf "%-11s = %s\n" "COMP_SRCS" "$(COMP_SRCS)"
	@ printf "%-11s = %s\n" "COMP_OBJS" "$(COMP_OBJS)"
	@ printf "%-11s = %s\n" "RUN_HDRS" "$(RUN_HDRS)"
	@ printf "%-11s = %s\n" "RUN_SRCS" "$(RUN_SRCS)"
	@ printf "%-11s = %s\n" "RUN_OBJS" "$(RUN_OBJS)"
	@ printf "%-11s = %s\n" "TARGET_OBJS" "$(TARGET_OBJS)"

# Wayland-scanner files
$(WAY_DIR)/xdg-shell.c:
	wayland-scanner private-code < "$$($(PKG_CONFIG) --variable=pkgdatadir wayland-protocols)/stable/xdg-shell/xdg-shell.xml" > $@

$(WAY_DIR)/wlr-layer-shell-unstable-v1.h: $(WAY_DIR)/wlr-layer-shell-unstable-v1.xml
	wayland-scanner client-header < $^ > $@

$(WAY_DIR)/wlr-layer-shell-unstable-v1.c: $(WAY_DIR)/wlr-layer-shell-unstable-v1.xml
	wayland-scanner private-code < $^ > $@

# Main package
$(BUILD_DIR)/$(NAME): $(OBJECTS) $(COMM_OBJS) $(COMP_OBJS) $(RUN_OBJS) $(TARGET_OBJS)
	@ printf "%s %s %s\n" $(CC) "$@ $^" "$(CFLAGS) $(LDFLAGS)"
	@ mkdir -p $(BUILD_DIR)
	@ $(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@
	@ cp build/$(NAME) $(NAME)

# Main source targets
$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c $(HEADERS) $(COMM_HDRS) $(COMP_HDRS) $(RUN_HDRS)
	@ printf "%s %s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)
	@ $(CC) -c $(C_LANG) $(CFLAGS) -iquote$(SOURCE_DIR) -iquote. -o $@ $<

# Common targets
$(BUILD_DIR)/common/%.o: $(COMMON_DIR)/%.c $(COMM_HDRS)
	@ printf "%s %s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)/common
	@ $(CC) -c $(C_LANG) $(CFLAGS) -iquote$(SOURCE_DIR) -iquote$(COMMON_DIR) -o $@ $<

# Compiler targets
$(BUILD_DIR)/compiler/%.o: $(COMPILER_DIR)/%.c $(COMP_HDRS) $(COMM_HDRS)
	@ printf "%s %s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)/compiler
	@ $(CC) -c $(C_LANG) $(CFLAGS) -iquote$(SOURCE_DIR) -iquote$(COMPILER_DIR) -o $@ $<

# Runtime targets
$(BUILD_DIR)/runtime/%.o: $(RUNTIME_DIR)/%.c $(RUN_HDRS) $(COMM_HDRS)
	@ printf "%s %s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)/runtime
	@ $(CC) -c $(C_LANG) $(CFLAGS) -iquote$(SOURCE_DIR) -iquote $(RUNTIME_DIR) -o $@ $<

# X11 targets
$(BUILD_DIR)/runtime/x11/%.o: $(X11_DIR)/%.c $(X11_HDRS) $(COMM_HDRS) $(RUN_HDRS)
	@ printf "%s %s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)/runtime/x11
	@ $(CC) -c $(C_LANG) $(CFLAGS) -iquote$(SOURCE_DIR) -iquote$(X11_DIR) -o $@ $<

# Wayland targets
$(BUILD_DIR)/runtime/wayland/%.o: $(WAY_DIR)/%.c $(WAY_HDRS) $(COMM_HDRS) $(RUN_HDRS)
	@ printf "%s %s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)/runtime/wayland
	@ $(CC) -c $(C_LANG) $(CFLAGS) -iquote$(SOURCE_DIR) -iquote$(WAY_DIR) -o $@ $<

# Config files
$(CONF_DIR)/config.h:
	cp $(CONF_DIR)/config.def.h $@

$(CONF_DIR)/key_chords.h:
	cp $(CONF_DIR)/key_chords.def.h $@

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(NAME)
# rm -rf config.h
# rm -rf key_chords.h

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
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	cp -R $(MANFILES) $(MAN_DIR)
	chmod 644 $(patsubst %,$(MAN_DIR)/%,$(notdir $(MAN)))

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(NAME)
	rm -f $(foreach F,$(patsubst %,$(MAN_DIR)/%,$(notdir $(MANFILES))),$(DESTDDIR)$(MANPREFIX)/$(F))

-include $(DEPS) $(X11_DEPS) $(WAY_DEPS)
.PHONY: all debug x11 wayland options clean dist install uninstall
