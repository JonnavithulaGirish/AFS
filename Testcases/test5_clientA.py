#! /usr/bin/env python3

import os
import fs_util
import sys
import time
'''
This is ClientA.
'''

cs739_env_vars = [
    'CS739_CLIENT_A', 'CS739_CLIENT_B', 'CS739_SERVER', 'CS739_MOUNT_POINT','CS739_CLIENT_SERVER_POINT','CS739_CLIENT_CACHE_POINT'
]
ENV_VARS = {var_name: os.environ.get(var_name) for var_name in cs739_env_vars}
for env_var in ENV_VARS.items():
    print(env_var)
    assert env_var is not None
TEST_DATA_DIR = ENV_VARS['CS739_MOUNT_POINT'] + '/client_cache_consistency'
FNAME = f'{TEST_DATA_DIR}/case1'

print(TEST_DATA_DIR)
TEST_CASE_NO = 5


def run_test_5a():
    if not fs_util.path_exists(TEST_DATA_DIR):
        fs_util.mkdir(TEST_DATA_DIR)

    # init
    if not fs_util.path_exists(FNAME):
        fs_util.create_file(FNAME)

    init_str = fs_util.gen_str_by_repeat('0', 32768)
    fd = fs_util.open_file(FNAME)
    fs_util.write_file(fd, init_str)

    fs_util.record_test_result(TEST_CASE_NO, 'A', 'local Write is done we are kill the client server')



    # pid = fs_util.get_pid_client("unreliable",ENV_VARS['CS739_MOUNT_POINT'])
    # fs_util.kill_pid(pid)
    # fs_util.umount(ENV_VARS['CS739_MOUNT_POINT'])

    # fs_util.startClientServer(ENV_VARS['CS739_CLIENT_SERVER_POINT'],ENV_VARS['CS739_MOUNT_POINT'],ENV_VARS['CS739_CLIENT_CACHE_POINT'])
    # pid = fs_util.get_pid_client("unreliable",ENV_VARS['CS739_MOUNT_POINT'])
    # fs_util.kill_pid(pid)
    # fs_util.umount(ENV_VARS['CS739_MOUNT_POINT'])




    # # read the old content


def run_test_5b():
    fd = fs_util.open_file(FNAME)
    cur_str = fs_util.read_file(fd, 32768)
    assert len(cur_str) == 32768
    for idx, rc in enumerate(cur_str):
        assert rc == '0'
    fs_util.close_file(fd)

   
    # done
    fs_util.record_test_result(TEST_CASE_NO, 'A', 'after restarting the server also we are seeing the same content')
    print("done TEST-5")

if __name__ == '__main__':
    #we need to give input param 
    #first we need to run the 5a to open and write
    #then we are manaully killing and restarting the unreliable 
    #then we are running 5b to open, read and close.
    # we are testing it to check even if the unreliable crashed nothing happened to our data. 
    n = len(sys.argv)
    flag = sys.argv[1]
    if(flag == "0"):
      run_test_5a()    
    else:
        run_test_5b()