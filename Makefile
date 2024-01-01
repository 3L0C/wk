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

# flags
CFLAGS      := -Wall -Wextra -Werror -Wno-unused-parameter -DVERSION=\"$(VERSION)\" -MMD -MP -iquote.

# x11 files
XHEADERS    := $(wildcard $(X11_DIR)/*.h)
XSOURCES    := $(wildcard $(X11_DIR)/*.c)
XOBJECTS    := $(addprefix $(BUILD_DIR)/lib/x11/, $(notdir $(XSOURCES:.c=.o)))

# wayland files
WHEADERS    := $(wildcard $(WAY_DIR)/*.h)
WSOURCES    := $(wildcard $(WAY_DIR)/*.c)
WOBJECTS    := $(addprefix $(BUILD_DIR)/lib/wayland/, $(notdir $(WSOURCES:.c=.o)))

# common files
HEADERS     := $(wildcard $(SOURCE_DIR)/*.h) ./config.h ./chords.h
SOURCES     := $(wildcard $(SOURCE_DIR)/*.c)
OBJECTS     := $(addprefix $(BUILD_DIR)/, $(notdir $(SOURCES:.c=.o)))
MANFILES    := $(wildcard $(MAN_DIR)/*.1)
LIBHEADERS  := $(wildcard $(LIB_DIR)/*.h)
LIBSOURCES  := $(wildcard $(LIB_DIR)/*.c)
LIBOBJS     := $(addprefix $(BUILD_DIR)/lib/, $(notdir $(LIBSOURCES:.c=.o)))
DEPS        := $(SOURCES:.c=.d) $(LIBSOURCES:.c=.d) $(XSOURCES:.c=.d) $(WSOURCES:.c=.d)

.SECONDEXPANSION:
all: x11

x11: libx11 $(BUILD_DIR)/$(NAME)

libx11: $$(eval RENDOBJS += $(XOBJECTS))

wayland: libwayland $(BUILD_DIR)/$(NAME)

libwayland: $$(eval RENDOBJS+= $(WOBJECTS))

debug: $$(eval CFLAGS += -ggdb)

debug-x11: debug libx11 options $(BUILD_DIR)/$(NAME)

debug-wayland: debug libwayland options $(BUILD_DIR)/$(NAME)

options:
	@ printf "%-8s = %s\n" "CFLAGS"  "$(CFLAGS)"
	@ printf "%-8s = %s\n" "HEADERS" "$(HEADERS)"
	@ printf "%-8s = %s\n" "SOURCES" "$(SOURCES)"
	@ printf "%-8s = %s\n" "OBJECTS" "$(OBJECTS)"
	@ printf "%-8s = %s\n" "LIBOBJS" "$(LIBOBJS)"
	@ printf "%-8s = %s\n" "RENDOBJS" "$(RENDOBJS)"

$(BUILD_DIR)/$(NAME): $(OBJECTS) $(LIBOBJS) $$(RENDOBJS) FORCE
	@ printf "%-8s %-40s %s\n" $(CC) "$@ $(^:FORCE=)" "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)
	@ $(CC) $(CFLAGS) $(^:FORCE=) -o $@
	@ cp build/$(NAME) $(NAME)

$(BUILD_DIR)/lib/%.o: $(LIB_DIR)/%.c $(LIBHEADERS)
	@ printf "%-8s %-40s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)/lib
	@ $(CC) -c $(C_LANG) $(CFLAGS) -iquote$(LIB_DIR) -o $@ $<

$(BUILD_DIR)/lib/x11/%.o: $(X11_DIR)/%.c $(XHEADERS) $(LIBHEADERS)
	@ printf "%-8s %-40s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)/lib/x11
	@ $(CC) -c $(C_LANG) $(CFLAGS) -iquote$(X11_DIR) -o $@ $<

$(BUILD_DIR)/lib/wayland/%.o: $(WAY_DIR)/%.c $(WHEADERS) $(LIBHEADERS)
	@ printf "%-8s %-40s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)/lib/wayland
	@ $(CC) -c $(C_LANG) $(CFLAGS) -iquote$(WAY_DIR) -o $@ $<

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c $(HEADERS) $(LIBHEADERS)
	@ printf "%-8s %-40s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)
	@ $(CC) -c $(C_LANG) $(CFLAGS) -o $@ $<

config.h:
	cp config.def.h $@

chords.h:
	cp chords.def.h $@

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(NAME)

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

FORCE:

-include $(DEPS)
.PHONY: all x11 libx11 wayland libwayland debug debug-x11 debug-wayland options clean dist install uninstall FORCE
