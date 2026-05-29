#!/bin/bash
# mk_toolchain
# This bootstraps the Fractal toolchain: binutils, the compiler, libc, and debuggers
# This builds for all architectures supported by Fractal
set -o errexit
set -o nounset
set -o pipefail

export BINUTILS_VERSION="binutils-2.42"
export GCC_VERSION="gcc-14.1.0"
export GDB_VERSION="gdb-15.1"
export AUTOMAKE_VERSION="automake-1.15.1"
export AUTOCONF_VERSION="autoconf-2.69"
export NEWLIB_VERSION="newlib-4.4.0"
export MAKE_VERSION="make-4.4.1"
export GMP_VERSION="gmp-6.2.1"
export MPFR_VERSION="mpfr-4.1.0"
export GENEXT2FS_VERSION="v1.5.0"
export INSTALL_PREFIX="${PWD}/out"
# BUILDTOOLS is used for make, autoconf, automake that are used just when building things
# as part of this script. This way you can add out/bin to your path without changing
# your system make, autoconf, automake, etc.
export BUILDTOOLS_PREFIX="${PWD}/buildtools"

export GRUB_VERS="grub-2.12"
export GRUB_EXT="tar.gz"
export GRUB_URL="ftp://ftp.gnu.org/gnu/grub/${GRUB_VERS}.${GRUB_EXT}"

export TERMCAP_VERS="termcap-1.3.1"
export TERMCAP_EXT="tar.gz"
export TERMCAP_URL="https://ftp.gnu.org/gnu/termcap/${TERMCAP_VERS}.${TERMCAP_EXT}"
export TERMCAP_PATCH="$PWD/termcap.patch"

export BINUTILS_URL="https://ftp.gnu.org/gnu/binutils/${BINUTILS_VERSION}.tar.xz"
export GCC_URL="https://ftp.gnu.org/gnu/gcc/${GCC_VERSION}/${GCC_VERSION}.tar.xz"
export GDB_URL="https://ftp.gnu.org/gnu/gdb/${GDB_VERSION}.tar.xz"
export AUTOMAKE_URL="https://ftp.gnu.org/gnu/automake/${AUTOMAKE_VERSION}.tar.xz"
export AUTOCONF_URL="https://ftp.gnu.org/gnu/autoconf/${AUTOCONF_VERSION}.tar.xz"
export MAKE_URL="https://ftp.gnu.org/gnu/make/${MAKE_VERSION}.tar.gz"
export GMP_URL="https://ftp.gnu.org/gnu/gmp/${GMP_VERSION}.tar.xz"
export MPFR_URL="https://ftp.gnu.org/gnu/mpfr/${MPFR_VERSION}.tar.xz"
export NEWLIB_REPO="https://sourceware.org/git/newlib-cygwin.git"
export GENEXT2FS_REPO="https://github.com/bestouff/genext2fs.git"
export PATH="${INSTALL_PREFIX}/bin:${BUILDTOOLS_PREFIX}/bin:$PATH"

export SRCDIR="$PWD/src"
export WORKDIR="$PWD/build"
export REDZONE_PATCH="$PWD/redzone.patch"
export NEWLIB_PATCH="$PWD/newlib.patch"

export GCC_RISCV_ELF_PATH="gcc/config/riscv/elf.h"
export GCC_RISCV_ELF_PATCH="$PWD/gcc.patch"

# fix zlib fdopen bug on macOS.
# This affects binutils, gcc, and gdb.
# see: https://github.com/madler/zlib/commit/4bd9a71f3539b5ce47f0c67ab5e01f3196dc8ef9
export ZLIB_BINUTILS_PATCH="$PWD/zutil_binutils.patch"
export ZLIB_GCC_PATCH="$PWD/zutil_gcc.patch"
export ZLIB_ZUTIL_PATH="zlib/zutil.h"

export FRACTAL_LIBC="${PWD}/../libc"
export NEWLIB_LIBC="${SRCDIR}/newlib/newlib/libc/sys"

setup_dirs() {
    mkdir -p "${SRCDIR}"
    mkdir -p "${WORKDIR}"
    mkdir -p "${INSTALL_PREFIX}"
}

# Make is distributed as a tar.gz, not tar.xz like the others,
# so just use a bespoke routine for fetching make's source
# instead of using get_project
get_make() {
    if [[ -d "${SRCDIR}/${MAKE_VERSION}" ]]; then
        return
    fi

    cd "${SRCDIR}" || exit
    wget "${MAKE_URL}"
    tar -xf "${SRCDIR}/${MAKE_VERSION}.tar.gz"
    rm "${SRCDIR}/${MAKE_VERSION}.tar.gz"
}

# get_project
# Downloads a project from a project URL to the sourcedir
# Expects the file to be $(PROJECT_VERSION).tar.xz
# Argument 1: the project version
# Argument 2: the project URL
get_project() {
    local PROJECT_VERSION="${1}"
    local PROJECT_URL="${2}"
    if [[ -d "${SRCDIR}/${PROJECT_VERSION}" ]]; then
        return
    fi

    cd "${SRCDIR}" || exit
    wget "${PROJECT_URL}"
    tar -xf "${SRCDIR}/${PROJECT_VERSION}.tar.xz"
    rm "${SRCDIR}/${PROJECT_VERSION}.tar.xz"
}

# get_tarball_src
# Argument 1: the project name + version (eg. "coreutils-9.5")
# Argument 2: the project URL (eg. "https://ftp.gnu.org/gnu/coreutils/coreutils-9.5.tar.xz")
# Argument 3: the project extension (eg. "tar.xz")
get_tarball_src() {
    local PROJECT_VERSION="${1}"
    local PROJECT_URL="${2}"
    local PROJECT_EXT="${3}"
    if [[ -d "${SRCDIR}/${PROJECT_VERSION}" ]]; then
        return
    fi

    cd "${SRCDIR}" || exit
    wget "${PROJECT_URL}"
    tar -xf "${SRCDIR}/${PROJECT_VERSION}.${PROJECT_EXT}"
    rm "${SRCDIR}/${PROJECT_VERSION}.${PROJECT_EXT}"
}

get_automake() {
    get_project "${AUTOMAKE_VERSION}" "${AUTOMAKE_URL}"
}

get_autoconf() {
    get_project "${AUTOCONF_VERSION}" "${AUTOCONF_URL}"
}

get_binutils() {
    get_project "${BINUTILS_VERSION}" "${BINUTILS_URL}"
    patch_binutils
}

get_gdb() {
    get_project "${GDB_VERSION}" "${GDB_URL}"

    # GDB depends on GMP and MPFR as well
    # Putting them in the GDB tree will build them with GDB
    if [[ ! -d "${SRCDIR}/${GDB_VERSION}/gmp" ]]; then
        cd "${SRCDIR}/${GDB_VERSION}"
        wget "${GMP_URL}"
        tar -xf "${GMP_VERSION}.tar.xz"
        rm "${GMP_VERSION}.tar.xz"
        mv "${GMP_VERSION}" "gmp"
    fi
    if [[ ! -d "${SRCDIR}/${GDB_VERSION}/mpfr" ]]; then
        cd "${SRCDIR}/${GDB_VERSION}"
        wget "${MPFR_URL}"
        tar -xf "${MPFR_VERSION}.tar.xz"
        rm "${MPFR_VERSION}.tar.xz"
        mv "${MPFR_VERSION}" "mpfr"
    fi

    patch_gdb
}

get_gcc() {
    get_project "${GCC_VERSION}" "${GCC_URL}"

    # Download all GCC dependencies (GMP, MPFR, MPC, and ISL), in case they aren't installed
    # Two of these dependencies are reused by GDB, but it will download its own copy
    # So that you can build GDB without GCC and vice versa
    cd "${SRCDIR}/${GCC_VERSION}"
    ./contrib/download_prerequisites

    patch_gcc
}

# Fixup redzone for x86_64 targets and LIB_SPEC for riscv64 targets
patch_gcc() {
    cd "${SRCDIR}/${GCC_VERSION}"
    if git apply --check "${REDZONE_PATCH}" > /dev/null 2>&1; then
        git apply "${REDZONE_PATCH}"
    fi

    if patch -f --dry-run "${GCC_RISCV_ELF_PATH}" "${GCC_RISCV_ELF_PATCH}" > /dev/null 2>&1; then
        patch -f "${GCC_RISCV_ELF_PATH}" "${GCC_RISCV_ELF_PATCH}"
    fi

    if patch -f --dry-run "${ZLIB_ZUTIL_PATH}" "${ZLIB_GCC_PATCH}" > /dev/null 2>&1; then
        patch -f "${ZLIB_ZUTIL_PATH}" "${ZLIB_GCC_PATCH}"
    fi
}

patch_binutils() {
    cd "${SRCDIR}/${BINUTILS_VERSION}"

    if patch -f --dry-run "${ZLIB_ZUTIL_PATH}" "${ZLIB_BINUTILS_PATCH}" > /dev/null 2>&1; then
        patch -f "${ZLIB_ZUTIL_PATH}" "${ZLIB_BINUTILS_PATCH}"
    fi
}

patch_gdb() {
    cd "${SRCDIR}/${GDB_VERSION}"

    # note that this is the same patch as binutils:
    # gdb and binutils share a zlib patch file
    if patch -f --dry-run "${ZLIB_ZUTIL_PATH}" "${ZLIB_BINUTILS_PATCH}" > /dev/null 2>&1; then
        patch -f "${ZLIB_ZUTIL_PATH}" "${ZLIB_BINUTILS_PATCH}"
    fi
}

get_newlib() {
    if [[ -d "${SRCDIR}/newlib" ]]; then
        return
    fi

    cd "${SRCDIR}" || exit
    git clone "${NEWLIB_REPO}" --branch "${NEWLIB_VERSION}" --depth 1 newlib
}

get_genext2fs() {
    if [[ -d "${SRCDIR}/genext2fs" ]]; then
        return
    fi

    cd "${SRCDIR}" || exit
    git clone "${GENEXT2FS_REPO}" --branch "${GENEXT2FS_VERSION}" --depth 1 genext2fs
}

build_binutils() {
    local TARGET="${1}"
    if [[ -f "${INSTALL_PREFIX}/bin/${TARGET}-as" ]]; then
        echo "${TARGET}-binutils already built"
        return
    fi

    get_binutils
    mkdir -p "${WORKDIR}/build-binutils-${TARGET}"
    cd "${WORKDIR}/build-binutils-${TARGET}" || exit
    "${SRCDIR}/${BINUTILS_VERSION}/configure" --target="${TARGET}" --prefix="${INSTALL_PREFIX}" --with-sysroot="${INSTALL_PREFIX}" --with-newlib=yes --disable-nls --disable-werror
    make -j"$(nproc)"
    make install
}

build_gcc_round1() {
    local TARGET="${1}"
    if [[ -f "${INSTALL_PREFIX}/bin/${TARGET}-gcc" ]]; then
        echo "${TARGET}-gcc already built"
        return
    fi

    get_gcc
    mkdir -p "${WORKDIR}/build-gcc-${TARGET}"
    cd "${WORKDIR}/build-gcc-${TARGET}" || exit
    "${SRCDIR}/${GCC_VERSION}/configure" --target="${TARGET}" --prefix="${INSTALL_PREFIX}" --with-sysroot="${INSTALL_PREFIX}" --enable-languages=c,c++ --disable-nls \
                        --with-newlib=yes --disable-shared --disable-multilib --disable-threads --disable-libatomic --disable-libgomp \
                        --disable-libquadmath --disable-libssp --disable-libvtv --disable-tls --disable-libgloss
    make -j"$(nproc)" all-gcc
    make install-gcc
}

build_gcc_round2() {
    local TARGET="${1}"
    if [[ "$(find "${INSTALL_PREFIX}/lib/gcc/${TARGET}" -name '*libgcc*' | wc -l )" -gt 0 ]]; then
        echo "${TARGET}-libgcc already built"
        return
    fi

    cd "${WORKDIR}/build-gcc-${TARGET}" || exit

    # Make just the things we need (see gcc/configre:2846 target_libraries variable)
    make -j"$(nproc)" all-target-libgcc
    make install-target-libgcc
    make -j"$(nproc)" all-target-libstdc++-v3
    make install-target-libstdc++-v3
}

build_gdb() {
    local TARGET="${1}"
    if [[ -f "${INSTALL_PREFIX}/bin/${TARGET}-gdb" ]]; then
        echo "${TARGET}-gdb already built"
        return
    fi

    get_gdb
    mkdir -p "${WORKDIR}/build-gdb-${TARGET}"
    cd "${WORKDIR}/build-gdb-${TARGET}" || exit
    "${SRCDIR}/${GDB_VERSION}/configure" --target="${TARGET}" --prefix="${INSTALL_PREFIX}" --enable-multilib --disable-nls
    make -j"$(nproc)"
    make install
}

build_autoconf() {
    if [[ -f "${BUILDTOOLS_PREFIX}/bin/autoconf" ]]; then
        echo "autoconf already built"
        return
    fi

    get_autoconf
    mkdir -p "${WORKDIR}/build-autoconf"
    cd "${WORKDIR}/build-autoconf" || exit
    "${SRCDIR}/${AUTOCONF_VERSION}/configure" --prefix="${BUILDTOOLS_PREFIX}" --disable-nls
    make -j"$(nproc)"
    make install
}

build_automake() {
    if [[ -f "${BUILDTOOLS_PREFIX}/bin/automake" ]]; then
        echo "automake already built"
        return
    fi

    get_automake
    mkdir -p "${WORKDIR}/build-automake"
    cd "${WORKDIR}/build-automake" || exit
    "${SRCDIR}/${AUTOMAKE_VERSION}/configure" --prefix="${BUILDTOOLS_PREFIX}" --disable-nls
    make -j"$(nproc)"
    make install
}

check_if_newlib_exists() {
    local TARGET="${1}"
    if [[ -f "${INSTALL_PREFIX}/${TARGET}/lib/libc.a" ]]; then
        return 0
    fi

    return 1
}

build_newlib_no_check() {
    local TARGET="${1}"
    if [[ -f "${INSTALL_PREFIX}/${TARGET}/lib/libc.a" ]]; then
        echo "newlib already built, rebuilding anyways"
    fi

    get_newlib
    echo "Building newlib"
    echo "Using autoconf: " "$(which autoconf)"
    echo "Using automake: " "$(which automake)"
    mkdir -p "${WORKDIR}/build-newlib-${TARGET}"
    cd "${SRCDIR}/newlib" || exit
    if git apply --check "${NEWLIB_PATCH}" > /dev/null 2>&1; then
        git apply "${NEWLIB_PATCH}"
    fi
    copyin_fractal_libc
    cd "${SRCDIR}/newlib" || exit
    autoreconf
    cd "${SRCDIR}/newlib/newlib" || exit
    autoreconf
    cd "${WORKDIR}/build-newlib-${TARGET}" || exit
    # Need to disable libgloss, because crt0.S is defined for aarch64 and riscv64 in libgloss
    # When libgloss is enabled, linker gets confused and uses that crt0.S, instead of the one we wrote
    # in newlib/libc/sys/fractal_$(ARCH)/crt0.S
    "${SRCDIR}/newlib/configure" --target="${TARGET}" --prefix="${INSTALL_PREFIX}" --with-sysroot="${INSTALL_PREFIX}" --disable-libgloss --enable-newlib-multithread=no
    make -j
    make install
}

build_newlib() {
    if check_if_newlib_exists "${1}"; then
        echo "${1}-newlib already built"
        return
    fi
    build_newlib_no_check "${1}"
}

# We only build grub for the x86_64-elf target
build_grub() {
    if [[ -f "${BUILDTOOLS_PREFIX}/bin/grub-mkimage" ]]; then
        echo "grub already built"
        return
    fi
    get_tarball_src "${GRUB_VERS}" "${GRUB_URL}" "${GRUB_EXT}"

    # This file is missing from tarball and needs to be created
    # see: https://www.mail-archive.com/grub-devel@gnu.org/msg37958.html
    touch "${SRCDIR}/${GRUB_VERS}/grub-core/extra_deps.lst"

    mkdir -p "${WORKDIR}/build-${GRUB_VERS}"
    cd "${WORKDIR}/build-${GRUB_VERS}" || exit
    "${SRCDIR}/${GRUB_VERS}/configure" --prefix="${BUILDTOOLS_PREFIX}" --disable-nls --target="x86_64-fractal-elf"
    make -j"$(nproc)"
    make install
}

build_tools_for_target() {
    build_binutils "${1}"
    build_gcc_round1 "${1}"
    build_newlib "${1}"
    build_termcap "${1}"
    build_gcc_round2 "${1}"

    # Default to skipping GDB
    # build_gdb "${1}"

    echo "Done building ${1}"
}

build_autotools() {
    build_autoconf
    build_automake
}

build_genext2fs() {
    if [[ -f "${INSTALL_PREFIX}/bin/genext2fs" ]]; then
        echo "genext2fs already built"
        return
    fi

    get_genext2fs
    echo "Building genext2fs"
    cd "${SRCDIR}/genext2fs" || exit
    ./autogen.sh
    mkdir -p "${WORKDIR}/build-genext2fs"
    cd "${WORKDIR}/build-genext2fs" || exit
    "${SRCDIR}/genext2fs/configure" --prefix="${INSTALL_PREFIX}" --disable-debug --disable-dependency-tracking --disable-silent-rules
    make -j
    make install
}

build_make() {
    if [[ -f "${BUILDTOOLS_PREFIX}/bin/make" ]]; then
        echo "make already built"
        return
    fi

    get_make
    mkdir -p "${WORKDIR}/build-make"
    cd "${WORKDIR}/build-make" || exit
    "${SRCDIR}/${MAKE_VERSION}/configure" --prefix="${BUILDTOOLS_PREFIX}" --disable-nls
    make -j"$(nproc)"
    make install
}

install_termcap() {
    local TARGET="${1}"
    cp "${WORKDIR}/build-termcap-${TARGET}/libtermcap.a" "${INSTALL_PREFIX}/${TARGET}/lib/libtermcap.a"
    cp "${SRCDIR}/${TERMCAP_VERS}/termcap.h" "${INSTALL_PREFIX}/${TARGET}/include/termcap.h"
}

build_termcap() {
    local TARGET="${1}"

    if [[ -f "${INSTALL_PREFIX}/${TARGET}/lib/libtermcap.a" ]]; then
        echo "termcap already built"
        return
    fi
    get_tarball_src "${TERMCAP_VERS}" "${TERMCAP_URL}" "${TERMCAP_EXT}"
    cd "${SRCDIR}/${TERMCAP_VERS}" || exit
    if patch -f --dry-run < "${TERMCAP_PATCH}" > /dev/null 2>&1; then
        patch < "${TERMCAP_PATCH}"
    fi

    mkdir -p "${WORKDIR}/build-termcap-${TARGET}"
    cd "${WORKDIR}/build-termcap-${TARGET}" || exit

    # This project doesn't respect the configure options.
    # Instead, we force our cross compiler and tools with the CC, AR, etc. vars.
    CC="${TARGET}"-gcc AR="${TARGET}"-ar RANLIB="${TARGET}"-ranlib "${SRCDIR}/${TERMCAP_VERS}/configure" --prefix="${INSTALL_PREFIX}" --host="${TARGET}"
    make -j"$(nproc)"

    install_termcap "${TARGET}"
}

copyin_fractal_libc() {
    if [[ ! -d "${FRACTAL_LIBC}" ]]; then
        echo "${FRACTAL_LIBC} not found"
        exit 1
    fi

    if [[ ! -d "${FRACTAL_LIBC}/fractal" ]]; then
        echo "Fractal libc is missing the newlib support sys target"
        exit 1
    fi

    rm -rf "${NEWLIB_LIBC}/fractal"
    cp -r "${FRACTAL_LIBC}/fractal" "${NEWLIB_LIBC}/fractal"
}

rebuild_newlib() {
    local TARGET="${1}"

    # If the build directory is missing, rebuild the whole thing
    if [[ ! -d "${WORKDIR}/build-newlib-${TARGET}" ]]; then
        echo "No newlib builddir found for ${TARGET}, rebuilding whole thing"
        build_newlib_no_check "${TARGET}"
        return
    fi

    # Otherwise just run make
    echo "Rebuilding from cached newlib builddir"
    copyin_fractal_libc
    cd "${WORKDIR}/build-newlib-${TARGET}" || exit
    make -j
    make install
}

# Clean the entire newlib source tree, remove artifacts
# this will force a full rebuild of newlib
reset_newlib() {
    local TARGET="${1}"

    cd "${SRCDIR}/newlib" || exit

    if ! git rev-parse --is-inside-work-tree > /dev/null 2>&1; then
        echo "ERROR: ${PWD} is not a git repository"
        exit 1
    fi

    CURRENT_GIT_URL=$(git remote get-url origin)
    if [[ "${CURRENT_GIT_URL}" != "${NEWLIB_REPO}" ]]; then
        echo "ERROR: Attempted to run git clean on ${CURRENT_GIT_URL}, which is not the newlib repo"
        echo "Stopping now to prevent unwanted file deletion!"
        exit 1
    fi

    git clean -xdf
    rm -rf "${NEWLIB_LIBC}/fractal_arm"
    rm -rf "${NEWLIB_LIBC}/fractal_x86"
    rm -rf "${NEWLIB_LIBC}/fractal_rv"
    git reset --hard HEAD

    rm -rf "${WORKDIR}/build-newlib-${TARGET}"
    rm -f "${INSTALL_PREFIX}/${TARGET}/lib/libc.a"
    rm -f "${INSTALL_PREFIX}/${TARGET}/lib/crt0.o"
}

main() {
    # Ensure workdir, srcdir, and builddir exist:
    setup_dirs

    case "${1:-}" in
        "" | "apple")
            echo "Building only tools required for running on Apple Silicon"
            build_make
            build_autotools
            build_genext2fs
            build_tools_for_target "aarch64-fractal-elf"
            ;;
        "all")
            echo "Building everything"
            build_make
            build_autotools
            build_genext2fs
            build_tools_for_target "x86_64-fractal-elf"
            build_tools_for_target "aarch64-fractal-elf"
            build_tools_for_target "riscv64-fractal-elf"
            build_grub
            ;;
        "autotools")
            echo "Building autotools"
            build_autotools
            ;;
        "grub")
            echo "Building grub"
            build_grub
            ;;
        "make")
            echo "Building make"
            build_make
            ;;
        "newlib")
            echo "Building newlib"
            build_make
            build_autotools
            build_newlib "x86_64-fractal-elf"
            build_newlib "aarch64-fractal-elf"
            build_newlib "riscv64-fractal-elf"
            ;;
        "gdb")
            echo "Building gdb"
            build_gdb "x86_64-fractal-elf"
            build_gdb "aarch64-fractal-elf"
            build_gdb "riscv64-fractal-elf"
            ;;
        "copyin-libc")
            # Copy in fractal's libc into newlib, but don't compile it
            echo "Copying in the Fractal libc -> newlib"
            copyin_fractal_libc
            ;;
        "reset-newlib")
            echo "Cleaning and resetting newlib"
            reset_newlib "x86_64-fractal-elf"
            reset_newlib "aarch64-fractal-elf"
            reset_newlib "riscv64-fractal-elf"
            ;;
        "genext2fs")
            echo "Building genext2fs"
            build_genext2fs
            ;;
        "install-termcap")
            echo "Installing libtermcap"
            install_termcap "x86_64-fractal-elf"
            install_termcap "aarch64-fractal-elf"
            install_termcap "riscv64-fractal-elf"
            ;;
        *)
            echo "Invalid argument"
            ;;
    esac

    echo "Done"
}

main "$@"
