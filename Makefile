################################################################################
# Top-level convenience targets — build the sample/greenhouse application
################################################################################

.PHONY: all clean run run-all
all:
	$(MAKE) -C sample/greenhouse

clean:
	$(MAKE) -C sample/greenhouse clean
	rm -f greenhouse

run: all
	./sample/greenhouse/posix/greenhouse.elf normal

run-all: all
	./sample/greenhouse/posix/greenhouse.elf all

greenhouse: all
	ln -sf sample/greenhouse/posix/greenhouse.elf greenhouse
