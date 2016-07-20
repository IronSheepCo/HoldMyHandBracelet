#include <stdio.h>
#include <stdint.h>

#include "../graph.h"

void test_find_route_end( uint8_t end, uint8_t* expected_result )
{
 uint8_t* response = find_route(end); 
 uint8_t passed = 1;

 for( int i = 0; i < 13; i++ )
 {
    printf("%d ", response[i] );
    if( response[i] != expected_result[i] )
    {
        passed = 0;
    }
 }  
 
 if( passed )
    printf(" OK ");
 else
    printf(" FAILED ");
 printf("\n");
}

void test_find_route()
{
    uint8_t expected_result[]={0, 5, 1, 2, 1, 6, 9, 6, 6, 0, 9, 9, 11};
    test_find_route_end(9, expected_result);

    uint8_t expected_result2[]={0, 5, 1, 2, 1, 6, 8, 6, 0, 6, 9, 9, 11};
    test_find_route_end(8, expected_result2);
    
    uint8_t expected_result3[]={0, 4, 1, 4, 0, 1, 5, 6, 6, 6, 9, 9, 11};
    test_find_route_end(4, expected_result3);
}

int main()
{
 test_find_route();

 return 0;
}
