FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

hostname = "localhost"

do_configure:append() {
    system="MyPoky"
    version=$(date "+%Y%m%d")
    echo "${system} ${version}" > ${WORKDIR}/issue
    echo "${system} ${version}" > ${WORKDIR}/issue.net
}