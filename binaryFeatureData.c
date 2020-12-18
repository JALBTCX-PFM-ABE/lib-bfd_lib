
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



#include <errno.h>

#include "binaryFeatureData.h"
#include "binaryFeatureData_internals.h"
#include "binaryFeatureData_version.h"


/*  This is where we'll store the headers of all open BFD files (in addition to a bunch of other things, see above).  */

static INTERNAL_BFDATA_STRUCT bfdh[BFDATA_MAX_FILES];


/*  Startup flag used by either binaryFeatureData_create_file or binaryFeatureData_open_file to initialize the internal struct array.  */

static uint8_t first = 1;


static BFDATA_ERROR_STRUCT bfd_error;


/*  Some static functions that don't need to be in this file.  */

#include "binaryFeatureData_functions.h"


/*!  Compute the actual size of the BFDATA_RECORD as it will be stored on disk.  */

static int32_t binaryFeatureData_compute_record_size (int32_t hnd)
{
  int32_t size = 0;


  /*  RECORD NUMBER IS NOT WRITTEN TO DISK.  */

  size += sizeof (char) * 15;                     /*  contact_id[15]  */


  /*  Pre 2.00 screwup.  I used the structure size even though I wasn't writing structures.  DOH!  */

  if (bfdh[hnd].major_version < 2)
    {
      size += sizeof (time_t);                    /*  event_tv_sec  */
      size += sizeof (long);                      /*  event_tv_nsec  */
    }
  else
    {
      size += sizeof (int64_t);                   /*  event_tv_sec (64 bits for after 2038)  */
      size += sizeof (int32_t);                   /*  event_tv_nsec */
    }

  size += sizeof (double);                        /*  latitude  */
  size += sizeof (double);                        /*  longitude  */
  size += sizeof (float);                         /*  length  */
  size += sizeof (float);                         /*  width  */
  size += sizeof (float);                         /*  height  */
  size += sizeof (float);                         /*  depth  */
  size += sizeof (float);                         /*  datum  */
  size += sizeof (float);                         /*  horizontal_orientation  */
  size += sizeof (float);                         /*  vertical_orientation  */
  size += sizeof (char) * 128;                    /*  description[128]  */
  size += sizeof (char) * 128;                    /*  remarks[128]  */
  size += sizeof (uint8_t);                       /*  sonar_type  */
  size += sizeof (uint8_t);                       /*  equip_type  */
  size += sizeof (uint8_t);                       /*  platform_type  */
  size += sizeof (uint8_t);                       /*  nav_system  */
  size += sizeof (float);                         /*  heading  */
  size += sizeof (uint8_t);                       /*  confidence_level  */
  size += sizeof (char) * 40;                     /*  analyst_activity[40]  */
  size += sizeof (int64_t);                       /*  poly_address  */
  size += sizeof (uint32_t);                      /*  poly_count  */
  size += sizeof (uint8_t);                       /*  poly_type  */
  size += sizeof (uint32_t);                      /*  image_size  */
  size += sizeof (char) * 128;                    /*  image_name[128]  */
  size += sizeof (int64_t);                       /*  image_address  */
  size += sizeof (uint32_t);                      /*  parent_record  */
  size += sizeof (uint32_t);                      /*  child_record  */


  /*  Version 3.0 dependency.  */

  if (bfdh[hnd].major_version >= 3)
    {
      size += sizeof (uint8_t);                   /*  feature type  */
    }

  return (size);
}




static void binaryFeatureData_swap_record (BFDATA_RECORD *bfd_record, int32_t hnd)
{
  /*  Pre 2.00 screwup.  I used the structure size even though I wasn't writing structures.  DOH!  */

  if (bfdh[hnd].major_version < 2)
    {
      binaryFeatureData_swap_int ((int32_t *) &bfd_record->event_tv_sec);
      binaryFeatureData_swap_int ((int32_t *) &bfd_record->event_tv_nsec);
    }

  binaryFeatureData_swap_double (&bfd_record->latitude);
  binaryFeatureData_swap_double (&bfd_record->longitude);
  binaryFeatureData_swap_float (&bfd_record->length);
  binaryFeatureData_swap_float (&bfd_record->width);
  binaryFeatureData_swap_float (&bfd_record->height);
  binaryFeatureData_swap_float (&bfd_record->depth);
  binaryFeatureData_swap_float (&bfd_record->horizontal_orientation);
  binaryFeatureData_swap_float (&bfd_record->vertical_orientation);
  binaryFeatureData_swap_float (&bfd_record->heading);
  binaryFeatureData_swap_double ((double *) &bfd_record->poly_address);
  binaryFeatureData_swap_int ((int32_t *) &bfd_record->poly_count);
  binaryFeatureData_swap_double ((double *) &bfd_record->image_address);
  binaryFeatureData_swap_int ((int32_t *) &bfd_record->image_size);
  binaryFeatureData_swap_int ((int32_t *) &bfd_record->parent_record);
  binaryFeatureData_swap_int ((int32_t *) &bfd_record->child_record);
}



static void binaryFeatureData_swap_polygon (int32_t count, BFDATA_POLYGON *bfd_polygon)
{
  int32_t     i;


  for (i = 0 ; i < count ; i++)
    {
      binaryFeatureData_swap_double (&bfd_polygon->latitude[i]);
      binaryFeatureData_swap_double (&bfd_polygon->longitude[i]);
    }
}



static uint8_t binaryFeatureData_get_record (int32_t hnd, BFDATA_RECORD *bfd_record)
{
  int64_t tmp_d;
  int32_t tmp_s;


  if (!fread (&bfd_record->contact_id, 15, 1, bfdh[hnd].fp)) return (0);


  /*  Pre 2.00 screwup.  I used the structure size even though I wasn't writing structures.  DOH!  */

  if (bfdh[hnd].major_version < 2)
    {
      if (!fread (&bfd_record->event_tv_sec, sizeof (time_t), 1, bfdh[hnd].fp)) return (0);
      if (!fread (&bfd_record->event_tv_nsec, sizeof (long), 1, bfdh[hnd].fp)) return (0);
    }
  else
    {
      /*  We swap here because we're storing tv_sec in 64 bits but we stuff it into a time_t which is 32 bits on 32 bit systems.  */

      if (!fread (&tmp_d, sizeof (int64_t), 1, bfdh[hnd].fp)) return (0);
      if (bfdh[hnd].swap) binaryFeatureData_swap_double ((double *) &tmp_d);
      bfd_record->event_tv_sec = tmp_d;


      /*  We swap here because we're storing tv_nsec in 32 bits but we stuff it into a long which is 64 bits on 64 bit systems.  */

      if (!fread (&tmp_s, sizeof (int32_t), 1, bfdh[hnd].fp)) return (0);
      if (bfdh[hnd].swap) binaryFeatureData_swap_int ((int32_t *) &tmp_s);
      bfd_record->event_tv_nsec = (long) tmp_s;
    }

  if (!fread (&bfd_record->latitude, sizeof (double), 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->longitude, sizeof (double), 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->length, sizeof (float), 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->width, sizeof (float), 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->height, sizeof (float), 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->depth, sizeof (float), 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->datum, sizeof (float), 1, bfdh[hnd].fp)) return (0);


  /*  Applying the datum shift (or 0.0).  */

  bfd_record->depth -= bfd_record->datum;


  if (!fread (&bfd_record->horizontal_orientation, sizeof (float), 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->vertical_orientation, sizeof (float), 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->description, 128, 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->remarks, 128, 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->sonar_type, sizeof (uint8_t), 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->equip_type, sizeof (uint8_t), 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->platform_type, sizeof (uint8_t), 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->nav_system, sizeof (uint8_t), 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->heading, sizeof (float), 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->confidence_level, sizeof (uint8_t), 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->analyst_activity, 40, 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->poly_address, sizeof (int64_t), 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->poly_count, sizeof (uint32_t), 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->poly_type, sizeof (uint8_t), 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->image_address, sizeof (int64_t), 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->image_size, sizeof (uint32_t), 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->image_name, 128, 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->parent_record, sizeof (uint32_t), 1, bfdh[hnd].fp)) return (0);
  if (!fread (&bfd_record->child_record, sizeof (uint32_t), 1, bfdh[hnd].fp)) return (0);


  /*  Version 3.0 dependency.  */

  if (bfdh[hnd].major_version >= 3)
    {
      if (!fread (&bfd_record->feature_type, sizeof (uint8_t), 1, bfdh[hnd].fp)) return (0);
    }
  else
    {
      bfd_record->feature_type = 0;
    }


  if (bfdh[hnd].swap) binaryFeatureData_swap_record (bfd_record, hnd);

  return (1);
}




static uint8_t binaryFeatureData_put_record (int32_t hnd, BFDATA_RECORD *bfd_record)
{
  int64_t tmp_d = (int64_t) bfd_record->event_tv_sec;
  int32_t tmp_s = (int64_t) bfd_record->event_tv_nsec;


  /*  Pre 2.00 screwup.  I used the structure size even though I wasn't writing structures.  DOH!  */

  if (bfdh[hnd].major_version < 2)
    {
      if (!fwrite (&bfd_record->event_tv_sec, sizeof (time_t), 1, bfdh[hnd].fp)) return (0);
      if (!fwrite (&bfd_record->event_tv_nsec, sizeof (long), 1, bfdh[hnd].fp)) return (0);
    }
  else
    {
      if (bfdh[hnd].swap)
        {
          binaryFeatureData_swap_record (bfd_record, hnd);
          binaryFeatureData_swap_double ((double *) &tmp_d);
          binaryFeatureData_swap_int ((int32_t *) &tmp_s);
        }
    }


  if (!fwrite (&bfd_record->contact_id, 15, 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&tmp_d, sizeof (int64_t), 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&tmp_s, sizeof (int32_t), 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->latitude, sizeof (double), 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->longitude, sizeof (double), 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->length, sizeof (float), 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->width, sizeof (float), 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->height, sizeof (float), 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->depth, sizeof (float), 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->datum, sizeof (float), 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->horizontal_orientation, sizeof (float), 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->vertical_orientation, sizeof (float), 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->description, 128, 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->remarks, 128, 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->sonar_type, sizeof (uint8_t), 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->equip_type, sizeof (uint8_t), 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->platform_type, sizeof (uint8_t), 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->nav_system, sizeof (uint8_t), 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->heading, sizeof (float), 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->confidence_level, sizeof (uint8_t), 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->analyst_activity, 40, 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->poly_address, sizeof (int64_t), 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->poly_count, sizeof (uint32_t), 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->poly_type, sizeof (uint8_t), 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->image_address, sizeof (int64_t), 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->image_size, sizeof (uint32_t), 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->image_name, 128, 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->parent_record, sizeof (uint32_t), 1, bfdh[hnd].fp)) return (0);
  if (!fwrite (&bfd_record->child_record, sizeof (uint32_t), 1, bfdh[hnd].fp)) return (0);


  /*  Version 3.0 dependency.  */

  if (bfdh[hnd].major_version >= 3)
    {
      /*  Make sure that any application that is unaware of the 3.0 addition of feature_type will set it to BFDATA_HYDROGRAPHIC.  */

      if (bfd_record->feature_type >= BFDATA_FEATURE_TYPES) bfd_record->feature_type = BFDATA_HYDROGRAPHIC;


      if (!fwrite (&bfd_record->feature_type, sizeof (uint8_t), 1, bfdh[hnd].fp)) return (0);
    }


  return (1);
}




static uint8_t binaryFeatureData_get_polygon (int32_t hnd, int32_t count, BFDATA_POLYGON *poly)
{
  int32_t i;


  for (i = 0 ; i < count ; i++)
    {
      if (!fread (&poly->latitude[i], sizeof (double), 1, bfdh[hnd].afp)) return (0);
      if (!fread (&poly->longitude[i], sizeof (double), 1, bfdh[hnd].afp)) return (0);
    }

  if (bfdh[hnd].swap) binaryFeatureData_swap_polygon (count, poly);

  return (1);
}




static uint8_t binaryFeatureData_put_polygon (int32_t hnd, int32_t count, BFDATA_POLYGON *poly)
{
  int32_t i;


  if (bfdh[hnd].swap) binaryFeatureData_swap_polygon (count, poly);

  for (i = 0 ; i < count ; i++)
    {
      if (!fwrite (&poly->latitude[i], sizeof (double), 1, bfdh[hnd].afp)) return (0);
      if (!fwrite (&poly->longitude[i], sizeof (double), 1, bfdh[hnd].afp)) return (0);
    }

  return (1);
}




/********************************************************************************************/
/*!

 - Function:    binaryFeatureData_write_record

 - Purpose:     Swaps the bytes (if needed) and writes the record and (optionally) the 
                polygon/polyline.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        03/27/09

 - Arguments:
                - hnd            =    The BFD file handle
                - recnum         =    The record number to write or BFDATA_NEXT_RECORD to append
                - bfd_record     =    The BFD point record to receive the data
                - poly           =    The polygon/polyline structure or NULL if you haven't
                                       modified or created the polygon/polyline 
                - image          =    The image or NULL if you haven't modified or created an image 

 - Returns:
                - BFDATA_SUCCESS
                - BFDATA_INVALID_RECORD_NUMBER
                - BFDATA_POLYGON_TOO_LARGE_ERROR
                - BFDATA_POLY_WRITE_FSEEK_ERROR
                - BFDATA_POLY_WRITE_ERROR
                - BFDATA_RECORD_WRITE_FSEEK_ERROR
                - BFDATA_RECORD_WRITE_ERROR

*********************************************************************************************/

BFDATA_DLL int32_t binaryFeatureData_write_record (int32_t hnd, int32_t recnum, BFDATA_RECORD *bfd_record, BFDATA_POLYGON *poly, uint8_t *image)
{
  int32_t i, pos;


  if (recnum < BFDATA_NEXT_RECORD)
    {
      bfd_error.recnum = recnum;
      strcpy (bfd_error.file, bfdh[hnd].path);
      return (bfd_error.bfd = BFDATA_INVALID_RECORD_NUMBER);
    }


  if (bfd_record->poly_count && poly != NULL)
    {
      if (bfd_record->poly_count > BFDATA_POLY_ARRAY_SIZE)
        {
          bfd_error.recnum = recnum;
          strcpy (bfd_error.file, bfdh[hnd].a_path);
          return (bfd_error.bfd = BFDATA_POLYGON_TOO_LARGE_ERROR);
        }


      /*  We always want to tack the polygon data on to the end of the file.  If there was a pre-existing address
          we're just going to change it and let the data stay there.  We're not going to try to pack the file or 
          save space since this should be a temporary file anyway.  After all, this is a working format and, at the
          end of the processing cycle should be converted to the NAVO standard MIW XML format.  */

      if (fseeko64 (bfdh[hnd].afp, 0LL, SEEK_END) < 0)
        {
          bfd_error.system = errno;
          bfd_error.recnum = recnum;
          strcpy (bfd_error.file, bfdh[hnd].a_path);
          return (bfd_error.bfd = BFDATA_POLY_WRITE_FSEEK_ERROR);
        }


      bfd_record->poly_address = ftello64 (bfdh[hnd].afp);


      for (i = 0 ; i < bfd_record->poly_count ; i++)
        {
          if (!binaryFeatureData_put_polygon (hnd, bfd_record->poly_count, poly))
            {
              bfd_error.system = errno;
              bfd_error.recnum = recnum;
              strcpy (bfd_error.file, bfdh[hnd].a_path);
              return (bfd_error.bfd = BFDATA_POLY_WRITE_ERROR);
            }
        }
    }


  if (bfd_record->image_size && image != NULL)
    {
      /*  We always want to tack the image data on to the end of the file.  If there was a pre-existing address
          we're just going to change it and let the data stay there.  We're not going to try to pack the file or 
          save space since this should be a temporary file anyway.  After all, this is a working format and, at the
          end of the processing cycle should be converted to the NAVO standard MIW XML format.  */

      if (fseeko64 (bfdh[hnd].afp, 0LL, SEEK_END) < 0)
        {
          bfd_error.system = errno;
          bfd_error.recnum = recnum;
          strcpy (bfd_error.file, bfdh[hnd].a_path);
          return (bfd_error.bfd = BFDATA_IMAGE_WRITE_FSEEK_ERROR);
        }


      bfd_record->image_address = ftello64 (bfdh[hnd].afp);


      if (!fwrite (image, bfd_record->image_size, 1, bfdh[hnd].afp))
        {
          bfd_error.system = errno;
          bfd_error.recnum = recnum;
          strcpy (bfd_error.file, bfdh[hnd].a_path);
          return (bfd_error.bfd = BFDATA_IMAGE_WRITE_ERROR);
        }
    }


  if (recnum == BFDATA_NEXT_RECORD)
    {
      bfdh[hnd].recnum = bfdh[hnd].header.number_of_records;
      pos = bfdh[hnd].recnum * bfdh[hnd].record_size + bfdh[hnd].header_size;
    }
  else
    {
      pos = recnum * bfdh[hnd].record_size + bfdh[hnd].header_size;
      bfdh[hnd].recnum = recnum;
    }


  if (fseek (bfdh[hnd].fp, pos, SEEK_SET) < 0)
    {
      bfd_error.system = errno;
      bfd_error.recnum = recnum;
      strcpy (bfd_error.file, bfdh[hnd].path);
      return (bfd_error.bfd = BFDATA_RECORD_WRITE_FSEEK_ERROR);
    }


  if (!binaryFeatureData_put_record (hnd, bfd_record))
    {
      bfd_error.system = errno;
      bfd_error.recnum = recnum;
      strcpy (bfd_error.file, bfdh[hnd].path);
      return (bfd_error.bfd = BFDATA_RECORD_WRITE_ERROR);
    }


  bfdh[hnd].modified = 1;
  bfdh[hnd].write = 1;
  bfdh[hnd].last_rec = -1;


  bfdh[hnd].recnum++;


  if (recnum == BFDATA_NEXT_RECORD) bfdh[hnd].header.number_of_records++;


  bfd_error.system = 0;
  return (bfd_error.bfd = BFDATA_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    binaryFeatureData_write_record_image_file

 - Purpose:     This function reads the specified image file instead of a binary image.  It
                stores the size and basename of the image in the bfd_record and then calls
                binaryFeatureData_write_record to write the record, polygon, and image.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        03/27/09

 - Arguments:
                - hnd            =    The BFD file handle
                - recnum         =    The record number to write or BFDATA_NEXT_RECORD to append
                - bfd_record     =    The BFD point record to receive the data
                - poly           =    The polygon/polyline structure or NULL if you haven't
                                      modified or created the polygon/polyline 
                - image_file     =    The image file path or "" if no file available

 - Returns:
                - BFDATA_SUCCESS
                - BFDATA_INVALID_RECORD_NUMBER
                - BFDATA_POLYGON_TOO_LARGE_ERROR
                - BFDATA_POLY_WRITE_FSEEK_ERROR
                - BFDATA_POLY_WRITE_ERROR
                - BFDATA_RECORD_WRITE_FSEEK_ERROR
                - BFDATA_RECORD_WRITE_ERROR
                - BFDATA_IMAGE_FILE_OPEN_ERROR
                - BFDATA_IMAGE_FILE_READ_ERROR

*********************************************************************************************/

BFDATA_DLL int32_t binaryFeatureData_write_record_image_file (int32_t hnd, int32_t recnum, BFDATA_RECORD *bfd_record, BFDATA_POLYGON *poly,
                                                              const char *image_file)
{
  FILE *ifp;
  uint8_t *image = NULL;
  int32_t ret;


  if (strlen (image_file) >= 2)
    {
      if ((ifp = fopen (image_file, "rb")) == NULL)
        {
          bfd_error.system = errno;
          bfd_error.recnum = recnum;
          strcpy (bfd_error.file, image_file);
          return (bfd_error.bfd = BFDATA_IMAGE_FILE_OPEN_ERROR);
        }


      strcpy (bfd_record->image_name, binaryFeatureData_gen_basename (image_file));


      fseek (ifp, 0, SEEK_END);
      bfd_record->image_size = ftell (ifp);
      fseek (ifp, 0, SEEK_SET);


      image = (uint8_t *) malloc (bfd_record->image_size);

      if (image == NULL)
        {
          perror ("Allocating image memory");
	  fflush (stderr);
          exit (-1);
        }


      if (!fread (image, bfd_record->image_size, 1, ifp))
        {
          bfd_error.system = errno;
          bfd_error.recnum = recnum;
          strcpy (bfd_error.file, image_file);
          free (image);
          fclose (ifp);
          return (bfd_error.bfd = BFDATA_IMAGE_FILE_READ_ERROR);
        }

      fclose (ifp);
    }


  ret = binaryFeatureData_write_record (hnd, recnum, bfd_record, poly, image);


  free (image);

  return (ret);
}



/********************************************************************************************/
/*!

 - Function:    binaryFeatureData_write_header

 - Purpose:     Write the bfd_header to the BFD file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        03/27/09

 - Arguments:   hnd            =    The BFD file handle

 - Returns:
                - BFDATA_SUCCESS
                - BFDATA_HEADER_WRITE_FSEEK_ERROR
                - BFDATA_HEADER_WRITE_ERROR

*********************************************************************************************/

static int32_t binaryFeatureData_write_header (int32_t hnd)
{
  char space = ' ';
  int32_t i, size, year, jday, hour, minute, month, day;
  float second;


  if (fseek (bfdh[hnd].fp, 0, SEEK_SET) < 0)
    {
      bfd_error.system = errno;
      strcpy (bfd_error.file, bfdh[hnd].path);
      return (bfd_error.bfd = BFDATA_HEADER_WRITE_FSEEK_ERROR);
    }


  fprintf (bfdh[hnd].fp, "[VERSION] = %s\n", BFDATA_VERSION);

  if (binaryFeatureData_big_endian ())
    {
      fprintf (bfdh[hnd].fp, "[ENDIAN] = BIG\n");
    }
  else
    {
      fprintf (bfdh[hnd].fp, "[ENDIAN] = LITTLE\n");
    }


  binaryFeatureData_cvtime (bfdh[hnd].header.creation_tv_sec, bfdh[hnd].header.creation_tv_nsec, &year, &jday, &hour, &minute, &second);
  binaryFeatureData_jday2mday (year, jday, &month, &day);
  month++;

  fprintf (bfdh[hnd].fp, "[CREATION YEAR] = %d\n", year + 1900);
  fprintf (bfdh[hnd].fp, "[CREATION MONTH] = %02d\n", month);
  fprintf (bfdh[hnd].fp, "[CREATION DAY] = %02d\n", day);
  fprintf (bfdh[hnd].fp, "[CREATION DAY OF YEAR] = %03d\n", jday);
  fprintf (bfdh[hnd].fp, "[CREATION HOUR] = %02d\n", hour);
  fprintf (bfdh[hnd].fp, "[CREATION MINUTE] = %02d\n", minute);
  fprintf (bfdh[hnd].fp, "[CREATION SECOND] = %5.2f\n", second);
  if (strlen (bfdh[hnd].header.creation_software) > 2) fprintf (bfdh[hnd].fp, "[CREATION SOFTWARE] = %s\n", bfdh[hnd].header.creation_software);


  binaryFeatureData_cvtime (bfdh[hnd].header.modification_tv_sec, bfdh[hnd].header.modification_tv_nsec, &year, &jday, &hour, &minute, &second);
  binaryFeatureData_jday2mday (year, jday, &month, &day);
  month++;

  fprintf (bfdh[hnd].fp, "[MODIFICATION YEAR] = %d\n", year + 1900);
  fprintf (bfdh[hnd].fp, "[MODIFICATION MONTH] = %02d\n", month);
  fprintf (bfdh[hnd].fp, "[MODIFICATION DAY] = %02d\n", day);
  fprintf (bfdh[hnd].fp, "[MODIFICATION DAY OF YEAR] = %03d\n", jday);
  fprintf (bfdh[hnd].fp, "[MODIFICATION HOUR] = %02d\n", hour);
  fprintf (bfdh[hnd].fp, "[MODIFICATION MINUTE] = %02d\n", minute);
  fprintf (bfdh[hnd].fp, "[MODIFICATION SECOND] = %5.2f\n", second);
  if (strlen (bfdh[hnd].header.modification_software) > 2) fprintf (bfdh[hnd].fp, "[MODIFICATION SOFTWARE] = %s\n", bfdh[hnd].header.modification_software);

  if (strlen (bfdh[hnd].header.security_classification) > 2) fprintf (bfdh[hnd].fp, "[SECURITY CLASSIFICATION] = %s\n",
                                                                      bfdh[hnd].header.security_classification);
  if (strlen (bfdh[hnd].header.distribution) > 2) fprintf (bfdh[hnd].fp, "{DISTRIBUTION = \n%s\n}\n", bfdh[hnd].header.distribution);
  if (strlen (bfdh[hnd].header.declassification) > 2) fprintf (bfdh[hnd].fp, "{DECLASSIFICATION = \n%s\n}\n", bfdh[hnd].header.declassification);
  if (strlen (bfdh[hnd].header.class_just) > 2) fprintf (bfdh[hnd].fp, "{SECURITY CLASSIFICATION JUSTIFICATION = \n%s\n}\n", bfdh[hnd].header.class_just);
  if (strlen (bfdh[hnd].header.downgrade) > 2) fprintf (bfdh[hnd].fp, "{DOWNGRADE = \n%s\n}\n", bfdh[hnd].header.downgrade);

  fprintf (bfdh[hnd].fp, "[NUMBER OF RECORDS] = %d\n", bfdh[hnd].header.number_of_records);
  bfdh[hnd].header_size = BFDATA_HEADER_SIZE;
  fprintf (bfdh[hnd].fp, "[HEADER SIZE] = %d\n", bfdh[hnd].header_size);


  /*  Major version 2 or greater.  */

  if (bfdh[hnd].major_version >= 2) fprintf (bfdh[hnd].fp, "[RECORD SIZE] = %d\n", bfdh[hnd].record_size);


  if (strlen (bfdh[hnd].header.comments) > 2) fprintf (bfdh[hnd].fp, "{COMMENTS = \n%s\n}\n", bfdh[hnd].header.comments);


  /*  Space fill the rest.  */

  size = bfdh[hnd].header_size - ftell (bfdh[hnd].fp);


  for (i = 0 ; i < size ; i++)
    {
      if (!fwrite (&space, 1, 1, bfdh[hnd].fp))
        {
          bfd_error.system = errno;
          strcpy (bfd_error.file, bfdh[hnd].path);
          return (bfd_error.bfd = BFDATA_HEADER_WRITE_ERROR);
        }
    }


  bfdh[hnd].modified = 1;
  bfdh[hnd].write = 1;


  bfd_error.system = 0;
  return (bfd_error.bfd = BFDATA_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    binaryFeatureData_create_file

 - Purpose:     Create a BFD file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        03/27/09

 - Arguments:
                - path           =    The BFD file path
                - bfd_header     =    BFDATA_HEADER structure to be written to the file

 - Returns:
                - The file handle (0 or positive)
                - BFDATA_TOO_MANY_OPEN_FILES
                - BFDATA_CREATE_ERROR
                - BFDATA_CREATE_POLY_ERROR
                - BFDATA_HEADER_WRITE_FSEEK_ERROR
                - BFDATA_HEADER_WRITE_ERROR

*********************************************************************************************/

BFDATA_DLL int32_t binaryFeatureData_create_file (const char *path, BFDATA_HEADER bfd_header)
{
  char space = ' ', info[128];
  int32_t i, hnd, size;
  float tmpf;


  /*  The first time through we want to initialize (zero) the BFD handle array.  */

  if (first)
    {
      for (i = 0 ; i < BFDATA_MAX_FILES ; i++) 
        {
          memset (&bfdh[i], 0, sizeof (INTERNAL_BFDATA_STRUCT));
          bfdh[i].fp = NULL;
          bfdh[i].afp = NULL;
          bfdh[i].short_feature = NULL;
        }

      first = 0;
    }


  /*  Find the next available handle and make sure we haven't opened too many.  */

  hnd = BFDATA_MAX_FILES;
  for (i = 0 ; i < BFDATA_MAX_FILES ; i++)
    {
      if (bfdh[i].fp == NULL)
        {
          hnd = i;
          bfdh[hnd].last_rec = -1;
          break;
        }
    }


  if (hnd == BFDATA_MAX_FILES) return (bfd_error.bfd = BFDATA_TOO_MANY_OPEN_FILES);


  /*  Set the major version.  */

  strcpy (info, strstr (BFDATA_VERSION, "library V"));
  sscanf (info, "library V%f", &tmpf);
  bfdh[hnd].major_version = (int32_t) tmpf;


  /*  Make sure we know the BFDATA_RECORD size as stored on disk.  */

  bfdh[hnd].record_size = binaryFeatureData_compute_record_size (hnd);


  /*  Save the file name for error messages.  */

  strcpy (bfdh[hnd].path, path);


  /*  Open the file and write the header.  */

  if ((bfdh[hnd].fp = fopen (path, "wb+")) == NULL)
    {
      bfd_error.system = errno;
      strcpy (bfd_error.file, bfdh[hnd].path);
      return (bfd_error.bfd = BFDATA_CREATE_ERROR);
    }


  /*  Open the associated polygon file and write the version string.  */

  if (hnd >= 0)
    {
      strcpy (bfdh[hnd].a_path, path);
      sprintf (&bfdh[hnd].a_path[strlen (bfdh[hnd].a_path) - 3], "bfa");

      if ((bfdh[hnd].afp = fopen64 (bfdh[hnd].a_path, "wb+")) == NULL)
        {
          bfd_error.system = errno;
          strcpy (bfd_error.file, bfdh[hnd].a_path);
          return (bfd_error.bfd = BFDATA_CREATE_POLY_ERROR);
        }

      fprintf (bfdh[hnd].afp, "%s\n", BFDATA_VERSION);


      /*  Space fill the rest.  */

      size = BFDATA_POLY_VERSION_SIZE - ftello64 (bfdh[hnd].afp);


      for (i = 0 ; i < size ; i++)
        {
          if (!fwrite (&space, 1, 1, bfdh[hnd].afp))
            {
              bfd_error.system = errno;
              strcpy (bfd_error.file, bfdh[hnd].a_path);
              return (bfd_error.bfd = BFDATA_HEADER_WRITE_ERROR);
            }
        }
    }


  bfdh[hnd].header = bfd_header;


  bfdh[hnd].header.number_of_records = 0;


  /*  Write the header.  */

  if (binaryFeatureData_write_header (hnd) < 0) return (bfd_error.bfd = BFDATA_HEADER_WRITE_ERROR);


  bfdh[hnd].modified = 1;
  bfdh[hnd].created = 1;
  bfdh[hnd].write = 1;


  bfd_error.system = 0;
  return (hnd);
}


/********************************************************************************************/
/*!

 - Function:    binaryFeatureData_open_file

 - Purpose:     Open a BFD file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        03/25/09

 - Arguments:
                - path           =    The BFD file path
                - bfd_header     =    BFDATA_HEADER structure to be populated
                - mode           =    BFDATA_UPDATE or BFDATA_READ_ONLY

 - Returns:
                - The file handle (0 or positive) or
                - BFDATA_TOO_MANY_OPEN_FILES
                - BFDATA_OPEN_UPDATE_ERROR
                - BFDATA_OPEN_POLY_UPDATE_ERROR
                - BFDATA_OPEN_READONLY_ERROR
                - BFDATA_OPEN_POLY_READONLY_ERROR
                - BFDATA_NOT_BFD_FILE_ERROR

*********************************************************************************************/

BFDATA_DLL int32_t binaryFeatureData_open_file (const char *path, BFDATA_HEADER *bfd_header, int32_t mode)
{
  int32_t i, hnd, year[4], jday[4], hour[4], minute[4], eof;
  float second[4], tmpf;
  char varin[8192], info[8192];


  /*  The first time through we want to initialize (zero) the bfd handle array.  */

  if (first)
    {
      for (i = 0 ; i < BFDATA_MAX_FILES ; i++) 
        {
          memset (&bfdh[i], 0, sizeof (INTERNAL_BFDATA_STRUCT));
          bfdh[i].fp = NULL;
          bfdh[i].afp = NULL;
          bfdh[i].short_feature = NULL;
        }

      first = 0;
    }


  /*  Find the next available handle and make sure we haven't opened too many.  */

  hnd = BFDATA_MAX_FILES;
  for (i = 0 ; i < BFDATA_MAX_FILES ; i++)
    {
      if (bfdh[i].fp == NULL)
        {
          hnd = i;
          bfdh[hnd].last_rec = -1;
          break;
        }
    }


  if (hnd == BFDATA_MAX_FILES) return (bfd_error.bfd = BFDATA_TOO_MANY_OPEN_FILES);


  /*  Internal structs are zeroed above and on close of file so we don't have to do it here.  */


  /*  Save the file name for error messages.  */

  strcpy (bfdh[hnd].path, path);


  /*  Open the file and read the header.  */

  switch (mode)
    {
    case BFDATA_UPDATE:
      if ((bfdh[hnd].fp = fopen (path, "rb+")) == NULL)
        {
          bfd_error.system = errno;
          strcpy (bfd_error.file, bfdh[hnd].path);
          return (bfd_error.bfd = BFDATA_OPEN_UPDATE_ERROR);
        }


      strcpy (bfdh[hnd].a_path, path);
      sprintf (&bfdh[hnd].a_path[strlen (bfdh[hnd].a_path) - 3], "bfa");

      if ((bfdh[hnd].afp = fopen64 (bfdh[hnd].a_path, "rb+")) == NULL)
        {
          bfd_error.system = errno;
          strcpy (bfd_error.file, bfdh[hnd].a_path);
          return (bfd_error.bfd = BFDATA_OPEN_POLY_UPDATE_ERROR);
        }
      break;

    case BFDATA_READONLY:
      if ((bfdh[hnd].fp = fopen (path, "rb")) == NULL)
        {
          bfd_error.system = errno;
          strcpy (bfd_error.file, bfdh[hnd].path);
          return (bfd_error.bfd = BFDATA_OPEN_READONLY_ERROR);
        }


      strcpy (bfdh[hnd].a_path, path);
      sprintf (&bfdh[hnd].a_path[strlen (bfdh[hnd].a_path) - 3], "bfa");

      if ((bfdh[hnd].afp = fopen64 (bfdh[hnd].a_path, "rb")) == NULL)
        {
          bfd_error.system = errno;
          strcpy (bfd_error.file, bfdh[hnd].a_path);
          return (bfd_error.bfd = BFDATA_OPEN_POLY_READONLY_ERROR);
        }
      break;
    }


  year[0] = year[1] = 0;


  /*  We want to try to read the first line (version info) with an fread in case we mistakenly asked to
      load a binary file.  If we try to use bfd_ngets to read a binary file and there are no line feeds in 
      the first sizeof (varin) characters we would segfault.  */

  if (!fread (varin, 128, 1, bfdh[hnd].fp))
    {
      strcpy (bfd_error.file, bfdh[hnd].path);
      return (bfd_error.bfd = BFDATA_NOT_BFD_FILE_ERROR);
    }


  /*  Check for the "Binary Feature Data library" or "BFD library" string at the beginning of the file.  Either is acceptable, 
      it was changed in version 1.02).  */

  if (!strstr (varin, "Binary Feature Data library") && !strstr (varin, "BFD library"))
    {
      strcpy (bfd_error.file, bfdh[hnd].path);
      return (bfd_error.bfd = BFDATA_NOT_BFD_FILE_ERROR);
    }


  /*  Rewind to the beginning of the file.  Yes, we'll read the version again but we need to check the version number anyway.  */

  fseek (bfdh[hnd].fp, 0, SEEK_SET);


  /*  Note, we're using binaryFeatureData_ngets instead of fgets since we really don't want the CR/LF in the strings.  */

  while (binaryFeatureData_ngets (varin, sizeof (varin), bfdh[hnd].fp))
    {
      if (strstr (varin, "[END OF HEADER]")) break;


      /*  Put everything to the right of the equals sign into 'info'.   */

      binaryFeatureData_get_string (varin, info);


      /*  Read the version string and check the major version number against the library major version.  */

      if (strstr (varin, "[VERSION]"))
        {
          strcpy (bfdh[hnd].header.version, info);
          strcpy (info, strstr (varin, "library V"));
          sscanf (info, "library V%f", &tmpf);
          bfdh[hnd].major_version = (int16_t) tmpf;

          strcpy (info, strstr (BFDATA_VERSION, "library V"));
          sscanf (info, "library V%f", &tmpf);
          if (bfdh[hnd].major_version > (int16_t) tmpf)
            {
              strcpy (bfd_error.file, bfdh[hnd].path);
              return (bfd_error.bfd = BFDATA_NEWER_FILE_VERSION_ERROR);
            }
        }


      if (strstr (varin, "[ENDIAN]"))
        {
          if (binaryFeatureData_big_endian ())
            {
              if (strstr (info, "LITTLE"))
                {
                  bfdh[hnd].swap = 1;
                }
              else
                {
                  bfdh[hnd].swap = 0;
                }
            }
          else
            {
              if (strstr (info, "BIG"))
                {
                  bfdh[hnd].swap = 1;
                }
              else
                {
                  bfdh[hnd].swap = 0;
                }
            }
        }


      if (strstr (varin, "[CREATION YEAR]")) sscanf (info, "%d", &year[0]);
      if (strstr (varin, "[CREATION DAY OF YEAR]")) sscanf (info, "%d", &jday[0]);
      if (strstr (varin, "[CREATION HOUR]")) sscanf (info, "%d", &hour[0]);
      if (strstr (varin, "[CREATION MINUTE]")) sscanf (info, "%d", &minute[0]);
      if (strstr (varin, "[CREATION SECOND]")) sscanf (info, "%f", &second[0]);
      if (strstr (varin, "[CREATION SOFTWARE]")) strcpy (bfdh[hnd].header.creation_software, info);

      if (strstr (varin, "[MODIFICATION YEAR]")) sscanf (info, "%d", &year[1]);
      if (strstr (varin, "[MODIFICATION DAY OF YEAR]")) sscanf (info, "%d", &jday[1]);
      if (strstr (varin, "[MODIFICATION HOUR]")) sscanf (info, "%d", &hour[1]);
      if (strstr (varin, "[MODIFICATION MINUTE]")) sscanf (info, "%d", &minute[1]);
      if (strstr (varin, "[MODIFICATION SECOND]")) sscanf (info, "%f", &second[1]);
      if (strstr (varin, "[MODIFICATION SOFTWARE]")) strcpy (bfdh[hnd].header.modification_software, info);

      if (strstr (varin, "[SECURITY CLASSIFICATION]")) strcpy (bfdh[hnd].header.security_classification, info);

      if (strstr (varin, "{DISTRIBUTION ="))
        {
          strcpy (bfdh[hnd].header.distribution, "");
          while (fgets (varin, sizeof (varin), bfdh[hnd].fp))
            {
              if (varin[0] == '}') break;
              strcat (bfdh[hnd].header.distribution, varin);
            }
        }

      if (strstr (varin, "{DECLASSIFICATION ="))
        {
          strcpy (bfdh[hnd].header.declassification, "");
          while (fgets (varin, sizeof (varin), bfdh[hnd].fp))
            {
              if (varin[0] == '}') break;
              strcat (bfdh[hnd].header.declassification, varin);
            }
        }

      if (strstr (varin, "{SECURITY CLASSIFICATION JUSTIFICATION ="))
        {
          strcpy (bfdh[hnd].header.class_just, "");
          while (fgets (varin, sizeof (varin), bfdh[hnd].fp))
            {
              if (varin[0] == '}') break;
              strcat (bfdh[hnd].header.class_just, varin);
            }
        }


      if (strstr (varin, "{DOWNGRADE ="))
        {
          strcpy (bfdh[hnd].header.downgrade, "");
          while (fgets (varin, sizeof (varin), bfdh[hnd].fp))
            {
              if (varin[0] == '}') break;
              strcat (bfdh[hnd].header.downgrade, varin);
            }
        }


      if (strstr (varin, "[SECURITY CLASSIFICATION]")) strcpy (bfdh[hnd].header.security_classification, info);

      if (strstr (varin, "[NUMBER OF RECORDS]")) sscanf (info, "%d", &bfdh[hnd].header.number_of_records);

      if (strstr (varin, "{COMMENTS ="))
        {
          strcpy (bfdh[hnd].header.comments, "");
          while (fgets (varin, sizeof (varin), bfdh[hnd].fp))
            {
              if (varin[0] == '}') break;
              strcat (bfdh[hnd].header.comments, varin);
            }
        }


      if (strstr (varin, "[HEADER SIZE]")) sscanf (info, "%d", &bfdh[hnd].header_size);
      if (strstr (varin, "[RECORD SIZE]")) sscanf (info, "%d", &bfdh[hnd].record_size);
    }


  /*  Pre 2.00 screwup.  I used the structure size even though I wasn't writing structures.  DOH!  */

  if (bfdh[hnd].major_version < 2)
    {
      /*  Try to determine the correct record size since it was different when written on 32 vs 64 bit.  */


      fseek (bfdh[hnd].fp, 0, SEEK_END);
      eof = ftell (bfdh[hnd].fp);
      fseek (bfdh[hnd].fp, 0, SEEK_SET);

      eof -= bfdh[hnd].header_size;
      bfdh[hnd].record_size = NINT ((float) eof / (float) bfdh[hnd].header.number_of_records);

      if (bfdh[hnd].record_size != sizeof (BFDATA_RECORD))
        {
	  /*  Record size of 544 was written on a 32 bit system.  */

	  if (bfdh[hnd].record_size == 544)
	    {
	      fprintf (stderr, "\n\nBig problem!  Pre 2.00 file written on 32 bit system.\n");
	      fprintf (stderr, "The easiest solution is to log in to a 32 bit system and run\n");
	      fprintf (stderr, "bfd2miw then miw2bfd.  This will convert the file to BFD 2.0 format.\n\n\n");
	    }
	  else
	    {
	      fprintf (stderr, "\n\nBig problem!  Pre 2.00 file written on 64 bit system.\n");
	      fprintf (stderr, "The easiest solution is to log in to a 64 bit system and run\n");
	      fprintf (stderr, "bfd2miw then miw2bfd.  This will convert the file to BFD 2.0 format.\n\n\n");
	    }
	  exit (-1);
        }
    }


  bfdh[hnd].modified = 0;
  bfdh[hnd].created = 0;
  bfdh[hnd].write = 0;


  binaryFeatureData_inv_cvtime (year[0] - 1900, jday[0], hour[0], minute[0], second[0], &bfdh[hnd].header.creation_tv_sec,
                  &bfdh[hnd].header.creation_tv_nsec);

  binaryFeatureData_inv_cvtime (year[1] - 1900, jday[1], hour[1], minute[1], second[1], &bfdh[hnd].header.modification_tv_sec,
                  &bfdh[hnd].header.modification_tv_nsec);


  *bfd_header = bfdh[hnd].header;


  bfd_error.system = 0;
  return (hnd);
}


/********************************************************************************************/
/*!

 - Function:    binaryFeatureData_close_file

 - Purpose:     Close a BFD file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        03/27/09

 - Arguments:   hnd            =    The file handle

 - Returns:
                - BFDATA_SUCCESS
                - BFDATA_CLOSE_ERROR
                - BFDATA_CLOSE_POLY_ERROR

*********************************************************************************************/

BFDATA_DLL int32_t binaryFeatureData_close_file (int32_t hnd)
{
  time_t t;
  struct tm *cur_tm;


  /*  Just in case we've already closed this file.  */

  if (bfdh[hnd].fp == NULL) return (bfd_error.bfd = BFDATA_SUCCESS);


  if (bfdh[hnd].modified)
    {
      t = time (&t);
      cur_tm = gmtime (&t);
      binaryFeatureData_inv_cvtime (cur_tm->tm_year, cur_tm->tm_yday + 1, cur_tm->tm_hour, cur_tm->tm_min, cur_tm->tm_sec, 
				    &bfdh[hnd].header.modification_tv_sec, &bfdh[hnd].header.modification_tv_nsec);
    }

  if (bfdh[hnd].created)
    {
      t = time (&t);
      cur_tm = gmtime (&t);
      binaryFeatureData_inv_cvtime (cur_tm->tm_year, cur_tm->tm_yday + 1, cur_tm->tm_hour, cur_tm->tm_min, cur_tm->tm_sec, 
				    &bfdh[hnd].header.creation_tv_sec, &bfdh[hnd].header.creation_tv_nsec);
    }

  if (bfdh[hnd].created || bfdh[hnd].modified)
    {
      if (binaryFeatureData_write_header (hnd) < 0) return (bfd_error.bfd = BFDATA_HEADER_WRITE_ERROR);
    }


  if (fclose (bfdh[hnd].fp))
    {
      bfd_error.system = errno;
      strcpy (bfd_error.file, bfdh[hnd].path);
      return (bfd_error.bfd = BFDATA_CLOSE_ERROR);
    }


  if (fclose (bfdh[hnd].afp))
    {
      bfd_error.system = errno;
      strcpy (bfd_error.file, bfdh[hnd].a_path);
      return (bfd_error.bfd = BFDATA_CLOSE_POLY_ERROR);
    }


  /*  Clear up the short_feature memory if it was allocated.  */

  if (bfdh[hnd].short_feature != NULL) free (bfdh[hnd].short_feature);


  /*  Clear the internal structure.  */

  memset (&bfdh[hnd], 0, sizeof (INTERNAL_BFDATA_STRUCT));
  bfdh[hnd].fp = NULL;
  bfdh[hnd].afp = NULL;
  bfdh[hnd].short_feature = NULL;


  bfd_error.system = 0;
  return (bfd_error.bfd = BFDATA_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    binaryFeatureData_read_record

 - Purpose:     Retrieve a BFD point from a BFD file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        03/25/09

 - Arguments:
                - hnd            =    The file handle
                - recnum         =    The record number of the BFD record to be retrieved (or
                                      BFDATA_NEXT_RECORD)
                - bfd_record     =    The returned BFDATA_RECORD structure

 - Returns:
                - BFDATA_SUCCESS
                - BFDATA_INVALID_RECORD_NUMBER
                - BFDATA_READ_FSEEK_ERROR
                - BFDATA_END_OF_FILE

*********************************************************************************************/

BFDATA_DLL int32_t binaryFeatureData_read_record (int32_t hnd, int32_t recnum, BFDATA_RECORD *bfd_record)
{
  int32_t pos;


  if (recnum >= bfdh[hnd].header.number_of_records || recnum < BFDATA_NEXT_RECORD)
    {
      bfd_error.recnum = recnum;
      strcpy (bfd_error.file, bfdh[hnd].path);
      return (bfd_error.bfd = BFDATA_INVALID_RECORD_NUMBER);
    }

  if (recnum == BFDATA_NEXT_RECORD)
    {
      if (bfdh[hnd].recnum == bfdh[hnd].header.number_of_records)
        {
          bfd_error.recnum = bfdh[hnd].recnum;
          strcpy (bfd_error.file, bfdh[hnd].path);
          return (bfd_error.bfd = BFDATA_END_OF_FILE);
        }

      pos = bfdh[hnd].recnum * bfdh[hnd].record_size + bfdh[hnd].header_size;
    }
  else
    {
      pos = recnum * bfdh[hnd].record_size + bfdh[hnd].header_size;

      bfdh[hnd].recnum = recnum;
    }

  if (fseek (bfdh[hnd].fp, pos, SEEK_SET) < 0)
    {
      bfd_error.system = errno;
      bfd_error.recnum = recnum;
      strcpy (bfd_error.file, bfdh[hnd].path);
      return (bfd_error.bfd = BFDATA_RECORD_READ_FSEEK_ERROR);
    }


  if (!binaryFeatureData_get_record (hnd, bfd_record))
    {
      bfd_error.system = errno;
      bfd_error.recnum = recnum;
      strcpy (bfd_error.file, bfdh[hnd].path);
      return (bfd_error.bfd = BFDATA_RECORD_READ_ERROR);
    }



  bfd_record->record_number = bfdh[hnd].recnum;


  bfdh[hnd].write = 0;


  bfdh[hnd].last_rec = bfdh[hnd].recnum;
  bfdh[hnd].record = *bfd_record;


  bfdh[hnd].recnum++;


  bfd_error.system = 0;
  return (bfd_error.bfd = BFDATA_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    binaryFeatureData_read_all_short_features

 - Purpose:     Reads all BFD records in a file, allocates memory for them, and returns the 
                allocated array to the caller.  Memory will be cleaned up on bfd_file_close.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        03/25/09

 - Arguments:
                - hnd            =    The file handle
                - bfd_feature    =    The returned array of BFDATA_SHORT_FEATURE structures.

 - Returns:
                - BFDATA_SUCCESS
                - BFDATA_INVALID_RECORD_NUMBER
                - BFDATA_READ_FSEEK_ERROR
                - BFDATA_END_OF_FILE

*********************************************************************************************/

BFDATA_DLL int32_t binaryFeatureData_read_all_short_features (int32_t hnd, BFDATA_SHORT_FEATURE **bfd_feature)
{
  int32_t i;
  BFDATA_RECORD bfd_record;


  bfdh[hnd].short_feature = (BFDATA_SHORT_FEATURE *) realloc (bfdh[hnd].short_feature, bfdh[hnd].header.number_of_records * sizeof (BFDATA_SHORT_FEATURE));

  if (bfdh[hnd].short_feature == NULL)
    {
      perror ("Allocating short feature array in bfd_read_all_short_features");
      fflush (stderr);
      exit (-1);
    }

  for (i = 0 ; i < bfdh[hnd].header.number_of_records ; i++)
    {
      if (binaryFeatureData_read_record (hnd, i, &bfd_record) < 0) return (bfd_error.bfd);

      bfdh[hnd].short_feature[i].record_number = i;
      bfdh[hnd].short_feature[i].feature_type = bfd_record.feature_type;
      bfdh[hnd].short_feature[i].event_tv_sec = bfd_record.event_tv_sec;
      bfdh[hnd].short_feature[i].event_tv_nsec = bfd_record.event_tv_nsec;
      bfdh[hnd].short_feature[i].latitude = bfd_record.latitude;
      bfdh[hnd].short_feature[i].longitude = bfd_record.longitude;
      bfdh[hnd].short_feature[i].depth = bfd_record.depth;
      bfdh[hnd].short_feature[i].confidence_level = bfd_record.confidence_level;
      strcpy (bfdh[hnd].short_feature[i].description, bfd_record.description);
      strcpy (bfdh[hnd].short_feature[i].remarks, bfd_record.remarks);
      bfdh[hnd].short_feature[i].poly_count = bfd_record.poly_count;
      bfdh[hnd].short_feature[i].poly_type = bfd_record.poly_type;
      bfdh[hnd].short_feature[i].parent_record = bfd_record.parent_record;
      bfdh[hnd].short_feature[i].child_record = bfd_record.child_record;
    }


  *bfd_feature = bfdh[hnd].short_feature;


  bfdh[hnd].write = 0;


  bfd_error.system = 0;
  return (bfd_error.bfd = BFDATA_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    binaryFeatureData_read_polygon

 - Purpose:     Retrieve a BFD polygon structure for the given record.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        03/25/09

 - Arguments:
                - hnd            =    The file handle
                - recnum         =    The record number of the BFD polygon to be retrieved
                - poly           =    The BFDATA_POLYGON structure to hold the data.

 - Returns:
                - BFDATA_SUCCESS
                - BFDATA_NO_POLYGON_AVAILABLE
                - BFDATA_INVALID_RECORD_NUMBER
                - BFDATA_POLY_READ_FSEEK_ERROR
                - BFDATA_POLY_READ_ERROR

*********************************************************************************************/

BFDATA_DLL int32_t binaryFeatureData_read_polygon (int32_t hnd, int32_t recnum, BFDATA_POLYGON *poly)
{
  if (recnum != bfdh[hnd].last_rec)
    {
      if (binaryFeatureData_read_record (hnd, recnum, &bfdh[hnd].record) < 0) return (bfd_error.bfd);
    }


  if (!bfdh[hnd].record.poly_address)
    {
      bfd_error.recnum = recnum;
      strcpy (bfd_error.file, bfdh[hnd].a_path);
      return (bfd_error.bfd = BFDATA_NO_POLYGON_AVAILABLE);
    }


  if (fseeko64 (bfdh[hnd].afp, bfdh[hnd].record.poly_address, SEEK_SET) < 0)
    {
      bfd_error.system = errno;
      bfd_error.recnum = recnum;
      strcpy (bfd_error.file, bfdh[hnd].a_path);
      return (bfd_error.bfd = BFDATA_POLY_READ_FSEEK_ERROR);
    }


  if (!binaryFeatureData_get_polygon (hnd, bfdh[hnd].record.poly_count, poly))
    {
      bfd_error.system = errno;
      bfd_error.recnum = recnum;
      strcpy (bfd_error.file, bfdh[hnd].a_path);
      return (bfd_error.bfd = BFDATA_POLY_READ_ERROR);
    }


  bfdh[hnd].write = 0;


  bfd_error.system = 0;
  return (bfd_error.bfd = BFDATA_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    binaryFeatureData_read_image

 - Purpose:     Retrieve the associated image for the given record.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        03/25/09

 - Arguments:
                - hnd            =    The file handle
                - recnum         =    The record number of the BFD polygon to be retrieved
                - image          =    The image.

 - Returns:
                - BFDATA_SUCCESS
                - BFDATA_NO_IMAGE_AVAILABLE
                - BFDATA_INVALID_RECORD_NUMBER
                - BFDATA_IMAGE_READ_FSEEK_ERROR
                - BFDATA_IMAGE_READ_ERROR

*********************************************************************************************/

BFDATA_DLL int32_t binaryFeatureData_read_image (int32_t hnd, int32_t recnum, uint8_t *image)
{
  if (recnum != bfdh[hnd].last_rec)
    {
      if (binaryFeatureData_read_record (hnd, recnum, &bfdh[hnd].record) < 0) return (bfd_error.bfd);
    }


  if (!bfdh[hnd].record.image_size)
    {
      bfd_error.recnum = recnum;
      strcpy (bfd_error.file, bfdh[hnd].a_path);
      return (bfd_error.bfd = BFDATA_NO_IMAGE_AVAILABLE);
    }


  if (fseeko64 (bfdh[hnd].afp, bfdh[hnd].record.image_address, SEEK_SET) < 0)
    {
      bfd_error.system = errno;
      bfd_error.recnum = recnum;
      strcpy (bfd_error.file, bfdh[hnd].a_path);
      return (bfd_error.bfd = BFDATA_IMAGE_READ_FSEEK_ERROR);
    }


  if (!fread (image, bfdh[hnd].record.image_size, 1, bfdh[hnd].afp))
    {
      bfd_error.system = errno;
      bfd_error.recnum = recnum;
      strcpy (bfd_error.file, bfdh[hnd].a_path);
      return (bfd_error.bfd = BFDATA_IMAGE_READ_ERROR);
    }


  bfdh[hnd].write = 0;


  bfd_error.system = 0;
  return (bfd_error.bfd = BFDATA_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    binaryFeatureData_update_header

 - Purpose:     Update the modifiable fields of the header record.  See BFDATA_HEADER in bfd.h to
                determine which fields are modifiable after file creation.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        03/27/09

 - Arguments:
                - hnd            =    The file handle
                - bfd_header     =    The BFDATA_HEADER structure.

 - Returns:
                - void

*********************************************************************************************/

BFDATA_DLL void binaryFeatureData_update_header (int32_t hnd, BFDATA_HEADER bfd_header)
{
  strcpy (bfdh[hnd].header.modification_software, bfd_header.modification_software);
  strcpy (bfdh[hnd].header.security_classification, bfd_header.security_classification);
  strcpy (bfdh[hnd].header.distribution, bfd_header.distribution);
  strcpy (bfdh[hnd].header.declassification, bfd_header.declassification);
  strcpy (bfdh[hnd].header.class_just, bfd_header.class_just);
  strcpy (bfdh[hnd].header.downgrade, bfd_header.downgrade);
  strcpy (bfdh[hnd].header.comments, bfd_header.comments);


  /*  Force a header write when we close the file.  */

  bfdh[hnd].modified = 1;
  bfdh[hnd].write = 1;
}



/********************************************************************************************/
/*!

 - Function:    binaryFeatureData_strerror

 - Purpose:     Returns the error string related to the latest error.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        03/27/09

 - Arguments:   void

 - Returns:     Error message

*********************************************************************************************/

BFDATA_DLL char *binaryFeatureData_strerror ()
{
  static char message[1024];

  switch (bfd_error.bfd)
    {
    case BFDATA_SUCCESS:
      sprintf (message, "SUCCESS!\n");
      break;

    case BFDATA_POLYGON_TOO_LARGE_ERROR:
      sprintf (message ,"File : %s\nRecord : %d\nToo many points in polygon.\n",
               bfd_error.file, bfd_error.recnum);
      break;

    case BFDATA_POLY_WRITE_FSEEK_ERROR:
      sprintf (message ,"File : %s\nError during fseek prior to writing polygon data :\n%s\n",
               bfd_error.file, strerror (bfd_error.system));
      break;

    case BFDATA_POLY_WRITE_ERROR:
      sprintf (message ,"File : %s\nError writing polygon data :\n%s\n",
               bfd_error.file, strerror (bfd_error.system));
      break;

    case BFDATA_RECORD_WRITE_FSEEK_ERROR:
      sprintf (message ,"File : %s\nError during fseek prior to writing record data :\n%s\n",
               bfd_error.file, strerror (bfd_error.system));
      break;

    case BFDATA_RECORD_WRITE_ERROR:
      sprintf (message ,"File : %s\nError writing record data :\n%s\n",
               bfd_error.file, strerror (bfd_error.system));
      break;

    case BFDATA_HEADER_WRITE_FSEEK_ERROR:
      sprintf (message ,"File : %s\nError during fseek prior to writing header :\n%s\n",
               bfd_error.file, strerror (bfd_error.system));
      break;

    case BFDATA_HEADER_WRITE_ERROR:
      sprintf (message ,"File : %s\nError writing header :\n%s\n",
               bfd_error.file, strerror (bfd_error.system));
      break;

    case BFDATA_TOO_MANY_OPEN_FILES:
      sprintf (message ,"Too many BFD files are already open.\n");
      break;

    case BFDATA_CREATE_ERROR:
      sprintf (message ,"File : %s\nError creating BFD file :\n%s\n",
               bfd_error.file, strerror (bfd_error.system));
      break;

    case BFDATA_CREATE_POLY_ERROR:
      sprintf (message ,"File : %s\nError creating associated polygon file :\n%s\n",
               bfd_error.file, strerror (bfd_error.system));
      break;

    case BFDATA_OPEN_UPDATE_ERROR:
      sprintf (message ,"File : %s\nError opening BFD file for update :\n%s\n",
               bfd_error.file, strerror (bfd_error.system));
      break;

    case BFDATA_OPEN_POLY_UPDATE_ERROR:
      sprintf (message ,"File : %s\nError opening associated BFD polygon file for update :\n%s\n",
               bfd_error.file, strerror (bfd_error.system));
      break;

    case BFDATA_OPEN_READONLY_ERROR:
      sprintf (message ,"File : %s\nError opening BFD file read-only :\n%s\n",
               bfd_error.file, strerror (bfd_error.system));
      break;

    case BFDATA_OPEN_POLY_READONLY_ERROR:
      sprintf (message ,"File : %s\nError opening associated BFD polygon file read-only :\n%s\n",
               bfd_error.file, strerror (bfd_error.system));
      break;

    case BFDATA_NOT_BFD_FILE_ERROR:
      sprintf (message ,"File : %s\nThe file version string is corrupt or indicates that this is not a BFD file.\n",
               bfd_error.file);
      break;

    case BFDATA_NEWER_FILE_VERSION_ERROR:
      sprintf (message ,"File : %s\nThe file version is newer than the BFD library version.\n", bfd_error.file);
      break;

    case BFDATA_CLOSE_ERROR:
      sprintf (message ,"File : %s\nError closing BFD file :\n%s\n",
               bfd_error.file, strerror (bfd_error.system));
      break;

    case BFDATA_CLOSE_POLY_ERROR:
      sprintf (message ,"File : %s\nError closing associated BFD polygon file :\n%s\n",
               bfd_error.file, strerror (bfd_error.system));
      break;

    case BFDATA_INVALID_RECORD_NUMBER:
      sprintf (message ,"File : %s\nRecord : %d\nInvalid record number.\n",
               bfd_error.file, bfd_error.recnum);
      break;

    case BFDATA_NO_POLYGON_AVAILABLE:
      sprintf (message ,"File : %s\nRecord : %d\nNo polygon data is available for this record.\n",
               bfd_error.file, bfd_error.recnum);
      break;

    case BFDATA_POLY_READ_FSEEK_ERROR:
      sprintf (message ,"File : %s\nRecord : %d\nError during fseek prior reading polygon data :\n%s\n",
               bfd_error.file, bfd_error.recnum, strerror (bfd_error.system));
      break;

    case BFDATA_POLY_READ_ERROR:
      sprintf (message ,"File : %s\nRecord : %d\nError reading polygon data :\n%s\n",
               bfd_error.file, bfd_error.recnum, strerror (bfd_error.system));
      break;

    case BFDATA_RECORD_READ_FSEEK_ERROR:
      sprintf (message ,"File : %s\nRecord : %d\nError during fseek prior reading a record :\n%s\n",
               bfd_error.file, bfd_error.recnum, strerror (bfd_error.system));
      break;

    case BFDATA_RECORD_READ_ERROR:
      sprintf (message ,"File : %s\nRecord : %d\nError reading record :\n%s\n",
               bfd_error.file, bfd_error.recnum, strerror (bfd_error.system));
      break;

    case BFDATA_END_OF_FILE:
      sprintf (message ,"File : %s\nRecord : %d\nEnd of file reached when reading BFDATA_NEXT_RECORD.\n",
               bfd_error.file, bfd_error.recnum);
      break;

    case BFDATA_IMAGE_WRITE_FSEEK_ERROR:
      sprintf (message ,"File : %s\nError during fseek prior to writing image data :\n%s\n",
               bfd_error.file, strerror (bfd_error.system));
      break;

    case BFDATA_IMAGE_WRITE_ERROR:
      sprintf (message ,"File : %s\nError writing image data :\n%s\n",
               bfd_error.file, strerror (bfd_error.system));
      break;

    case BFDATA_NO_IMAGE_AVAILABLE:
      sprintf (message ,"File : %s\nRecord : %d\nNo image data is available for this record.\n",
               bfd_error.file, bfd_error.recnum);
      break;

    case BFDATA_IMAGE_READ_FSEEK_ERROR:
      sprintf (message ,"File : %s\nRecord : %d\nError during fseek prior reading image data :\n%s\n",
               bfd_error.file, bfd_error.recnum, strerror (bfd_error.system));
      break;

    case BFDATA_IMAGE_READ_ERROR:
      sprintf (message ,"File : %s\nRecord : %d\nError reading image data :\n%s\n",
               bfd_error.file, bfd_error.recnum, strerror (bfd_error.system));
      break;

    case BFDATA_IMAGE_FILE_OPEN_ERROR:
      sprintf (message ,"File : %s\nError opening image file :\n%s\n",
               bfd_error.file, strerror (bfd_error.system));
      break;

    case BFDATA_IMAGE_FILE_READ_ERROR:
      sprintf (message ,"File : %s\nError reading image file :\n%s\n",
               bfd_error.file, strerror (bfd_error.system));
      break;
    }

  return (message);
}



/********************************************************************************************/
/*!

 - Function:    binaryFeatureData_perror

 - Purpose:     Prints (to stderr) the latest error messages.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        03/27/09

 - Arguments:   void

 - Returns:     void

*********************************************************************************************/

BFDATA_DLL void binaryFeatureData_perror ()
{
  fprintf (stderr, "%s", binaryFeatureData_strerror ());
  fflush (stderr);
}



/********************************************************************************************/
/*!

 - Function:    binaryFeatureData_get_version

 - Purpose:     Returns the BFD library version string

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        03/27/09

 - Arguments:   void

 - Returns:     Version string

*********************************************************************************************/

BFDATA_DLL char *binaryFeatureData_get_version ()
{
  return (BFDATA_VERSION);
}



/********************************************************************************************/
/*!

 - Function:    binaryFeatureData_dump_record

 - Purpose:     Print the BFD record to stdout.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        03/27/09

 - Arguments:   bfd_record     =    The BFD record

 - Returns:     void

*********************************************************************************************/

BFDATA_DLL void binaryFeatureData_dump_record (BFDATA_RECORD bfd_record)
{
  int32_t        year, day, hour, minute, month, mday;
  float          second;


  printf ("******************************************************************\n");

  printf ("Record number : %d\n", bfd_record.record_number);
  printf ("Feature type : %d\n", bfd_record.feature_type);


  binaryFeatureData_cvtime (bfd_record.event_tv_sec, bfd_record.event_tv_nsec, &year, &day, &hour, &minute, &second);
  binaryFeatureData_jday2mday (year, day, &month, &mday);
  month++;

  printf ("Event date/time : %d-%02d-%02d (%d) %02d:%02d:%05.9f\n", year + 1900, month, mday, day, hour, minute, second);


  printf ("Latitude : %.9f\n", bfd_record.latitude);
  printf ("Longitude : %.9f\n", bfd_record.longitude);
  printf ("Length : %f\n", bfd_record.length);
  printf ("Width : %f\n", bfd_record.width);
  printf ("Height : %f\n", bfd_record.height);
  printf ("Depth (corrected/uncorrected) : %f/%f\n", bfd_record.depth, bfd_record.depth + bfd_record.datum);
  printf ("Datum offset : %f\n", bfd_record.datum);
  printf ("Horizontal orientation : %f\n", bfd_record.horizontal_orientation);
  printf ("Vertical orientation : %f\n", bfd_record.vertical_orientation);
  printf ("Description : %s\n", bfd_record.description);
  printf ("Remarks : %s\n", bfd_record.remarks);
  printf ("Sonar type : %d\n", bfd_record.sonar_type);
  printf ("Equipment type : %d\n", bfd_record.equip_type);
  printf ("Platform type : %d\n", bfd_record.platform_type);
  printf ("Heading : %f\n", bfd_record.heading);
  printf ("Confidence level : %d\n", bfd_record.confidence_level);
  printf ("Analyst activity : %s\n", bfd_record.analyst_activity);
  printf ("Polygon address : ");
  printf ("%"PRId64"\n", bfd_record.poly_address);
  printf ("Polygon point count : %d\n", bfd_record.poly_count);
  printf ("Polygon type : %d\n", bfd_record.poly_type);
  printf ("Image size : %d\n", bfd_record.image_size);
  printf ("Image name : %s\n", bfd_record.image_name);
  printf ("Image address : ");
  printf ("%"PRId64"\n", bfd_record.image_address);
  printf ("\n");
  printf ("Parent record number : %d\n", bfd_record.parent_record);
  printf ("Child record number : %d\n", bfd_record.child_record);

  fflush (stdout);
}



/*  Months start at zero, days at 1 (go figure).  */

static int32_t              months[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};



/********************************************************************************************/
/*!

 - Function:    binaryFeatureData_cvtime

 - Purpose:     Convert from POSIX time to year, day of year, hour, minute,
                second.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Arguments:
                - time_t tv_sec  =    POSIX seconds from epoch (Jan. 1, 1970)
                - long tv_nsec   =    POSIX nanoseconds of second
                - year           =    4 digit year - 1900
                - jday           =    day of year
                - hour           =    hour of day
                - minute         =    minute of hour
                - second         =    seconds of minute

 - Returns:
                - void

 - Caveats:     The localtime function will return the year as a 2 digit year (offset from 1900).

*********************************************************************************************/
 
BFDATA_DLL void binaryFeatureData_cvtime (time_t tv_sec, long tv_nsec, int32_t *year, int32_t *jday, int32_t *hour,
					  int32_t *minute, float *second)
{
  static int32_t       tz_set = 0;
  struct tm            time_struct, *time_ptr = &time_struct;

  if (!tz_set)
    {
#ifdef NVWIN3X
#if defined (__MINGW64__) || defined (__MINGW32__)
      putenv ("TZ=GMT");
      tzset ();
  #else
      _putenv ("TZ=GMT");
      _tzset ();
  #endif
#else
      putenv ("TZ=GMT");
      tzset ();
#endif
      tz_set = 1;
    }

  time_ptr = localtime (&tv_sec);

  *year = (int16_t) time_ptr->tm_year;
  *jday = (int16_t) time_ptr->tm_yday + 1;
  *hour = (int16_t) time_ptr->tm_hour;
  *minute = (int16_t) time_ptr->tm_min;
  *second = (float) time_ptr->tm_sec + (float) ((double) tv_nsec / 1000000000.);
}



/********************************************************************************************/
/*!

 - Function:    binaryFeatureData_inv_cvtime

 - Purpose:     Convert from year, day of year, hour, minute, second to POSIX time.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/20/11

 - Arguments:
                - year           =    4 digit year - 1900
                - jday           =    day of year
                - hour           =    hour of day
                - minute         =    minute of hour
                - second         =    seconds of minute
                - time_t tv_sec  =    POSIX seconds from epoch (Jan. 1, 1970)
                - long tv_nsec   =    POSIX nanoseconds of second

 - Returns:
                - void

 - Caveats:     The mktime function takes a 2 digit year (offset from 1900).

*********************************************************************************************/
 
BFDATA_DLL void binaryFeatureData_inv_cvtime (int32_t year, int32_t jday, int32_t hour, int32_t min, float sec,
					      time_t *tv_sec, long *tv_nsec)
{
  struct tm                    tm;
  static int32_t               tz_set = 0;

  void                    binaryFeatureData_jday2mday (int32_t, int32_t, int32_t *, int32_t *);


  tm.tm_year = year;

  binaryFeatureData_jday2mday (year, jday, &tm.tm_mon, &tm.tm_mday);

  tm.tm_hour = hour;
  tm.tm_min = min;
  tm.tm_sec = (int32_t) sec;
  tm.tm_isdst = 0;

  if (!tz_set)
    {
#ifdef NVWIN3X
#if defined (__MINGW64__) || defined (__MINGW32__)
      putenv ("TZ=GMT");
      tzset ();
  #else
      _putenv ("TZ=GMT");
      _tzset ();
  #endif
#else
      putenv ("TZ=GMT");
      tzset ();
#endif
      tz_set = 1;
    }

  *tv_sec = mktime (&tm);
  *tv_nsec = (long)(fmod ((double) sec, 1.0) * 1.0e9);
}



/********************************************************************************************/
/*!

 - Function:    binaryFeatureData_jday2mday

 - Purpose:     Convert from day of year to month and day.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Arguments:
                - year           =    4 digit year or offset from 1900
                - jday           =    day of year
                - mon            =    month
                - mday           =    day of month

 - Returns:
                - void

 - Caveats:     The returned month value will start at 0 for January.

*********************************************************************************************/
 
BFDATA_DLL void binaryFeatureData_jday2mday (int32_t year, int32_t jday, int32_t *mon, int32_t *mday)
{
  int32_t l_year;

  l_year = year;

  if (year < 1899) l_year += 1900;


  /*  If the year is evenly divisible by 4 but not by 100, or it's evenly divisible by 400, this is a leap year.  */

  if ((!(l_year % 4) && (l_year % 100)) || !(l_year % 400))
    {
      months[1] = 29;
    }
  else
    {
      months[1] = 28;
    }

  *mday = jday;
  for (*mon = 0 ; *mon < 12 ; (*mon)++)
    {
      if (*mday - months[*mon] <= 0) break;

      *mday -= months[*mon];
    }
}



/********************************************************************************************/
/*!

 - Function:    binaryFeatureData_mday2jday

 - Purpose:     Convert from month and day to day of year.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Arguments:
                - year           =    4 digit year or offset from 1900
                - mon            =    month
                - mday           =    day of month
                - jday           =    day of year

 - Returns:
                - void

 - Caveats:     The month value must start at 0 for January.

*********************************************************************************************/
 
BFDATA_DLL void binaryFeatureData_mday2jday (int32_t year, int32_t mon, int32_t mday, int32_t *jday)
{
  int32_t i, l_year;

  l_year = year;

  if (year < 1899) l_year += 1900;


  /*  If the year is evenly divisible by 4 but not by 100, or it's evenly divisible by 400, this is a leap year.  */

  if ((!(l_year % 4) && (l_year % 100)) || !(l_year % 400))
    {
      months[1] = 29;
    }
  else
    {
      months[1] = 28;
    }


  *jday = mday;
  for (i = 0 ; i < mon - 1 ; i++) *jday += months[i];
}
