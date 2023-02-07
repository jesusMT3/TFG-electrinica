# -*- coding: utf-8 -*-
"""
Created on Mon Feb  6 17:42:48 2023

@author: Jesús
"""
import numpy as np

def mean_filter(arr, k):
    # applies mean filter to 1-d array with the kernel size 2k+1 . Write your code here
    p=len(arr)
    arr2=np.zeros(2*k+p, dtype=float)
    arr3=np.zeros(2*k+p, dtype=float)
    arr4=np.zeros(p, dtype=float)

    for i in range(p):
        arr2[k+i]=arr[i]

    for i in range(k,k+p):
        sum=0
        for j in range(-k,k+1):
            sum+=arr2[i+j]
        arr3[i]=sum/float(2*k+1)

    for i in range(p):
        arr4[i]=arr3[k+i]

    return arr4
        