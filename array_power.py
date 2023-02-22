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
        plate1[i] = filtered_data['W1']
    elif i == (m / 2) - 1:
        plate1[i] = filtered_data['W2']
    elif i == (m / 2) + 1:
        plate1[i] = filtered_data['W3']
    elif i == m - 1:
        plate1[i] = filtered_data['W4']
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
pvsys = pvsystem.PVsystem(numberStrs=1, numberMods=1)
pmp = np.zeros(86400)

name = plate1.index[50000]
data = plate1.loc[name]
pannel = np.zeros([m,n])
locations = np.zeros([m,n])
z = 0
for i in range (0, n):
    if i % 2 != 0:
        z += m - 1
    for j in range (0, m):
        pannel[j][i] = data[j]
        locations[j][i] = z
        if i % 2 != 0:
            z -= 1
        if i % 2 == 0:
            z += 1
        

pvsys.setSuns({0: {0: [data/1000, pannel}})

power = pvsys.Pmp

plt.ion()
func =  pvsys.plotSys()