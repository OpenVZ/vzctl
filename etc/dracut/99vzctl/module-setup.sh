#!/bin/bash
# module-setup for vz modules workaround

# called by dracut
check() {
    return 0
}

# called by dracut
depends() {
    echo bash
    return 0
}

# called by dracut
install() {
    rm -f "$initdir/etc/modules-load.d/vz.conf" || :
}

