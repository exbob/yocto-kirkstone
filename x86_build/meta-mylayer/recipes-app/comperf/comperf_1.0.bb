DESCRIPTION = "Linux UART Serial test application"
SECTION = "application"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://comperf.c \
           file://Makefile \
           file://README.md \
           "

S = "${WORKDIR}"

EXTRA_OEMAKE = "CC='${CC}' CFLAGS='${CFLAGS}' LDCFLAGS='${LDFLAGS}' DESTDIR='${D}${bindir}'"

do_install() {
    oe_runmake install
}

FILES:${PN} = "${bindir}/comperf" 