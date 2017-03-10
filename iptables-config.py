#!/usr/bin/python
#  Copyright (c) 1999-2017, Parallels International GmbH. All rights reserved.
#
#  This file is part of OpenVZ. OpenVZ is free software; you can redistribute
#  it and/or modify it under the terms of the GNU General Public License as
#  published by the Free Software Foundation; either version 2 of the License,
#  or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
#  02110-1301, USA.
#
#  Our contact details: Parallels International GmbH, Vordergasse 59, 8200
#  Schaffhausen, Switzerland.

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

