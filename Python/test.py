# -*- coding: utf-8 -*-
"""
Created on Wed Mar  1 12:16:05 2023

@author: Jesus
"""

import sys, os, time
sys.path.append(os.path.dirname(__file__))
import pvmismatch as pvm
import numpy as np
import datalogger as dl
import pandas as pd
import matplotlib.pyplot as plt
import multiprocessing

cell_params = {
    'Rs': 0.01,
    'Isat1_T0': 1e-10}

n = 6
m = 12

east = [('W1', 'W2', 'W3', 'W4'), ('W15')]
north_west = [('W5', 'W6', 'W7', 'W8'), ('W15')]
south_west = [('W11', 'W12', 'W13', 'W14'), ('W15')]
model_power = np.zeros(86400)

def main():
    
    global filtered_data
    global cell
    global module
    global system
    global model_power
    
    # import and filter data
    filtered_data = dl.datalogger_filter(df = dl.datalogger_import(dl.cols), 
                                         filt = None, 
                                         mean_coeff = 1000, 
                                         irr_coef = dl.irr_coef)
    
    east_plate = create_plate(channels = east, data = filtered_data)
    north_west_plate = create_plate(channels = north_west, data = filtered_data)
    south_west_plate = create_plate(channels = south_west, data = filtered_data)
    
    # create a cell with custom parameters
    cell = pvm.PVcell(**cell_params)

    # create a new module with the updated cell position pattern
    module = pvm.PVmodule(pvcells=cell, cell_pos = pvm.pvmodule.STD72)
    
    system = pvm.PVsystem(numberStrs=1, numberMods=10, pvmods=module)

    start_time = time.time()
    for i in range (50000, 50100):
        x, power = process_element(i, 
                               east_plate = east_plate, 
                               north_west_plate = north_west_plate, 
                               south_west_plate = south_west_plate, 
                               m = m, n = n, 
                               system = system)
        
        model_power[i] = power
        
    end_time = time.time()
    processing_time = end_time - start_time
    print(f"Processing time: {processing_time:.4f} seconds")
    
    print_plate(east_plate, north_west_plate, south_west_plate, '14:00:00', system)

# gets the array based on series configuration of cells
def module_irr(n, m, data):
    z = 0
    suns = np.empty(n*m)
    for i in range(0, n):
        if i != 0:
            if i % 2 == 0:
                z += m + 1
            elif i % 2 != 0:
                z += m - 1
        for j in range(0, m):
            suns[z] = data[j]
            if i % 2 != 0:
                z -= 1
            if i % 2 == 0:
                z += 1
    return suns

# creates interpolated channel plate
def create_plate(channels, data):
    plate = pd.DataFrame()
    for i in range (0, m):
        
        if i == 0 or i == 1:
            plate[i] = data[channels[0][0]] + data[channels[1]]
            
        elif i == (m / 2) - 1:
            plate[i] = data[channels[0][1]] + data[channels[1]]
            
        elif i == (m / 2) + 1:
            plate[i] = data[channels[0][2]] + data[channels[1]]
            
        elif i == m - 1 or i == m - 2:
            plate[i] = data[channels[0][3]] + data[channels[1]]
            
        else:
            plate[i] = np.empty(len(filtered_data)) * np.nan
            
    plate_interpolated = plate.interpolate(method = 'linear', axis = 1)
    return plate_interpolated

#prints the irradiation map of the hour
def print_plate(east_plate, north_west_plate, south_west_plate, hour, system):
    
    system.plotSys()
    
    array = np.zeros([m*2, 5])
    for i in range(0, m):
        array[i][0] = north_west_plate[i].loc[hour]
        array[i][1] = north_west_plate[i].loc[hour]
        array[i][2] = (south_west_plate[i].loc[hour] + north_west_plate[i].loc[hour]) / 2
        array[i][3] = south_west_plate[i].loc[hour]
        array[i][4] = south_west_plate[i].loc[hour]
        
    for i in range (0, m):
        for j in range (0, 5):
            array[i+m][j] = east_plate[i].loc[hour]
        
    dl.plot_insolation(figure = array, title = 'Irradiation map at '+ hour)
    
def process_element(x, east_plate, north_west_plate, south_west_plate, m, n, system):
    
    hour = filtered_data.index[x]
    temp = filtered_data['T_av'].loc[hour]
    
    east_irr = module_irr(n, m, data = east_plate.loc[east_plate.index[x]])
    north_west_irr = module_irr(n, m, data = north_west_plate.loc[north_west_plate.index[x]])
    south_west_irr = module_irr(n, m, data = south_west_plate.loc[south_west_plate.index[x]])
    
    Ee = {0: {0: south_west_irr / 1000,
              1: south_west_irr / 1000,
              2: (north_west_irr + south_west_irr) / (2 * 1000),
              3: north_west_irr / 1000,
              4: north_west_irr / 1000,
              5: east_irr / 1000,
              6: east_irr / 1000,
              7: east_irr / 1000,
              8: east_irr / 1000,
              9: east_irr / 1000}}
    
    system.setSuns(Ee)
    system.setTemps(temp + 273.15)
    power = system.Pmp
    return x, power

if __name__ == "__main__":
    main()