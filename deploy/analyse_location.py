from Tkinter import *
import sys
import select
import re
import math

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

def using_sensor(info):
    print( "using sensor no %s with hash %s"%(info("sensor"), info("hash")))

def in_hotspot(info):
    global previous_hotspot
    debug_area.insert(END, "%s %s\n" % (info("type"), info("id")) )
    debug_area.see(END)
    
    print( "using hotspot %s"%info("id"))
    
    if int( info("id") ) == 0 or int( info("id") ) == 255:
        return
    
    #color the previous hotspot
    if previous_hotspot != -1:
        nx.draw_networkx_nodes( house_graph, house_graph_pos, nodelist=[previous_hotspot], node_color=default_hotspot_color )
    #color the current hotspot
    previous_hotspot = int(info("id"))
    nx.draw_networkx_nodes( house_graph, house_graph_pos, nodelist=[previous_hotspot], node_color=current_hotspot_color ) 
    plt.show(block=False)

def show_zone(info):
    x, y = get_position(info)
    radius = int( info("radius") )
    x1 = x - radius
    y1 = y - radius
    x2 = x + radius
    y2 = y + radius
   
    room.create_oval(x1, y1, x2, y2, fill = zone_color)

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
    x, y = get_position(beacon)
    x1 = x-radius
    y1 = y-radius
    x2 = x+radius
    y2 = y+radius
    
    room.coords( beacon_radius_drawing[ info("hash") ], x1, y1, x2, y2 )
    
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
    text_to_show = "Beacon: "+info("hash")+" tx: "+info("tx")+" x: "+info("x")+" y: "+info("y")
    label = Label(top_frame, text=text_to_show)
    label.pack()
    beacon_labels[ info("hash") ] = label
     
    #draw the beacon, just do it
    x, y = get_position(info)
    beacon_circle = room.create_oval( x-beacon_radius, y-beacon_radius, x+beacon_radius, y+beacon_radius, fill = beacon_color ) 
   
    #draw the radius and save it
    beacon_radius_drawing[info("hash")] = room.create_oval( x-beacon_radius, y-beacon_radius, x+beacon_radius, y+beacon_radius, outline=signal_color )
    
    #put a label next to the beacon
    label_text = room.create_text( x + 35, y, text = info("hash") )
        
    #save the beacon info
    beacon_info[ info("hash") ] = info
    print( info("hash")+" "+info("tx")+" "+info("x")+" "+info("y") )

def read_input():

    while sys.stdin in select.select( [sys.stdin], [], [], 0)[0]:
        line = sys.stdin.readline()
        if line:
            mo = re.match( "Found new beacon hash: (?P<hash>-?[0-9]+), tx_power: (?P<tx>-?[0-9]+), pos_x: (?P<x>-?[0-9]+),  pos_y: (?P<y>-?[0-9]+)", line)
            if mo:
                print("raw data: "+line)
                add_beacon( mo.group )
            else:
                mo = re.match( "current position x:(?P<x>-?[0-9]+), y:(?P<y>-?[0-9]+)", line)
                
                if mo:
                    print("current position: "+mo.group("x")+" "+mo.group("y"))
                    show_position( mo.group )
                else:
                    
                    mo = re.match("hash: (?P<hash>-?[0-9]+) distance: (?P<distance>-?[0-9]+) cm rssi:(?P<rssi>-?[0-9]+)", line)
                    
                    if mo:
                       show_signal( mo.group )
                    else:
                        
                        mo = re.match("inter(?P<index>[0-9]) (?P<x>-?[0-9]+) (?P<y>-?[0-9]+)", line)
                        
                        if mo:
                            show_intersection_point( mo.group )
                        else:
                            
                            mo = re.match("zone def (?P<x>-?[0-9]+) (?P<y>-?[0-9]+) (?P<z>-?[0-9]+) (?P<radius>-?[0-9]+)", line)
                            
                            if mo:
                                show_zone( mo.group )
                            else:
                                
                                mo = re.match("using the (?P<type>far|near) hotspot (?P<id>-?[0-9]+)", line)
                                if mo:
                                    in_hotspot( mo.group )
                                else:
                                    
                                    mo = re.match("using hotspot number (?P<sensor>[0-9]+) with hash (?P<hash>-?[0-9]+)", line)
                                    
                                    if mo:
                                        using_sensor( mo.group )
    
        else:
            print("end of input")
    
    root.after(delay, read_input)

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

debug_area = Text(bottom_frame);
debug_area.pack()

#show the graph
house_graph = nx.Graph()

#nodes and position
house_graph.add_node(1, pos=(0,0) )
house_graph.add_node(2, pos=(0,1) )
house_graph.add_node(3, pos=(-1,1) )
house_graph.add_node(4, pos=(-1,0) )
house_graph.add_node(5, pos=(1,0) )
house_graph.add_node(6, pos=(2,0) )
house_graph.add_node(7, pos=(2,1) )
house_graph.add_node(8, pos=(2,-1) )
house_graph.add_node(9, pos=(3,0) )
house_graph.add_node(10, pos=(4,0) )
house_graph.add_node(11, pos=(3,1) )
house_graph.add_node(12, pos=(4,1) )

#edges
house_graph.add_edge(1,2)
house_graph.add_edge(2,3)
house_graph.add_edge(3,4)
house_graph.add_edge(4,1)
house_graph.add_edge(1,5)
house_graph.add_edge(5,6)
house_graph.add_edge(6,8)
house_graph.add_edge(6,7)
house_graph.add_edge(6,9)
house_graph.add_edge(9,11)
house_graph.add_edge(9,10)
house_graph.add_edge(11,12)

#labels
house_graph_labels = {1:"1",2:"2",3:"3",4:"4",5:"5",6:"6",7:"7",8:"8",9:"9",10:"10",11:"11",12:"12"}

house_graph_pos = nx.get_node_attributes(house_graph, 'pos')

nx.draw_networkx_nodes( house_graph, house_graph_pos )
nx.draw_networkx_edges( house_graph, house_graph_pos )
nx.draw_networkx_labels( house_graph, house_graph_pos, house_graph_labels )

plt.axis('off')
plt.show(block=False)

root.after(delay, read_input)
root.mainloop()
