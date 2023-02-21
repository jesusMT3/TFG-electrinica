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
import pvlib as pv

df = f.datalogger_import(f.cols)
filtered_data = f.datalogger_filter(df = df, 
                                    filt = input('Date filter: '), 
                                    mean_coeff = 1000,
                                    irr_coef = f.irr_coef)

east_plate = ['W1', 'W2', 'W3', 'W4']
north_west = ['CH5', 'CH6', 'CH7', 'CH8']
south_west = ['CH11', 'CH12', 'CH13', 'CH14']

height_plate = 210 #mm
width_plate = 110 #mm
area = height_plate*width_plate/1000 #m^2

#power of East plate
f.plot_channels(magnitude = 'Photocurrent [mA]', 
                dataframe = filtered_data, 
                plate = ['IL1', 'IL2', 'IL3', 'IL4'], 
                title = 'East plate photocurrent')

