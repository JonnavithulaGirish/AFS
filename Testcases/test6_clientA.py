#! /usr/bin/env python3

import os
import fs_util
import sys
import time
import signal
import subprocess
'''
This is ClientA.
'''

cs739_env_vars = [
    'CS739_CLIENT_A', 'CS739_CLIENT_B', 'CS739_SERVER', 'CS739_MOUNT_POINT'
]
ENV_VARS = {var_name: os.environ.get(var_name) for var_name in cs739_env_vars}
for env_var in ENV_VARS.items():
    print(env_var)
    assert env_var is not None
TEST_DATA_DIR = ENV_VARS['CS739_MOUNT_POINT'] + '/test_consistency'
FNAME = f'{TEST_DATA_DIR}/case6'
print(TEST_DATA_DIR)
TEST_CASE_NO = 6


def run_test():
    server = ENV_VARS['CS739_SERVER']
    assert fs_util.test_ssh_access(server)
    
    if not fs_util.path_exists(TEST_DATA_DIR):
        fs_util.mkdir(TEST_DATA_DIR)

    # init
    if not fs_util.path_exists(FNAME):
        fs_util.create_file(FNAME)

    init_str = fs_util.gen_str_by_repeat('0', 32768)
    fd = fs_util.open_file(FNAME)
    fs_util.write_file(fd, init_str)
    fs_util.close_file(fd)
    # open again
    fd = fs_util.open_file(FNAME)

    # kill Server
    fs_util.killServer(server, 50051) #port number

    #write should work
    cur_str = fs_util.gen_str_by_repeat('a', 100)
    fs_util.write_file(fd, cur_str, start_off=0)

    #read should retrive data
    cur_str = fs_util.read_file(fd, 32768)
    assert len(cur_str) == 32768
    for idx, rc in enumerate(cur_str):
        if idx < 100:
            assert rc == 'a'
        else:
            assert rc == '0'
    # done
    fs_util.record_test_result(TEST_CASE_NO, 'A', 'OK')
    print("donee")


if __name__ == '__main__':
    run_test()