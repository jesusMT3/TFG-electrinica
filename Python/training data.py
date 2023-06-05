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
from datetime import timedelta
import datalogger as dl

# Global variables
gcr = 0.27
max_angle = 50

# Import dataframes
data = filedialog.askopenfile()

# First, datalogger data
df = pd.read_csv(data,
                 sep=",", # two types of separation
                 header = 0, # first row used as index
                 skiprows = 0, # skip index
                 engine = 'python',
                 parse_dates = True,
                 index_col = 0)

df.index = pd.to_datetime(df.index) 

# Location object for tracking data
location = location.Location(latitude = 40.453201, 
                             longitude = -3.726968)

# Second, meteodata
# meteodata = dl.data_import('meteodata')
# meteodata.index = pd.to_datetime(meteodata.index)

# Empty processed dataframe
processed_df = pd.DataFrame({})


df['group'] = np.nan

# Clasificate groups
aux = 1
for i in df.index:
    if (df['angle'].loc[i] == -50) or (df['angle'].loc[i] == 50):
        aux += 1

    df['group'].loc[i] = int(aux / 2)
    
# Sun tracking
sun_data = location.get_solarposition(times = df.index - timedelta(hours = 2))
backtrack_angle = tracking.singleaxis(apparent_zenith = sun_data['apparent_zenith'],
                              apparent_azimuth = sun_data['azimuth'],
                              axis_tilt = 0,
                              axis_azimuth = 0,
                              max_angle = 50,
                              backtrack = False,
                              gcr = 0.27)
backtrack_angle.index = backtrack_angle.index + timedelta(hours = 2)

# Get max power and max angle
for i in range(1, int(df['group'].max())):
    
    data = pd.DataFrame(df.loc[df['group'] == i])
    
    # Max data
    max_power = data['power_value'].max()
    maximum_power = data['power_value'].loc[data['power_value'] == max_power]  
    max_angle = data['angle'].loc[data['power_value'] == max_power]  
    ghi = data['GHI'].loc[data['power_value'] == max_power] 
    
    new_data = pd.DataFrame()
    new_data['max power'] = maximum_power
    new_data['max angle'] = -max_angle
    new_data['GHI'] = ghi / 1000
    
    processed_df = pd.concat([processed_df, new_data])
   
# Get tracker tilt
processed_df['tilt'] = backtrack_angle['tracker_theta'].loc[processed_df.index]
processed_df.index = processed_df.index.floor('T')


#Weather data
# weather_data = meteodata.loc[processed_df.index]
# processed_df['GHI'] = weather_data['Gh'] / 1000

# Get power from tracker tilt
processed_df['tracker power'] = pd.Series(dtype='float64')
for i in range(1, int(df['group'].max())):
    tracker_angle = processed_df['tilt'].iloc[i - 1]
    data = pd.DataFrame(df.loc[df['group'] == i])
    
    diff = abs(data['angle'] - tracker_angle)
    nearest_index = diff.idxmin()
    tracker_power = data.loc[nearest_index, 'power_value']

        
    processed_df['tracker power'].iloc[i - 1] = tracker_power

# Power gains
total_diference = 100 * (processed_df['max power'].sum() - processed_df['tracker power'].sum())/processed_df['max power'].sum()

# Plottings

# Power and ghi
fig, ax1 = plt.subplots(figsize=(10, 6))
ax1.plot(processed_df.index, processed_df['max power'], color='blue', label='Max Power')
ax1.plot(processed_df.index, processed_df['GHI'], color='red', label='GHI')
ax1.set_xlabel('Datetime')
ax1.set_ylabel('Power / Clarity')
ax1.set_title('Max Power and clarity index')
ax1.legend(loc='best')
ax1.tick_params(axis='x', rotation=45)
plt.tight_layout()  # Adjust spacing between subplots
plt.show()


# Angle and tilt
fig, ax2 = plt.subplots(figsize=(10, 6))
ax2.scatter(processed_df.index, processed_df['max angle'], color='green', label='Max Angle')
ax2.plot(processed_df.index, processed_df['tilt'], color='orange', label='Tilt')
ax2.set_xlabel('Datetime')
ax2.set_ylabel('Angle')
ax2.set_ylim([-60, 60])
ax2.set_title('Max Angle and Tilt')
ax2.legend(loc='best')
ax2.tick_params(axis='x', rotation=45)
plt.tight_layout()  # Adjust spacing between subplots
plt.show()

# Power difference
fig, ax3 = plt.subplots(figsize=(10, 6))
ax3.scatter(processed_df.index, processed_df['max power'], color='blue', label='Max Power')
ax3.scatter(processed_df.index, processed_df['tracker power'], color='orange', label='Tracker Power')
ax3.set_xlabel('Datetime')
ax3.set_ylabel('Power')
ax3.set_title(f'Power gains: {total_diference:.2f}%')
ax3.legend(loc='best')
ax3.tick_params(axis='x', rotation=45)

plt.tight_layout()  # Adjust spacing between subplots
plt.show()















