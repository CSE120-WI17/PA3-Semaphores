#!/usr/bin/env python

from __future__ import print_function
from subprocess import Popen, PIPE
from sys import stdout
import os
import re

TEST_FILE = 'pa3d.c'
TEST_EXEC = 'pa3d'
TEST_DIR = 'tests'
TEST_BEG = '__TEST_START__'
TEST_END = '__TEST_END__'
TESTCASE_RE = re.compile(r'^[wex]+$')
RESULT_RE = re.compile(r'^Road Trace: (.*)$')
DEVNULL = open(os.devnull, 'wb')

def test(f):
    name = f.name.split(os.path.sep)[1]

    stdout.write('[%s] ' % name)
    stdout.flush()

    # Translate filename to expected
    expected = name.replace('w', '>').replace('e', '<').replace('x', '|')

    # Substitute in this file's contents
    with open(TEST_FILE, 'r') as inf:
        lines = inf.readlines()

    in_test = False
    with open(TEST_FILE, 'w') as outf:
        for line in lines:
            if TEST_END in line:
                in_test = False
            if in_test:
                continue
            outf.write(line)
            if TEST_BEG in line:
                in_test = True
                outf.write(f.read())

    # Run make and test
    stdout.write('making... ')
    stdout.flush()
    make_proc = Popen('make clean pa3'.split(), stdout=DEVNULL, stderr=DEVNULL)
    make_proc.communicate()
    stdout.write('running... ')
    stdout.flush()
    test_proc = Popen(('./%s' % TEST_EXEC).split(), stdout=PIPE)
    test_out, _ = test_proc.communicate()

    passed = False
    for line in test_out.splitlines():
        match = RESULT_RE.match(line.strip())
        if match:
            if match.group(1) == expected:
                passed = True
            break

    if passed:
        print('PASSED')
    else:
        print('FAILED')
        print('Expected: %s' % expected)
        print('Received: %s' % match.group(1))

for fn in os.listdir(TEST_DIR):
    path = os.path.join(TEST_DIR, fn)
    if os.path.isfile(path) and TESTCASE_RE.match(fn):
        with open(path) as f:
            test(f)
