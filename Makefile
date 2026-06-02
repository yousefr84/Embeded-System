################################################################################
# Greenhouse — official TinyTimber (TinyTimber-master) on Linux / env/posix
################################################################################

ENV       ?= posix
TT_ROOT   := kernel/tT
ENV_ROOT  := kernel/env
BUILD_DIR := build/$(ENV)

CC        ?= gcc
CFLAGS    ?= -std=c99 -Wall -Wextra -pedantic -O2
CFLAGS    += -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
CFLAGS    += -DENV_POSIX=1
KERNEL_CFLAGS = $(CFLAGS) -Wno-sign-compare -Wno-unused-parameter -Wno-return-type
# Official env headers inline into app translation units
CFLAGS += -Wno-sign-compare
CFLAGS    += -Iinclude -Isrc
CFLAGS    += -I$(TT_ROOT) -I$(ENV_ROOT) -I$(ENV_ROOT)/$(ENV)
LDFLAGS   ?= -pthread -lrt

APP_SRCS = \
	src/main.c \
	src/greenhouse_argv.c \
	src/log.c \
	src/sensors.c \
	src/controller.c \
	src/actuators.c \
	src/scenarios.c

KERNEL_SRCS = \
	$(TT_ROOT)/kernel.c \
	$(ENV_ROOT)/$(ENV)/env.c \
	$(ENV_ROOT)/$(ENV)/ack.c

APP_OBJS    = $(APP_SRCS:src/%.c=$(BUILD_DIR)/app_%.o)
KERNEL_OBJS = $(BUILD_DIR)/kernel.o $(BUILD_DIR)/env.o $(BUILD_DIR)/ack.o
OBJS        = $(KERNEL_OBJS) $(APP_OBJS)
TARGET      = greenhouse

.PHONY: all clean run run-all

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/kernel.o: $(TT_ROOT)/kernel.c | $(BUILD_DIR)
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

$(BUILD_DIR)/env.o: $(ENV_ROOT)/$(ENV)/env.c | $(BUILD_DIR)
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

$(BUILD_DIR)/ack.o: $(ENV_ROOT)/$(ENV)/ack.c | $(BUILD_DIR)
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

$(BUILD_DIR)/app_%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build $(TARGET)

run: $(TARGET)
	./$(TARGET) normal

run-all: $(TARGET)
	./$(TARGET) all
