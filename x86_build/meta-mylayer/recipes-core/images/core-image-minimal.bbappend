#poky/meta/classes/core-image.bbclass
#https://docs.yoctoproject.org/4.0.14/ref-manual/variables.html#term-FEATURE_PACKAGES
# 
IMAGE_FEATURES:append = " tools-debug"

IMAGE_INSTALL:append = " strace comperf"

IMAGE_INSTALL:append = " hello-mod"
