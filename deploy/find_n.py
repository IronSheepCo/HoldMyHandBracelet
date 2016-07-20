from scipy import optimize
import math

x = [ -68, -70, -72]#, -80]
y = [ 1, 1.6, 2]#, 3]
ref_tx = x[0]

def distance_function(n):
    sum = 0
    
    for i in range(0, len(x)):
        square = y[i] - 10 ** ( (ref_tx-x[i])/(10.0*n) )
        square = square * square
        sum = sum + square
    
    return sum

def distance_function_prime(n):
    sum = 0
    prod = 2.0*math.log(10)
    
    for i in range(0, len(x)):
        power = 10 ** ( (ref_tx-x[i])/(10.0*n) )
        sum = sum + (y[i]-power)*power*( (ref_tx-x[i])/10.0 )*( 1.0/(n*n) )
    return prod*sum

def distance_for_rssi(n, rssi):
    return 10 ** ( (ref_tx-rssi)/(10.0*n) )

def main():
    n = optimize.minimize( distance_function, 3 ).x[0] 
    #n = optimize.newton(distance_function, 2 )
    print( "coef: "+str(n))
    #n = 3
    print( "error: "+str(distance_function(n)))
    
    for rssi in x:
        print( distance_for_rssi(n, rssi) )
    
    print( distance_for_rssi(n, -80) )

if __name__ == "__main__":
    main()
