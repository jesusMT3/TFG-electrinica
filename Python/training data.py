# -*- coding: utf-8 -*-
"""
Created on Sat May  6 11:18:40 2023

@author: Jesus
"""

# Import libraries
from tkinter import filedialog
import pandas as pd
import matplotlib.pyplot as plt
from pvlib import tracking, location
import datalogger as dl

# Import dataframe
data = filedialog.askopenfile()
df = pd.read_csv(data,
                 sep=",", # two types of separation
                 header = 0, # first row used as index
                 skiprows = 0, # skip index
                 engine = 'python',
                 index_col = 0) 

# Location object for tracking data
location = location.Location(latitude = 40.453201, longitude = -3.726968)

meteodata = dl.data_import('meteodata')

# Empty processed dataframe
processed_df = pd.DataFrame({})

# Flags to go through dataframe index
flag_55 = False
flag_m55 = False
array_index = []

for i in df.index:
    
    if df['angle'].loc[i] == -55:
        flag_55 = True
        flag_m55 = False
        
        # Get max power data
        max_power = df['power_value'].loc[array_index].max()
        max_data = df[df['power_value'] == max_power]
        processed_df = pd.concat([processed_df,max_data])
        
        # Delete array_index
        array_index = []
        
    if df['angle'].loc[i] == 55:
        flag_m55 = True
        flag_55 = False
        
        # Get max power data
        max_power = df['power_value'].loc[array_index].max()
        max_data = df[df['power_value'] == max_power]
        processed_df = pd.concat([processed_df,max_data])
        
        # Delete array_index
        array_index = []
        
    if flag_55 == True:
        array_index.append(i)
        
    if flag_m55 == True:
        array_index.append(i)

# Backtrack angle
weather_data = meteodata.loc[processed_df.index]
tracker = tracking.SingleAxisTrackerMount(backtrack = True, 
                                          gcr = 0.27)

back_track_angle = tracker.get_orientation(weather_data['Elev.Sol_2'],
                                    weather_data['Orient.Sol_2'])

processed_df['tracker_theta'] = back_track_angle['tracker_theta']

# Maximum power with its respect angle
plt.figure(figsize = (10, 6))
processed_df['power_value'].plot() 
processed_df['angle'].plot()
plt.tight_layout()

# Comparison between backtrack angle and maximum angle
plt.figure(figsize = (10, 6))
processed_df['angle'].plot()
processed_df['tracker_theta'].plot()
plt.tight_layout()

