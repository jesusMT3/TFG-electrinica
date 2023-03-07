# -*- coding: utf-8 -*-
"""
Created on Tue Mar  7 12:10:36 2023

@author: Jesus
"""

import sys, os
sys.path.append(os.path.dirname(__file__))
import datalogger as dl
import pandas as pd
import scipy

callibration_data = pd.DataFrame()
plate = ['CH1', 'CH2', 'CH3', 'CH4']

def main():
    global meteodata
    global data
    global callibration_data
    global coefficients
    
    meteodata = dl.datalogger_import(dl.cols2)
    data = dl.datalogger_import(dl.cols)
    data.index = pd.to_datetime(data['DateTime'])
    callibration_data['GHI'] = meteodata['Gh']
    for i in plate:
        callibration_data[i] = data[i].resample('T').mean()
    callibration_data.index = callibration_data['GHI']
    
    dl.plot_channels(magnitude = 'Vshunt[V]', dataframe = callibration_data, 
                     plate = plate, title = 'Sensor callibration', xaxis = 'Irradiance [W/m$^2$]')
    

    for i in plate:
        
        coefficients = scipy.stats.linregress(callibration_data['GHI'], callibration_data[i])
        print('coefficient:' ,coefficients[0],'Error:', coefficients[4]*100, '%')
    
if __name__ == "__main__":
    main()