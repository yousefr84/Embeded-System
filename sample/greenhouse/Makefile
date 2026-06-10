################################################################################
# Greenhouse sample application — TinyTimber on env/posix (Linux)
################################################################################

ENV=posix

ifndef ENV
$(error Variable ENV was not defined.)
endif

BUILD_ROOT=./$(ENV)

TT_ROOT=../../kernel/tT/
ENV_ROOT=../../kernel/env/
APP_ROOT=./src/

CFLAGS += -std=c99 -Wall -Wextra -pedantic -O2
CFLAGS += -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE

.PHONY: all build_root build rebuild clean
all: build_root build

include $(ENV_ROOT)/$(ENV)/Makefile
include $(TT_ROOT)/Makefile
include $(APP_ROOT)/Makefile

$(BUILD_ROOT): build_root

build_root:
	@test -d $(BUILD_ROOT) || mkdir -p $(BUILD_ROOT)

build: $(ENV_OBJECTS) $(TT_OBJECTS) $(APP_OBJECTS) $(APP_BINARY)

$(APP_BINARY): $(ENV_OBJECTS) $(TT_OBJECTS) $(APP_OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

rebuild: clean build_root build

clean:
	rm -rf $(BUILD_ROOT)
