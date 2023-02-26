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
FNAME = f'{TEST_DATA_DIR}/case1'
FNAME1 = f'{TEST_DATA_DIR}/case4'

print(TEST_DATA_DIR)
TEST_CASE_NO1 = 4 

def run_test():
    host_b = ENV_VARS['CS739_CLIENT_B']
    assert fs_util.test_ssh_access(host_b)
    signal_name_gen = fs_util.get_fs_signal_name()
    # # print(signal_name_gen.gi_yieldfrom)


    cur_signal_name = next(signal_name_gen)
    print("-------a")
    print(cur_signal_name)
    fs_util.start_another_client_flag(host_b, 4, 'B', cur_signal_name,0)
    # wait until client_b finish
    while True:
        removed = fs_util.poll_signal_remove(host_b, cur_signal_name)
        if removed:
            break
        time.sleep(1000)
    print('ClientB Write finished')

    #we are opening the file in client A and removing it from the client B.
    fd = fs_util.open_file(FNAME1)

    # fs_util.wait_for_signal(cur_signal_name)
    cur_signal_name = next(signal_name_gen)
    # cur_signal_name = next(signal_name_gen)
    print("-------b")
    print(cur_signal_name)
    fs_util.start_another_client_flag(host_b, 4, 'B', cur_signal_name,1)
    # wait until client_b finish
    while True:
        removed = fs_util.poll_signal_remove(host_b, cur_signal_name)
        if removed:
            break
        time.sleep(1000)
    print('ClientB finished after deleting')
    

    # but we should be able to read the local cache content 
    # even though we delete the file from the server.
    cur_str = fs_util.read_file(fd, 100)
    assert len(cur_str) == 100
    for idx, rc in enumerate(cur_str):
        assert rc == 'b'

    fs_util.close_file(fd)
    print("ClientA read done")
    
    # done
    fs_util.record_test_result(TEST_CASE_NO1, 'A', 'OK')
    print("done TEST4")



if __name__ == '__main__':
    run_test()