#!/usr/bin/python
# Copyright (c) 1999-2015 Parallels IP Holdings GmbH. All rights reserved.

import sys
from subprocess import call

try:
    from firewall.core.io.direct import Direct
except:
    # Firewall not installed.
    sys.exit(0)

if len(sys.argv) < 2 or sys.argv[1] not in ["add", "remove"]:
    print "Invalid action. Valid: add|remove."
    sys.exit(1)

FIREWALLD_DIRECT = "/etc/firewalld/direct.xml"
RULE_ARGS = ["-i", "venet0", "-j", "DROP"]

d = Direct(FIREWALLD_DIRECT)
try:
    d.read()
except:
    # Config not created.
    pass

for ip in ["ipv4", "ipv6"]:
    try:
        getattr(d, sys.argv[1] + "_rule")(ip, "filter", "INPUT_direct", 0, RULE_ARGS)
    except:
        # No rule in config on delete.
        pass

try:
    d.write()
except:
    # No access
    pass

for ip in ["ipv4", "ipv6"]:
    call(["/usr/bin/firewall-cmd", "--direct", "--" + sys.argv[1] + "-rule",
          ip, "filter", "INPUT_direct", "0"] + RULE_ARGS)

