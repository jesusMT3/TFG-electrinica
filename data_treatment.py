# -*- coding: utf-8 -*-
"""
Created on Wed Mar  1 10:21:19 2023

@author: Jesus
"""

import sys, os
sys.path.append(os.path.dirname(__file__))
import datalogger as dl
import numpy as np

cols = ["No", "DateTime", "ms", "CH1", "CH2", "CH3", "CH4", "CH5", 
        "CH6", "CH7", "CH8", "CH9", "CH11", "CH12", "CH13", "CH14", 
        "CH15", "T1", "T2", "GS1", "GS2", "GS3", "GS4", "Alarm1", 
        "Alarm2", "Alarm3", "AlarmOut"] # columns in which the data from data is organised

irr_coef = [0.1658, 0.1638, 0.1664, 0.1678, 0.3334, 0.1686, 0.1673, np.inf, np.inf, np.inf, np.inf, 0.3306, 0.3317, 0.3341, 0.3361]


def main():
    global filtered_data
    filtered_data = dl.datalogger_filter(df = dl.datalogger_import(cols), 
                                        filt = None, 
                                        mean_coeff = 1000, 
                                        irr_coef = irr_coef)
    
    east = ['W1', 'W2', 'W3', 'W4']
    north_west = ['W5', 'W6', 'W7', 'W8']
    south_west = ['W11', 'W12', 'W13', 'W14']
    
    dl.plot_channels(magnitude = 'Irradiance [W/m$^2$]', dataframe = filtered_data, plate = east, title = 'East plate')
    dl.plot_channels(magnitude = 'Irradiance [W/m$^2$]', dataframe = filtered_data, plate = north_west, title = 'North-West plate')
    dl.plot_channels(magnitude = 'Irradiance [W/m$^2$]', dataframe = filtered_data, plate = south_west, title = 'South-West plate')

    # Average plate irradiance

    filtered_data['e_average'] = filtered_data[east].mean(axis=1)   
    filtered_data['NW_average'] = filtered_data[north_west].mean(axis=1)
    filtered_data['SW_average'] = filtered_data[south_west].mean(axis=1)

    dl.plot_channels(magnitude = 'Irradiance [W/m$^2$]', 
                    dataframe = filtered_data, 
                    plate = ['e_average', 'NW_average', 'SW_average'], 
                    title = 'Average plate irradiance')

if __name__ == "__main__":
    main()