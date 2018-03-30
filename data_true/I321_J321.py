import matplotlib.pyplot as pl
import numpy as np
from mpl_toolkits.mplot3d import Axes3D

data = np.loadtxt('delY.dat')
data1 = np.loadtxt('I321_J321.dat')

pl.plot(data,data1[:,0],'c-',label='l0=7.67')
pl.plot(data,data1[:,1],'r-',label='l1=2.41')
pl.plot(data,data1[:,2],'m-',label='l2=0.97')
pl.plot(data,data1[:,2],'y-',label='l3=0.80')



pl.legend()
pl.grid()
pl.title('I321_J321.dat')
pl.xlabel('delY')
pl.ylabel('coeff')
pl.show()
