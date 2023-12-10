FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# KBUILD_DEFCONFIG = "i386_defconfig"
# KBUILD_DEFCONFIG:qemux86-64 = "x86_64_defconfig"

SRC_URI:append = " file://0001-feat-add-boot-message.patch"

SRC_URI:append = " file://modify-hostname.cfg"


