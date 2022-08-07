/*----------------------------------------------------------------------------*/
/*                                                                            */
/*    Copyright (c) James Pearman 2020, All rights reserved.                  */
/*    Licensed under the MIT license.                                         */
/*                                                                            */
/*    Module:     mpojoin.c                                                   */
/*    Author:     James Pearman                                               */
/*    Created:    26 Sept 2020                                                */
/*                                                                            */
/*    Revisions:  V0.1                                                        */
/*                                                                            */
/*----------------------------------------------------------------------------*/
//
// A bit of a hack to join two jpeg images and add metadata to convert into an
// mpo file for use by 3D systems
//
// Code assumes it is running on little endian processor
// If source images already have an MPY application section then it probably
// will not work, use exiftool or similar to strip that metadata out of the jpeg
// images first.
//
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include "mpo.h"

/*----------------------------------------------------------------------------*/

// code version
#define VERSION_STR   "V1.01"

/*----------------------------------------------------------------------------*/

// some helper functions
#define debug_print(fmt, ...) do { if(bVerbose) printf(fmt, ##__VA_ARGS__); }while(0)

static inline uint16_t Swap16( uint16_t x ) {
    return( (x >> 8) + ((x & 0xFF) << 8));
}
static inline uint32_t Swap32( uint32_t x ) {
    return( (x >> 24) + ((x & 0xFF0000) >> 8) + ((x & 0xFF00) << 8) + ((x & 0xFF) << 24));
}

FILE     *fp1;
FILE     *fp2;
FILE     *fpo;
bool      bFileBigEndian = true;

app2_mfc1 mfc1;
app2_mfc2 mfc2;

// set by command line options
int       convergence = 0;
bool      bVerbose = false;
int       bVersion = 0;
char     *leftFilename = NULL;
char     *rightFilename = NULL;
char     *outputFilename = NULL;

/*----------------------------------------------------------------------------*/
/** @brief      setup app2 marker for first image                             */
/*----------------------------------------------------------------------------*/
void
init_mfc1( uint32_t addr_mfc, uint32_t addr_1, uint32_t len_1, uint32_t addr_2, uint32_t len_2 ) {
    memset( &mfc1, 0, sizeof(app2_mfc1));
  
    mfc1.marker = Swap16(0xFFE2);
    mfc1.length = Swap16(sizeof(app2_mfc1) - 2 );
    mfc1.id     = Swap32(0x4d504600);
    mfc1.type   = Swap32(0x49492A00);
    
    mfc1.ifd0_offset = (uint32_t)((uintptr_t)&mfc1.ifd0_count  - (uintptr_t)&mfc1.type);;
    mfc1.ifd0_count  = 3;
  
    // first directory
    mfc1.tag0_1.tag   = 0xB000;
    mfc1.tag0_1.type  = IFD_TYPE_UND;
    mfc1.tag0_1.count = 4;
    mfc1.tag0_1.value = 0x30303130;
    
    mfc1.tag0_2.tag   = 0xB001;
    mfc1.tag0_2.type  = IFD_TYPE_ULONG;
    mfc1.tag0_2.count = 1;
    mfc1.tag0_2.value = 2;

    mfc1.tag0_3.tag   = 0xB002;
    mfc1.tag0_3.type  = IFD_TYPE_UND;
    mfc1.tag0_3.count = 32;
    mfc1.tag0_3.value = (uint32_t)((uintptr_t)&mfc1.data[0].attr - (uintptr_t)&mfc1.type);

    // second directory
    mfc1.ifd1_offset = (uint32_t)((uintptr_t)&mfc1.ifd1_count  - (uintptr_t)&mfc1.type);;
    mfc1.ifd1_count  = 4;

    mfc1.data[0].attr   = 0x20020002;
    mfc1.data[0].size   = len_1;
    mfc1.data[0].offset = addr_1; // should be 0
    mfc1.data[0].dep1   = 0;
    mfc1.data[0].dep2   = 0;

    mfc1.data[1].attr   = 0x00020002;
    mfc1.data[1].size   = len_2;
    mfc1.data[1].offset = addr_2 - addr_mfc - 8;
    mfc1.data[1].dep1   = 0;
    mfc1.data[1].dep2   = 0;
  
    mfc1.tag1_1.tag   = 0xB101;
    mfc1.tag1_1.type  = IFD_TYPE_ULONG;
    mfc1.tag1_1.count = 1;
    mfc1.tag1_1.value = 1;

    mfc1.tag1_2.tag   = 0xB204;
    mfc1.tag1_2.type  = IFD_TYPE_ULONG;
    mfc1.tag1_2.count = 1;
    mfc1.tag1_2.value = 1;

    mfc1.tag1_3.tag   = 0xB205;
    mfc1.tag1_3.type  = IFD_TYPE_SRATIONAL;
    mfc1.tag1_3.count = 1;
    mfc1.tag1_3.value = (uint32_t)((uintptr_t)&mfc1.c1 - (uintptr_t)&mfc1.type);

    mfc1.tag1_4.tag   = 0xB206;
    mfc1.tag1_4.type  = IFD_TYPE_URATIONAL;
    mfc1.tag1_4.count = 1;
    mfc1.tag1_4.value = (uint32_t)((uintptr_t)&mfc1.b1 - (uintptr_t)&mfc1.type);
  
    mfc1.ifd2_offset = 0 ;
  
    // convergence 0
    mfc1.c1 = 0;
    mfc1.c2 = 1;
  
    // baseline 0.077
    mfc1.b1 = 77;
    mfc1.b2 = 1000;
}

/*----------------------------------------------------------------------------*/
/** @brief      setup app2 marker for second image                            */
/*----------------------------------------------------------------------------*/
void
init_mfc2() {
    memset( &mfc2, 0, sizeof(app2_mfc2));

    mfc2.marker = Swap16(0xFFE2);
    mfc2.length = Swap16(sizeof(app2_mfc2) - 2 );
    mfc2.id     = Swap32(0x4d504600);
    mfc2.type   = Swap32(0x49492A00);
    
    mfc2.ifd0_offset = (uint32_t)((uintptr_t)&mfc2.ifd0_count  - (uintptr_t)&mfc2.type);;
    mfc2.ifd0_count  = 5;
  
    // first directory
    mfc2.tag0_1.tag   = 0xB000;
    mfc2.tag0_1.type  = IFD_TYPE_UND;
    mfc2.tag0_1.count = 4;
    mfc2.tag0_1.value = 0x30303130;
  
    mfc2.tag0_2.tag   = 0xB101;
    mfc2.tag0_2.type  = IFD_TYPE_ULONG;
    mfc2.tag0_2.count = 1;
    mfc2.tag0_2.value = 2;

    mfc2.tag0_3.tag   = 0xB204;
    mfc2.tag0_3.type  = IFD_TYPE_ULONG;
    mfc2.tag0_3.count = 1;
    mfc2.tag0_3.value = 1;

    mfc2.tag0_4.tag   = 0xB205;
    mfc2.tag0_4.type  = IFD_TYPE_SRATIONAL;
    mfc2.tag0_4.count = 1;
    mfc2.tag0_4.value = (uint32_t)((uintptr_t)&mfc2.c1 - (uintptr_t)&mfc2.type);

    mfc2.tag0_5.tag   = 0xB206;
    mfc2.tag0_5.type  = IFD_TYPE_URATIONAL;
    mfc2.tag0_5.count = 1;
    mfc2.tag0_5.value = (uint32_t)((uintptr_t)&mfc2.b1 - (uintptr_t)&mfc2.type);
  
    mfc2.ifd1_offset  = 0;
  
    // user defined in 1/10 units
    mfc2.c1 = convergence;
    mfc2.c2 = 10;

    // 0.077
    mfc2.b1 = 77;
    mfc2.b2 = 1000;
}

// We assume we are on intel platform
// no error checking on these
/*----------------------------------------------------------------------------*/
/** @brief      read byte from file                                           */
/*----------------------------------------------------------------------------*/
static uint8_t
get_fu8( FILE *fp ) {
    uint8_t d;
    fread(&d, 1, 1, fp);
    return(d);
}

/*----------------------------------------------------------------------------*/
/** @brief      read 16 bit short from file, byte swap as necessary           */
/*----------------------------------------------------------------------------*/
static uint16_t
get_fu16( FILE *fp ) {
    uint16_t d;
    fread(&d, 1, 2, fp);
    if( bFileBigEndian )
      d = Swap16(d);
    return(d);
}
/*----------------------------------------------------------------------------*/
/** @brief      get and print app section sig                                 */
/*----------------------------------------------------------------------------*/
void
debug_sig( FILE *fp ) {
    uint8_t   sig[8];  
    memset( sig, 0, sizeof(sig) );

    // get sig for debug
    fread(sig, 1, 4, fp);
    debug_print(" - %s\n", sig );
}

/*----------------------------------------------------------------------------*/
/** @brief      cleanup, close files                                          */
/*----------------------------------------------------------------------------*/
void
cleanup() {
    if(fp1)
      fclose(fp1);
    if(fp2)
      fclose(fp2);
    if(fpo)
      fclose(fpo);
}

/*----------------------------------------------------------------------------*/
/** @brief      get file size helper                                          */
/*----------------------------------------------------------------------------*/
uint32_t
file_size( FILE *fp ) {
    long pos = ftell(fp);
    uint32_t size;
  
    fseek( fp, 0, SEEK_END);
    size = (uint32_t)ftell(fp);
    fseek( fp, pos, SEEK_SET);
  
    return(size);
}

/*----------------------------------------------------------------------------*/
/** @brief      search for app1 marker                                        */
/*----------------------------------------------------------------------------*/
//
// find app1 and app2 markers and return offset to position in file following the
// last one.
//
uint32_t
search_app1( FILE *fp ) {
    uint16_t  marker = 0;
    int       length = 0;
    long      tpos;
    int       num_marker = 0;
    uint32_t  end_of_app12 = 0;
    uint32_t  end_of_app0  = 0;
    
    do {    
        marker = get_fu16( fp );
        tpos = ftell(fp);
        length = get_fu16( fp );
      
        if( num_marker++ > 100 )
            {
            debug_print("To many JPEG markers\n");
            return(-1);
            }
  
        switch(marker)
            {
            case    0xFFD8:
                debug_print("SOI   @ %08lX\n", tpos - 2);
                length = 0;
                break;
                
            case    0xFFD9:
                debug_print("EOI   @ %08lX\n", tpos - 2);
                length = 0;
                break;
                
            case    0xFFE0:
                debug_print("APP0  @ %08lX length %-7d", tpos - 2, length );
                end_of_app0 = (uint32_t)(tpos + length);
                debug_sig( fp );
                break;
                
            case    0xFFE1:
                debug_print("APP1  @ %08lX length %-7d", tpos - 2, length );
                end_of_app12 = (uint32_t)(tpos + length);
                debug_sig( fp );
                break;
                
            case    0xFFE2:
                debug_print("APP2  @ %08lX length %-7d", tpos - 2, length );
                end_of_app12 = (uint32_t)(tpos + length);
                debug_sig( fp );
                break;
                
            case    0xFFDB:
                debug_print("DQT   @ %08lX length %d\n", tpos - 2, length );
                break;
                
            case    0xFFC4:
                debug_print("DHT   @ %08lX length %d\n", tpos - 2, length );
                break;
                
            case    0xFFDD:
                debug_print("DRI   @ %08lX length %d\n", tpos - 2, length );
                break;

            case    0xFFC0:
                debug_print("SOF0  @ %08lX length %d\n", tpos - 2, length );
                break;

            case    0xFFDA:
                debug_print("SOS   @ %08lX length %d\n", tpos - 2, length );
                
                do
                    {
                    if( get_fu8(fp) == 0xFF )
                        marker = 0xFF00 + get_fu8(fp);
                    } while(marker != 0xFFD9);
                
                debug_print("SOSE  @ %08lX\n", ftell(fp) );
                // back up 2 to marker 
                fseek( fp, -2, SEEK_CUR );
                break;

            default:
                if( (marker & 0xFFE0) == 0xFFE0 ) {
                  // some other application specific section
                  debug_print("APP%X  @ %08lX length %-7d", marker & 0x000F, tpos - 2, length );
                  debug_sig( fp );
                }
                else {
                  // no idea what this is
                  debug_print("%04X* @ %08lX length %d\n",marker, tpos - 2, length );
                }
                break;
          }
          fseek( fp, tpos + length, SEEK_SET );
      
      } while( marker != 0xFFD9 );

      // fallback
      if(end_of_app12 == 0) {
        end_of_app12 = end_of_app0;
      }
  
      return(end_of_app12);
}

/*----------------------------------------------------------------------------*/
/** @brief      Show help for this program                                    */
/*----------------------------------------------------------------------------*/
void 
show_help(char *name) {
        fprintf( stderr,
                "usage: %s [options]\n\n"
                "left, right and output image filenames are required\n\n"
                "options:\n"
                "  -l --left  left image file\n"
                "  -r --right right image file\n"
                "  -o --output output file\n"
                "  -c --convergence set convergence in 1/10 degree\n"
                "  -v --verbose show jpeg scan details\n"
                "     --version show version\n"
                , name );
}

/*----------------------------------------------------------------------------*/
/** @brief      parse options for this program                                */
/*----------------------------------------------------------------------------*/
int 
parse_options(int argc, char *argv[]) {
    int c;
    int option_index = 0;

    if( argc <= 1 ) {
      show_help(argv[0]);
      return 1;
    }

    static struct option long_options[] =
      {
        {"left",   required_argument, 0, 'l'},
        {"right",  required_argument, 0, 'r'},
        {"output", required_argument, 0, 'o'},
        {"convergence", required_argument, 0, 'c'},
        {"verbose", no_argument, 0, 'v'},
        {"version", no_argument, &bVersion, 1 },
        {0, 0, 0, 0}
      };
        
    while( (c = getopt_long(argc, argv, "l:r:o:c:v", long_options, &option_index)) != -1) {
      switch(c) {
        case  'v':
          bVerbose = true;
          break;
          
        case  'l':
          leftFilename = optarg;
          break;
          
        case  'r':
          rightFilename = optarg;
          break;
          
        case  'o':
          outputFilename = optarg;
          break;

        case  'c':
          convergence = strtol( optarg, NULL, 0 );
          if( convergence < 0 )
            convergence = 0;
          break;

        default:
          break;          
        }
    }
    
    if( bVersion ) {
      bVerbose = true;
      debug_print("version %s\n", VERSION_STR );
      return 1;
    }
    
    if( leftFilename == NULL || rightFilename == NULL || outputFilename == NULL ) {
      show_help(argv[0]);
      return 1;
    }    
    
    return 0;
}

/*----------------------------------------------------------------------------*/

int main(int argc, char * argv[]) {
    if (parse_options(argc, argv) != 0)
      return(-1);

    // open all the files, bail if some issue
    //
    fp1 = fopen( leftFilename, "rb" );
    if( !fp1 )
      return(-1);

    fp2 = fopen( rightFilename, "rb" );
    if( !fp2 ) {
      cleanup();
      return(-1);
    }
  
    fpo = fopen( outputFilename, "wb" );
    if( !fpo ) {
      cleanup();
      return(-1);
    }


    // search input images, find where we are going to inject the new app2
    // marker.
    //
    debug_print("image 1\n===============\n");
    uint32_t pos1 = search_app1( fp1 );
    debug_print("image 2\n===============\n");
    uint32_t pos2 = search_app1( fp2 );
    uint32_t len1 = file_size(fp1);
    uint32_t len2 = file_size(fp2);
    uint32_t start_2 = ((len1 + 15) / 16) * 16;
  
    // now initialize the new app2 sections
    init_mfc1( pos1, 0, start_2 + sizeof(app2_mfc1), start_2 + sizeof(app2_mfc1), len2 + sizeof(app2_mfc2) );
    init_mfc2();

    // lazy, buffers to read data into
    uint8_t *buf1 = (uint8_t *)malloc( start_2 );
    if( buf1 == NULL ) {
      cleanup();
      return(-1);
    }
    memset( buf1, 0, start_2 );

    uint8_t *buf2 = (uint8_t *)malloc( len2 );
    if( buf2 == NULL ) {
      cleanup();
      return(-1);
    }
  
    // read file data
    fseek( fp1, 0, SEEK_SET);
    fseek( fp2, 0, SEEK_SET);
    fread( buf1, 1, len1, fp1 );
    fread( buf2, 1, len2, fp2 );
  
    // write image 1 with app2 section
    fwrite( buf1, 1, pos1, fpo );
    fwrite( &mfc1, 1, sizeof(app2_mfc1), fpo );
    fwrite( &buf1[pos1], 1, start_2-pos1, fpo );

    // write image 2 with app2 section
    fwrite( buf2, 1, pos2, fpo );
    fwrite( &mfc2, 1, sizeof(app2_mfc2), fpo );
    fwrite( &buf2[pos2], 1, len2-pos2, fpo );
  
    free(buf1);
    free(buf2);
    
    // close files
    cleanup();

    // only check/debug output file if verbose set
    if( bVerbose ) {
      debug_print("output\n===============\n");
      fpo = fopen( outputFilename, "rb" );
      if( !fpo ) {
        cleanup();
        return(-1);
      }
      uint32_t pos3 = search_app1( fpo );
      (void)pos3;
    }
    
    return(0);
}


