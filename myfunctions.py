# -*- coding: utf-8 -*-
"""
Created on Wed Feb 15 12:49:19 2023

@author: Jesús
"""

import tkinter as tk
from tkinter import filedialog
import pandas as pd
import numpy as np

cols = ["No", "DateTime", "ms", "CH1", "CH2", "CH3", "CH4", "CH5", 
        "CH6", "CH7", "CH8", "CH9", "CH11", "CH12", "CH13", "CH14", 
        "CH15", "CH19", "CH20", "GS1", "GS2", "GS3", "GS4", "Alarm1", 
        "Alarm2", "Alarm3", "AlarmOut"] # columns in which the data from data is organised

def datalogger_read():
    root = tk.Tk()
    try:
        data = filedialog.askopenfilename()
    except FileNotFoundError:
        print('Error: File not found')
        root.destroy()
    df = pd.read_csv(data,
                     sep="\s+|,", # two types of separation
                     names = cols, # names of the columns
                     header = None, # csv file with no header, customized "cols"
                     engine = "python",
                     skiprows = 40, # first 40 rows are datalogger specifications
                     index_col = 1) #to search for specific hours in dataframe
    
    # filedialog.asksaveasfile(defaultextension = ".csv")
    root.destroy()
    return df

def smooth(y, box_pts):
    box = np.ones(box_pts)/box_pts
    y_smooth = np.convolve(y, box, mode='same')
    return y_smooth

df = datalogger_read()