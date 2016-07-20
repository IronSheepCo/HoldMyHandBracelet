import sys
import re
import numpy

def main():
    readings = []
    
    for line in sys.stdin:
        match = re.match(".*rssi: -([0-9]*)", line ) 
        if match:
            readings.append(int(match.group(1)))
    
    print( "avg: "+str(sum(readings)/len(readings)) )
    print( "median: "+str(numpy.median(readings)) )
if __name__ == "__main__":
    main()
