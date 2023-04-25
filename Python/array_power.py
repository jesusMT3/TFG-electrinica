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
    global plate1, plate_west, plate_east
    global results, suns
    global power
    global filtered_data
    global system

    power = np.zeros(86400)

    data = dl.data_import('datalogger')
    filtered_data = dl.datalogger_filter(df = data,
                                      mean_coeff = 1, 
                                      irr_coef = dl.irr_coef, 
                                      ch_temp = 'CH19')

    plate_west, plate_east = create_plates_df(filtered_data)
    plate_west = plate_west.interpolate(method = 'linear', axis = 1)
    plate_east = plate_east.interpolate(method = 'linear', axis = 1)
    
    start_time = time.time()
    
    if REPROCESS_ARRAY:
        with multiprocessing.Pool() as pool:
            args = [(x, plate_west, plate_east, system, m, n) for x in filtered_data.index]
            results = pool.starmap(calc_power, args)
            
        results = pd.DataFrame(results, columns = ['DateTime', 'power_value'])
        power = pd.DataFrame(results['power_value'])
        power.index = results['DateTime']
        
        np.savetxt('power_array.txt', power)
    else:
        power = np.loadtxt('power_array.txt')
    
    end_time = time.time()
    processing_time = end_time - start_time
    print(f"Processing time: {processing_time:.4f} seconds")

    
    plt.figure(figsize = (10,6))
    power.plot()
    plt.title('Power')
    plt.tight_layout()
    
    iv_irr_data('2023-03-16 11:46:00')
    iv_irr_data('2023-03-16 11:47:00')
    iv_irr_data('2023-03-16 12:37:00')

def create_plate1_df(filtered_data, plate_front):
    plate1 = pd.DataFrame()
    for i in range (0, m):
        if i == 0 or i == 1:
            plate1[i] = filtered_data['W1'] + plate_front
        elif i == (m / 2) - 1:
            plate1[i] = filtered_data['W2'] + plate_front
        elif i == (m / 2) + 1:
            plate1[i] = filtered_data['W3'] + plate_front
        elif i == m - 1 or i == m - 2:
            plate1[i] = filtered_data['W4'] + plate_front
        else:
            plate1[i] = np.empty(len(filtered_data)) * np.nan
            
        plate1['T'] = filtered_data['CH19']
    return plate1

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
        else:
            plate_west[i] = np.empty(len(filtered_data)) * np.nan
            plate_east[i] = np.empty(len(filtered_data)) * np.nan
            
        # Set temperature data
    plate_west['T'] = filtered_data['CH19']
    plate_east['T'] = filtered_data['CH19']
        
    return plate_west, plate_east

def process_element(x, plate1, module, m, n):
    
    data = plate1.loc[x]
    temp_data = plate1['T'].loc[x]
    suns = np.zeros(m*n)
    z = 0
        
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
    
    module.setSuns(suns/1000)
    module.setTemps(temp_data + 273.15)
    power = module.Pmod.max()
    return x, power

def calc_power(x, plate_west, plate_east, system, m, n):


    irr_west = np.zeros(module.numberCells)
    irr_east = np.zeros(module.numberCells)
    
    data_west = plate_west.loc[x]
    data_east = plate_east.loc[x]
    
    # Create irradiance arrays
    for i in range(0, module.numberCells):
        irr_west[i] = data_west[i // m]
        
    for i in module.cell_pos:
        for j in i:
            
            aux = 0
            for k in j:
                idx = k['idx']
                irr_west[idx] = data_west[aux]/1000
                irr_east[idx] = data_east[aux]/1000
                aux += 1
    
    # There can't be non zero irradiance values 
    irr_west[irr_west <= 0] = 0.001
    irr_east[irr_east <= 0] = 0.001

    dict_suns = {0: {0: irr_west, 1: irr_east}}
    
    system.setSuns(dict_suns)
    system.setTemps(data_west['T'] + 273.15)
    power = system.Pmp
    return x, power
    

def iv_irr_data(hour):
    
    # Plot the IV curve
    x, power = calc_power(hour, plate_west, plate_east, system, m, n)
    
    fig1 = plt.Figure()
    fig1 = system.plotSys()
    fig1.tight_layout()

if __name__ == "__main__":
    main()
