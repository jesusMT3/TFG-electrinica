# -*- coding: utf-8 -*-
"""
Created on Mon Feb  6 17:14:55 2023
Data treatment from datalogger CSV file.
CSV Data from datalogger must be in "data" directory
@author: Jesús
"""

import pandas as pd
from matplotlib import pyplot as plt
import os
from scipy.signal import medfilt
 
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

#%%
# Example of data extracted from a given date

filter_day = '2022-07-21'
data_day = df[df.index.str.startswith(filter_day)]
data_day.index = data_day['DateTime']
mean_coef = 201

#filtering data
filtered_data = pd.DataFrame(data_day)

for i in range(1, 19):
    try:
        aux_str = "CH" + str(i)
        filtered_data[aux_str] = medfilt(data_day[aux_str], mean_coef)
    except KeyError:
        print("Channel ",i ," does not exist")
#%%

#Temperature
for i in range(19, 21):
    
    aux_str = "CH" + str(i)
    filtered_data[aux_str].plot()
    
plt.xlabel('Date Time')
plt.ylabel('Temperature [\Degree]')
plt.title("Model temperature from " + filter_day)
plt.legend()
plt.grid(True)

#%%
# East plate
for i in range(1, 5):
    
    aux_str = "CH" + str(i)
    filtered_data[aux_str].plot()
    
plt.xlabel('Date Time')
plt.ylabel('Voltage [mV]')
plt.title("East plate from " + filter_day)
plt.legend()
plt.grid(True)

#%%
# North West plate
for i in range(5, 9):
    
    aux_str = "CH" + str(i)
    filtered_data[aux_str].plot()
    
plt.xlabel('Date Time')
plt.ylabel('Voltage [mV]')
plt.title("North-West plate from " + filter_day)
plt.legend()
plt.grid(True)

#%%
# North East plate
for i in range(11, 15):
    
    aux_str = "CH" + str(i)
    filtered_data[aux_str].plot()
    
plt.xlabel('Date Time')
plt.ylabel('Voltage [mV]')
plt.title("North-East plate from " + filter_day)
plt.legend()
plt.grid(True)
