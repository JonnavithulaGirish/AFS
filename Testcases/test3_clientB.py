#! /usr/bin/env python3

import os
import fs_util
import sys
import test3_clientA as test3  # Simply do not want to do test1_common
'''
This is ClientB.
'''


def run_test():
    print("Hello in ClientB")
    signal_name_gen = fs_util.get_fs_signal_name()

    cur_signal_name = next(signal_name_gen)
    fs_util.record_test_result(test3.TEST_CASE_NO, 'B',
                               f'START fname:{test3.FNAME}')
    fs_util.wait_for_signal(cur_signal_name)

    # first execution, read all-zero file
    if fs_util.path_exists(test3.FNAME):
        fs_util.record_test_result(test3.TEST_CASE_NO, 'B', 'exists')
        sys.exit(1)

    # should not find a file in server 
    fs_util.create_file(test3.FNAME)
    fd = fs_util.open_file(test3.FNAME)

    # let's do some write
    cur_str = fs_util.gen_str_by_repeat('b', 100)
    fs_util.write_file(fd, cur_str, start_off=0)
    fs_util.record_test_result(test3.TEST_CASE_NO, 'B',
                               f'Finish Read and Write of b')
    fs_util.close_file(fd)
    
    # let's do some write
    cur_str = fs_util.gen_str_by_repeat('b', 200)
    fs_util.write_file(fd, cur_str, start_off=100)
    fs_util.record_test_result(test3.TEST_CASE_NO, 'B',
                               f'Finish Read and Write of b')
    fs_util.close_file(fd)

    # let's do some write
    cur_str = fs_util.gen_str_by_repeat('b', 300)
    fs_util.write_file(fd, cur_str, start_off=300)
    fs_util.record_test_result(test3.TEST_CASE_NO, 'B',
                               f'Finish Read and Write of b')
    fs_util.close_file(fd)

    #verify length on server
    fd = fs_util.open_file(test3.FNAME)
    cur_str = fs_util.read_file(fd, read_len, 0)
    if len(cur_str) != 600:
        fs_util.record_test_result(test3.TEST_CASE_NO, 'B',
                                   f'read_len: 600')
    fs_util.close_file(fd)

    last_signal_name = cur_signal_name
    cur_signal_name = next(signal_name_gen)
    fs_util.wait_for_signal(cur_signal_name, last_signal_name=last_signal_name)

    fd = fs_util.open_file(test3.FNAME)
    read_len = 1000
    cur_str = fs_util.read_file(fd, read_len, 0)
    if len(cur_str) != read_len:
        fs_util.record_test_result(test3.TEST_CASE_NO, 'B',
                                   f'read_len:{len(cur_str)}')
    for idx, c in enumerate(cur_str):
        if idx < 1000:
            if c != 'a':
                fs_util.record_test_result(test3.TEST_CASE_NO, 'B',
                                           f'idx:{idx} c:{c}')
                sys.exit(1)
    
    fs_util.delete_file(test3.FNAME)
    # done
    fs_util.record_test_result(test3.TEST_CASE_NO, 'B', 'OK')


if __name__ == '__main__':
    # print("Hi int he ouput")
    run_test()