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
location = location.Location(latitude = 40.453201, 
                             longitude = -3.726968)

meteodata = dl.data_import('meteodata')


# Empty processed dataframe
processed_df = pd.DataFrame()
backtrack_df = pd.DataFrame({})

# Flags to go through dataframe index
flag_55 = False
flag_m55 = False
array_index = []

# Backtrack angle
weather_data = meteodata.loc[df.index]
sun_data = location.get_solarposition(times = df.index)
tracker = tracking.SingleAxisTrackerMount(backtrack = True, 
                                          gcr = 0.27,
                                          max_angle = 55)

backtrack_angle = tracker.get_orientation(solar_zenith = sun_data['apparent_zenith'],
                                           solar_azimuth = sun_data['azimuth'])




