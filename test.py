# -*- coding: utf-8 -*-
"""
Created on Wed Mar  1 12:16:05 2023

@author: Jesus
"""

import pvmismatch as pvm
import numpy as np

def main():
    cell_params = {
        'Rs': 0.01,
        'Isat1_T0': 1e-10}
    
    cell = pvm.PVcell(**cell_params)
    
    n = 8
    m = 12
    
    # create a new module with the updated cell position pattern
    module = pvm.PVmodule(pvcells=cell)
    
    system = pvm.PVsystem(numberStrs=1, numberMods=10, pvmods=module)
    suns = np.empty(n*m)
    
    suns = module_irr(n,m)
    
    Ee = {0: {0: 1, 1: 1, 2: 1}}
    for i in Ee[0]:
        Ee[0][i] = suns* (i+1)
    print(Ee)
    system.setSuns(Ee)
    system.plotSys()

def module_irr(n, m):
    z = 0
    suns = np.empty(n*m)
    for i in range(0, n):
        if i != 0:
            if i % 2 == 0:
                z += m + 1
            elif i % 2 != 0:
                z += m - 1
        for j in range(0, m):
            suns[z] = (j+1)/10
            if i % 2 != 0:
                z -= 1
            if i % 2 == 0:
                z += 1
    return suns

if __name__ == "__main__":
    main()