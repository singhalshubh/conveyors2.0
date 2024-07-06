#!/usr/bin/env python

import json
import numpy
import matplotlib.pyplot as plt
import sys

def main():
    nodes = [8, 16, 32, 64]

    c_1 = []
    w = 1
    for n in nodes:
        df = 0
        with open("./bin/a" + str(w) + "-" + str(n) + ".txt", "r") as file:
            for line in file.readlines():
                if line != '':
                    df += float(line)
            df = df//5
            c_1.append(df)

    c_2 = []
    w = 2
    for n in nodes:
        df = 0
        with open("./bin/a" + str(w) + "-" + str(n) + ".txt", "r") as file:
            for line in file.readlines():
                if line != '':
                    df += float(line)
            df = df//5
            c_2.append(df)

    c_4 = []
    w = 4
    for n in nodes:
        df = 0
        with open("./bin/a" + str(w) + "-" + str(n) + ".txt", "r") as file:
            for line in file.readlines():
                if line != '':
                    df += float(line)
            df = df//5
            c_4.append(df)


    c_8 = []
    w = 8
    for n in nodes:
        df = 0
        with open("./bin/a" + str(w) + "-" + str(n) + ".txt", "r") as file:
            for line in file.readlines():
                if line != '':
                    df += float(line)
            df = df//5
            c_8.append(df)

    c_16 = []
    w = 16
    for n in nodes:
        df = 0
        with open("./bin/a" + str(w) + "-" + str(n) + ".txt", "r") as file:
            for line in file.readlines():
                if line != '':
                    df += float(line)
            df = df//5
            c_16.append(df)
    
    c_base = []
    df = 0
    for n in nodes:
        with open("./bin/b" + "-" + str(n) + ".txt", "r") as file:
            for line in file.readlines():
                if line != '':
                    df += float(line)
            df = df//5
            c_base.append(df)

    workers = [192, 384, 768, 1536]

    plt.plot(workers, c_1, marker = 's', color='tab:red', label='1 worker')
    plt.plot(workers, c_2, marker = 'o', color='tab:orange', label='2 workers')
    plt.plot(workers, c_4, marker = '^', color='tab:blue', label='4 workers')
    plt.plot(workers, c_8, marker = '*',color='tab:green', label='8 workers')
    plt.plot(workers, c_16, marker = '|',color='black', label='16 workers')
    plt.plot(workers, c_base, marker = '_',color='magenta', label='1 worker(no async)', linestyle='dashed')
    plt.xlabel("Number of cores", fontsize="x-large")
    plt.ylabel("Execution time (in seconds)", fontsize="x-large")
    plt.title("BFS single-source multi-graph, scale = " + str(sys.argv[1]), fontsize="x-large")
    plt.legend(fontsize="x-large")
    plt.savefig("pic_ss_" + str(sys.argv[1]) + ".png", dpi = 600)

if __name__=="__main__": 
    main()