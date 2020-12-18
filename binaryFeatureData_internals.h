
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



#ifndef __BFDATA_INTERNALS_H__
#define __BFDATA_INTERNALS_H__

#ifdef  __cplusplus
extern "C" {
#endif


#ifndef         DPRINT
#define         DPRINT          fprintf (stderr, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);fflush (stderr);
#endif
#ifndef         NINT
#define         NINT(a)         ((a) < 0.0 ? (int) ((a) - 0.5) : (int) ((a) + 0.5))
#endif


/*!  This is the structure we use to keep track of important formatting data for an open BFD file.  */

typedef struct
{
  FILE          *fp;                        /*!<  BFD file pointer.  */
  FILE          *afp;                       /*!<  File pointer for associated polygons and images.  */
  uint8_t       swap;                       /*!<  1 if we need to byte swap the records.  */
  char          path[1024];                 /*!<  File name.  */
  char          a_path[1024];               /*!<  Associated polygon/polyline/image file name.  */
  uint8_t       modified;                   /*!<  Set if the file has been modified.  */
  uint8_t       created;                    /*!<  Set if we created the file.  */
  uint8_t       write;                      /*!<  Set if the last action to the file was a write.  */
  uint32_t      recnum;                     /*!<  Number of next record to read or write (for BFDATA_NEXT_RECORD).  */
  uint32_t      last_rec;                   /*!<  Number of the last record read (so we might not have to re-read the
                                                  BFDATA_RECORD to get the polygon data.  */
  BFDATA_RECORD record;                     /*!<  Last BFDATA_RECORD read (not written).  */
  uint16_t      major_version;              /*!<  Major version number for backward compatibility.  */
  uint32_t      header_size;                /*!<  Header size in bytes.  */
  uint32_t      record_size;                /*!<  Record size in bytes.  */
  BFDATA_SHORT_FEATURE *short_feature;      /*!<  Allocated array of truncated records for fast memory access in applications.  */
  BFDATA_HEADER header;                     /*!<  BFD file header.  */
} INTERNAL_BFDATA_STRUCT;


/*!  BFD error handling variables.  */

typedef struct 
{
  int32_t       system;            /*!<  Last system error condition encountered.  */
  int32_t       bfd;               /*!<  Last BFD error condition encountered.  */
  char          file[512];         /*!<  Name of file being accessed when last error encountered (if applicable).  */
  int32_t       recnum;            /*!<  Record number being accessed when last error encountered (if applicable).  */
  char          info[128];
} BFDATA_ERROR_STRUCT;


#ifdef  __cplusplus
}
#endif


#endif


