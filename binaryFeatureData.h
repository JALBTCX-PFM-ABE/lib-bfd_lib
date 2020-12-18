
/*********************************************************************************************

    This is public domain software that was developed by or for the U.S. Naval Oceanographic
    Office and/or the U.S. Army Corps of Engineers.

    This is a work of the U.S. Government. In accordance with 17 USC 105, copyright protection
    is not available for any work of the U.S. Government.

    Neither the United States Government, nor any employees of the United States Government,
    nor the author, makes any warranty, express or implied, without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, or assumes any liability or
    responsibility for the accuracy, completeness, or usefulness of any information,
    apparatus, product, or process disclosed, or represents that its use would not infringe
    privately-owned rights. Reference herein to any specific commercial products, process,
    or service by trade name, trademark, manufacturer, or otherwise, does not necessarily
    constitute or imply its endorsement, recommendation, or favoring by the United States
    Government. The views and opinions of authors expressed herein do not necessarily state
    or reflect those of the United States Government, and shall not be used for advertising
    or product endorsement purposes.
*********************************************************************************************/


/****************************************  IMPORTANT NOTE  **********************************

    Comments in this file that start with / * ! are being used by Doxygen to document the
    software.  Dashes in these comment blocks are used to create bullet lists.  The lack of
    blank lines after a block of dash preceeded comments means that the next block of dash
    preceeded comments is a new, indented bullet list.  I've tried to keep the Doxygen
    formatting to a minimum but there are some other items (like <br> and <pre>) that need
    to be left alone.  If you see a comment that starts with / * ! and there is something
    that looks a bit weird it is probably due to some arcane Doxygen syntax.  Be very
    careful modifying blocks of Doxygen comments.

*****************************************  IMPORTANT NOTE  **********************************/



#ifndef __BINARYFEATUREDATA_H__
#define __BINARYFEATUREDATA_H__


#ifdef  __cplusplus
extern "C" {
#endif


#ifndef DOXYGEN_SHOULD_SKIP_THIS


#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "binaryFeatureData_macros.h"


  /*  Windoze insanity.  Note that there may be nothing defined for any of these DLL values.  When building statically on Windows I
      strip out the __declspec(dllexport) and __declspec(dllimport) definitions before I copy the .h file to the central include
      location.  That's so I don't have to modify a bunch of application Makefiles to check for static build adn set the _STATIC
      option.  Of course, on Linux/UNIX we don't have to do anything to these ;-)  */

#ifdef BFDATA_DLL_EXPORT
#  define BFDATA_DLL __declspec(dllexport)
#else
#  ifdef NVWIN3X
#    ifdef BFDATA_STATIC
#      define BFDATA_DLL
#    else
#      define BFDATA_DLL __declspec(dllimport)
#    endif
#  else
#    define BFDATA_DLL
#  endif
#endif


#endif /*  DOXYGEN_SHOULD_SKIP_THIS  */


  /*! \mainpage Binary Feature Data (BFD) Format

       <br><br>\section disclaimer Disclaimer

       This is a work of the US Government. In accordance with 17 USC 105, copyright
       protection is not available for any work of the US Government.

       Neither the United States Government nor any employees of the United States
       Government, makes any warranty, express or implied, without even the implied
       warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, or assumes
       any liability or responsibility for the accuracy, completeness, or usefulness
       of any information, apparatus, product, or process disclosed, or represents
       that its use would not infringe privately-owned rights. Reference herein to
       any specific commercial products, process, or service by trade name, trademark,
       manufacturer, or otherwise, does not necessarily constitute or imply its
       endorsement, recommendation, or favoring by the United States Government. The
       views and opinions of authors expressed herein do not necessarily state or
       reflect those of the United States Government, and shall not be used for
       advertising or product endorsement purposes.


       <br><br>\section intro Introduction

       The BFD (Binary Feature Data) data type is a simple binary format for hydrographic 
       feature/target data.  This format is designed to be easily imported into the NAVO
       standard XML target file format when all editing of the feature/target data is complete.


       <br><br>\section sec1 Header

       Over the years I have found that it is really nice if you can get an idea of what is in a
       particular file without having to resort to running a special program.  To this end I have
       used tagged ASCII headers for this format so that you can dump the header and get some
       information about the file by just using the "type /page" command (in an MS Command prompt
       window) or the "more" or "less" commands on Linux/UNIX/Mac OS/X.  This is a version 1.0
       ASCII header example:

       <pre>
       [VERSION] = PFM Software - Binary Feature Data library V1.00 - 03/27/09
       [ENDIAN] = LITTLE
       [CREATION YEAR] = 2009
       [CREATION MONTH] = 03
       [CREATION DAY] = 30
       [CREATION DAY OF YEAR] = 089
       [CREATION HOUR] = 13
       [CREATION MINUTE] = 52
       [CREATION SECOND] = 31.00
       [CREATION SOFTWARE] = PFM Software - miw2bfd V1.00 - 03/27/09
       [MODIFICATION YEAR] = 2009
       [MODIFICATION MONTH] = 03
       [MODIFICATION DAY] = 30
       [MODIFICATION DAY OF YEAR] = 089
       [MODIFICATION HOUR] = 13
       [MODIFICATION MINUTE] = 52
       [MODIFICATION SECOND] = 31.00
       [NUMBER OF RECORDS] = 211
       [HEADER SIZE] = 65536
       </pre>


       <br><br>\section sec2 Feature Record

       The "endian-ness" of the file will be indicated by the [ENDIAN] field in the header.
       All swapping will be performed by the API.
        


       <br><br>\section sec3 Polygon Record

       Any feature may have associated polygon or polyline data.  This will be stored in a separate file that has a
       .bfa file extension as opposed to the .bfd extension.  The address of each set of polygons will be stored in
       the bfd_record.poly_address field.  The count and type will be stored in the bfd_record.poly_count and 
       bfd_record.poly_type fields respectively.  The polygon/polyline data will not be editable.  If you change the
       polygon/polyline data it will be appended to the file and the address, count, and type fields will be changed.


       <br><br>\section sec4 Image Record

       Any feature may have an associated image.  This will be stored in the .bfa file along with the polgon/polyline
       data.  The name, type, and address of each image in the .bfa file will be stored in the bfd_record.image_size,
       image_name, and image_address fields respectively.  The image data will not be editable.  If you change the
       image data it will be appended to the file and the size, name, and address fields will be changed.


       <br><br>\section sec5 BFD API I/O function definitions

       The BFD API is very simple and consists of only about 20 functions.  The public functions and data structures
       documentation can be accessed from binaryFeatureData.h in the Doxygen generated HTML documentation.

*/


  /*!
      - Binary Feature Data header structure.

      - Header keys:
          - (0)  = Populated by the API.
          - (1)  = Optional.
  */

  typedef struct
  {
    char             version[128];                 /*!<  Library version information  (0)  */
    uint8_t          little_endian;                /*!<  1 if file is little endian  (0)  */
    time_t           creation_tv_sec;              /*!<  File creation POSIX seconds from 01-01-1970  (0)  */
    long             creation_tv_nsec;             /*!<  File creation POSIX nanoseconds of second from 01-01-1970  (0)  */
    char             creation_software[128];       /*!<  File creation software  (1)  */
    time_t           modification_tv_sec;          /*!<  File modification POSIX seconds from 01-01-1970  (0)  */
    long             modification_tv_nsec;         /*!<  File modification POSIX nanoseconds of second from 01-01-1970  (0)  */
    char             modification_software[128];   /*!<  File modification software  (1)  */
    char             security_classification[32];  /*!<  Security classification  (1)  */
    char             distribution[1024];           /*!<  Security distribution statement  (1)  */
    char             declassification[256];        /*!<  Security declassification statement  (1)  */
    char             class_just[256];              /*!<  Security classification justification  (1)  */
    char             downgrade[128];               /*!<  Security downgrade statement  (1)  */
    uint32_t         number_of_records;            /*!<  Number of records in file  (0)  */
    char             comments[4096];               /*!<  Comments  (1)  */
  } BFDATA_HEADER;


  /*!

      Point record structures.  No attempt to save space has been made since these are going to be small,
      (hopefully) temporary, files.

      IMPORTANT NOTE:  If you change this you need to modify the "compute_record_size" function in binaryFeatureData.c to match.  You will also 
      need to add a version change and version checking.  Remember that the record number is not written to disk.
  */

  typedef struct
  {
    uint32_t         record_number;                  /*!<  Starts at 0 as all well-behaved counts do ;-)  */
    char             contact_id[15];                 /*!<  Alphanumeric contact ID (application defined).  */
    uint8_t          feature_type;                   /*!<  0 = hydrographic, 1 = informational (defined as 0 for pre-3.0 files).  */
    time_t           event_tv_sec;                   /*!<  POSIX seconds of the related data point (hydrographic).  */
    long             event_tv_nsec;                  /*!<  POSIX nanoseconds of the second of the related data point (hydrographic).  */
    double           latitude;                       /*!<  Latitude  */
    double           longitude;                      /*!<  Longitude  */
    float            length;                         /*!<  Length of feature.  */
    float            width;                          /*!<  Width of feature.  */
    float            height;                         /*!<  Height of feature.  */
    float            depth;                          /*!<  Depth of feature.  */
    float            datum;                          /*!<  This value is subtracted from the depth at read  (it is not applied internally).  */
    float            horizontal_orientation;         /*!<  East/west orientation of feature.  */
    float            vertical_orientation;           /*!<  North/south orientation of feature.  */
    char             description[128];               /*!<  Description of feature, usually an S44 description.  */
    char             remarks[128];                   /*!<  Remarks  */
    uint8_t          sonar_type;                     /*!<  Sonar/LiDAR type (hydrographic).  */
    uint8_t          equip_type;                     /*!<  Equipment type (hydrographic).  */
    uint8_t          platform_type;                  /*!<  Survey platform type (hydrographic).  */
    uint8_t          nav_system;                     /*!<  Navigation system type (hydrographic).  */
    float            heading;                        /*!<  Heading of survey platform (hydrographic).  */
    uint8_t          confidence_level;               /*!<  Confidence level: 0 = invalid, 1-2 = unverified, 3-5 = verified.  */
    char             analyst_activity[40];           /*!<  Name of organization that defined the feature.  */
    int64_t          poly_address;                   /*!<  Address in .bfa file of polygon (or NULL).  */
    uint32_t         poly_count;                     /*!<  Number of points in associated polygon/polyline.  */
    uint8_t          poly_type;                      /*!<  0 - polyline, 1 - polygon  */
    int64_t          image_address;                  /*!<  Address in .bfa file of image (or NULL).  */
    uint32_t         image_size;                     /*!<  Size of associated image in bytes.  */
    char             image_name[128];                /*!<  Base image file name (unqualified) */
    uint32_t         parent_record;                  /*!<  Defined as parent record number plus 1 so that 0 indicates no parent record.  */
    uint32_t         child_record;                   /*!<  Defined as child record number plus 1 so that 0 indicates no child record.  */
  } BFDATA_RECORD;



  typedef struct
  {
    double           latitude[BFDATA_POLY_ARRAY_SIZE];
    double           longitude[BFDATA_POLY_ARRAY_SIZE];
  } BFDATA_POLYGON;



  /*!  Short feature (target) structure - to be held in memory by the API for use by an application.  See BFDATA_RECORD above for 
       field definitions.  */

  typedef struct
  {
    uint32_t         record_number;
    uint8_t          feature_type;
    time_t           event_tv_sec;
    long             event_tv_nsec;
    double           latitude;
    double           longitude;
    float            depth;
    uint32_t         confidence_level;
    char             description[128];
    char             remarks[128];
    uint32_t         poly_count;
    uint32_t         poly_type;
    uint32_t         parent_record;
    uint32_t         child_record;
  } BFDATA_SHORT_FEATURE;




  /*  Public API functions.  */

  BFDATA_DLL int32_t binaryFeatureData_write_record (int32_t hnd, int32_t recnum, BFDATA_RECORD *bfd_record, BFDATA_POLYGON *poly, uint8_t *image);
  BFDATA_DLL int32_t binaryFeatureData_write_record_image_file (int32_t hnd, int32_t recnum, BFDATA_RECORD *bfd_record, BFDATA_POLYGON *poly,
                                                                const char *image_file);
  BFDATA_DLL int32_t binaryFeatureData_create_file (const char *path, BFDATA_HEADER bfd_header);
  BFDATA_DLL int32_t binaryFeatureData_open_file (const char *path, BFDATA_HEADER *bfd_header, int32_t mode);
  BFDATA_DLL int32_t binaryFeatureData_close_file (int32_t hnd);
  BFDATA_DLL int32_t binaryFeatureData_read_record (int32_t hnd, int32_t recnum, BFDATA_RECORD *bfd_record);
  BFDATA_DLL int32_t binaryFeatureData_read_all_short_features (int32_t hnd, BFDATA_SHORT_FEATURE **bfd_feature);
  BFDATA_DLL int32_t binaryFeatureData_read_polygon (int32_t hnd, int32_t recnum, BFDATA_POLYGON *poly);
  BFDATA_DLL int32_t binaryFeatureData_read_image (int32_t hnd, int32_t recnum, uint8_t *image);
  BFDATA_DLL void binaryFeatureData_update_header (int32_t hnd, BFDATA_HEADER bfd_header);
  BFDATA_DLL char *binaryFeatureData_strerror ();
  BFDATA_DLL void binaryFeatureData_perror ();
  BFDATA_DLL char *binaryFeatureData_get_version ();
  BFDATA_DLL void binaryFeatureData_dump_record (BFDATA_RECORD bfd_record);
  BFDATA_DLL void binaryFeatureData_cvtime (time_t tv_sec, long tv_nsec, int32_t *year, int32_t *jday, int32_t *hour, int32_t *minute, float *second);
  BFDATA_DLL void binaryFeatureData_inv_cvtime (int32_t year, int32_t jday, int32_t hour, int32_t min, float sec, time_t *tv_sec, long *tv_nsec);
  BFDATA_DLL void binaryFeatureData_jday2mday (int32_t year, int32_t jday, int32_t *mon, int32_t *mday);
  BFDATA_DLL void binaryFeatureData_mday2jday (int32_t year, int32_t mon, int32_t mday, int32_t *jday);


#ifdef  __cplusplus
}
#endif

#endif
