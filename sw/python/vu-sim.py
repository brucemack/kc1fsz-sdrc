ts = 8
target = 300
c = 0.12
t = 0

vavg = 0
vin = 1

for i in range(0, 100):

    print(i, t, 100 * vavg)
    t = t + ts
    vavg = vavg + c * (vin - vavg)
    
    if i == 36:
        vin = 0
    
