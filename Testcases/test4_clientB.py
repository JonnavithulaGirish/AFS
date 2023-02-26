#! /usr/bin/env python3

import os
import fs_util
import sys
import test1_clientA as test1  # Simply do not want to do test1_common
'''
This is ClientB.
'''



def run_test1():
    print("Hello in ClientB")
    if not fs_util.path_exists(test1.TEST_DATA_DIR):
        fs_util.mkdir(test1.TEST_DATA_DIR)

    # init
    if not fs_util.path_exists(test1.FNAME1):
        fs_util.create_file(test1.FNAME1)
        

    signal_name_gen = fs_util.get_fs_signal_name()
    cur_signal_name = next(signal_name_gen)
    fs_util.record_test_result(test1.TEST_CASE_NO1, 'B',
                               f'START fname:{test1.FNAME1} +" "+{cur_signal_name}')
    fs_util.wait_for_signal(cur_signal_name)

    fd = fs_util.open_file(test1.FNAME1)
    # let's do some write
    cur_str = fs_util.gen_str_by_repeat('b', 100)
    fs_util.write_file(fd, cur_str, start_off=0)
    fs_util.record_test_result(test1.TEST_CASE_NO1, 'B',
                               f'Finish Read and Write of b')
    # suppose to flush
    fs_util.close_file(fd)
        
    last_signal_name = cur_signal_name
    cur_signal_name = next(signal_name_gen)
    fs_util.record_test_result(test1.TEST_CASE_NO1, 'B', f'Write of {test1.FNAME1} is done')
    fs_util.wait_for_signal(cur_signal_name, last_signal_name=last_signal_name)

    # last_signal_name = cur_signal_name
    # cur_signal_name = next(signal_name_gen)
    # fs_util.wait_for_signal(cur_signal_name, last_signal_name=last_signal_name)

    # done
    


def run_test2():
    print("Hi in the run test2")
    signal_name_gen = fs_util.get_fs_signal_name()
    cur_signal_name = next(signal_name_gen)
    cur_signal_name = next(signal_name_gen)
    last_signal_name = cur_signal_name
    cur_signal_name = next(signal_name_gen)
    
    print(test1.FNAME1)
    fs_util.delete_file(test1.FNAME1)
    #  # done
    fs_util.record_test_result(test1.TEST_CASE_NO1, 'B',
                               f'Removing the file {test1.FNAME1} is done')

    # fs_util.create_file(cur_signal_name)
    
    if not fs_util.path_exists(cur_signal_name):
        fs_util.create_file(cur_signal_name)
    fs_util.wait_for_signal(cur_signal_name, last_signal_name=last_signal_name)
    


if __name__ == '__main__':
    # print("Hi int he ouput")
    n = len(sys.argv)
    flag = sys.argv[1]
    if(flag == "0"):
        # print("Hi")
        # fs_util.record_test_result(test1.TEST_CASE_NO1, 'B',"Hi1")
        run_test1()
    else:
        # fs_util.record_test_result(test1.TEST_CASE_NO1, 'B',"Hi2")
        run_test2()