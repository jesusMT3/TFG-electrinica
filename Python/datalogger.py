# -*- coding: utf-8 -*-
"""
Created on Wed Feb 15 12:49:19 2023
Functions for managing data from datalogger CSV files
@author: Jesús
"""

from tkinter import filedialog
import pandas as pd
import numpy as np
inf = np.inf
import matplotlib.pyplot as plt

cols = ["No", "DateTime", "ms", "CH1", "CH2", "CH3", "CH4", "CH5", 
        "CH6", "CH7", "CH8", "CH9", "CH11", "CH12", "CH13", "CH14", 
        "CH15", "T1", "T2", "GS1", "GS2", "GS3", "GS4", "Alarm1", 
        "Alarm2", "Alarm3", "AlarmOut"] # columns in which the data from data is organised

cols2 = ['yyyy/mm/dd hh:mm','Temp. Ai 1','Bn','Gh','Dh','Celula Top','Celula Mid','Celula Bot',
         'Top - Cal' ,'Mid - Cal' ,'Bot - Cal','Presion','V.Vien.1','D.Vien.1','Elev.Sol',
         'Orient.Sol','Temp. Ai 2','Hum. Rel','Bn_2','G(41)','Gn','Pirgeo','Temp_Pirgeo',
         'Auxil.01','V.Vien.2','D.Vien.2','Lluvia','Limpieza','Elev.Sol_2','Orient.Sol_2'
]

# Irradiance conversion coefficients
irr_coef = [0.1658, 0.1638, 0.1664, 0.1678, 0.3334, 0.1686, 0.1673, inf, inf, inf, inf, 0.3306, 0.3317, 0.3341, 0.3361]

def datalogger_import(cols):
    # root = tk.Tk()
    try:
        data = filedialog.askopenfilename()
    except FileNotFoundError:
        print('Error: File not found')
        # root.destroy()
        exit()
    df = pd.read_csv(data,
                     sep="\s+|,", # two types of separation
                     names = cols, # names of the columns
                     header = None, # csv file with no header, customized "cols"
                     engine = "python",
                     skiprows = 1, # first 40 rows are datalogger specifications
                     index_col = 1) #to search for specific hours in dataframe
    
    # filedialog.asksaveasfile(defaultextension = ".csv")
    # root.destroy()
    return df

def meteodata_import(cols):
    try:
        data = filedialog.askopenfilename()
    except FileNotFoundError:
        print('Error: File not found')
        # root.destroy()
        exit()
    df = pd.read_csv(data,
                     sep="\t", # two types of separation
                     names = cols, # names of the columns
                     header = None, # csv file with no header, customized "cols"
                     engine = "python",
                     skiprows = 1, # first 40 rows are datalogger specifications
                     index_col = 1) #to search for specific hours in dataframe
    
    return df

def datalogger_filter(df, filt, mean_coeff, irr_coef):
    # filtered_data = df[df.index.str.startswith(filt)].copy()
    # filtered_data = df.filter(like = filt, axis = 0).copy()
    filtered_data = df.copy()
    filtered_data.index = filtered_data['DateTime']

    for i in range(1, 19):
        try:
            aux_str = "CH" + str(i)
            filtered_data[aux_str] = smooth(filtered_data[aux_str], 1000)
        except KeyError:
            # print("Channel ",i ," does not exist")
            continue
            
    filtered_data['T_av'] = filtered_data[['T1', 'T2']].mean(axis=1) #average temperature
    alpha = 4.522e-4 # pu units
    T0  = 298.15 # STC temperature

    for i in range(1, 19):
        # Irradiance conversion with temperature dependance
        try:
            coef = 1 + alpha * ((filtered_data['T_av'] + 273.15)- T0)
            filtered_data['W' +  str(i)] = filtered_data["CH" + str(i)] / coef
            filtered_data['W' +  str(i)] /= irr_coef[i-1]
  
        except KeyError:
            continue
        
    return filtered_data
    

def smooth(y, box_pts):
    box = np.ones(box_pts)/box_pts
    y_smooth = np.convolve(y, box, mode='same')
    return y_smooth

def plot_channels(magnitude, dataframe, plate, title, ax = None, xaxis = 'Date Time'):
    plt.figure()
    for i in plate:
        dataframe[i].plot()
        
    plt.xlabel(xaxis)
    plt.ylabel(magnitude)
    plt.title(title)
    plt.legend()
    plt.grid(True)
    
def plot_insolation(figure, title):
    plt.figure()
    vmin = 0
    vmax = np.max(figure)
    cmap = plt.cm.get_cmap('RdYlBu')
    cmap.set_under('red')
    plt.imshow(figure, vmin=vmin, vmax=vmax, cmap=cmap, extent=[0, 42, 33, 0])
    plt.title(title)
    plt.colorbar()