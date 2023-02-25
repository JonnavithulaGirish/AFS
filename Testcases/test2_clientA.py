#! /usr/bin/env python3

import os
import fs_util
import sys
import time
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
FNAME = f'{TEST_DATA_DIR}/case2'
print(TEST_DATA_DIR)
TEST_CASE_NO = 2


def run_test():
    host_b = ENV_VARS['CS739_CLIENT_B']
    assert fs_util.test_ssh_access(host_b)
    signal_name_gen = fs_util.get_fs_signal_name()
    # print(signal_name_gen.gi_yieldfrom)
    if not fs_util.path_exists(TEST_DATA_DIR):
        fs_util.mkdir(TEST_DATA_DIR)

    # init
    if not fs_util.path_exists(FNAME):
        fs_util.create_file(FNAME)

    # A opens and writes "0" to Server  
    init_str =  fs_util.gen_str_by_repeat('0',32768)
    fd = fs_util.open_file(FNAME)
    fs_util.write_file(fd, init_str)
    fs_util.close_file(fd);

    #Open and write to local and not close the file
    fd = fs_util.open_file(FNAME)
    cur_str = fs_util.gen_str_by_repeat('a', 100)
    fs_util.write_file(fd, cur_str, start_off=0)

    # time for client_b to work, host_b should read the all-zero file
    print(signal_name_gen)
    print("Hello world")
    cur_signal_name = next(signal_name_gen)
    print(cur_signal_name)
    fs_util.start_another_client(host_b, 2, 'B', cur_signal_name)

    # wait until client_b finish
    while True:
        removed = fs_util.poll_signal_remove(host_b, cur_signal_name)
        if removed:
            break
        time.sleep(1000)
    print('ClientB finished')

    # read the old content
    cur_str = fs_util.read_file(fd, 32768)
    assert len(cur_str) == 32768
    for idx, rc in enumerate(cur_str):
        if idx < 100:
            assert rc == 'a'
        else:
            assert rc == '0'
    fs_util.close_file(fd);

    # Should not find the changes specific to clientB -- last update wins
    fd = fs_util.open_file(FNAME)
    cur_str = fs_util.read_file(fd, 200, start_off=0)
    assert len(cur_str) == 200
    for idx, c in enumerate(cur_str):
        if idx < 100:
            assert c == 'a'
        else:
            assert c == '0'

    last_signal_name = cur_signal_name
    cur_signal_name = next(signal_name_gen)
    fs_util.send_signal(host_b, cur_signal_name)

    # done
    fs_util.record_test_result(TEST_CASE_NO, 'A', 'OK')
    print("donee")


if __name__ == '__main__':
    run_test()