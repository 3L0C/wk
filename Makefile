# Package info
NAME         := wk
VERSION      := 0.2.0

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
X11_DIR      := $(RUNTIME_DIR)/x11
WAY_DIR      := $(RUNTIME_DIR)/wayland

# Files
HEADERS      := $(wildcard $(SOURCE_DIR)/*.h) $(CONF_DIR)/config.h $(CONF_DIR)/key_chords.h
SOURCES      := $(wildcard $(SOURCE_DIR)/*.c)
OBJECTS      := $(addprefix $(BUILD_DIR)/, $(notdir $(SOURCES:.c=.o)))
COMM_OBJS    := $(patsubst $(COMMON_DIR)/%.c, $(BUILD_DIR)/common/%.o, \
					$(wildcard $(COMMON_DIR)/*.c))
COMP_OBJS    := $(patsubst $(COMPILER_DIR)/%.c, $(BUILD_DIR)/compiler/%.o, \
					$(wildcard $(COMPILER_DIR)/*.c))
RUN_OBJS     := $(patsubst $(RUNTIME_DIR)/%.c, $(BUILD_DIR)/runtime/%.o, \
					$(wildcard $(RUNTIME_DIR)/*.c))
X11_OBJS     := $(patsubst $(X11_DIR)/%.c, $(BUILD_DIR)/runtime/x11/%.o, \
					$(wildcard $(X11_DIR)/*.c))
WAY_SRCS     := $(WAY_DIR)/xdg-shell.c $(WAY_DIR)/wlr-layer-shell-unstable-v1.c
WAY_HDRS     := $(WAY_DIR)/wlr-layer-shell-unstable-v1.h
WAY_FILES    := $(WAY_SRCS) $(WAY_HDRS)
WAY_OBJS     := $(patsubst $(WAY_DIR)/%.c, $(BUILD_DIR)/runtime/wayland/%.o, \
					$(wildcard $(WAY_DIR)/*.c) $(WAY_SRCS))
MAN_FILES    := $(wildcard $(MAN_DIR)/*.man)

# Flags
CFLAGS       := -Wall -Wextra -Werror -Wno-unused-parameter -DVERSION=\"$(VERSION)\" -MMD -MP \
					-iquote. -iquote$(SOURCE_DIR)
CFLAGS       += $(shell $(PKG_CONFIG) --cflags cairo pango pangocairo)
LDFLAGS      += $(shell $(PKG_CONFIG) --libs cairo pango pangocairo)
X11_CFLAGS   += -DWK_X11_BACKEND $(shell $(PKG_CONFIG) --cflags x11 xinerama)
X11_LDFLAGS  += $(shell $(PKG_CONFIG) --libs x11 xinerama)
WAY_CFLAGS   += -DWK_WAYLAND_BACKEND $(shell $(PKG_CONFIG) --cflags wayland-client xkbcommon)
WAY_LDFLAGS  += $(shell $(PKG_CONFIG) --libs wayland-client xkbcommon)

# Make goals
ALL_GOALS    := all debug
X11_GOALS    := x11 debug-x11
WAY_GOALS    := wayland debug-wayland

# Insert implicit 'all' target
ifeq (0,$(words $(MAKECMDGOALS)))
MAKECMDGOALS := all
endif

# Include relevant objects and flags for ALL_GOALS
ifneq (0,$(words $(filter $(ALL_GOALS),$(MAKECMDGOALS))))
TARGET_OBJS  := $(X11_OBJS) $(WAY_OBJS)
CFLAGS       += $(X11_CFLAGS) $(WAY_CFLAGS)
LDFLAGS      += $(X11_LDFLAGS) $(WAY_LDFLAGS)
endif

# Include relevant objects and flags for X11_GOALS
ifneq (0,$(words $(filter $(X11_GOALS),$(MAKECMDGOALS))))
TARGET_OBJS  := $(X11_OBJS)
CFLAGS       += $(X11_CFLAGS)
LDFLAGS      += $(X11_LDFLAGS)
endif

# Include relevant objects and flags for WAY_GOALS
ifneq (0,$(words $(filter $(WAY_GOALS),$(MAKECMDGOALS))))
TARGET_OBJS  := $(WAY_OBJS)
CFLAGS       += $(WAY_CFLAGS)
LDFLAGS      += $(WAY_LDFLAGS)
endif

# Targets
all: options
all: $(WAY_FILES)
all: $(BUILD_DIR)/$(NAME)

options:
	@ printf "%-11s = %s\n" "CFLAGS"  "$(CFLAGS)"
	@ printf "%-11s = %s\n" "HEADERS" "$(HEADERS)"
	@ printf "%-11s = %s\n" "SOURCES" "$(SOURCES)"
	@ printf "%-11s = %s\n" "OBJECTS" "$(OBJECTS)"
	@ printf "%-11s = %s\n" "COMM_OBJS" "$(COMM_OBJS)"
	@ printf "%-11s = %s\n" "COMP_OBJS" "$(COMP_OBJS)"
	@ printf "%-11s = %s\n" "RUN_OBJS" "$(RUN_OBJS)"
	@ printf "%-11s = %s\n" "TARGET_OBJS" "$(TARGET_OBJS)"

x11: options
x11: $(BUILD_DIR)/$(NAME)

wayland: options
wayland: $(WAY_FILES)
wayland: $(BUILD_DIR)/$(NAME)

debug: CFLAGS += -ggdb
debug: all

debug-x11: CFLAGS += -ggdb
debug-x11: x11

debug-wayland: CFLAGS += -ggdb
debug-wayland: wayland

$(BUILD_DIR)/$(NAME): $(OBJECTS) $(COMM_OBJS) $(COMP_OBJS) $(RUN_OBJS) $(TARGET_OBJS)
	@ printf "%s %s %s\n" $(CC) "$@ $^" "$(CFLAGS) $(LDFLAGS)"
	@ $(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)
	@ cp $@ $(NAME)

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c $(HEADERS)
	@ printf "%s %s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(@D)
	@ $(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/common/%.o: $(COMMON_DIR)/%.c
	@ printf "%s %s %s\n" $(CC) $< "$(CFLAGS) -iquote$(COMMON_DIR)"
	@ mkdir -p $(@D)
	@ $(CC) -c $(CFLAGS) -iquote$(COMMON_DIR) -o $@ $<

$(BUILD_DIR)/compiler/%.o: $(COMPILER_DIR)/%.c
	@ printf "%s %s %s\n" $(CC) $< "$(CFLAGS) -iquote$(COMPILER_DIR)"
	@ mkdir -p $(@D)
	@ $(CC) -c $(CFLAGS) -iquote$(COMPILER_DIR) -o $@ $<

$(BUILD_DIR)/runtime/%.o: $(RUNTIME_DIR)/%.c
	@ printf "%s %s %s\n" $(CC) $< "$(CFLAGS) -iquote$(RUNTIME_DIR)"
	@ mkdir -p $(@D)
	@ $(CC) -c $(CFLAGS) -iquote$(RUNTIME_DIR) -o $@ $<

$(BUILD_DIR)/runtime/x11/%.o: $(X11_DIR)/%.c
	@ printf "%s %s %s\n" $(CC) $< "$(CFLAGS) $(X11_CFLAGS) -iquote$(X11_DIR)"
	@ mkdir -p $(@D)
	@ $(CC) -c $(CFLAGS) $(X11_CFLAGS) -iquote$(X11_DIR) -o $@ $<

$(BUILD_DIR)/runtime/wayland/%.o: $(WAY_DIR)/%.c
	@ printf "%s %s %s\n" $(CC) $< "$(CFLAGS) $(WAY_CFLAGS) -iquote$(WAY_DIR)"
	@ mkdir -p $(@D)
	@ $(CC) -c $(CFLAGS) $(WAY_CFLAGS) -iquote$(WAY_DIR) -o $@ $<

$(CONF_DIR)/config.h:
	cp $(CONF_DIR)/config.def.h $@

$(CONF_DIR)/key_chords.h:
	cp $(CONF_DIR)/key_chords.def.h $@

# Wayland-scanner rules
$(WAY_DIR)/xdg-shell.c:
	wayland-scanner private-code < \
		"$$($(PKG_CONFIG) --variable=pkgdatadir wayland-protocols)/stable/xdg-shell/xdg-shell.xml" \
		> $@

$(WAY_DIR)/wlr-layer-shell-unstable-v1.h: $(WAY_DIR)/wlr-layer-shell-unstable-v1.xml
	wayland-scanner client-header < $^ > $@

$(WAY_DIR)/wlr-layer-shell-unstable-v1.c: $(WAY_DIR)/wlr-layer-shell-unstable-v1.xml
	wayland-scanner private-code < $^ > $@

# Manfile generation and cleanup
$(MAN_DIR)/%.1: $(MAN_DIR)/%.1.man
	cp -f $< $@
	sh clean_man_files.sh "General Commands Manual" "$@"

$(MAN_DIR)/%.5: $(MAN_DIR)/%.5.man
	cp -f $< $@
	sh clean_man_files.sh "File Formats Manual" "$@"

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(NAME)
	rm -f $(MAN_FILES:.man=)
	rm -f $(WAY_FILES)

dist: clean
	mkdir -p $(NAME)-$(VERSION)
	cp -R src man config.def.h key_chords.def.h LICENSE Makefile README.md $(NAME)-$(VERSION)
	tar -czf $(NAME)-$(VERSION).tar.gz $(NAME)-$(VERSION)
	rm -rf $(NAME)-$(VERSION)

install: $(BUILD_DIR)/$(NAME) $(MAN_FILES:.man=)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f $(NAME) $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/$(NAME)
	for section in 1 5; do \
		mkdir -p $(DESTDIR)$(MANPREFIX)/man$${section}; \
		cp -f $(MAN_DIR)/*.$$section $(DESTDIR)$(MANPREFIX)/man$${section}; \
		chmod 644 $(DESTDIR)$(MANPREFIX)/man$${section}/$(NAME)*.$${section}; \
	done

man: $(MAN_FILES:.man=)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(NAME)
	for section in 1 5; do \
		rm -f $(DESTDIR)$(MANPREFIX)/man$${section}/$(NAME)*.$${section}; \
	done

.PHONY: all debug x11 wayland debug-x11 debug-wayland clean dist install uninstall

-include $(OBJECTS:.o=.d) $(COMM_OBJS:.o=.d) $(COMP_OBJS:.o=.d) $(RUN_OBJS:.o=.d) $(X11_OBJS:.o=.d) $(WAY_OBJS:.o=.d)
