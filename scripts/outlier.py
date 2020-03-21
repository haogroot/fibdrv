#!/usr/bin/python
import os
import random 
import numpy as np 
from scipy import mean
from scipy.stats import sem, t

# Reference from: https://ppt.cc/fqlqox
def reject_outliers_from_one_deviation(data, m = 1.):
    d = np.abs(data - np.median(data))
    mdev = np.median(d)
    s = d/mdev if mdev else 0.
    return data[s<m]

def reject_outliers_from_two_deviation(data, m = 2.):
    d = np.abs(data - np.median(data))
    mdev = np.median(d)
    s = d/mdev if mdev else 0.
    return data[s<m]

def reject_outliers_from_three_deviation(data, m = 3.):
    d = np.abs(data - np.median(data))
    mdev = np.median(d)
    s = d/mdev if mdev else 0.
    return data[s<m]

script_dir = os.path.dirname(__file__)
folder_path = os.path.join(script_dir, '../result')
folder_abs_path = os.path.abspath(folder_path)
os.chdir(folder_abs_path)

open('../ans.txt', 'w').close() 
open('../ans1.txt', 'w').close() 
open('../ans2.txt', 'w').close() 
open('../ans3.txt', 'w').close() 

folder = os.listdir(folder_abs_path)
# read each line of each file
lines = 101 
for i in range(lines):
    test_list = []
    for file in folder:
        test_result = open(file)
        all_lines = test_result.readlines()
        string = all_lines[i]
        test_list.append(string.split(" ",2)[1]) 
        test_result.close()
    np_arr = np.array(test_list, dtype='float_')
    np_arr_one = reject_outliers_from_one_deviation(np_arr)
    np_arr_two = reject_outliers_from_two_deviation(np_arr)
    np_arr_three = reject_outliers_from_three_deviation(np_arr)

    str2 = str(i) + ": " + str(sum(np_arr)/len(np_arr)) + " " + str(len(np_arr)) + '\n'
    with open('../ans.txt', 'a') as the_file:
        the_file.writelines(str2)

    str2 = str(i) + ": " + str(np.sum(np_arr_one)/len(np_arr_one)) + " " + str(len(np_arr_one)) + '\n'
    with open('../ans1.txt', 'a') as the_file:
        the_file.writelines(str2)
        
    str2 = str(i) + ": " + str(np.sum(np_arr_two)/len(np_arr_two)) + " " + str(len(np_arr_two)) + '\n'
    with open('../ans2.txt', 'a') as the_file:
        the_file.writelines(str2)

    str2 = str(i) + ": " + str(np.sum(np_arr_three)/len(np_arr_three)) + " " + str(len(np_arr_three)) + '\n'
    with open('../ans3.txt', 'a') as the_file:
        the_file.writelines(str2)
