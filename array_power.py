# -*- coding: utf-8 -*-
"""
Created on Mon Feb 20 10:25:10 2023

Program which calculates real power from solar module

@author: Jesus
"""
import sys
import os
sys.path.append(os.path.dirname(__file__))
import myfunctions as f
import numpy as np
import pandas as pd
import pvlib as pv
from pvmismatch import *
import matplotlib.pyplot as plt

df = f.datalogger_import(f.cols)
filtered_data = f.datalogger_filter(df = df, 
                                    filt = input('Date filter: '), 
                                    mean_coeff = 1000,
                                    irr_coef = f.irr_coef)

#%%
n = 8
m = 12
plate1 = pd.DataFrame()
plate_front = filtered_data['W15']
for i in range (0, m):
    if i == 0:
        plate1[i] = filtered_data['W1'] + plate_front
    elif i == (m / 2) - 1:
        plate1[i] = filtered_data['W2'] + plate_front
    elif i == (m / 2) + 1:
        plate1[i] = filtered_data['W3'] + plate_front
    elif i == m - 1:
        plate1[i] = filtered_data['W4'] + plate_front
    else:
        plate1[i] = np.empty(len(filtered_data)) * np.nan


    
plate1 = plate1.interpolate(method = 'linear', axis = 1)

# plate1 = plate_back
# for i in plate1:
#     plate1[i] += plate_front

f.plot_channels(magnitude = 'Irradiance [W/m$^2$]', dataframe = plate1, 
                plate = plate1, 
                title = 'East plate global Irradiance')
#%%
import numpy as np
import pvmismatch as pvm

n = 8
m = 12

module = PVmodule()
power = np.zeros(86400)
for x in range (50000, 60000):
    name = plate1.index[x]
    data = plate1.loc[name]
    dic = {}
    suns = np.zeros(m*n)
    z = 0
    
    for i in range(0, n):
        if i != 0:
            if i % 2 == 0:
                z += m + 1
            elif i % 2 != 0:
                z += m - 1
        for j in range(0, m):
            suns[z] = data[j]
            if i % 2 != 0:
                z -= 1
            if i % 2 == 0:
                z += 1
    
    module.setSuns(suns/1000)
    power[x] = module.Pmod.max()

plt.plot(power)