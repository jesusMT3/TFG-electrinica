# -*- coding: utf-8 -*-
"""
Created on Mon Mar 20 19:22:55 2023

@author: Jesus
"""

# Imports
import sys, os, time
sys.path.append(os.path.dirname(__file__))
import pvmismatch as pvm
import datalogger as dl
import numpy as np

#constant defs
n = 6 # Columns of module
m = 12 # Rows of module
east_plate = ['CH1', 'CH2', 'CH3', 'CH4']

def main():
    
    #global variables
    global df
    global system
    global module
    global cell
    
    #create cell, module and system
    cell = pvm.pvcell.PVcell()
    module = pvm.pvmodule.PVmodule()
    
    #import data from datalogger
    df = dl.data_import('datalogger')
    
    #obtain irradiance and temperature corrected values
    df = dl.datalogger_filter(df = df, mean_coeff = 1000, irr_coef = dl.irr_coef, ch_temp = 'CH19')
    
    #plot channels (eg. East plate)
    dl.plot_channels(magnitude = 'Irradiance [W/m$^2$]', 
                     dataframe = df, 
                     plate = east_plate, 
                     title = 'East plate irradiance')

    
# Irradiance map, which gets the zigzag position of a given plate value
def irradiance_map(plate):
    
    #initialize some variables
    z = 0
    suns = np.zeros(n*m)
    
    #for loop of the iterations
    for i in range(0, n):
        if i != 0:
            if i % 2 == 0:
                z += m + 1
            elif i % 2 != 0:
                z += m - 1
        for j in range(0, m):
            suns[z] = plate[j]
            if i % 2 != 0:
                z -= 1
            if i % 2 == 0:
                z += 1
                
    return(suns)
    
if __name__ == '__main__':
    main()