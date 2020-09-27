/*----------------------------------------------------------------------------*/
/*                                                                            */
/*    Copyright (c) James Pearman 2020, All rights reserved.                  */
/*    Licensed under the MIT license.                                         */
/*                                                                            */
/*    Module:     mpo.h                                                       */
/*    Author:     James Pearman                                               */
/*    Created:    26 Sept 2020                                                */
/*                                                                            */
/*    Revisions:  V0.1                                                        */
/*                                                                            */
/*----------------------------------------------------------------------------*/

#ifndef _MPO_H
#define _MPO_H

#ifdef __cplusplus
extern "C" {
#endif

#define IFD_TYPE_UBYTE      1
#define IFD_TYPE_ASCII      2
#define IFD_TYPE_USHORT     3
#define IFD_TYPE_ULONG      4
#define IFD_TYPE_URATIONAL  5
#define IFD_TYPE_SBYTE      6
#define IFD_TYPE_UND        7
#define IFD_TYPE_SSHORT     8
#define IFD_TYPE_SLONG      9
#define IFD_TYPE_SRATIONAL  10
#define IFD_TYPE_FLOAT      11
#define IFD_TYPE_DOUBLE     12

typedef struct __attribute((__packed__)) _tag {
    uint16_t    tag;
    uint16_t    type;
    uint32_t    count;
    uint32_t    value;
} tag;

typedef struct __attribute((__packed__)) _mfc_data {
    uint32_t    attr;
    uint32_t    size;
    uint32_t    offset;
    uint16_t    dep1;
    uint16_t    dep2;
} mfc_data;

//
// structure to hold metadata for first image in the MPO file
//
typedef struct __attribute((__packed__)) _app2_mfc1 {
    uint16_t    marker;
    uint16_t    length;
    uint32_t    id;
    uint32_t    type;
  
    uint32_t    ifd0_offset;
    uint16_t    ifd0_count;
    tag         tag0_1;
    tag         tag0_2;
    tag         tag0_3;
    uint32_t    ifd1_offset;

    mfc_data    data[2];

    uint16_t    ifd1_count;
    tag         tag1_1;
    tag         tag1_2;
    tag         tag1_3;
    tag         tag1_4;
    uint32_t    ifd2_offset;

    uint32_t    c1;
    uint32_t    c2;
    uint32_t    b1;
    uint32_t    b2;
  
    uint8_t     res[32];
  
} app2_mfc1;

//
// structure to hold metadata for second image in the MPO file
//
typedef struct __attribute((__packed__)) _app2_mfc2 {
    uint16_t    marker;
    uint16_t    length;
    uint32_t    id;
    uint32_t    type;
  
    uint32_t    ifd0_offset;
    uint16_t    ifd0_count;
    tag         tag0_1;
    tag         tag0_2;
    tag         tag0_3;
    tag         tag0_4;
    tag         tag0_5;
    uint32_t    ifd1_offset;

    uint32_t    c1;
    uint32_t    c2;
    uint32_t    b1;
    uint32_t    b2;
  
    uint8_t     res[30];
} app2_mfc2;

#ifdef __cplusplus
}
#endif
#endif /* _MPO_H */