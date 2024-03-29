#!/usr/bin/env python3

import os
import re
import pdb
import sys
import time
import fcntl
import select
import shutil
import socket
import logging
import unittest
import subprocess

from io import StringIO
from test_util.test_util import make_store

from baler import Bq as bq

STORE_PATH = "./store"
HOSTS = [ "node%05d" % i for i in range(1, 17) ]
MAKE_STORE = True

PATTERNS = [
    "Pattern Zero:",
    "Pattern One:",
    "Pattern Three:",
    "Pattern Four:",
    "Pattern Five:",
    "Pattern Six:",
    "Pattern Seven:",
    "Pattern Eight:",
    "Pattern Nine:",
    "Pattern Ten:",
    "Pattern Eleven:",
    "Pattern Twelve:",
    "Pattern Thirteen:",
    "Pattern Fourteen:",
    "Pattern Fifteen:",
    "Pattern Sixteen:",
    "Pattern Seventeen:",
    "Pattern Eighteen:",
    "Pattern Nineteen:",
    "Pattern Twenty:",
    "Pattern Twenty One:",
    "Pattern Twenty Two:",
    "Pattern Twenty Three:",
    "Pattern Twenty Four:",
    "Pattern Twenty Five:",
    "Pattern Twenty Six:",
    "Pattern Twenty Seven:",
    "Pattern Twenty Eight:",
    "Pattern Twenty Nine:",
    "Pattern Thirty:",
]

log = logging.getLogger(__name__)

class Debug(object): pass

DEBUG = Debug()

class icmd(object):
    """Interactive command"""
    def __init__(self, cmd = str()):
        self.proc = subprocess.Popen(cmd, shell=True,
                                     stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE,
                                     stderr=subprocess.STDOUT)
        fd = self.proc.stdout.fileno()
        fl = fcntl.fcntl(fd, fcntl.F_GETFL)
        fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)
        assert(self.is_running())
        self.poll = select.poll()
        self.poll.register(fd, select.POLLIN)

    def is_running(self):
        self.proc.poll()
        return self.proc.returncode == None

    def comm(self, text):
        sio = StringIO()
        text = text.rstrip()
        self.proc.stdin.write((text + '\n').encode())
        self.proc.stdin.flush()
        while self.poll.poll(1000):
            data = self.proc.stdout.read(4096)
            sio.write(data.decode())
        sio.seek(0)
        return [l.rstrip() for l in sio.readlines()]

    def term(self):
        if not self.is_running():
            return
        self.proc.stdin.close()
        self.proc.wait()

    def __del__(self):
        self.term()


class bclient(icmd):
    """Interactive bclient"""
    def __init__(self, path = None):
        cmd = "bclient"
        if path:
            cmd += " -p="+path
        super(bclient, self).__init__(cmd)


def RAW_MESSAGES():
    for h in HOSTS:
        for ptn in PATTERNS:
            msg = "<1>1 2016-12-31T01:02:03.456789-06:00 %s %s\n" % (h, ptn)
            yield msg


def cmd(*args):
    text = ' '.join(args)
    p = subprocess.Popen(text, shell=True, stdout=subprocess.PIPE,
                                           stderr=subprocess.STDOUT)
    (out, err) = p.communicate()
    return out

def bq_cmd(*args):
    _cmd = "bq -p " + STORE_PATH + ' ' + (' '.join(args))
    out = cmd(_cmd)
    return out

def parse_attrs(line0, line1):
    r = re.compile(r'\s+(\d+)')
    m = r.match(line0)
    if not m:
        raise ValueError('Bad line format: ' + line0)
    ptn_id = int(m.group(1))
    r = re.compile(r'ATTRIBUTES: \[(.*)\]')
    m = r.match(line1)
    if not m:
        raise ValueError('Bad line format: ' + line1)
    lst = m.groups()[0]
    ret = []
    if not lst:
        return ret
    r = re.compile(r"'(.*)=(.*)'")
    for av in lst.split(', '):
        m = r.match(av)
        (t, v) = m.groups()
        ret.append( (ptn_id, t, v) )
    return ret

class TestAttr(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        log.info("------- setUpClass -------")
        if not MAKE_STORE:
            return
        shutil.rmtree(STORE_PATH, ignore_errors = True)
        make_store(STORE_PATH, HOSTS, RAW_MESSAGES())
        bs = bq.Bstore()
        bs.open(STORE_PATH)
        # Create TAG
        self = cls
        bs.attr_new("TAG")
        bs.attr_new("ATTR0")
        bs.attr_new("ATTR1")
        pitr = bq.Bptn_iter(bs)
        # Add some tags `odd` or `even`
        for ptn in pitr:
            ptn_id = ptn.ptn_id()
            if ptn_id % 2:
                bs.ptn_attr_value_add(ptn_id, "TAG", "odd")
            else:
                bs.ptn_attr_value_add(ptn_id, "TAG", "even")
            # to demonstrate that we can have multiple TAG values
            if ptn_id % 3 == 0:
                bs.ptn_attr_value_add(ptn_id, "TAG", "three")
        bs.close()
        log.info("------- setUpClass COMPLETED -------")

    @classmethod
    def tearDownClass(cls):
        log.info("------- tearDownClass -------")

    def get_attrs(self, **fltr):
        bs = bq.Bstore()
        bs.open(STORE_PATH)
        itr = bq.Bptn_attr_iter(bs)
        itr.set_filter(**fltr)
        attrs = [ent.as_tuple() for ent in itr]
        bs.close()
        return attrs

    def test_001_bq_cmd_ptn_attr_list(self):
        lines = bq_cmd("--ptn", "--ptn_attr_list")
        sio = StringIO(lines.decode())
        hdr0 = sio.readline()
        hdr1 = sio.readline()
        attrs = []
        while True:
            l0 = sio.readline()
            l1 = sio.readline()
            if l0[0] == '-':
                # expecting footer
                self.assertRegex(l1, r'Messages\(s\)')
                break
            tmp = parse_attrs(l0, l1)
            attrs.extend(tmp)
        attrs.sort()
        comp = self.get_attrs()
        comp.sort()
        self.assertEqual(attrs, comp)

    def test_002_bq_cmd_query_ptn_attr(self):
        lines = bq_cmd("--ptn", "--match", "--ptn_attr", "TAG=odd")
        sio = StringIO(lines.decode())
        hdr0 = sio.readline()
        hdr1 = sio.readline()
        ptn_ids = set()
        while True:
            l = sio.readline()
            if l[0] == '-':
                break
            m = re.match(r'\s+(\d+)', l)
            if not m:
                raise ValueError("Bad line format: " + l)
            ptn_id = int(m.group(1))
            ptn_ids.add(ptn_id)
        comp = set([x[0] for x in self.get_attrs(attr_type = 'TAG',
                                                 attr_value = 'odd')])
        self.assertEqual(ptn_ids, comp)

    def test_003_bq_cmd_query_ptn_attr_tkn(self):
        lines = bq_cmd("--ptn", "--match", "--ptn_attr", "TAG=odd",
                       "--tkn_str", "node00001")
        sio = StringIO(lines.decode())
        hdr0 = sio.readline()
        hdr1 = sio.readline()
        ptn_ids = set()
        while True:
            l = sio.readline()
            sio.readline()
            if l[0] == '-':
                break
            m = re.match(r'\s+(\d+)', l)
            if not m:
                raise ValueError("Bad line format: " + l)
            ptn_id = int(m.group(1))
            ptn_ids.add(ptn_id)
        comp = set([x[0] for x in self.get_attrs(attr_type = 'TAG',
                                                 attr_value = 'odd')])
        self.assertEqual(ptn_ids, comp)

    def test_004_bq_cmd_ptn_attr_add_by_attr(self):
        ptn_id = 260
        lines = bq_cmd("--ptn_attr_add", "--ptn_id", str(ptn_id),
                       "--ptn_attr", "HEX=%s" % hex(ptn_id))
        attrs = self.get_attrs(attr_type = 'HEX')
        self.assertEqual(attrs, [(260, 'HEX', hex(260))] )

    def test_005_bq_cmd_ptn_attr_add_by_file(self):
        comp = [ (256, 'mark', 'a'),
                 (257, 'mark', 'b'),
                 (258, 'mark', 'c') ]
        f = open('attr.txt', 'w')
        for ent in comp:
            f.write('%d %s %s\n' % ent)
        f.close()
        bq_cmd("--ptn_attr_add", "--ptn_attr_file", 'attr.txt')
        attrs = self.get_attrs(attr_type = 'mark')
        comp.sort()
        attrs.sort()
        self.assertEqual(attrs, comp)

    def test_006_bq_cmd_ptn_attr_add_by_attr(self):
        ptn_id = 260
        lines = bq_cmd("--ptn_attr_rm", "--ptn_id", str(ptn_id),
                       "--ptn_attr", "HEX=%s" % hex(ptn_id))
        attrs = self.get_attrs(attr_type = 'HEX')
        self.assertEqual(attrs, [])

    def test_007_bq_cmd_ptn_attr_rm_by_file(self):
        bq_cmd("--ptn_attr_rm", "--ptn_attr_file", 'attr.txt')
        attrs = self.get_attrs(attr_type = 'mark')
        self.assertEqual(attrs, [])

    def test_008_000_bclient_ptn_attr_list(self):
        bc = bclient(STORE_PATH)
        out = bc.comm("ptn_attr_list")
        attrs = []
        _id = 0
        r = re.compile(r'^(?P<ID>\d+).*$|^    (?P<T>\w+)=(?P<V>.+)$')
        for line in out:
            m = r.match(line)
            self.assertIsNotNone(m)
            _tmp = m.group('ID')
            if _tmp:
                _id = int(_tmp)
            else:
                ent = (_id, m.group('T'), m.group('V'))
                attrs.append(ent)
        comp = self.get_attrs()
        self.assertEqual(set(attrs), set(comp))

    def test_008_001_bclient_ptn_attr_list_filter(self):
        bc = bclient(STORE_PATH)
        out = bc.comm("ptn_attr_list ids=256")
        attrs = []
        _id = 0
        r = re.compile(r'^(?P<ID>\d+).*$|^    (?P<T>\w+)=(?P<V>.+)$')
        for line in out:
            m = r.match(line)
            self.assertIsNotNone(m)
            _tmp = m.group('ID')
            if _tmp:
                _id = int(_tmp)
            else:
                ent = (_id, m.group('T'), m.group('V'))
                attrs.append(ent)
        comp = self.get_attrs(ptn_id=256)
        self.assertEqual(set(attrs), set(comp))

    def test_009_bclient_ptn_query_by_attr(self):
        bc = bclient(STORE_PATH)
        out = bc.comm("ptn_query attr=TAG=odd")
        ptn_ids = set([int(l.split(' ')[0]) for l in out])
        comp = set([x[0] for x in self.get_attrs(attr_type = 'TAG',
                                                 attr_value = 'odd')])
        self.assertEqual(ptn_ids, comp)

    def test_010_bclient_ptn_attr_add(self):
        bc = bclient(STORE_PATH)
        out = bc.comm("ptn_attr_add ids=256,257 attr=FLAG=first_two")
        ptn_ids = set([ x[0] for x in self.get_attrs(attr_type = 'FLAG',
                                                     attr_value = 'first_two')])
        self.assertEqual(ptn_ids, set([256, 257]))

        out = bc.comm("ptn_attr_add ids=256-258 attr=FLAG=first_three")
        ptn_ids = set([ x[0] for x in self.get_attrs(attr_type = 'FLAG',
                                                 attr_value = 'first_three')])
        self.assertEqual(ptn_ids, set([256, 257, 258]))

    def test_011_bclient_ptn_attr_rm(self):
        bc = bclient(STORE_PATH)
        out = bc.comm("ptn_attr_rm ids=256 attr=FLAG=first_two")
        ptn_ids = set([ x[0] for x in self.get_attrs(attr_type = 'FLAG',
                                                     attr_value = 'first_two')])
        self.assertEqual(ptn_ids, set([257]))

    def test_012_bclient_ptn_attr_set(self):
        bc = bclient(STORE_PATH)
        out = bc.comm("ptn_attr_set ids=257 attr=FLAG=new")
        ptn_ids = set([ x[0] for x in self.get_attrs(attr_type = 'FLAG',
                                                     attr_value = 'first_two')])
        self.assertEqual(ptn_ids, set([]))
        ptn_ids = set([ x[0] for x in self.get_attrs(attr_type = 'FLAG',
                                                     attr_value = 'first_three')])
        self.assertEqual(ptn_ids, set([256, 258]))
        ptn_ids = set([ x[0] for x in self.get_attrs(attr_type = 'FLAG',
                                                     attr_value = 'new')])
        self.assertEqual(ptn_ids, set([257]))

    def test_013_bq_ptn_attr_list(self):
        """bq -p STORE --ptn_attr_list"""
        lines = bq_cmd("--ptn_attr_list")
        attrs = set(b.decode() for b in lines.splitlines())
        bs = bq.Bstore()
        bs.open(STORE_PATH)
        itr = bq.Battr_iter(bs)
        comp = set(x for x in itr)
        del itr
        bs.close()
        self.assertEqual(attrs, comp)

    def test_014_bq_ptn_attr_list_v(self):
        """bq -p STORE --ptn_attr_list -v"""
        lines = bq_cmd("--ptn_attr_list", "-v")
        sio = StringIO(lines.decode())
        ents = []
        attr = None
        for l in sio.readlines():
            if l.startswith(' '):
                # value
                value = l.strip()
                ents.append((attr, value))
            else:
                # attr type
                attr = l.strip()[:-1] # remove the ':' suffix
        bs = bq.Bstore()
        bs.open(STORE_PATH)
        itr = bq.Bptn_attr_iter(bs)
        comp = set((x.attr_type(), x.attr_value()) for x in itr)
        del itr
        bs.close()
        self.assertEqual(len(ents), len(comp))
        self.assertEqual(set(ents), comp)

    def test_015_bq_ptn_attr_list_attr(self):
        """bq -p STORE --ptn_attr_list --ptn_attr TYPE"""
        lines = bq_cmd("--ptn_attr_list", "--ptn_attr", "TAG")
        sio = StringIO(lines.decode())
        ents = []
        attr = None
        for l in sio.readlines():
            if l.startswith(' '):
                # value
                value = l.strip()
                ents.append((attr, value))
            else:
                # attr type
                attr = l.strip()[:-1] # remove the ':' suffix
        bs = bq.Bstore()
        bs.open(STORE_PATH)
        itr = bq.Bptn_attr_iter(bs)
        itr.set_filter(attr_type = "TAG")
        comp = set((x.attr_type(), x.attr_value()) for x in itr)
        del itr
        bs.close()
        self.assertEqual(len(ents), len(comp))
        self.assertEqual(set(ents), comp)


if __name__ == "__main__":
    LOGFMT = "%(asctime)s %(name)s %(levelname)s: %(message)s"
    DATEFMT = "%F %T"
    logging.basicConfig(format=LOGFMT, datefmt=DATEFMT)
    log.setLevel(logging.INFO)
    # unittest.TestLoader.testMethodPrefix = "test_"
    MAKE_STORE = True
    unittest.main(failfast=True, verbosity=2)
