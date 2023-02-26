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
    if not fs_util.path_exists(test3.FNAME):
        fs_util.record_test_result(test3.TEST_CASE_NO, 'B', 'not exists')
        sys.exit(1)

    # should find a file in server with lenght 0
    fd = fs_util.open_file(test3.FNAME)
    cur_str = fs_util.read_file(fd, 32768)
    if(len(cur_str) != 0):
        fs_util.record_test_result(test3.TEST_CASE_NO, 'B',
                               f'not empty')

    # let's do some write
    cur_str = fs_util.gen_str_by_repeat('b', 100)
    fs_util.write_file(fd, cur_str, start_off=0)
    fs_util.record_test_result(test3.TEST_CASE_NO, 'B',
                               f'Finish Read and Write of b')
    
    # let's do some write
    cur_str = fs_util.gen_str_by_repeat('b', 200)
    fs_util.write_file(fd, cur_str, start_off=100)
    fs_util.record_test_result(test3.TEST_CASE_NO, 'B',
                               f'Finish Read and Write of b')

    # let's do some write
    cur_str = fs_util.gen_str_by_repeat('b', 300)
    fs_util.write_file(fd, cur_str, start_off=300)
    fs_util.record_test_result(test3.TEST_CASE_NO, 'B',
                               f'Finish Read and Write of b')
    fs_util.close_file(fd)

    fd = fs_util.open_file(test3.FNAME)
    cur_str = fs_util.read_file(fd, 1000)
    for idx, rc in enumerate(cur_str):
        if idx<600 and rc != 'b':
            fs_util.record_test_result(test3.TEST_CASE_NO, 'B',
                                   f'read_str:{rc}')
    fs_util.close_file(fd)

    last_signal_name = cur_signal_name
    cur_signal_name = next(signal_name_gen)
    fs_util.record_test_result(test3.TEST_CASE_NO, 'B',
                               f'{cur_signal_name}')
    print(cur_signal_name)
    print(last_signal_name)
    fs_util.wait_for_signal(cur_signal_name, last_signal_name=last_signal_name)

    # fd = fs_util.open_file(test3.FNAME)
    # cur_str = fs_util.read_file(fd, 1000)
    # for idx, rc in enumerate(cur_str):
    #     if idx<1000 and rc != 'a':
    #         fs_util.record_test_result(test3.TEST_CASE_NO, 'B',
    #                                f'read_str:{rc}')
    # fs_util.close_file(fd)

    # done
    fs_util.record_test_result(test3.TEST_CASE_NO, 'B',
                               f'delete file b')
    
    fs_util.record_test_result(test3.TEST_CASE_NO, 'B', 'OK')


if __name__ == '__main__':
    # print("Hi int he ouput")
    run_test()