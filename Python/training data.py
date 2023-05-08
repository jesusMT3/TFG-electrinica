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
backtrack_df = pd.DataFrame({})

# Flags to go through dataframe index
flag_55 = False
flag_m55 = False
array_index = []

# Backtrack angle
weather_data = meteodata.loc[df.index]
sun_data = location.get_solarposition(times = df.index)
tracker = tracking.SingleAxisTrackerMount(backtrack = True, 
                                          gcr = 0.27)

back_track_angle = tracker.get_orientation(solar_zenith = sun_data['apparent_zenith'],
                                           solar_azimuth = sun_data['azimuth'])

df.index = back_track_angle.index

for i in df.index:
    
    if df['angle'].loc[i] == -55:
        flag_55 = True
        flag_m55 = False
        
        # Get max power data
        max_power = df['power_value'].loc[array_index].max()
        max_data = pd.DataFrame(df[df['power_value'] == max_power])
        
        # Get backtrack angle and power
        angle = back_track_angle['tracker_theta'].loc[i]
        backtrack_power = df.loc[(df['angle'] - angle).abs().idxmin(), 'power_value']
        backtrack_data = pd.DataFrame(df[df['power_value'] == backtrack_power])
        processed_df = pd.concat([processed_df,max_data])
        backtrack_df = pd.concat([backtrack_df, backtrack_data])
        
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
      

# processed_df['backtrack_angle'] = back_track_angle['tracker_theta'].loc[processed_df.index]

# # Maximum power with its respect angle
# plt.figure(figsize = (10, 6))
# processed_df['power_value'].plot() 
# processed_df['angle'].plot()
# plt.tight_layout()

# # Comparison between backtrack angle and maximum angle
# plt.figure(figsize = (10, 6))
# processed_df['angle'].plot()
# processed_df['backtrack_angle'].plot()
# plt.tight_layout()

# # Comparison between max power and back track power
# back_track_power = processed_df
# back_track_power['DateTime'] = processed_df.index
# back_track_power.index = processed_df['angle']


# backtrack_power = processed_df.loc[back_track_angle['tracker_theta'].index]

