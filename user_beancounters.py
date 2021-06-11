#!/usr/bin/env python3
#
# Script to emulate deprecated /proc/user_beancounter output.
#
# Copyright (c) 2021 Virtuozzo International GmbH. All rights reserved.
#

import os
import re


ve_uid_len = len('00000000-0000-0000-0000-000000000000')
cg_prefix = '/sys/fs/cgroup'
unlimited = str(0x7fffffffffffffff)
page_size = 4096


class Resource():
    '''
    This is general beancounter resource representation class. It contains
    "default" fill logic for held/maxheld/limit/barrier/failcnt fields and
    some useful helper methods.
    '''
    def __init__(self, resource, ve):
        self.resource = resource
        self.ve = ve

        self.held = None
        self.set_held()
        if self.held == None:
            Resource.set_held(self)

        self.maxheld = None
        self.set_maxheld()
        if self.maxheld == None:
            Resource.set_maxheld(self)

        self.limit = None
        self.set_limit()
        if self.limit == None:
            Resource.set_limit(self)

        self.barrier = None
        self.set_barrier()
        if self.barrier == None:
            Resource.set_barrier(self)

        self.failcnt = None
        self.set_failcnt()
        if self.failcnt == None:
            Resource.set_failcnt(self)

    def set_held(self):
        ''' By default set the resource usage to 0 '''
        self.held = '0'

    def set_maxheld(self):
        ''' By default set the resource max usage to usage '''
        self.maxheld = self.held

    def set_limit(self):
        ''' By default set the resource limit to unlimited '''
        self.limit = unlimited

    def set_barrier(self):
        ''' By default set the resource barrier to limit '''
        self.barrier = self.limit

    def set_failcnt(self):
        ''' By default set the resource fail count to 0 '''
        self.failcnt = '0'

    def bytes_to_pages(self, byte):
        return str(int(byte) // page_size)

    def pages_to_bytes(self, pages):
        return str(int(pages) * page_size)

    def file_get(self, path, pattern=None):
        '''
        Get content from the file.
        Pattern can be Null or a python "re"-regexp string describing the line
        in file from which to get content, it should have at least one group
        '()' describing the content we want to get.
        '''
        result = None
        try:
            with open(path) as f:
                if pattern:
                    for line in f:
                        m = re.match(pattern, line)
                        if m and len(m.groups()):
                            result = m.groups()[0]
                else:
                    result = f.read().strip()
        except FileNotFoundError:
            pass

        return result

    def cg_get(self, cg, fname, pattern=None):
        ''' Get content of cgroup file in container-root cgroup '''
        ve_cg_path = cg_prefix + '/' + cg + '/'
        if self.ve != None:
            ve_cg_path += 'machine.slice/' + self.ve + '/'
        ve_cg_path += fname

        return self.file_get(ve_cg_path, pattern=pattern)

    def __repr__(self):
        ''' Beancounter resource print format '''
        return f'{self.resource:<12s} {self.held:>20s} {self.maxheld:>20s} {self.barrier:>20s} {self.limit:>20s} {self.failcnt:>20s}'

    def print(self, print_ve=False):
        ''' Print it like in /proc/user_beancounters file '''
        if print_ve:
            ve = self.ve
            if ve == None:
                ve = '0'
            sep = ':'
        else:
            ve = ''
            sep = ' '

        print(f'{ve:>{ve_uid_len}}{sep} {self}')


'''
Each non-"fake" resource has it's own fill logic in overriding methods, we only
override methods for fields that are available on VZ8
'''
class KmemsizeResource(Resource):
    def __init__(self, ve):
        super().__init__("kmemsize", ve)

    def set_held(self):
        self.held = self.cg_get('memory', 'memory.kmem.usage_in_bytes')

    def set_maxheld(self):
        self.maxheld = self.cg_get('memory', 'memory.kmem.max_usage_in_bytes')

    def set_limit(self):
        self.limit = self.cg_get('memory', 'memory.kmem.limit_in_bytes')

    def set_failcnt(self):
        self.failcnt = self.cg_get('memory', 'memory.kmem.failcnt')


class ShmpagesResource(Resource):
    def __init__(self, ve):
        super().__init__("shmpages", ve)

    def set_held(self):
        held_bytes = self.cg_get('memory', 'memory.stat', 'total_shmem (.*)')
        if held_bytes != None:
            self.held = self.bytes_to_pages(held_bytes)


class NumprocResource(Resource):
    def __init__(self, ve):
        super().__init__("numproc", ve)

    def set_held(self):
        self.held = self.cg_get('pids', 'pids.current')

    def set_limit(self):
        self.limit = self.cg_get('pids', 'pids.max')


class PhyspagesResource(Resource):
    def __init__(self, ve):
        super().__init__("physpages", ve)

    def set_held(self):
        held_bytes = self.cg_get('memory', 'memory.usage_in_bytes')
        if held_bytes != None:
            self.held = self.bytes_to_pages(held_bytes)

    def set_maxheld(self):
        maxheld_bytes = self.cg_get('memory', 'memory.max_usage_in_bytes')
        if maxheld_bytes != None:
            self.maxheld = self.bytes_to_pages(maxheld_bytes)

    def set_limit(self):
        limit_bytes = self.cg_get('memory', 'memory.limit_in_bytes')
        if limit_bytes != None:
            self.limit = self.bytes_to_pages(limit_bytes)

    def set_failcnt(self):
        self.failcnt = self.cg_get('memory', 'memory.failcnt')


class OomguarpagesResource(Resource):
    def __init__(self, ve):
        super().__init__("oomguarpages", ve)

    def set_held(self):
        held_bytes = self.cg_get('memory', 'memory.memsw.usage_in_bytes')
        if held_bytes != None:
            self.held = self.bytes_to_pages(held_bytes)

    def set_maxheld(self):
        maxheld_bytes = self.cg_get('memory', 'memory.memsw.max_usage_in_bytes')
        if maxheld_bytes != None:
            self.maxheld = self.bytes_to_pages(maxheld_bytes)

    def set_limit(self):
        oom_guarantee = self.cg_get('memory', 'memory.oom_guarantee')
        if oom_guarantee != None:
            self.limit = self.bytes_to_pages(oom_guarantee)

    def set_failcnt(self):
        self.failcnt = self.cg_get('memory', 'memory.stat', 'total_oom (.*)')


class DcachesizeResource(Resource):
    def __init__(self, ve):
        super().__init__("dcachesize", ve)

    def set_held(self):
        if self.ve != None:
            held = self.cg_get('memory', 'memory.kmem.slabinfo', 'dentry.*: slabdata\s*\S*\s*(\S*)\s.*')
        else:
            held = self.file_get('/proc/slabinfo', 'dentry.*: slabdata\s*\S*\s*(\S*)\s.*')

        if held != None:
            self.held = self.pages_to_bytes(held)


class SwappagesResource(Resource):
    ''' This one is not shown as UB_SWAPPAGES >= UB_RESOURCES_COMPAT '''
    def __init__(self, ve):
        super().__init__("swappages", ve)

    def set_held(self):
        memsw_held_bytes = self.cg_get('memory', 'memory.memsw.usage_in_bytes')
        memory_held_bytes = self.cg_get('memory', 'memory.usage_in_bytes')
        if memsw_held_bytes != None and memory_held_bytes != None:
            swap_held_bytes = int(memsw_held_bytes) - int(memory_held_bytes)
            if swap_held_bytes >= 0:
                self.held = self.bytes_to_pages(swap_held_bytes)

    def set_limit(self):
        limit_bytes = self.cg_get('memory', 'memory.memsw.limit_in_bytes')
        if limit_bytes != None:
            self.limit = self.bytes_to_pages(limit_bytes)

    def set_failcnt(self):
        self.failcnt = self.cg_get('memory', 'memory.memsw.failcnt')


def ve_print_beancounters(ve):
    KmemsizeResource(ve).print(print_ve=True)

    # Fake
    Resource("lockedpages", ve).print()

    # Fake
    Resource("privvmpages", ve).print()

    ShmpagesResource(ve).print()

    # Fake
    Resource("dummy", ve).print()

    NumprocResource(ve).print()

    PhyspagesResource(ve).print()

    # Fake
    Resource("vmguarpages", ve).print()

    OomguarpagesResource(ve).print()

    # Fake
    Resource("numtcpsock", ve).print()

    # Fake
    Resource("numflock", ve).print()

    # Fake
    Resource("numpty", ve).print()

    # Fake
    Resource("numsiginfo", ve).print()

    # Fake
    Resource("tcpsndbuf", ve).print()

    # Fake
    Resource("tcprcvbuf", ve).print()

    # Fake
    Resource("othersockbuf", ve).print()

    # Fake
    Resource("dgramrcvbuf", ve).print()

    # Fake
    Resource("numothersock", ve).print()

    DcachesizeResource(ve).print()

    # Fake
    Resource("numfile", ve).print()

    # Fake
    Resource("dummy", ve).print()

    # Fake
    Resource("dummy", ve).print()

    # Fake
    Resource("dummy", ve).print()

    # Fake
    Resource("numiptent", ve).print()


if __name__ == '__main__':
    virtual_environments = [f.path.split('/')[-1] for f in os.scandir('/sys/fs/cgroup/ve') if f.is_dir()]

    print('Version: 2.5')
    print(f'{"uid":>{ve_uid_len}}  {"resource":<12s} {"held":>20s} {"maxheld":>20s} {"barrier":>20s} {"limit":>20s} {"failcnt":>20s}')

    for ve in virtual_environments:
        ve_print_beancounters(ve)
    ve_print_beancounters(None)
