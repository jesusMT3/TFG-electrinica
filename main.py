# -*- coding: utf-8 -*-
"""
Created on Mon Feb  6 17:14:55 2023
Data treatment from datalogger CSV file.
@author: Jesús
"""

import pandas as pd
from matplotlib import pyplot as plt
import os
from mylibs import myfunctions as fn
 
#%%

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
data_21_07 = df[df.index.str.startswith(filter_day)]
data_21_07.index = data_21_07['DateTime']


# East plate
for i in range(1, 5):
    aux_str = "CH" + str(i)
    data_21_07[aux_str].plot()
    
plt.xlabel('Date Time')
plt.ylabel('Voltage [mV]')
plt.title("East plate from "+ filter_day)
plt.legend()

#%%
# Mobile-mean filtering of channel data