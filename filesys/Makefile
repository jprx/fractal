SUPPORTED_TARGETS:=aarch64
DEST_DISKS:=$(patsubst %,disk.%.tar,$(SUPPORTED_TARGETS)) $(patsubst %,disk.%.ext2,$(SUPPORTED_TARGETS))
SYSROOT_DIRS:=$(patsubst %,fs_%,$(SUPPORTED_TARGETS))
.PHONY: all

all: $(DEST_DISKS) $(SYSROOT_DIRS)

fs_%:
	@echo "Making $@"
	@mkdir -p "$@"
	@mkdir -p "$@/bin"

disk.%.ext2: disk.%.tar fs_%
	genext2fs -a $< --squash --size-in-blocks 16384 --block-size 4096 -m 0 --number-of-inodes 4096 $@

disk.%.tar: fs_%
	tar --format ustar --exclude .DS_Store -cf $@ -C $< .

.PHONY: clean
clean:
	rm -f $(DEST_DISKS)

