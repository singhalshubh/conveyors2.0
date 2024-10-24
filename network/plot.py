#!/usr/bin/env python

import json
import numpy as np
import matplotlib.pyplot as plt
import sys
import csv

def main():
    time = []
    avg_time = []
    stddev_time = []
    size = 1
    x = []
    while(1):
        with open("debug-" + str(size) + ".txt", "r") as filestream:
            for line in filestream:
                time.append(int(line))
                #time.append(int(line)/(size*4))
            x.append(size)
            avg_time.append(np.mean(time))
            stddev_time.append(np.std(time))
            time = []
            if size == 67108864:
                break
            size = size*2

    plt.errorbar(x, avg_time, stddev_time, marker='*')
    plt.xlabel("#For elem = #total-bytes/4")
    plt.ylabel("#cycles per byte")
    plt.yscale('log', base=2)
    plt.xscale('log', base=2)
    plt.title("Memcpy between 2 PEs, 1 node")
    plt.legend(fontsize="x-large")
    plt.savefig("memcpy.png", dpi = 600)

if __name__=="__main__": 
    main()