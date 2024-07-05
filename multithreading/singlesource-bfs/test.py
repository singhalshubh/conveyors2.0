#!/usr/bin/env python

import json
import numpy
import matplotlib.pyplot as plt
import sys

def main():
    workers = [1, 2, 4, 6, 12, 18, 24]

    c_8_pe1 = []
    c_8_pe2 = []
    for w in workers:
        df = 0
        with open("./bin/a" + str(w) + "-8.txt", "r") as file:
            for line in file.readlines():
                if line != '':
                    df += float(line)
            df = df//5
            c_8_pe1.append(df)
    
  
    df = 0
    with open("./bin/b-8.txt", "r") as file:
        for line in file.readlines():
            if line != '':
                df += float(line)
        df = df//5
        c_8_pe2.append(df)


    c_16_pe1 = []
    c_16_pe2 = []
    for w in workers:
        df = 0
        with open("./bin/a" + str(w) + "-16.txt", "r") as file:
            for line in file.readlines():
                if line != '':
                    df += float(line)
            df = df//5
            c_16_pe1.append(df)
    

    df = 0
    with open("./bin/b-16.txt", "r") as file:
        for line in file.readlines():
            if line != '':
                df += float(line)
        df = df//5
        c_16_pe2.append(df)

    c_32_pe1 = []
    c_32_pe2 = []
    for w in workers:
        df = 0
        with open("./bin/a" + str(w) + "-32.txt", "r") as file:
            for line in file.readlines():
                if line != '':
                    df += float(line)
            df = df//5
            c_32_pe1.append(df)
    
    
    df = 0
    with open("./bin/b-32.txt", "r") as file:
        for line in file.readlines():
            if line != '':
                df += float(line)
        df = df//5
        c_32_pe2.append(df)


    c_64_pe1 = []
    c_64_pe2 = []
    for w in workers:
        df = 0
        with open("./bin/a" + str(w) + "-64.txt", "r") as file:
            for line in file.readlines():
                if line != '':
                    df += float(line)
            df = df//5
            c_64_pe1.append(df)
    
    
    df = 0
    with open("./bin/b-64.txt", "r") as file:
        for line in file.readlines():
            if line != '':
                df += float(line)
        df = df//5
        c_64_pe2.append(df)

    plt.plot(workers, c_8_pe1, marker = 's', color='tab:red', label='8 nodes')
    plt.plot(1, c_8_pe2, 's', color='tab:red')
    plt.plot(workers, c_16_pe1, marker = 'o', color='tab:orange', label='16 nodes')
    plt.plot(1, c_16_pe2, 'o', color='tab:orange')
    plt.plot(workers, c_32_pe1, marker = '^', color='tab:blue', label='32 nodes')
    plt.plot(1, c_32_pe2, '^', color='tab:blue')
    plt.plot(workers, c_64_pe1, marker = '*',color='tab:green', label='64 nodes')
    plt.plot(1, c_64_pe2, '*', color='tab:green')
    plt.xlabel("Number of HClib workers", fontsize="x-large")
    plt.ylabel("Execution time (in seconds)", fontsize="x-large")
    plt.title("BFS single-source multi-graph, scale = " + str(sys.argv[1]), fontsize="x-large")
    plt.legend(fontsize="x-large")
    plt.savefig("pic_" + str(sys.argv[1]) + ".png", dpi = 600)

if __name__=="__main__": 
    main()