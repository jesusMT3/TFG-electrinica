# -*- coding: utf-8 -*-
"""
Created on Fri Feb 10 10:03:43 2023

@author: Jesús
"""
import tkinter as tk
from tkinter.filedialog import askopenfilename
import pandas as pd
import numpy as np
from matplotlib import pyplot as plt
import os
from scipy.signal import medfilt
import scipy

inf = np.inf


def import_csv_data():
    global v
    global df
    csv_file_path = askopenfilename()
    print('Opening file...')
    v.set(csv_file_path)
    
    cols = ["No", "DateTime", "ms", "CH1", "CH2", "CH3", "CH4", "CH5", 
            "CH6", "CH7", "CH8", "CH9", "CH11", "CH12", "CH13", "CH14", 
            "CH15", "CH19", "CH20", "GS1", "GS2", "GS3", "GS4", "Alarm1", 
            "Alarm2", "Alarm3", "AlarmOut"] # columns in which the data from data is organised
    
    df = pd.read_csv(csv_file_path,
                     sep="\s+|,", # two types of separation
                     names = cols, # names of the columns
                     header = None, # csv file with no header, customized "cols"
                     engine = "python",
                     skiprows = 40, # first 40 rows are datalogger specifications
                     index_col = 1) #to search for specific hours in dataframe
    
    print('File openend!')
    print(df.head())

def filter_csv_data():
    print('Sorting data...')
    global filtered_data
    global filter_day
    filter_day = '2022-07-21'
    data_day = df[df.index.str.startswith(filter_day)]
    data_day.index = data_day['DateTime']
    mean_coef = 3
    filtered_data = pd.DataFrame(data_day)
    #average temperature


    #filtering data through mobile-mean function from pandas library
    # filtered_data = pd.DataFrame(data_day)

    for i in range(1, 19):
        try:
            aux_str = "CH" + str(i)
            filtered_data[aux_str] = medfilt(data_day[aux_str], mean_coef)
        except KeyError:
            print("Channel ",i ," does not exist")
            continue
    

    filtered_data['T_av'] = filtered_data[['CH19', 'CH20']].mean(axis=1) #average temperature
    k= [0.1658, 0.1638, 0.1664, 0.1678, 0.3334, 0.1686, 0.1673, inf, inf, inf, inf, 0.3306, 0.3317, 0.3341, 0.3361]
    alpha = 4.522e-4 # pu units
    T0  = 298.15 # STC temperature
    
    for i in range(1, 19):
        # Irradiance conversion with temperature dependance
        try:
            coef = 1 + alpha * ((filtered_data['T_av'] + 273.15) - T0)
            filtered_data['W' +  str(i)] = filtered_data["CH" + str(i)] / coef
            filtered_data['W' +  str(i)] /= k[i-1]
        except KeyError:
            continue
        
    print('Action completed!')
    print(data_day.head())
    
# def plot_data(msg):
#     print(msg)
    
    
root = tk.Tk()
tk.Label(root, text='File Path').grid(row=0, column=0)
v = tk.StringVar()
entry = tk.Entry(root, textvariable=v).grid(row=0, column=1)
tk.Button(root, text='Select',command=import_csv_data).grid(row=1, column=0)
tk.Button(root, text='Close',command=root.destroy).grid(row=9, column=2)
tk.Button(root, text='Sort Data',command=filter_csv_data).grid(row=0, column=2)
# tk.Button(root, text='Plot irradiance',command=plot_data('hello world')).grid(row=1, column=2)
root.mainloop()