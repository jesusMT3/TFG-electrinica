# -*- coding: utf-8 -*-
"""
Created on Wed Jun  7 18:02:50 2023

@author: Jesus
"""
from tkinter import filedialog
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib as mpl
import pvmismatch as pvm

bifaciality = 0.75
# Module, system and cell initialization
module = pvm.PVmodule(cell_pos = pvm.pvmismatch_lib.pvmodule.STD72)

# For 1V
# system = pvm.PVsystem(numberMods=2,numberStrs=1, pvmods = module)

# For 2V
system = pvm.PVsystem(numberMods=1,numberStrs=2, pvmods = module)

#Normalized power
system.setSuns(1)
system.setTemps(298.15)
normalized_power = system.Pmp

# Import dataframes
data = filedialog.askopenfile()

# First, datalogger data
processed_df = pd.read_csv(data,
                 sep=",", # two types of separation
                 header = 0, # first row used as index
                 skiprows = 0, # skip index
                 engine = 'python',
                 parse_dates = True,
                 index_col = 0)

processed_df.index = pd.to_datetime(processed_df.index) 
# Power and ghi
fig, ax1 = plt.subplots(figsize=(10, 6))
ax1.scatter(processed_df['GHI'], processed_df['optimal power'], color='blue', label='Pmax at optimum tilt')
ax1.set_xlabel('Global Horizontal Irradiance (GHI)')
ax1.set_ylabel('Referenced at 1000W/m$^2$')
ax1.set_title('Pmax at optimum tilt vs GHI')
ax1.legend(loc='best')
ax1.tick_params(axis='x', rotation=45)
plt.tight_layout()  # Adjust spacing between subplots
plt.show()


# Angle and tilt
fig, ax2 = plt.subplots(figsize=(10, 6))

ax2.plot(processed_df.index, processed_df['tilt'], color='black', label='Ephemeris tilt')
cmap = mpl.colormaps['hsv']
scatter2 = ax2.scatter(processed_df.index, processed_df['optimal angle'], c=processed_df['Wspeed'],
                      cmap=cmap, label='angular difference')

ax2.set_xlabel('Datetime')
ax2.set_ylabel('Degrees [$º$]')
ax2.set_ylim([-60, 60])
ax2.set_title('Optimum tilt vs Ephemeris')
ax2.legend(loc='best')
ax2.tick_params(axis='x', rotation=45)
ax2.legend(loc='best')

cbar = fig.colorbar(scatter2, ax=ax2)
cbar.ax.set_ylabel('Wind speed [m/s]')

plt.tight_layout()  # Adjust spacing between subplots
plt.show()

# Power difference
cmap = mpl.colormaps['magma']
fig, ax3 = plt.subplots(figsize=(10, 6))
scatter3 = ax3.scatter(processed_df['DHI/GHI'], 100*(processed_df['optimal power'] - processed_df['ephemeris power'])/processed_df['ephemeris power'], c=processed_df['GHI'],
                      cmap=cmap, label='Energy gain')
ax3.set_xlabel('DHI/GHI ratio')
ax3.set_ylabel('Energy gain [%]')
ax3.set_title('Energy gain')
ax3.legend(loc='best')
ax3.tick_params(axis='x', rotation=45)

cbar = fig.colorbar(scatter3, ax=ax3)
cbar.ax.set_ylabel('GHI [suns]')

plt.tight_layout()  # Adjust spacing between subplots
plt.show()

# Angular difference vs clarity index

cmap = mpl.colormaps['hot']

fig, ax4 = plt.subplots(figsize=(10, 6))
scatter4 = ax4.scatter(processed_df['DHI/GHI'], processed_df['optimal angle'] - processed_df['tilt'], c=processed_df['GHI'],
                      cmap=cmap, label='angular difference')
ax4.set_xlabel('DHI/GHI ratio')
ax4.set_ylabel('Degrees [º]')
ax4.set_title('Angular difference vs DHI/GHI ratio')
ax4.legend(loc='best')
ax4.tick_params(axis='x', rotation=45)
ax4.set_ylim([-50, 50])

cbar = fig.colorbar(scatter4, ax=ax4)
cbar.ax.set_ylabel('GHI [suns]')

plt.tight_layout()  # Adjust spacing between subplots
plt.show()

# Bifacial Gains

fig, ax5 = plt.subplots(figsize=(10, 6))
ax5.scatter(processed_df['GHI'], processed_df['Optimal Bifacial gains'], color = 'red',label='Bifacial Gains (energy)')
ax5.scatter(processed_df['GHI'], processed_df['Optimal Bifacial gains irr'], color = 'blue',label='Bifacial Gains (irradiance)')
ax5.set_xlabel('Global Horizontal Irradiance (GHI) [suns]')
ax5.set_ylabel('[%]')
ax5.set_title('Bifacial Gains')
ax5.legend(loc='best')
ax5.tick_params(axis='x', rotation=45)

plt.tight_layout()  # Adjust spacing between subplots
plt.show()

optimal_power = processed_df['optimal power'] * normalized_power / 60 # kWh
optimal_power_non_bifacial = processed_df['optimal power non bifacial'] * normalized_power / 60 #kWh
optimal_energy = optimal_power.sum()
optimal_energy_non_bifacial = optimal_power_non_bifacial.sum()
bifacial_gains = 100 * ((optimal_energy - optimal_energy_non_bifacial) / optimal_energy_non_bifacial)
bifacial_gains_irr = 100 * bifaciality * ((processed_df['BE'].sum() + processed_df['BW'].sum()) / (processed_df['FE'].sum() + processed_df['FW'].sum()))

ephemeris_power = processed_df['ephemeris power'] * normalized_power / 60 # kWh
ephemeris_energy = ephemeris_power.sum()
energy_gain = 100*(optimal_energy - ephemeris_energy) / (ephemeris_energy)

global_results = {'Optimal energy [kWh]': round(optimal_energy, 2),
                'Ephemeris energy [kWh]': round(ephemeris_energy, 2),
                'Bifacial Gains [%]': round(bifacial_gains, 2),
                'Irradiance Bifacial Gains [%]': round(bifacial_gains_irr, 2),
                'Energy Gain [%]': round(energy_gain, 2)}

# Save csv
file_path = filedialog.asksaveasfilename(defaultextension='.csv')
pd.DataFrame([global_results]).to_csv(file_path, index=False)


