#include "utils_math.h"

#include "SEGGER_RTT.h"

float static distance_between_circles( circle* c1, circle* c2 )
{
    int d;
    
    d = (c1->x - c2->x);
    d = d*d;
    
    d = d + (c1->y - c2->y)*(c1->y - c2->y);
    
    return sqrt(d);

}

int are_circles_contained( circle* c1, circle* c2)
{
    int d = distance_between_circles( c1, c2 );

    return d <= c1->radius || d <= c2->radius;    
}

int are_circles_intersecting( circle* c1, circle* c2 )
{
    return distance_between_circles(c1, c2) < c1->radius+c2->radius;
}

int is_point_in_circle( circle* c, point p)
{
 return (c->x-p.x)*(c->x-p.x)+(c->y-p.y)*(c->y-p.y) <= c->radius*c->radius;
}

void circle_intersection( circle* c1, circle* c2, point* p1, point* p2)
{
    int d, a, h;
    int px, py;    
    
    //distance between the circles
    do
    {
        d = (c1->x-c2->x);
        d = d*d;
        a = (c1->y-c2->y);
        a = a * a;
        d = sqrt( d + a );
        
        //if the circles don't intersect let's increase one of the circles radius
        if( d<=c1->radius+c2->radius ){
            break;
        }

        //lets increase the radius for the smaller circle
        h = d - c1->radius - c2->radius+10;

        SEGGER_RTT_printf(0, "increasing radius by %d\n", h);
        if( c1->radius < c2->radius )
        {
            c1->radius += h;
        }
        else
        {
            c2->radius += h;
        }
    }while( 1 );//d<c1->radius+c2->radius);

    a = c1->radius;
    a = a*a;
    h = c2->radius;
    h = h*h;
    a = ( a - h + d*d )/(2*d);

    h = sqrt( c1->radius*c1->radius - a*a );

    px = c1->x + a *(c2->x-c1->x)/d;
    py = c1->y + a *(c2->y-c1->y)/d;

    //SEGGER_RTT_printf(0, "d: %d a: %d h: %d px: %d py: %d\n", d, a, h, px, py);

    p1->x = px + h*(c2->y-c1->y)/d;
    p1->y = py - h*(c2->x-c1->x)/d;

    p2->x = px - h*(c2->y-c1->y)/d;
    p2->y = py + h*(c2->x-c1->x)/d;
}
