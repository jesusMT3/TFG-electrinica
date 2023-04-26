# -*- coding: utf-8 -*-
"""
Created on Mon Feb 20 10:25:10 2023

Program which calculates real power from solar bifacial system, based on experimental
front and back irradiance data

@author: Jesus
"""
import sys, os, time
sys.path.append(os.path.dirname(__file__))
import datalogger as dl
import numpy as np
import pandas as pd
from pvmismatch import PVmodule, PVsystem
import matplotlib.pyplot as plt
import multiprocessing



# cols = ["No", "DateTime", "ms", "BW-E", "BW-ME", "BW-MI", "BW-I", "FW-E", 
#         "FW-ME", "FW-MI", "FW-I", "BE-E", "BE-ME", "BE-MI", "BE-I", "FE-E", 
#         "FE-ME", "FE-MI", "FE-I", "GS1", "GS2", "GS3", "GS4", "Alarm1", 
#         "Alarm2", "Alarm3", "AlarmOut"] # columns in which the data from data is organised

# Irradiance conversion coefficients
# irr_coef = [0.1658, 0.1638, 0.1664, 0.1678, 0.3334, 0.1686, 0.1673, np.inf, np.inf, np.inf, np.inf, 0.3306, 0.3317, 0.3341, 0.3361]

module = PVmodule()
system = PVsystem(numberMods=2,numberStrs=1, pvmods = module)
n = 0
m = 0

for i in module.subStrCells:
    
    n += i
    
m = module.numberCells
m /= n
m = int(m)

power = None

# Configuration parameters


REPROCESS_ARRAY = True

def main():
    
    # Global variables
    global plate1, plate_west, plate_east
    global results, suns
    global power
    global filtered_data
    global system

    
    power = np.zeros(86400)

    # Import data from the datalogger
    data = dl.data_import('datalogger')
    
    # Convert data to irradiance, temperature correction, and smoothing
    filtered_data = dl.datalogger_filter(df = data,
                                      mean_coeff = 1, 
                                      irr_coef = dl.irr_coef, 
                                      ch_temp = 'CH19')

    # Create all plate irradiance levels through interpolation
    plate_west, plate_east = create_plates_df(filtered_data)
    plate_west = plate_west.interpolate(method = 'linear', axis = 1)
    plate_east = plate_east.interpolate(method = 'linear', axis = 1)
    
    #To measure performance ratio of the system
    start_time = time.time()
    
    if REPROCESS_ARRAY:
        
        # Multiprocess loop
        with multiprocessing.Pool() as pool:
            
            # Arguments of the calc_power function
            args = [(x, plate_west, plate_east, system, m, n) for x in filtered_data.index]
            
            # Actual calculation of system power
            results = pool.starmap(calc_power, args)
        
        # Rearrange results into a dataframe
        results = pd.DataFrame(results, columns = ['DateTime', 'power_value'])
        power = pd.DataFrame(results['power_value'])
        power.index = results['DateTime']
        
        # Save CSV file with the data obtained
    else:
        power = np.loadtxt('power_array.txt')
    
    # Finish time performance measure
    end_time = time.time()
    processing_time = end_time - start_time
    print(f"Processing time: {processing_time:.4f} seconds")

    # Plot data
    plt.figure(figsize = (10,6))
    power.plot()
    plt.title('Power')
    plt.tight_layout()
    
    # Plot specific data points
    iv_irr_data('2023-03-16 11:46:00')
    iv_irr_data('2023-03-16 11:47:00')
    iv_irr_data('2023-03-16 12:37:00')

# Place sensor data into a dataframe of all levels of irradiance
def create_plates_df(filtered_data):
    plate_west = pd.DataFrame(columns = range(0,m))
    plate_east = pd.DataFrame(columns = range(0,m))
    
    for i in range (0, m):
        
        # Exterior sensor
        if i == 0 or i == 1:
            plate_west[i] = filtered_data['W1'] + filtered_data['W5']
            plate_east[i] = filtered_data['W9'] + filtered_data['W13']
            
        # Mid exterior sensor
        elif i == (m / 2) - 1:
            plate_west[i] = filtered_data['W2'] + filtered_data['W6']
            plate_east[i] = filtered_data['W10'] + filtered_data['W14']
            
        # Mid interior sensor
        elif i == (m / 2):
            plate_west[i] = filtered_data['W3'] + filtered_data['W7']
            plate_east[i] = filtered_data['W11'] + filtered_data['W15']
            
        # Interior sensor
        elif i == m - 1 or i == m - 2:
            plate_west[i] = filtered_data['W4'] + filtered_data['W8']
            plate_east[i] = filtered_data['W12'] + filtered_data['W16']
            
        # Spots without sensors (they will be interpolated)
        else:
            plate_west[i] = np.empty(len(filtered_data)) * np.nan
            plate_east[i] = np.empty(len(filtered_data)) * np.nan
            
    # Set temperature data
    plate_west['T'] = filtered_data['CH19']
    plate_east['T'] = filtered_data['CH19']
        
    return plate_west, plate_east

# Function that outputs peak power with given irradiance maps of the system
def calc_power(x, plate_west, plate_east, system, m, n):

    # Arrays of irradiance of all cells
    irr_west = np.zeros(module.numberCells)
    irr_east = np.zeros(module.numberCells)
    
    # Data from the datalogger
    data_west = plate_west.loc[x]
    data_east = plate_east.loc[x]
    
    # Create irradiance arrays
    for i in range(0, module.numberCells):
        irr_west[i] = data_west[i // m]
    
    # Place irradiance in the given order of the solar pannel module 
    for i in module.cell_pos:
        for j in i:
            
            aux = 0
            for k in j:
                idx = k['idx']
                irr_west[idx] = data_west[aux]/1000
                irr_east[idx] = data_east[aux]/1000
                aux += 1
    
    # There can't be non zero irradiance values (the funct won't work)
    irr_west[irr_west <= 0] = 0.001
    irr_east[irr_east <= 0] = 0.001

    dict_suns = {0: {0: irr_west, 1: irr_east}}
    
    # Set irradiance 
    system.setSuns(dict_suns)
    
    # Set temperature
    system.setTemps(data_west['T'] + 273.15)
    
    # Get peak power
    power = system.Pmp
    
    # Return data
    return x, power
    
# Function which calculates IV curve of specific points in the data
def iv_irr_data(hour):
    
    # Plot the IV curve
    x, power = calc_power(hour, plate_west, plate_east, system, m, n)
    
    fig1 = plt.Figure()
    fig1 = system.plotSys()
    fig1.tight_layout()


if __name__ == "__main__":
    main()
