# wk - Which-key via X and Wayland
# See LICENSE file for copyright and license details.

NAME 		:= wk
VERSION 	:= 0.0.1
PREFIX 		:= /usr/local
MANPREFIX 	:= $(PREFIX)/share/man
BUILD_DIR 	:= ./build
SOURCE_DIR 	:= ./src
MAN_DIR 	:= ./man
LIB_DIR		:= ./lib
X11_DIR		:= $(LIB_DIR)/x11
WAY_DIR		:= $(LIB_DIR)/wayland

# flags
CFLAGS := -Wall -Wextra -Werror -Wno-unused-parameter -DVERSION=\"$(VERSION)\" -MD -MP -I$(LIB_DIR) -I.

# common files
HEADERS 	:= $(wildcard $(SOURCE_DIR)/*.h) ./config.h ./chords.h
SOURCES 	:= $(wildcard $(SOURCE_DIR)/*.c)
OBJECTS 	:= $(addprefix $(BUILD_DIR)/, $(notdir $(SOURCES:.c=.o)))
MANFILES 	:= $(wildcard $(MAN_DIR)/*.1)
DEPS 		:= $(SOURCE:.c=.d)

# x11 files
XHEADERS	:= $(wildcard $(X11_DIR)/*.h)
XSOURCES 	:= $(wildcard $(X11_DIR)/*.c)
XOBJECTS 	:= $(addprefix $(BUILD_DIR)/x11/, $(notdir $(XSOURCES:.c=.o)))

# wayland files
WHEADERS	:= $(wildcard $(WAY_DIR)/*.h)
WSOURCES 	:= $(wildcard $(WAY_DIR)/*.c)
WOBJECTS 	:= $(addprefix $(BUILD_DIR)/wayland/, $(notdir $(WSOURCES:.c=.o)))

.SECONDEXPANSION:
all: x11

x11: $$(eval LIBOBJS += $(XOBJECTS)) options $(BUILD_DIR)/$(NAME)

wayland: $$(eval LIBOBJS+= $(WOBJECTS)) options $(BUILD_DIR)/$(NAME)

debug: CFLAGS += -ggdb

options:
	@ printf "%-8s = %s\n" "CFLAGS"  "$(CFLAGS)"
	@ printf "%-8s = %s\n" "HEADERS" "$(HEADERS)"
	@ printf "%-8s = %s\n" "SOURCES" "$(SOURCES)"
	@ printf "%-8s = %s\n" "OBJECTS" "$(OBJECTS)"
	@ printf "%-8s = %s\n" "LIBOBJS" "$(LIBOBJS)"

$(BUILD_DIR)/$(NAME): $(OBJECTS) $$(LIBOBJS) $$(eval $$(shell touch $$(LIBOBJS)))
	@ printf "%-8s %-40s %s\n" $(CC) "$@ $^" "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)
	@ $(CC) $(CFLAGS) $^ -o $@
	@ cp build/$(NAME) $(NAME)

$(BUILD_DIR)/x11/%.o: $(X11_DIR)/%.c $(XHEADERS)
	@ printf "%-8s %-40s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)/x11
	@ $(CC) -c $(C_LANG) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/wayland/%.o: $(WAY_DIR)/%.c $(WHEADERS)
	@ printf "%-8s %-40s %s\n" $(CC) $< "$(CFLAGS)"
	@ mkdir -p $(BUILD_DIR)/wayland
	@ $(CC) -c $(C_LANG) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c $(HEADERS)
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

-include $(DEPS)
.PHONY: all x11 wayland debug options clean dist install uninstall
