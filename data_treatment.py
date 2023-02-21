# -*- coding: utf-8 -*-
"""
Created on Mon Feb  6 17:14:55 2023
Data treatment from datalogger CSV file.
@author: Jesús
"""

#library import

import sys
import os
sys.path.append(os.path.dirname(__file__))

import pandas as pd
import numpy as np
from matplotlib import pyplot as plt
import myfunctions as f
inf = np.inf
 
#Import data from datalogger
df = f.datalogger_import(f.cols)
print("File imported")

# Sorting/filtering data 
filtered_data = f.datalogger_filter(df, 
                                    filt = input('Enter date filter: '), 
                                    mean_coeff = 1000,
                                    irr_coef = f.irr_coef) 

#Plotting

east = ['W1', 'W2', 'W3', 'W4']
north_west = ['W5', 'W6', 'W7', 'W8']
south_west = ['W11', 'W12', 'W13', 'W14']

f.plot_channels(magnitude = 'Irradiance [W/m$^2$]', dataframe = filtered_data, plate = east, title = 'East plate')
f.plot_channels(magnitude = 'Irradiance [W/m$^2$]', dataframe = filtered_data, plate = north_west, title = 'North-West plate')
f.plot_channels(magnitude = 'Irradiance [W/m$^2$]', dataframe = filtered_data, plate = south_west, title = 'South-West plate')

# Average plate irradiance

filtered_data['e_average'] = filtered_data[east].mean(axis=1)   
filtered_data['NW_average'] = filtered_data[north_west].mean(axis=1)
filtered_data['SW_average'] = filtered_data[south_west].mean(axis=1)

f.plot_channels(magnitude = 'Irradiance [W/m$^2$]', 
                dataframe = filtered_data, 
                plate = ['e_average', 'NW_average', 'SW_average'], 
                title = 'Average plate irradiance')


# Daily insolation array

dt = 1
matrix = [[0, 0,    5, 5,   0, 0,    11, 11,    0, 0],
          [0, 0,    0, 0,   0, 0,    0 , 0 ,    0, 0],
          [0, 0,    0, 0,   0, 0,    0 , 0 ,    0, 0],
          [0, 0,    6, 6,   0, 0,    12, 12,    0, 0],
          [0, 0,    7, 7,   0, 0,    13, 13,    0, 0],
          [0, 0,    0, 0,   0, 0,    0 , 0 ,    0, 0],
          [0, 0,    0, 0,   0, 0,    0 , 0 ,    0, 0],
          [0, 0,    8, 8,   0, 0,    14, 14,    0, 0],
          
          [0, 0,    0, 0,   4, 4,    0 , 0 ,    0, 0],
          [0, 0,    0, 0,   0, 0,    0 , 0 ,    0, 0],
          [0, 0,    0, 0,   0, 0,    0 , 0 ,    0, 0],
          [0, 0,    0, 0,   3, 3,    0 , 0 ,    0, 0],
          [0, 0,    0, 0,   2, 2,    0 , 0 ,    0, 0],
          [0, 0,    0, 0,   0, 0,    0 , 0 ,    0, 0],
          [0, 0,    0, 0,   0, 0,    0 , 0 ,    0, 0],
          [0, 0,    0, 0,   1, 1,    0 , 0 ,    0, 0]]

insolation2d = np.zeros([len(matrix), len(matrix[0])])

for i in range (0, len(matrix)):# get insolation matrix
    for j in range (0, len(matrix[0])):
        var = matrix[i][j]
        try:
            insolation2d[i][j] = filtered_data['W' + str(var)].sum() / 3600
            if insolation2d[i][j] < 100:
                insolation2d[i][j] = 0
        except KeyError:
            insolation2d[i][j] = np.nan
            
f.plot_insolation(figure = insolation2d, title = 'Sensor insolation')

#interpolation data

interpolation2d = pd.DataFrame(insolation2d)
#East plate
for i in [2, 3]:
    aux = interpolation2d[i].loc[0:7]  
    interpolation2d[i].loc[0:7] = aux.interpolate(method = 'linear').bfill()

#North West plate
for i in [4, 5]:
    aux = interpolation2d[i].loc[8:15]  
    interpolation2d[i].loc[8:15] = aux.interpolate(method = 'linear').bfill()
    
#South West plate
for i in [6, 7]:
    aux = interpolation2d[i].loc[0:7]  
    interpolation2d[i].loc[0:7] = aux.interpolate(method = 'linear').bfill()
    
#Horizontal interpolation
interpolation2d = interpolation2d.T
#horizontal interpolation
for i in range(0, 16):
    interpolation2d[i] = interpolation2d[i].interpolate(method = 'linear').bfill()

interpolation2d = interpolation2d.T


f.plot_insolation(figure = interpolation2d, title = 'Back side insolation')