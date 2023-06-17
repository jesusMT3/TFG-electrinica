# -*- coding: utf-8 -*-
"""
Created on Sat May  6 11:18:40 2023

@author: Jesus
"""

# Import libraries
from tkinter import filedialog
import pandas as pd
from pvlib import tracking, location
import numpy as np
from datetime import timedelta
import datalogger as dl
import pvlib as pv

# Global variables
gcr = 0.27
max_angle = 50

# Sensor distribution
BE = ['BE-exterior', 'BE-mid-exterior', 'BE-mid-interior', 'BE-interior']
FE = ['FE-exterior', 'FE-mid-exterior', 'FE-mid-interior', 'FE-interior']
BW = ['BW-exterior', 'BW-mid-exterior', 'BW-mid-interior', 'BW-interior']
FW = ['FW-exterior', 'FW-mid-exterior', 'FW-mid-interior', 'FW-interior']
sys = BE + FE + BW + FW
bifaciality = 0.75

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

# Meteodata
meteodata = dl.data_import('meteodata')

# Location object for tracking data
location = location.Location(latitude = 40.453201, 
                             longitude = -3.726968)

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
df['tilt'] = backtrack_angle['tracker_theta']

# Get max power and max angle
for i in range(1, int(df['group'].max())):
    data = pd.DataFrame(df.loc[df['group'] == i])
    
    # Max data
    max_power = data['power_value'].max()
    maximum_power = data['power_value'].loc[data['power_value'] == max_power]  
    max_angle = data['angle'].loc[data['power_value'] == max_power]  
    ghi = data['GHI'].loc[data['power_value'] == max_power] 
    desv_ghi = (data['GHI'].max() - data['GHI'].min()) / (data['GHI'].max() + data['GHI'].min()) / 2
    tilt = data['tilt'].loc[data['power_value'] == max_power] 
    
    # Tracker data
    diff = abs(data['angle'] - float(tilt))
    nearest_index = diff.idxmin()
    ephemeris_data = data.loc[nearest_index]
    
    # Concat to processed_df
    new_data = pd.DataFrame()
    new_data['optimal power'] = maximum_power
    new_data['optimal power non bifacial'] = data['power_value_non_bif']
    new_data['ephemeris power'] = ephemeris_data['power_value']
    new_data['ephemeris power non bifacial'] = ephemeris_data['power_value_non_bif']
    new_data['optimal angle'] = max_angle
    new_data['tilt'] = tilt
    new_data['GHI'] = ghi / 1000
    new_data['dGHI'] = desv_ghi
    
    new_data[sys] = data[sys].loc[data['power_value'] == max_power]
    new_data['BE'] = data['BE'].loc[data['power_value'] == max_power]
    new_data['FE'] = data['FE'].loc[data['power_value'] == max_power]
    new_data['BW'] = data['BW'].loc[data['power_value'] == max_power]
    new_data['FW'] = data['FW'].loc[data['power_value'] == max_power]
    
    for i in sys:
        new_data['ephemeris ' + i] = ephemeris_data[i]
        
    new_data['ephemeris BE'] = ephemeris_data['BE']
    new_data['ephemeris FE'] = ephemeris_data['FE']
    new_data['ephemeris BW'] = ephemeris_data['BW']
    new_data['ephemeris FW'] = ephemeris_data['FW']

    # Bifacial gains
    new_data['Optimal Bifacial gains'] = 100 * ((new_data['optimal power'] - new_data['optimal power non bifacial']) / new_data['optimal power non bifacial'])
    new_data['Optimal Bifacial gains irr'] = 100 * bifaciality * ((new_data['BE'] + new_data['BW']) / (new_data['FE'] + new_data['FW']))
    
    new_data['Ephemeris Bifacial gains'] = 100 * (new_data['ephemeris power'] - new_data['ephemeris power non bifacial'] / new_data['ephemeris power non bifacial'])
    new_data['Ephemeris Bifacial gains irr'] = 100 * bifaciality * ((new_data['ephemeris BE'] + new_data['ephemeris BW']) / (new_data['ephemeris FE'] + new_data['ephemeris FW']))

    processed_df = pd.concat([processed_df, new_data])
    
# Remove data with high GHi variability
processed_df = processed_df[processed_df['dGHI'] < 0.03]
processed_df.index = processed_df.index.floor('T')

# Ratio GHI/DHI
processed_df['DHI/GHI'] = meteodata['Dh'].loc[processed_df.index] / meteodata['Gh'].loc[processed_df.index]

# Wind speed and temperature
processed_df['Wspeed'] = meteodata['V.Vien.1'].loc[processed_df.index]
processed_df['Temp'] = meteodata['Temp. Ai 1'].loc[processed_df.index]

# CLearness index
sun_data = location.get_solarposition(times = processed_df.index)
extraterrestrial_radiation = pv.irradiance.get_extra_radiation(processed_df.index)
ghi = processed_df['GHI']
ghi *= 1000

kt = pv.irradiance.clearness_index(ghi = ghi, 
                                   solar_zenith = sun_data['apparent_zenith'], 
                                   extra_radiation = extraterrestrial_radiation)

kt = pd.DataFrame({'clearness index': kt})
processed_df['clearness index'] = kt['clearness index']

# Save csv
file_path = filedialog.asksaveasfilename(defaultextension='.csv')
processed_df.to_csv(file_path, index = True)