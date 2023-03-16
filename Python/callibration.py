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

df = pd.DataFrame()

plate = ['CH1', 'CH2', 'CH3', 'CH4', 'CH5', 'CH6', 'CH7', 'CH8']

def main():
    global datalogger_data
    global meteodata
    global df
    global data
    global coefficients
    
    #import data from datalogger and meteodata station
    datalogger_data = dl.data_import('datalogger')
    meteodata = dl.data_import('meteodata')
    
    # if datalogger data is in seconds, uncomment this line
    # datalogger_data = datalogger_data.resample('T').mean()
    
    # create dataframe with requested data
    for i in plate:
        df[i] = datalogger_data[i]
    
    df['GHI'] = meteodata['Gh'][meteodata.index.isin(datalogger_data.index)]
    df['Temp'] = datalogger_data['CH9']
    
    #temperature correction
    alpha = 4.522e-4 # pu units
    T0  = 25 # STC temperature
    
    for i in plate:
        df[i] /= (1 + alpha * (df['Temp'] - T0))
        
    # rearrange index to GHI for the plotting
    df.index = df['GHI']
    
    dl.plot_channels(magnitude = 'Vshunt[V]', dataframe = df, 
                      plate = plate, title = 'Callibration data', xaxis = 'Irradiance [W/m$^2$]')
    
    #linear regression to obtain coefficients
    for i in plate:
        
        coefficients = scipy.stats.linregress(df['GHI'], df[i])
        print('coefficient:' ,coefficients[0],'Error:', coefficients[4]*100, '%')
  
if __name__ == "__main__":
    main()