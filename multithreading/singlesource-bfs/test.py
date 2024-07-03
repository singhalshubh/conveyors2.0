#!/usr/bin/env python

import json
import numpy
import matplotlib.pyplot as plt
import sys

def main():
    workers = [1, 2, 4, 6, 12, 18, 24]
    corrupt = [0, 10, 30, 50, 70]
    
    c_0 = []
    for w in workers:
        with open("./bin/a" + str(w) + ".txt", "r") as file:
            c_0.append(float(file.read()))

    with open("./bin/b1.txt", "r") as file:
            c_00 = float(file.read())
    # c_10 = []
    # for w in workers:
    #     with open("./bin/a" + "10-"  + str(w) + ".txt", "r") as file:
    #         c_10.append(float(file.read()))

    # c_30 = []
    # for w in workers:
    #     with open("./bin/a" + "30-"  + str(w) + ".txt", "r") as file:
    #         c_30.append(float(file.read()))
    
    # c_50 = []
    # for w in workers:
    #     with open("./bin/a" + "50-"  + str(w) + ".txt", "r") as file:
    #         c_50.append(float(file.read()))

    # c_70 = []
    # for w in workers:
    #     with open("./bin/a" + "70-"  + str(w) + ".txt", "r") as file:
    #         c_70.append(float(file.read()))

    plt.plot(workers, c_0, marker = '', color='tab:brown', label='0%')
    plt.plot(1, c_00, 'o')
    # plt.plot(workers, c_10, marker = 's', color='tab:orange', label='10%')
    # plt.plot(workers, c_30, marker = 'o', color='tab:blue', label='30%')
    # plt.plot(workers, c_50, marker = '^', color='tab:green', label='50%')
    # plt.plot(workers, c_70, marker = '*', color='tab:red', label='70%')
    plt.xlabel("Number of HClib workers", fontsize="x-large")
    plt.ylabel("Execution time (in seconds)", fontsize="x-large")
    plt.title("256 cores BFS single-source, 10 graphs", fontsize="x-large")
    plt.legend(fontsize="x-large")
    plt.savefig("pic.png", dpi = 600)

if __name__=="__main__": 
    main()