# # -*- coding: utf-8 -*-
# """
# Created on Mon Feb 20 10:25:10 2023

# Program which calculates real power from solar module

# @author: Jesus
# """
import sys, os, time
sys.path.append(os.path.dirname(__file__))
import datalogger as dl
import numpy as np
import pandas as pd
from pvmismatch import PVmodule
import matplotlib.pyplot as plt
import multiprocessing

n = 8
m = 12

cols = ["No", "DateTime", "ms", "CH1", "CH2", "CH3", "CH4", "CH5", 
        "CH6", "CH7", "CH8", "CH9", "CH11", "CH12", "CH13", "CH14", 
        "CH15", "T1", "T2", "GS1", "GS2", "GS3", "GS4", "Alarm1", 
        "Alarm2", "Alarm3", "AlarmOut"] # columns in which the data from data is organised

# Irradiance conversion coefficients
irr_coef = [0.1658, 0.1638, 0.1664, 0.1678, 0.3334, 0.1686, 0.1673, np.inf, np.inf, np.inf, np.inf, 0.3306, 0.3317, 0.3341, 0.3361]

module = PVmodule()
plate1 = None
power = None

REPROCESS_ARRAY = True

def main():
    global plate1
    global power
    global filtered_data

    power = np.zeros(86400)

    filtered_data = dl.datalogger_filter(df = dl.datalogger_import(cols), 
                                        filt = None, 
                                        mean_coeff = 1000, 
                                        irr_coef = irr_coef)

    plate_front = filtered_data['W15']
    plate1 = create_plate1_df(filtered_data, plate_front).interpolate(method = 'linear', axis = 1)
    plate1['Mean'] = plate1.mean(axis = 1)

    dl.plot_channels(magnitude = 'Irradiance [W/m$^2$]', 
                    dataframe = plate1, 
                    plate = plate1, 
                    title = 'East plate global Irradiance')

    start_time = time.time()

    if REPROCESS_ARRAY:
        with multiprocessing.Pool() as pool:
            args = [(x, plate1, module, m, n) for x in range(50000, 51000)]
            results = pool.starmap(process_element, args)

        for x, power_value in results:
            power[x] = power_value
        np.savetxt('power_array.txt', power)
    else:
        power = np.loadtxt('power_array.txt')
    
    end_time = time.time()
    processing_time = end_time - start_time
    print(f"Processing time: {processing_time:.4f} seconds")

    plate1['Power'] = power
    
    plt.figure()
    plate1['Power'].loc[plate1['Power'] > 0].plot()
    plt.title('Power')
    
    plt.figure()
    plate1['Mean'].loc[plate1['Power'] > 0].plot()
    plt.title('Mean')
    
    iv_irr_data('14:00:00')

def create_plate1_df(filtered_data, plate_front):
    plate1 = pd.DataFrame()
    for i in range (0, m):
        if i == 0:
            plate1[i] = filtered_data['W1'] + plate_front
        elif i == (m / 2) - 1:
            plate1[i] = filtered_data['W2'] + plate_front
        elif i == (m / 2) + 1:
            plate1[i] = filtered_data['W3'] + plate_front
        elif i == m - 1:
            plate1[i] = filtered_data['W4']/100
        else:
            plate1[i] = np.empty(len(filtered_data)) * np.nan
    return plate1

def process_element(x, plate1, module, m, n):
    name = plate1.index[x]
    data = plate1.loc[name]
    # temp_data = filtered_data['T_av'].loc[name]
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
    # module.setTemps(temp_data)
    power = module.Pmod.max()
    return x, power

def iv_irr_data(hour):
    index = plate1.index.get_loc(hour)
    x, power = process_element(index, plate1, module, m, n)
    
    data = plate1.loc[hour]
    
    insolations = np.zeros([m,n])
    for i in range (0, m):
        var = data[i]
        for j in range(0, n):
            insolations[i][j] = var
            continue

    # Filter out positive values for both plots
    positive_mask = (module.Pmod > 0)
    Imod_positive = module.Imod[positive_mask]
    
    Vmod_positive = module.Vmod[positive_mask]
    Imod_positive = module.Imod[positive_mask]
    
    Pmod_positive = module.Pmod[positive_mask]
    Imod_positive = module.Imod[positive_mask]
    
    # Create the subplots
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 4))
    
    # Plot 1: Vmod vs Imod
    ax1.plot(Vmod_positive, Imod_positive)
    ax1.set_xlabel('Voltage [V]')
    ax1.set_ylabel('Current [A]')
    ax1.set_title('I-V curve')
    
    # Plot 2: Pmod vs Imod
    ax2.plot(Vmod_positive, Pmod_positive)
    ax2.set_xlabel('Voltage[V]')
    ax2.set_ylabel('Power [W]')
    ax2.set_title('P-V curve')
    
    # Set the same axis limits for both subplots
    ax1.set_xlim(left=0)
    ax2.set_xlim(left=0)
    ax2.set_ylim(bottom=0)
    
    # Show the plot
    plt.tight_layout()
    plt.show()
         
    dl.plot_insolation(figure=insolations, title='Irradiance at ' + hour)
    
    print('Power at ', hour, ': ', power)

if __name__ == "__main__":
    main()
