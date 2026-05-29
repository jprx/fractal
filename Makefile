# Fractal Kernel
# MIT License
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# Usage: `make` $(ARCH) VARIANT=$(VARIANT) BOARD=$(BOARD)
#  - VARIANT is one of: "RELEASE" or "DEBUG" (default: RELEASE)
#  - BOARD is one of the possible boards (see README.md) (default: QEMU)
#  - ARCH is one of: "x86" "arm" or "rv" (default: all)
# By default, we build all 3 architectures for qemu.
# You can omit $(ARCH) if you want, and by default all compatible architectures for that board will be built.

SUPPORTED_ARCHS:=x86 arm rv

# These variables are directly used in output file names, so they should be lowercase.
RASPI_BOARD_NAME:=raspi
APPLE_SI_BOARD_NAME:=apple
VIRT_BOARD_NAME:=qemu

SUPPORTED_BOARDS_x86:=$(VIRT_BOARD_NAME)
SUPPORTED_BOARDS_arm:=$(VIRT_BOARD_NAME) $(RASPI_BOARD_NAME) $(APPLE_SI_BOARD_NAME)
SUPPORTED_BOARDS_rv:=$(VIRT_BOARD_NAME)

SUPPORTED_FLAVORS=release debug kasan

# The current target variant of the kernel
# Override this in the make invocation, eg. by running `make x86 VARIANT=RELEASE`
# Possible options: "RELEASE" (all optimizations enabled) or "DEBUG" (best for debugging)
# These variables are defaults for user-supplied input, so they should be case-insensitive
VARIANT=release

# The board being compiled for.
# `qemu` is the default and is supported for all architectures.
# Override this in the make invocation eg. with `make arm BOARD=raspi`
BOARD=$(VIRT_BOARD_NAME)

# The SoC is the "kind" of board being compiled for,
# in the case where one board supports multiple kinds.
# We default to the "M1" as Apple Silicon is the only platform
# this applies to.
SOC=M1

# Are ASIDs enabled?
ifeq ($(CONFIG_USE_ASID),YES)
ASID_CONF:=-DCONFIG_USE_ASID
endif

# Where we save all the final kernel binaries
# Make sure to NOT set this to "." because then we may delete the entire project folder when cleaning up!
# OUTDIR is also a target that creates a fake directory tree in $OUTDIR/
# that matches every directory in $SRCDIR/
# This rule must be made before any source file is compiled to an object file,
# as the OBJS_$(1)_$(2) variable will use patsubst to try to place each object
# file in bin at its same location as it was in src.
# Eg. $(SRCDIR)/arch/x86/cpu.cc will be placed at $(OUTDIR)/arch/x86/cpu.$(ARCH).$(FLAVOR).o
# $(OUTDIR) is an order only prerequisite for every source file. This means it won't
# exhibit the bistable behavior where every other build rebuilds every source file because
# bin is newer and also a dependency of the file being built.
OUTDIR:=bin

# Where to find the source for the kernel
SRCDIR:=src

# Where to search for headers (and where to install auto-generated headers)
INCDIR:=inc

# Handle casing for all user-defined variables
# We want inputs to this Makefile to be case-insensitive,
# and we also have cases where we want specific casing on variables.
# So we define two variables for each user variable: $(THING)_LOWER and $(THING)_UPPER.
# We then only use either the _LOWER or _UPPER variant moving forward (never the raw $(THING)).
TOUPPER=$(shell echo '$1' | tr '[:lower:]' '[:upper:]')

TOLOWER=$(shell echo '$1' | tr '[:upper:]' '[:lower:]')

BOARD_LOWER:=$(call TOLOWER,$(BOARD))
BOARD_UPPER:=$(call TOUPPER,$(BOARD))
VARIANT_LOWER:=$(call TOLOWER,$(VARIANT))
VARIANT_UPPER:=$(call TOUPPER,$(VARIANT))
SOC_LOWER:=$(call TOLOWER,$(SOC))
SOC_UPPER:=$(call TOUPPER,$(SOC))

# Search in the supported boards lists to find which architectures the current board builds for
# This could return anything from "x86 arm rv" to ""
REQUIRED_ARCHS=$(strip $(foreach arch,$(SUPPORTED_ARCHS),$(if $(filter $(BOARD_LOWER),$(SUPPORTED_BOARDS_$(arch))),$(arch))))

ifneq ($(REQUIRED_ARCHS),)
all: $(REQUIRED_ARCHS)
else
$(error "Unsupported board: $(BOARD)")
endif

.PHONY: all
.PHONY: clean
clean: clean-implicit

KERNEL_NAME:=fractal

# TARGETBIN is the name of the final kernel binary being built
# Only define it for the current (flavor, board) tuple.
define DECLARE_TARGETBIN
$(1):   $$(OUTDIR)/$$(KERNEL_NAME).$$(VARIANT_LOWER).$$(BOARD_LOWER).$(1)
endef

$(foreach arch,$(SUPPORTED_ARCHS), \
	$(eval $(call DECLARE_TARGETBIN,$(arch))) \
)

ASFLAGS_EXTRA         = -x assembler-with-cpp
CXXFLAGS_EXTRA        = -fno-rtti -fno-exceptions -fno-threadsafe-statics

WARN_IGNORES    = -Wno-write-strings -Wno-address-of-packed-member -Wno-multichar

CFLAGS_COMMON   = -I$(INCDIR) -nostdlib -ffreestanding -fPIE -c -MMD $(WARN_IGNORES) -DBOARD_$(BOARD_UPPER) -DSOC_$(SOC_UPPER)
CFLAGS_release  = -O2 -g -DVARIANT_RELEASE
CFLAGS_debug    = -O0 -g -DVARIANT_DEBUG
CFLAGS_kasan    = -O0 -g -DVARIANT_KASAN -DKASAN -fsanitize=kernel-address

CFLAGS_x86      = -DARCH_X86 -mno-red-zone -masm=intel -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-ssse3 -msoft-float
CFLAGS_arm      = -DARCH_ARM -mgeneral-regs-only -mstrict-align $(ASID_CONF)
CFLAGS_rv       = -DARCH_RISCV -fno-pic -mcmodel=large

LDFLAGS1 =  -nostdlib -ffreestanding -fPIE
LDFLAGS2_COMMON  = -static-libgcc -lgcc -Wl,--no-warn-rwx-segments -Wl,--no-relax
LDFLAGS2_release =
LDFLAGS2_debug   =

# Only ARM kernels support full relocation (eg. linked with -pie)
# X86 can't be relocated due to stuff in start.s being non-position independent,
# and riscv is a whole other bag of worms I am not dealing with at the moment
LDFLAGS2_x86     =
LDFLAGS2_arm     = -Wl,-pie -Wl,-no-dynamic-linker
LDFLAGS2_rv      =

define CONSTRUCT_FLAGS_PER_ARCH
CFLAGS_$(2)_$(1) = $$(CFLAGS_COMMON) $$(CFLAGS_$(2)) $$(CFLAGS_$(1))
LDFLAGS2_$(2)_$(1) = $$(LDFLAGS2_COMMON) $$(LDFLAGS2_$(2)) $$(LDFLAGS2_$(1))
endef

$(foreach flavor,$(SUPPORTED_FLAVORS), $(foreach arch,$(SUPPORTED_ARCHS),$(eval $(call CONSTRUCT_FLAGS_PER_ARCH,$(arch),$(flavor)))))

# TOOL_PREFIX is the prefix appended to each tool name (eg. built by toolchain/mk_toolchain.sh)
x86:   TOOL_PREFIX:=x86_64-fractal-elf-
arm:   TOOL_PREFIX:=aarch64-fractal-elf-
rv:    TOOL_PREFIX:=riscv64-fractal-elf-

# Must prefix all target-dependent variables with the target-dependent prefix SUPPORTED_ARCHS
# Otherwise these variables are evaluated prior to ARCH, TOOL_PREFIX, etc. being properly defined
CC = $(TOOL_PREFIX)gcc
AS = $(TOOL_PREFIX)gcc
LD = $(TOOL_PREFIX)g++
CXX = $(TOOL_PREFIX)g++
OBJDUMP = $(TOOL_PREFIX)objdump
OBJCOPY = $(TOOL_PREFIX)objcopy
STRINGS = $(TOOL_PREFIX)strings
MKDIR = mkdir
RM = rm

# FIND_SOURCE(arch, flavor, board)
# Generates a variable binding the linker script for a given (arch, board) tuple.
# Grab all .c, .cc, or .s files in directories specified by target/$(ARCH).$(BOARD).target
# $(1) is the architecture being built for, $(2) is the variant, and $(3) is the board.
# GENERATORS is a list of C files that will be compiled and turned into header files
# before compiling any assembly files, stored in inc/gen, sourced from src/arch/$(arch)/gen.
# GENERATED_HEADERS is the output header files which must all be built before compiling
# GENERATED_OBJS is the temporary object files created before objcopy'ing them,
# stored in OUTDIR instead of INCDIR. INCDIR should only ever hold header files, even
# as intermediate products are being built.
# @NOTE: We assume that generated headers do not vary between boards for a given architecture.
# Make sure that is always the case going forward (it should be, as board configs only affect C++ object layout, no C structs).
define FIND_SOURCE
SRCS_$(1)_$(2)_$(3) := $(shell cat target/$(1).$(3).target | xargs -I {} find {} \( -name "*.c" -or -name "*.S" -or -name "*.s" -or -name "*.cc" \) -maxdepth 1)
SRCS_$(1)_$(2)_$(3) += $$(SRCDIR)/compiletimestuff/version_$(1)_$(2)_$(3).c
OBJS_$(1)_$(2)_$(3) = $$(patsubst $$(SRCDIR)/%,$$(OUTDIR)/%,$$(addsuffix .$(1).$(2).$(3).o, $$(basename $$(SRCS_$(1)_$(2)_$(3)))))
GENERATORS_$(1) := $(shell find src/arch/$(1)/gen -maxdepth 1 \( -name "*.c" \))
GENERATED_HEADERS_$(1) = $$(patsubst $$(SRCDIR)/arch/$(1)/gen/%,$$(INCDIR)/gen/$(1)_%,$$(addsuffix .h, $$(basename $$(GENERATORS_$(1)))))
GENERATED_OBJS_$(1) = $$(patsubst $$(SRCDIR)/%,$$(OUTDIR)/%,$$(addsuffix .o, $$(basename $$(GENERATORS_$(1)))))
endef

# FIND_LINKER(arch, flavor, board)
# Generates a variable binding the linker script for a given (arch, board) tuple.
define FIND_LINKER
LINKER_SCRIPT_$(1)_$(3) = target/$(1).$(3).ld
endef

define BUILD_C_OBJECT
$$(OUTDIR)/%.$(1).$(2).$(3).o: $$(SRCDIR)/%.c | $$(OUTDIR) $$(GENERATED_HEADERS_$(1))
	@echo "       \033[1;35mCC\033[0m   $(1)/$$(patsubst src/%,%,$$<)"
	@$$(CC) $$(CFLAGS_$(2)_$(1)) -o $$@ $$<
endef

define BUILD_ASM_OBJECT
$$(OUTDIR)/%.$(1).$(2).$(3).o: $$(SRCDIR)/%.s | $$(OUTDIR) $$(GENERATED_HEADERS_$(1))
	@echo "       \033[1;36mAS\033[0m   $(1)/$$(patsubst src/%,%,$$<)"
	@$$(AS) $$(CFLAGS_$(2)_$(1)) $$(ASFLAGS_EXTRA) -o $$@ $$<
endef

define BUILD_ASM_PREPROCESSED_OBJECT
$$(OUTDIR)/%.$(1).$(2).$(3).o: $$(SRCDIR)/%.S | $$(OUTDIR) $$(GENERATED_HEADERS_$(1))
	@echo "       \033[1;36mAS\033[0m   $(1)/$$(patsubst src/%,%,$$<)"
	@$$(AS) $$(CFLAGS_$(2)_$(1)) $$(ASFLAGS_EXTRA) -o $$@ $$<
endef

define BUILD_CXX_OBJECT
$$(OUTDIR)/%.$(1).$(2).$(3).o: $$(SRCDIR)/%.cc | $$(OUTDIR) $$(GENERATED_HEADERS_$(1))
	@echo "      \033[1;32mCXX\033[0m   $(1)/$$(patsubst src/%,%,$$<)"
	@$$(CXX) $$(CFLAGS_$(2)_$(1)) $$(CXXFLAGS_EXTRA) -o $$@ $$<
endef

define AUTOGENERATE_HEADERS
$$(INCDIR)/gen/$(1)_%.h: $$(OUTDIR)/arch/$(1)/gen/%.o | $$(OUTDIR)
	@echo "  \033[1;34mOBJCOPY\033[0m   $(1)/$$(patsubst src/%,%,$$<)"
	@$$(OBJCOPY) -O binary --only-section .text $$< $$(addsuffix .bin,$$(basename $$<))
	@echo "  \033[1;32mSTRINGS\033[0m   $(1)/$$@"
	@$$(STRINGS) $$(addsuffix .bin,$$(basename $$<)) > $$@

# Must not use -g here, otherwise aarch64 builds will complain about unaligned
# opcodes being inserted into an exec. section. Using .data doesn't fix it either,
# because other errors happen when generating debug info.
$$(OUTDIR)/arch/$(1)/gen/%.o: $$(SRCDIR)/arch/$(1)/gen/%.c | $$(OUTDIR)
	@echo "       \033[1;35mCC\033[0m   $(1)/$$(notdir $$<)"
	@$$(CC) $$(CFLAGS_COMMON) $$(CFLAGS_$(1)) -o $$@ $$<

# These empty rules exist to prevent Make from discarding the headers/ objects as
# unused intermediate files, since no explicit rule consumes them
$$(GENERATED_OBJS_$(1)):

$$(GENERATED_HEADERS_$(1)):

endef

# LINK_KERNEL(arch, flavor, board)
# The top-level build command that links the final kernel object.
define LINK_KERNEL
$$(OUTDIR)/$$(KERNEL_NAME).$(2).$(3).$(1): Makefile $$(LINKER_SCRIPT_$(1)_$(3)) $$(OBJS_$(1)_$(2)_$(3)) $$(SRCDIR)/compiletimestuff/version_$(1)_$(2)_$(3).c
	@echo "       \033[1;34mLD\033[0m   $$(notdir $$@)"
	@$$(LD) $$(LDFLAGS1) $$(OBJS_$(1)_$(2)_$(3)) -T $$(LINKER_SCRIPT_$(1)_$(3)) -o $$@ $$(LDFLAGS2_$(2)_$(1))
endef

# X86 requires one extra step- an objcopy to wrap the 64-bit kernel
# in a 32-bit ELF file so Qemu can actually load it (this sidesteps the PVH header thing)
X86_QEMU_HACK_EXTENSION=x86_qemu
x86:   $(OUTDIR)/$(KERNEL_NAME).$(VARIANT_LOWER).$(BOARD_LOWER).$(X86_QEMU_HACK_EXTENSION)
define FIXUP_X86_KERNEL
$$(OUTDIR)/$$(KERNEL_NAME).$(1).$(BOARD_LOWER).$$(X86_QEMU_HACK_EXTENSION): $$(OUTDIR)/$$(KERNEL_NAME).$(1).$(BOARD_LOWER).x86
	@echo "  \033[1;34mOBJCOPY\033[0m   $$(notdir $$@)"
	@$$(OBJCOPY) -O elf32-i386 $$(OUTDIR)/$$(KERNEL_NAME).$(1).$(BOARD_LOWER).x86 $$(OUTDIR)/$$(KERNEL_NAME).$(1).$(BOARD_LOWER).$$(X86_QEMU_HACK_EXTENSION)
endef

$(foreach flavor,$(SUPPORTED_FLAVORS), $(eval $(call FIXUP_X86_KERNEL,$(flavor))))

# Create Raspberry Pi bootable image
ifeq ($(BOARD_LOWER),$(RASPI_BOARD_NAME))

RASPBERRY_PI_EXTENSION=arm.img
arm:   $(OUTDIR)/$(KERNEL_NAME).$(VARIANT_LOWER).$(RASPI_BOARD_NAME).$(RASPBERRY_PI_EXTENSION)
define FIXUP_RPI_KERNEL
$$(OUTDIR)/$$(KERNEL_NAME).$(1).$(RASPI_BOARD_NAME).$$(RASPBERRY_PI_EXTENSION): $$(OUTDIR)/$$(KERNEL_NAME).$(1).$(RASPI_BOARD_NAME).arm
	@echo "  \033[1;34mOBJCOPY\033[0m   $$(notdir $$@)"
	@$$(OBJCOPY) -O binary $$(OUTDIR)/$$(KERNEL_NAME).$(1).$(RASPI_BOARD_NAME).arm $$(OUTDIR)/$$(KERNEL_NAME).$(1).$(RASPI_BOARD_NAME).$$(RASPBERRY_PI_EXTENSION)
endef

$(foreach flavor,$(SUPPORTED_FLAVORS), $(eval $(call FIXUP_RPI_KERNEL,$(flavor))))

endif # $(BOARD_LOWER) == $(RASPI_BOARD_NAME)

# Create Apple Silicon bootable image
ifeq ($(BOARD_LOWER),$(APPLE_SI_BOARD_NAME))

APPLESI_EXTENSION=arm.img
arm:   $(OUTDIR)/$(KERNEL_NAME).$(VARIANT_LOWER).$(APPLE_SI_BOARD_NAME).$(APPLESI_EXTENSION)
define FIXUP_APPLESI_KERNEL
$$(OUTDIR)/$$(KERNEL_NAME).$(1).$(APPLE_SI_BOARD_NAME).$$(APPLESI_EXTENSION): $$(OUTDIR)/$$(KERNEL_NAME).$(1).$(APPLE_SI_BOARD_NAME).arm
	@echo "  \033[1;34mOBJCOPY\033[0m   $$(notdir $$@)"
	@$$(OBJCOPY) -O binary $$(OUTDIR)/$$(KERNEL_NAME).$(1).$(APPLE_SI_BOARD_NAME).arm $$(OUTDIR)/$$(KERNEL_NAME).$(1).$(APPLE_SI_BOARD_NAME).$$(APPLESI_EXTENSION).tmp
	@echo "   \033[1;34mAPPEND\033[0m   filesys/disk.aarch64.ext2"
	@cat $$(OUTDIR)/$$(KERNEL_NAME).$(1).$(APPLE_SI_BOARD_NAME).$$(APPLESI_EXTENSION).tmp filesys/disk.aarch64.ext2 > $$(OUTDIR)/$$(KERNEL_NAME).$(1).$(APPLE_SI_BOARD_NAME).$$(APPLESI_EXTENSION)
	@$(RM) $$(OUTDIR)/$$(KERNEL_NAME).$(1).$(APPLE_SI_BOARD_NAME).$$(APPLESI_EXTENSION).tmp
endef

$(foreach flavor,$(SUPPORTED_FLAVORS), $(eval $(call FIXUP_APPLESI_KERNEL,$(flavor))))

endif # $(BOARD_LOWER) == $(APPLE_SI_BOARD_NAME)

define BUILD_VERSION_CFILE
.PHONY: $$(SRCDIR)/compiletimestuff/version_$(1)_$(2)_$(3).c
$$(SRCDIR)/compiletimestuff/version_$(1)_$(2)_$(3).c:
	@echo "  \033[1;34mTAGVERS\033[0m   $(1)/$$(basename $$(notdir $$@))"
	@mkdir -p $$(SRCDIR)/compiletimestuff
	@toolchain/mk_version.sh > $$(SRCDIR)/compiletimestuff/version_$(1)_$(2)_$(3).c
endef

.PHONY: clean-implicit
clean-implicit:
	@echo "       \033[1;32mRM\033[0m   $(OUTDIR)"
	@$(RM) -rf $(OUTDIR)
	@echo "       \033[1;32mRM\033[0m   gen"
	@$(RM) -rf $(INCDIR)/gen
	@echo "       \033[1;32mRM\033[0m   compiletimestuff"
	@$(RM) -rf $(SRCDIR)/compiletimestuff

.PHONY: $(OUTDIR)
$(OUTDIR):
	@echo "    \033[1;32mMKDIR\033[0m   $(OUTDIR)"
	@$(MKDIR) -p $(SRCDIR)/compiletimestuff
	@$(MKDIR) -p $(INCDIR)/gen
	@find $(SRCDIR) -type d | sed 's/$(SRCDIR)/$(OUTDIR)/' | xargs -I {} mkdir -p {}

define ADD_DEPENDENCY_FILE
-include $(OBJS_$(1)_$(2)_$(3):%.o=%.d)
endef

# EVALUATE_ALL_MACROS(arch, flavor, board)
# Invokes all the various macros to construct variables for this (arch, flavor, board) tuple.
define EVALUATE_ALL_MACROS
$$(eval $$(call FIND_SOURCE,$(1),$(2),$(3)))
$$(eval $$(call FIND_LINKER,$(1),$(2),$(3)))
$$(eval $$(call BUILD_C_OBJECT,$(1),$(2),$(3)))
$$(eval $$(call BUILD_ASM_OBJECT,$(1),$(2),$(3)))
$$(eval $$(call BUILD_ASM_PREPROCESSED_OBJECT,$(1),$(2),$(3)))
$$(eval $$(call BUILD_CXX_OBJECT,$(1),$(2),$(3)))
$$(eval $$(call AUTOGENERATE_HEADERS,$(1),$(2),$(3)))
$$(eval $$(call LINK_KERNEL,$(1),$(2),$(3)))
$$(eval $$(call BUILD_VERSION_CFILE,$(1),$(2),$(3)))
$$(eval $$(call ADD_DEPENDENCY_FILE,$(1),$(2),$(3)))
endef

$(foreach flavor,$(SUPPORTED_FLAVORS), \
	$(foreach arch,$(SUPPORTED_ARCHS), \
		$(foreach board,$(SUPPORTED_BOARDS_$(arch)), \
			$(eval $(call EVALUATE_ALL_MACROS,$(arch),$(flavor),$(board))) \
		) \
	) \
)
