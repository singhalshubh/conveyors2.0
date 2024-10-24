#!/usr/bin/env python

import json
import numpy as np
import matplotlib.pyplot as plt
import sys
import csv
import math

def main():
    time = []
    avg_time_1 = []
    stddev_time_1 = []
    avg_time_2 = []
    stddev_time_2 = []
    avg_time_3 = []
    stddev_time_3 = []

    data = []
    cores = []
    node = 1
    while node <= 128:
        cores.append(16*node)
        node=node*2
    data.append(2**26)
    for d in data:
        for pe in cores:
            with open("main_01-" + str(pe) + "-" + str(d) + ".txt", "r") as filestream:
                for line in filestream:
                    time.append(float(line))
                avg_time_1.append(np.mean(time))
                stddev_time_1.append(np.std(time))
                time = []
            with open("main_02-" + str(pe) + "-" + str(d) + ".txt", "r") as filestream:
                for line in filestream:
                    time.append(float(line))
                avg_time_2.append(np.mean(time))
                stddev_time_2.append(np.std(time))
                time = []
            with open("main_03-" + str(pe) + "-" + str(d) + ".txt", "r") as filestream:
                for line in filestream:
                    time.append(float(line))
                avg_time_3.append(np.mean(time))
                stddev_time_3.append(np.std(time))
                time = []
        plt.errorbar(cores, avg_time_1, stddev_time_1, label = "Conveyors")
        plt.errorbar(cores, avg_time_2, stddev_time_2, label = "Optimized Generic")
        plt.errorbar(cores, avg_time_3, stddev_time_3, label = "Naive Generic")
        plt.xlabel("#cores")
        plt.ylabel("Execution Time (in sec)")
        plt.yscale('log', base=2)
        plt.xscale('log', base=2)
        plt.title("One-to-all")
        plt.legend(fontsize="x-large")
        plt.grid(True)
        plt.savefig("one-to-all-" + str(math.log2(d)) + ".png", dpi = 600)

if __name__=="__main__": 
    main()