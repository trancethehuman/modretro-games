/* Generated BANKED dispatch; all caller coordinates are values. */
#pragma bank 255
#include "td_navigation_data.h"
UBYTE td_navigation_cell(UWORD goal,UBYTE onfoot,UBYTE x,UBYTE y) BANKED {
    UWORD pattern;UBYTE field,column;
    static const UBYTE districts[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6};
    if(goal>=107||onfoot>1||x>=128||y>=122)return 0;
    column=td_navigation_ordinal(districts[goal],onfoot,x,y);if(column==255)return 0;
    field=goal*2+onfoot;
    if(field<59)pattern=td_navigation_rows_0(field-0,y);
    else if(field<90)pattern=td_navigation_rows_1(field-59,y);
    else if(field<116)pattern=td_navigation_rows_2(field-90,y);
    else if(field<141)pattern=td_navigation_rows_3(field-116,y);
    else if(field<166)pattern=td_navigation_rows_4(field-141,y);
    else if(field<194)pattern=td_navigation_rows_5(field-166,y);
    else if(field<214)pattern=td_navigation_rows_6(field-194,y);
    else return 0;
    if(pattern<447)return td_navigation_pattern_0(pattern-0,column);
    if(pattern<918)return td_navigation_pattern_1(pattern-447,column);
    if(pattern<1363)return td_navigation_pattern_2(pattern-918,column);
    if(pattern<1818)return td_navigation_pattern_3(pattern-1363,column);
    if(pattern<2232)return td_navigation_pattern_4(pattern-1818,column);
    if(pattern<2619)return td_navigation_pattern_5(pattern-2232,column);
    if(pattern<3012)return td_navigation_pattern_6(pattern-2619,column);
    if(pattern<3401)return td_navigation_pattern_7(pattern-3012,column);
    if(pattern<3830)return td_navigation_pattern_8(pattern-3401,column);
    if(pattern<4253)return td_navigation_pattern_9(pattern-3830,column);
    if(pattern<4667)return td_navigation_pattern_10(pattern-4253,column);
    if(pattern<5161)return td_navigation_pattern_11(pattern-4667,column);
    if(pattern<5620)return td_navigation_pattern_12(pattern-5161,column);
    if(pattern<6027)return td_navigation_pattern_13(pattern-5620,column);
    if(pattern<6212)return td_navigation_pattern_14(pattern-6027,column);
    return 0;
}
