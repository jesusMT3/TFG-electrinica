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
import numpy as np

# Import dataframe
data = filedialog.askopenfile()
df = pd.read_csv(data,
                 sep=",", # two types of separation
                 header = 0, # first row used as index
                 skiprows = 0, # skip index
                 engine = 'python',
                 index_col = 0) 

# Location object for tracking data
location = location.Location(latitude = 40.453201, 
                             longitude = -3.726968)

meteodata = dl.data_import('meteodata')


# Empty processed dataframe
processed_df = pd.DataFrame()
backtrack_df = pd.DataFrame({})

# Backtrack angle
weather_data = meteodata.loc[df.index]
sun_data = location.get_solarposition(times = df.index)
tracker = tracking.SingleAxisTrackerMount(backtrack = True, 
                                          gcr = 0.27,
                                          max_angle = 55)

backtrack_angle = tracker.get_orientation(solar_zenith = sun_data['apparent_zenith'],
                                           solar_azimuth = sun_data['azimuth'])

df['group'] = np.nan

# Clasificate groups
aux = 1
for i in df.index:
    if (df['angle'].loc[i] == -55) or (df['angle'].loc[i] == 55):
        aux += 1
    
        
    df['group'].loc[i] = int(aux / 2)
    
for i in range(0, int(df['group'].max())):
    
    data = pd.DataFrame(df.loc[df['group'] == i])
    max_power = data['power_value'].max()
    max_angle = data['angle'].loc[data['power_value'] == max_power]   
    backtrack = backtrack_angle['tracker_theta'].loc[max_angle.index]
    
    # closest_index = (data['angle'] - backtrack).abs().idxmin()


