# -*- coding: utf-8 -*-
"""
Created on Mon Feb  6 17:14:55 2023
Data treatment from datalogger CSV file.
CSV Data from datalogger must be in "data" directory
@author: Jesús
"""

import pandas as pd
import numpy as np
from matplotlib import pyplot as plt
import os
from scipy.signal import medfilt

inf = np.inf
 
#%%Load data

#Import data from datalogger
dirname = os.path.dirname(__file__) # absolute route to path
data = os.path.join(dirname, 'data/220720-205300.csv') # file should be located in data directory

cols = ["No", "DateTime", "ms", "CH1", "CH2", "CH3", "CH4", "CH5", 
        "CH6", "CH7", "CH8", "CH9", "CH11", "CH12", "CH13", "CH14", 
        "CH15", "CH19", "CH20", "GS1", "GS2", "GS3", "GS4", "Alarm1", 
        "Alarm2", "Alarm3", "AlarmOut"] # columns in which the data from data is organised

df = pd.read_csv(data,
                 sep="\s+|,", # two types of separation
                 names = cols, # names of the columns
                 header = None, # csv file with no header, customized "cols"
                 engine = "python",
                 skiprows = 40, # first 40 rows are datalogger specifications
                 index_col = 1) #to search for specific hours in dataframe

#%% Example of data extracted from a given date

filter_day = '2022-07-21'
data_day = df[df.index.str.startswith(filter_day)]
data_day.index = data_day['DateTime']
mean_coef = 499
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
        
#%% # Get irradiance from voltage data

filtered_data['T_av'] = filtered_data[['CH19', 'CH20']].mean(axis=1) #average temperature
k= [0.1658, 0.1638, 0.1664, 0.1678, 0.3334, 0.1686, 0.1673, inf, inf, inf, inf, 0.3306, 0.3317, 0.3341, 0.3361]
alpha = 4.522e-4 # pu units
T0  = 298.15 # STC temperature

for i in range(1, 19):
    # Irradiance conversion with temperature dependance
    try:
        coef = 1 + alpha * ((filtered_data['T_av'] + 273.15)- T0)
        filtered_data['W' +  str(i)] = filtered_data["CH" + str(i)] / coef
        filtered_data['W' +  str(i)] /= k[i-1]
        print(k[i-1])
    except KeyError:
        continue

#%% Plottings

# East plate


plt.figure(num = 1)

for i in range (1,5):
    filtered_data['W' + str(i)].plot(linewidth = 1)
    
plt.xlabel('Date Time')
plt.ylabel('Irradiance [W/m$^2$]')
plt.title("East plate from " + filter_day)
plt.legend()
plt.grid(True)

# North West Plate

plt.figure(num = 2)

for i in range (5,9):
    filtered_data['W' + str(i)].plot(linewidth = 1)

plt.xlabel('Date Time')
plt.ylabel('Irradiance [W/m$^2$]')
plt.title("North-West plate from " + filter_day)
plt.legend()
plt.grid(True)

# North East Plate
plt.figure(num = 3)

for i in range (11,15):
    filtered_data['W' + str(i)].plot(linewidth = 1)

plt.xlabel('Date Time')
plt.ylabel('Irradiance [W/m$^2$]')
plt.title("South-West plate from " + filter_day)
plt.legend()
plt.grid(True)

# Comparison between plates
plt.figure(num = 4)
filtered_data['e_average'] = filtered_data[['W1', 'W2', 'W3', 'W4']].mean(axis=1)   
filtered_data['e_average'].plot(label = 'East', linewidth = 1.5)

filtered_data['NW_average'] = filtered_data[['W5', 'W6', 'W7', 'W8']].mean(axis=1)
filtered_data['NW_average'].plot(label = 'North West', linewidth = 1.5)

filtered_data['SW_average'] = filtered_data[['W11', 'W12', 'W13', 'W14']].mean(axis=1)
filtered_data['SW_average'].plot(label = 'South West', linewidth = 1.5)

plt.xlabel('Date Time')
plt.ylabel('Irradiance [W/m$^2$]')
plt.title("Average irradiance from " + filter_day)
plt.legend()
plt.grid(True)

#%% Daily insolation matrix

dt = 1
matrix = [[5, 5, 0, 0, 11, 11],
          [0, 0, 0, 0, 0 , 0 ],
          [0, 0, 0, 0, 0 , 0 ],
          [0, 6, 0, 0, 12, 12],
          [7, 0, 0, 0, 13, 13],
          [0, 0, 0, 0, 0 , 0 ],
          [0, 0, 0, 0, 0 , 0 ],
          [8, 8, 0, 0, 14, 14],
          [0, 0, 0, 4, 0 , 0 ],
          [0, 0, 0, 0, 0 , 0 ],
          [0, 0, 0, 0, 0 , 0 ],
          [0, 0, 0, 3, 0 , 0 ],
          [0, 0, 0, 2, 0 , 0 ],
          [0, 0, 0, 0, 0 , 0 ],
          [0, 0, 0, 0, 0 , 0 ],
          [0, 0, 0, 1, 0 , 0 ]]


insolation2d = np.zeros([len(matrix), len(matrix[0])])
#get insolation matrix
for i in range (0, len(matrix)):
    for j in range (0, len(matrix[0])):
        var = matrix[i][j]
        try:
            insolation2d[i][j] = filtered_data['CH' + str(var)].sum() / 3600
        except KeyError:
            insolation2d[i][j] = np.nan
            


plt.imshow(insolation2d, extent=[0, 42, 33, 0]) #sensor 
plt.title('Model insolation map')
plt.colorbar()