# -*- coding: utf-8 -*-
"""
Created on Tue Mar  7 12:10:36 2023
Program which calculates automatically irradiance/voltage coefficients
based on linnear regression algorithms
@author: Jesus
"""

import sys, os
sys.path.append(os.path.dirname(__file__))
import datalogger as dl
import pandas as pd
import scipy
import matplotlib.pyplot as plt
import numpy as np
from datetime import timedelta

df = pd.DataFrame()
change_hour = True

# plate = ['CH1', 'CH2', 'CH3', 'CH4', 'CH5', 'CH6', 'CH7', 'CH8']
plate = ['CH1', 'CH2', 'CH3', 'CH4']

def main():
    global df
    global datalogger_data
    global meteodata
    global coefficients
    
    # Import data from datalogger and meteodata station
    datalogger_data = dl.data_import('datalogger')
    meteodata = dl.data_import('meteodata')
    
    # Change of hour?
    if change_hour == True:
        datalogger_data.index += timedelta(hours=1)
    
    # If datalogger data is in seconds, uncomment this line
    datalogger_data = datalogger_data.resample('T').mean()
    
    # Create dataframe with requested data
    
    df['GHI'] = meteodata['Gh'][meteodata.index.isin(datalogger_data.index)]
    df['Temp'] = datalogger_data['CH9']
    
    for i in plate:
        df[i] = datalogger_data[i]

    # Temperature correction
    alpha = 4.522e-4 # pu units
    T0  = 25 # STC temperature
    for i in plate:
        df[i] /= (1 + alpha * (df['Temp'] - T0))
    
    # coefficient
    for i in plate:
        df['k ' + i] = df[i] / df['GHI']
   
    # Rearrange index to GHI for the plotting
    df.index = df['GHI'] 
        
    for i in plate:   
        
        # Get linear regress coefficients
        x = df['GHI'].to_numpy()
        y = df[i].to_numpy()
        coefficients = scipy.stats.linregress(x, y)
        
        # Eliminate outliers
        calc = df['GHI'] * coefficients[0] + coefficients[1]
        df['error ' + i] = ((df[i] - calc) / calc)*100
        for j in df.index:
            if df['error ' + i].loc[j] > 1 or df['error ' + i].loc[j] < -1:
                df[i].loc[j] = np.nan
                
        
        
        # Get new linear regress coefficients
        x = df['GHI'].to_numpy()
        y = df[i].to_numpy()
        
        # # Clean NaN data
        finiteYmask = np.isfinite(y)
        Yclean = y[finiteYmask]
        Xclean = x[finiteYmask]
        coefficients = scipy.stats.linregress(Xclean, Yclean)
        
        # Plottings
        fig, axs = plt.subplots(2, 1, figsize=(8, 8))

        # Calibration plot
        axs[0].scatter(df.index, df[i], label=i)
        axs[0].plot(Xclean, coefficients[0] * Xclean + coefficients[1], color='red', label='Regression line')
        axs[0].legend()
        axs[0].set_title('Channel ' + i)
        axs[0].set_xlabel('Global Horizontal Irradiance [W/m$^2$]')
        axs[0].set_ylabel('Vshunt [mV]')
        axs[0].set_ylim(0, 200)
        axs[0].set_xlim(0, 1000)
        
        # Temperature plot
        axs[1].scatter(df['Temp'], df['k ' + i], label = 'k ' + i)
        axs[1].legend()
        axs[1].set_title('Temperature distribution')
        axs[1].set_xlabel('Temperature [ºC]')
        axs[1].set_ylabel('k [mV/W/m$^2$]')
        axs[1].set_ylim((df['k ' + i].min()-0.2), (df['k ' + i].max()+0.2))
        
        plt.tight_layout()
        
        #print coefficients
        print(f'{i}', ": V(G) = {:.4f} * G + {:.4f}. Error: {:.4f} %".format(round(coefficients[0], 4), round(coefficients[1], 4), round(coefficients[4], 4)))
  
if __name__ == "__main__":
    main()