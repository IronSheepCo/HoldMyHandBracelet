from Tkinter import *
import sys
import getopt
import select
import re
import math
import time

import matplotlib
matplotlib.use('TkAgg')

import matplotlib.pyplot as plt
import networkx as nx

root = Tk()

delay = 40
beacon_radius = 5
intersection_point_radius = 2
intersection_point_color = "cyan"
room_width = 200
room_height = 200
beacon_color = "green"
bracelet_color = "magenta"
signal_color = "red"
real_position_color = "gray"

zone_color = "pale green"

real_position = [190,125]

beacon_radius_drawing = {}
final_intersection_points = {}
beacon_info = {}
beacon_labels = {}

current_position_drawing = None

current_hotspot_color = 'g'
default_hotspot_color = 'r'
previous_hotspot = -1

next_hotspot_color = 'y'
previous_next_hotspot = -1

node_size = 1500

def using_sensor(info):
    print( "using sensor no %s with hash %s"%(info("sensor"), info("hash")))

def in_hotspot(info):
    global previous_hotspot
    debug_area.insert(END, "in hotspot %s\n" % info("id") )
    debug_area.see(END)
    
    print( "in hotspot %s"%info("id"))
    
    if int( info("id") ) == 0 or int( info("id") ) == 255:
        return
    
    #no need to redraw, we're in the same place   
    if previous_hotspot == int(info("id")):
        return
    
    #color the previous hotspot
    if previous_hotspot != -1:
        nx.draw_networkx_nodes( house_graph, house_graph_pos, nodelist=[previous_hotspot], node_color=default_hotspot_color, node_size=[node_size] )
    #color the current hotspot
    previous_hotspot = int(info("id"))
    nx.draw_networkx_nodes( house_graph, house_graph_pos, nodelist=[previous_hotspot], node_color=current_hotspot_color, node_size=[node_size] ) 
    plt.show(block=False)

def considering_hotspot(info):
    global previous_hotspot
    debug_area.insert(END, "considering %s %s\n" % (info("type"), info("id")) )
    debug_area.see(END)
    
    print( "considering hotspot %s"%info("id"))
    
def next_hotspot(info):
    global previous_next_hotspot
    prev = int(info("id"))
    if prev <= 0 or previous_next_hotspot == prev:
        return
    print("next hotspot is %d"%prev)
    #colour the previous next hotspot
    if previous_next_hotspot != -1:
        nx.draw_networkx_nodes( house_graph, house_graph_pos, nodelist=[previous_next_hotspot], node_color=default_hotspot_color, node_size=[node_size] )
    #colour the current next hotspot
    previous_next_hotspot=prev
    nx.draw_networkx_nodes( house_graph, house_graph_pos, nodelist=[prev], node_color=next_hotspot_color, node_size=[node_size] )
    plt.show(block=False)

def show_zone(info):
    x, y = get_position(info)
    radius = int( info("radius") )
    x1 = x - radius
    y1 = y - radius
    x2 = x + radius
    y2 = y + radius
   
    room.create_oval(x1, y1, x2, y2, fill = zone_color)

def show_next_step(info):
    dir_to_text = {}
    dir_to_text["1"] = "forward"
    dir_to_text["2"] = "right"
    dir_to_text["3"] = "back"
    dir_to_text["4"] = "left"
    
    direction = ""
    
    if dir_to_text.has_key( info("dir") ):
        direction = dir_to_text[info("dir")]  
 
    print( "need to go %s\n"%direction )

def show_intersection_point(info):
    inter_drawing = final_intersection_points.get( info("index"), None )
    
    x, y = get_position(info)
    x1 = x - intersection_point_radius
    y1 = y - intersection_point_radius
    x2 = x + intersection_point_radius
    y2 = y + intersection_point_radius
    
    if inter_drawing is None:
        inter_drawing = room.create_oval( x1, y1, x2, y2, fill=intersection_point_color)
        final_intersection_points[ info("index") ] = inter_drawing
    else:
        room.coords( inter_drawing, x1, y1, x2, y2 ) 

def show_signal(info):
    beacon = beacon_info[ info("hash") ]
    radius = int( info("distance") )
    
    label = beacon_labels[ info("hash") ]
    beg = label['text'].split(" <> ")
    label['text'] = "%s <> %scm %srssi"%(beg[0], info("distance"), info("rssi") ) 

def get_position(info):
    x = int( info("x") )
    y = int( info("y") )
    #flip y
    y = room_width - y
    return [x,y]

def show_position(info):
    x,y = get_position(info)
    
    global current_position_drawing
    x1 = x - beacon_radius
    y1 = y - beacon_radius
    x2 = x + beacon_radius
    y2 = y + beacon_radius   
 
    if current_position_drawing is None:
        print('drawing current position for the first time')
        current_position_drawing = room.create_oval( x1, y1, x2, y2, fill = bracelet_color )
    else:
        room.coords( current_position_drawing, x1, y1, x2, y2 )
   
    global real_position
    error_label["text"] = "Error: "+str( math.sqrt( (int(info("x"))-real_position[0])**2 + (int(info("y"))-real_position[1])**2 ) ) 
    error_label.pack()

def add_beacon(info):
    text_to_show = "Beacon: "+hash_to_name[info("hash")]+" "+info("hash")+" tx: "+info("tx")
    label = Label(top_frame, text=text_to_show)
    label.pack(anchor=W)
    beacon_labels[ info("hash") ] = label
     
    #save the beacon info
    beacon_info[ info("hash") ] = info
    print( info("hash")+" "+info("tx") )

orientation_to_text={}
orientation_to_text["1"]="west"
orientation_to_text["2"]="south"
orientation_to_text["3"]="east"
orientation_to_text["4"]="north"

def show_new_orientation(info):
    or_to_print = orientation_to_text[ info("orientation") ]
    print( "new orientation %s"%or_to_print )
    debug_area.insert(END, "new orientation %s\n" % or_to_print )
    debug_area.see(END)

def show_current_orientation(info):
    or_to_print = orientation_to_text[ info("orientation") ]
    print( "using orientation %s"%or_to_print )
    orientation_label["text"] = "Current orientation: %s"%or_to_print.upper()

def show_error( info ):
    print("-----ERROR-----")

def pattern_match_log( info ):
    log_file.write( info+"\n" )

#add here patterns to watch, as a reg exp
#the second argument is the method to be called
#with the match.group argument
patterns_to_watch = [
    ["Found new beacon hash: (?P<hash>-?[0-9]+), tx_power: (?P<tx>-?[0-9]+)", add_beacon],
    ["current position x:(?P<x>-?[0-9]+), y:(?P<y>-?[0-9]+)", show_position],
    ["hash: (?P<hash>-?[0-9]+) distance: (?P<distance>-?[0-9]+) cm rssi:(?P<rssi>-?[0-9]+)", show_signal],
    ["inter(?P<index>[0-9]) (?P<x>-?[0-9]+) (?P<y>-?[0-9]+)", show_intersection_point],
    ["zone def (?P<x>-?[0-9]+) (?P<y>-?[0-9]+) (?P<z>-?[0-9]+) (?P<radius>-?[0-9]+)", show_zone],
    ["using the (?P<type>far|near) hotspot (?P<id>-?[0-9]+)", considering_hotspot],
    ["staying in node (?P<id>-?[0-9]+)", in_hotspot ],
    ["moving to node (?P<id>-?[0-9]+)", in_hotspot ],
    ["using hotspot number (?P<sensor>[0-9]+) with hash (?P<hash>-?[0-9]+)", using_sensor],
    ["next hotspot (?P<id>[0-9]+)", next_hotspot],
    ["dir to take (?P<dir>[0-9]+)", show_next_step],
    ["new orientation (?P<orientation>[0-9]+)", show_new_orientation],
    ["did not found a new orientation", show_error],
    ["using orientation (?P<orientation>[0-9]+)", show_current_orientation]
]

def read_input():

    while sys.stdin in select.select( [sys.stdin], [], [], 0)[0]:
        line = sys.stdin.readline()
        
        #log everything in the verbose log
        verbose_log.write(line)
        
        for pattern in patterns_to_watch:
            mo = re.match( pattern[0], line )
            if mo:
                pattern[1]( mo.group )
                if should_log:
                    pattern_match_log( mo.group() )
                break
        if timeskip > 0:
            time.sleep(timeskip/1000.0)
            break
    root.after(delay, read_input)
    return

#read args
opts, args = getopt.getopt( sys.argv[1:], "dt:")

should_log=True
timeskip=0

for opt, arg in opts:
    if opt == "-d":
        should_log=False
        print(should_log)
    elif opt == "-t":
        timeskip=int(arg)

if should_log:
    log_file = open("tracking_log.txt","w+")

verbose_log = open("verbose_log.txt","w+") 

hash_to_name = { "-12901":"a9", "19316":"a10", "31102":"noa", "26596":"a5", "32690":"a6", "-24576":"a7", "32217":"a8", "6368":"a11", "-7236":"a12" }

top_frame = Frame(root)
top_frame.pack()

label = Label(top_frame, text="Analysing data ....")
label.pack()

bottom_frame = Frame(root)
bottom_frame.pack()

label = Label(bottom_frame, text="Beacons position")
label.pack()

room = Canvas(bottom_frame, width=room_width, height=room_height)
room.pack()

room.create_oval( real_position[0]-beacon_radius, room_height-(real_position[1]-beacon_radius), real_position[0]+beacon_radius, room_height-(real_position[1]+beacon_radius), fill = real_position_color )

error_label = Label(bottom_frame, text="Error: ")
error_label.pack()

orientation_label = Label(bottom_frame, text="Current orienation: ")
orientation_label.pack()

debug_area = Text(bottom_frame);
debug_area.pack()

#show the graph
house_graph = nx.Graph()

#nodes and position
house_graph.add_node(1, pos=(0,0) )
house_graph.add_node(2, pos=(0,1) )
house_graph.add_node(3, pos=(-1,1) )
house_graph.add_node(4, pos=(-1,0) )
#house_graph.add_node(5, pos=(1,0) )
house_graph.add_node(6, pos=(2,0) )
house_graph.add_node(7, pos=(2,1) )
house_graph.add_node(8, pos=(2,-1) )
house_graph.add_node(9, pos=(3,0) )
house_graph.add_node(10, pos=(4,0) )
house_graph.add_node(11, pos=(3,1) )
house_graph.add_node(12, pos=(4,1) )
house_graph.add_node(13, pos=(0,-1) )

#edges
house_graph.add_edge(1,2)
house_graph.add_edge(2,3)
house_graph.add_edge(3,4)
house_graph.add_edge(4,1)
house_graph.add_edge(1,6)
#house_graph.add_edge(1,5)
#house_graph.add_edge(5,6)
house_graph.add_edge(6,8)
house_graph.add_edge(6,7)
house_graph.add_edge(6,9)
house_graph.add_edge(9,11)
house_graph.add_edge(9,10)
house_graph.add_edge(11,12)
house_graph.add_edge(1,13)

#labels
house_graph_labels = {1:"hol 1\n1", 2:"hol 2\n2", 3:"qa\n3", 4:"dev\n4", 6:"hol 3\n6", 7:"game\n7", 8:"r&e\n8", 9:"hol 4\n9", 10:"baie\n10", 11:"hol 5\n11", 12:"buc\n12", 13:"intrare\n13"}

house_graph_pos = nx.get_node_attributes(house_graph, 'pos')

nx.draw_networkx_nodes( house_graph, house_graph_pos, node_size=[node_size for i in range(0, len(house_graph_labels) )] )
nx.draw_networkx_edges( house_graph, house_graph_pos )
nx.draw_networkx_labels( house_graph, house_graph_pos, house_graph_labels )

plt.axis('off')
plt.show(block=False)

root.after(delay, read_input)
root.mainloop()
