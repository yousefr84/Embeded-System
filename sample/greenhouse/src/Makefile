################################################################################
# Application object rules
################################################################################

ifndef BUILD_ROOT
$(error Variable BUILD_ROOT was not defined.)
endif

ifndef APP_ROOT
$(error Variable APP_ROOT was not defined.)
endif

CFLAGS	:= -I$(APP_ROOT) $(CFLAGS)

APP_SOURCES := \
	$(APP_ROOT)/main.c \
	$(APP_ROOT)/argv.c \
	$(APP_ROOT)/log.c \
	$(APP_ROOT)/controller.c \
	$(APP_ROOT)/sensors.c \
	$(APP_ROOT)/actuators.c \
	$(APP_ROOT)/scenarios.c

APP_OBJECTS := \
	$(BUILD_ROOT)/main.o \
	$(BUILD_ROOT)/argv.o \
	$(BUILD_ROOT)/log.o \
	$(BUILD_ROOT)/controller.o \
	$(BUILD_ROOT)/sensors.o \
	$(BUILD_ROOT)/actuators.o \
	$(BUILD_ROOT)/scenarios.o

$(BUILD_ROOT)/%.o: $(APP_ROOT)/%.c
	$(CC) $(CFLAGS) $< -c -o $@

APP_BINARY	:= $(BUILD_ROOT)/greenhouse.elf
