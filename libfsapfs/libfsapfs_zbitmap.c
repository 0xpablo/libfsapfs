/*
 * ZBITMAP (ZBM) decompression functions
 *
 * Copyright (C) 2018-2025, Joachim Metz <joachim.metz@gmail.com>
 *
 * Refer to AUTHORS for acknowledgements.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <byte_stream.h>
#include <memory.h>
#include <types.h>

#include "libfsapfs_libcerror.h"
#include "libfsapfs_zbitmap.h"

#define LIBFSAPFS_ZBITMAP_MAGIC				0x094d425aUL
#define LIBFSAPFS_ZBITMAP_MAX_DECMP_CHUNK_SIZE		0x8000U

#define LIBFSAPFS_ZBITMAP_BITMAP_BASE			3
#define LIBFSAPFS_ZBITMAP_BITMAP_COUNT			12
#define LIBFSAPFS_ZBITMAP_BITMAP_BYTECNT		17
#define LIBFSAPFS_ZBITMAP_MAX_PERIOD_BYTECNT		2

typedef struct libfsapfs_zbitmap_bitmap libfsapfs_zbitmap_bitmap_t;

struct libfsapfs_zbitmap_bitmap
{
	uint8_t bitmap;
	uint8_t period_bytecnt;
};

typedef struct libfsapfs_zbitmap_nibble_reader libfsapfs_zbitmap_nibble_reader_t;

struct libfsapfs_zbitmap_nibble_reader
{
	const uint8_t *address;
	uint8_t nibble;
};

typedef struct libfsapfs_zbitmap_bit_reader libfsapfs_zbitmap_bit_reader_t;

struct libfsapfs_zbitmap_bit_reader
{
	const uint8_t *address;
	uint8_t bit_offset;
};

static int libfsapfs_zbitmap_read_uint24_little_endian(
            const uint8_t *data,
            size_t data_size,
            uint32_t *value,
            libcerror_error_t **error )
{
	static char *function = "libfsapfs_zbitmap_read_uint24_little_endian";

	if( data == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid data.",
		 function );

		return( -1 );
	}
	if( value == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid value.",
		 function );

		return( -1 );
	}
	if( data_size < 3 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: missing uint24 data.",
		 function );

		return( -1 );
	}
	*value = (uint32_t) data[ 0 ]
	       | ( (uint32_t) data[ 1 ] << 8 )
	       | ( (uint32_t) data[ 2 ] << 16 );

	return( 1 );
}

static int libfsapfs_zbitmap_read_bit(
            libfsapfs_zbitmap_bit_reader_t *reader,
            const uint8_t *limit,
            uint8_t *value,
            libcerror_error_t **error )
{
	static char *function = "libfsapfs_zbitmap_read_bit";

	if( reader == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid reader.",
		 function );

		return( -1 );
	}
	if( value == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid value.",
		 function );

		return( -1 );
	}
	if( ( reader->address == NULL )
	 || ( limit == NULL )
	 || ( reader->address >= limit ) )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: bit reader out of bounds.",
		 function );

		return( -1 );
	}
	*value = (uint8_t) ( ( *( reader->address ) >> reader->bit_offset ) & 0x01 );

	reader->bit_offset++;

	if( reader->bit_offset >= 8 )
	{
		reader->bit_offset = 0;
		reader->address++;
	}
	return( 1 );
}

static int libfsapfs_zbitmap_read_bitmap(
            libfsapfs_zbitmap_bit_reader_t *reader,
            const uint8_t *limit,
            libfsapfs_zbitmap_bitmap_t *bitmap,
            libcerror_error_t **error )
{
	static char *function = "libfsapfs_zbitmap_read_bitmap";
	uint8_t bit_value     = 0;
	int bit_index         = 0;

	if( bitmap == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid bitmap.",
		 function );

		return( -1 );
	}
	bitmap->bitmap         = 0;
	bitmap->period_bytecnt = 0;

	for( bit_index = 0;
	     bit_index < 8;
	     bit_index++ )
	{
		if( libfsapfs_zbitmap_read_bit(
		     reader,
		     limit,
		     &bit_value,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to read bitmap bit.",
			 function );

			return( -1 );
		}
		bitmap->bitmap |= (uint8_t) ( bit_value << bit_index );
	}
	for( bit_index = 0;
	     bit_index < 2;
	     bit_index++ )
	{
		if( libfsapfs_zbitmap_read_bit(
		     reader,
		     limit,
		     &bit_value,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to read bitmap period size bit.",
			 function );

			return( -1 );
		}
		bitmap->period_bytecnt |= (uint8_t) ( bit_value << bit_index );
	}
	return( 1 );
}

static int libfsapfs_zbitmap_read_nibble(
            libfsapfs_zbitmap_nibble_reader_t *reader,
            const uint8_t *limit,
            uint8_t *nibble_value,
            libcerror_error_t **error )
{
	static char *function = "libfsapfs_zbitmap_read_nibble";

	if( reader == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid reader.",
		 function );

		return( -1 );
	}
	if( nibble_value == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid nibble value.",
		 function );

		return( -1 );
	}
	if( ( reader->address == NULL )
	 || ( limit == NULL )
	 || ( reader->address >= limit ) )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: nibble reader out of bounds.",
		 function );

		return( -1 );
	}
	if( reader->nibble == 0 )
	{
		*nibble_value  = (uint8_t) ( *( reader->address ) & 0x0f );
		reader->nibble = 1;
	}
	else
	{
		*nibble_value  = (uint8_t) ( ( *( reader->address ) >> 4 ) & 0x0f );
		reader->nibble = 0;
		reader->address++;
	}
	return( 1 );
}

static void libfsapfs_zbitmap_rewind_nibble(
             libfsapfs_zbitmap_nibble_reader_t *reader )
{
	if( reader == NULL )
	{
		return;
	}
	if( reader->nibble == 0 )
	{
		reader->nibble  = 1;
		reader->address -= 1;
	}
	else
	{
		reader->nibble = 0;
	}
}

/* Decompresses ZBITMAP data.
 * Returns 1 if successful or -1 on error
 */
int libfsapfs_zbitmap_decompress(
     const uint8_t *compressed_data,
     size_t compressed_data_size,
     uint8_t *uncompressed_data,
     size_t *uncompressed_data_size,
     libcerror_error_t **error )
{
	libfsapfs_zbitmap_bitmap_t bitmaps[ LIBFSAPFS_ZBITMAP_BITMAP_COUNT ];

	libfsapfs_zbitmap_nibble_reader_t meta3_reader;
	libfsapfs_zbitmap_bit_reader_t bit_reader;

	const uint8_t *chunk_data          = NULL;
	const uint8_t *chunk_end           = NULL;
	const uint8_t *current_offset      = NULL;
	const uint8_t *data_pointer        = NULL;
	const uint8_t *meta1_pointer       = NULL;
	const uint8_t *meta2_pointer       = NULL;

	static char *function              = "libfsapfs_zbitmap_decompress";

	size_t output_capacity             = 0;
	size_t output_offset               = 0;

	uint32_t chunk_length              = 0;
	uint32_t chunk_decompressed_length = 0;
	uint32_t magic                     = 0;
	uint32_t meta_offset_1             = 0;
	uint32_t meta_offset_2             = 0;
	uint32_t meta_offset_3             = 0;
	uint32_t period                    = 8;

	uint16_t repeat_count              = 0;

	uint8_t bitmap_number              = 0;
	uint8_t nibble_value               = 0;

	int bitmap_index                   = 0;
	int bit_index                      = 0;
	int repeat_index                   = 0;

	if( compressed_data == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid compressed data.",
		 function );

		return( -1 );
	}
	if( uncompressed_data == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid uncompressed data.",
		 function );

		return( -1 );
	}
	if( uncompressed_data_size == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid uncompressed data size.",
		 function );

		return( -1 );
	}
	if( compressed_data_size < 4 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid compressed data size value out of bounds.",
		 function );

		return( -1 );
	}
	byte_stream_copy_to_uint32_little_endian(
	 compressed_data,
	 magic );

	if( magic != LIBFSAPFS_ZBITMAP_MAGIC )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_UNSUPPORTED_VALUE,
		 "%s: unsupported ZBITMAP signature.",
		 function );

		return( -1 );
	}
	output_capacity = *uncompressed_data_size;

	current_offset = compressed_data + 4;

	while( current_offset < ( compressed_data + compressed_data_size ) )
	{
		if( (size_t) ( ( compressed_data + compressed_data_size ) - current_offset ) < 6 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
			 "%s: missing chunk header.",
			 function );

			return( -1 );
		}
		if( libfsapfs_zbitmap_read_uint24_little_endian(
		     current_offset,
		     3,
		     &chunk_length,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to read chunk length.",
			 function );

			return( -1 );
		}
		if( libfsapfs_zbitmap_read_uint24_little_endian(
		     &( current_offset[ 3 ] ),
		     3,
		     &chunk_decompressed_length,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to read chunk decompressed length.",
			 function );

			return( -1 );
		}
		if( chunk_length < 6 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
			 "%s: invalid chunk length value out of bounds.",
			 function );

			return( -1 );
		}
		if( chunk_decompressed_length > LIBFSAPFS_ZBITMAP_MAX_DECMP_CHUNK_SIZE )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
			 "%s: invalid chunk decompressed length value out of bounds.",
			 function );

			return( -1 );
		}
		if( chunk_length > (uint32_t) ( ( compressed_data + compressed_data_size ) - current_offset ) )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
			 "%s: chunk length exceeds available data.",
			 function );

			return( -1 );
		}
		chunk_data = current_offset;
		chunk_end  = current_offset + chunk_length;

		if( chunk_decompressed_length == 0 )
		{
			break;
		}
		if( chunk_decompressed_length > ( output_capacity - output_offset ) )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
			 "%s: decompressed data exceeds output buffer.",
			 function );

			return( -1 );
		}
		/* Uncompressed chunks contain a chunk header and raw data.
		 */
		if( chunk_length == ( chunk_decompressed_length + 6 ) )
		{
			if( memory_copy(
			     &( uncompressed_data[ output_offset ] ),
			     &( chunk_data[ 6 ] ),
			     chunk_decompressed_length ) == NULL )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_MEMORY,
				 LIBCERROR_MEMORY_ERROR_COPY_FAILED,
				 "%s: unable to copy uncompressed chunk data.",
				 function );

				return( -1 );
			}
			output_offset += chunk_decompressed_length;

			current_offset += chunk_length;

			continue;
		}
		/* Compressed chunk header consists of:
		 * - chunk header (6 bytes)
		 * - 3 metadata offsets (9 bytes)
		 */
		if( chunk_length < 15 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
			 "%s: invalid compressed chunk length value out of bounds.",
			 function );

			return( -1 );
		}
		if( libfsapfs_zbitmap_read_uint24_little_endian(
		     &( chunk_data[ 6 ] ),
		     3,
		     &meta_offset_1,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to read meta offset 1.",
			 function );

			return( -1 );
		}
		if( libfsapfs_zbitmap_read_uint24_little_endian(
		     &( chunk_data[ 9 ] ),
		     3,
		     &meta_offset_2,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to read meta offset 2.",
			 function );

			return( -1 );
		}
		if( libfsapfs_zbitmap_read_uint24_little_endian(
		     &( chunk_data[ 12 ] ),
		     3,
		     &meta_offset_3,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to read meta offset 3.",
			 function );

			return( -1 );
		}
		if( ( meta_offset_1 >= chunk_length )
		 || ( meta_offset_2 >= chunk_length )
		 || ( meta_offset_3 >= chunk_length ) )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
			 "%s: invalid metadata offset value out of bounds.",
			 function );

			return( -1 );
		}
		data_pointer  = chunk_data + 15;
		meta1_pointer = chunk_data + meta_offset_1;
		meta2_pointer = chunk_data + meta_offset_2;

		meta3_reader.address = chunk_data + meta_offset_3;
		meta3_reader.nibble  = 0;

		/* Read the chunk bitmaps stored at the end of the chunk.
		 */
		if( chunk_length < LIBFSAPFS_ZBITMAP_BITMAP_BYTECNT )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
			 "%s: chunk too small to contain bitmap table.",
			 function );

			return( -1 );
		}
		bit_reader.address    = chunk_end - LIBFSAPFS_ZBITMAP_BITMAP_BYTECNT;
		bit_reader.bit_offset = 0;

		for( bitmap_index = 0;
		     bitmap_index < LIBFSAPFS_ZBITMAP_BITMAP_COUNT;
		     bitmap_index++ )
		{
			if( libfsapfs_zbitmap_read_bitmap(
			     &bit_reader,
			     chunk_end,
			     &bitmaps[ bitmap_index ],
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
				 "%s: unable to read bitmap.",
				 function );

				return( -1 );
			}
			if( bitmaps[ bitmap_index ].period_bytecnt > LIBFSAPFS_ZBITMAP_MAX_PERIOD_BYTECNT )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
				 "%s: invalid bitmap period size value out of bounds.",
				 function );

				return( -1 );
			}
		}
		period = 8;

		/* Decompress the chunk.
		 */
		{
			uint32_t chunk_written = 0;

			while( chunk_written < chunk_decompressed_length )
			{
				if( libfsapfs_zbitmap_read_nibble(
				     &meta3_reader,
				     chunk_end,
				     &bitmap_number,
				     error ) != 1 )
				{
					libcerror_error_set(
					 error,
					 LIBCERROR_ERROR_DOMAIN_RUNTIME,
					 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
					 "%s: unable to read bitmap number.",
					 function );

					return( -1 );
				}
				/* Determine if there is a repetition count.
				 */
				repeat_count = 1;

				if( ( chunk_decompressed_length - chunk_written ) > 8 )
				{
					if( libfsapfs_zbitmap_read_nibble(
					     &meta3_reader,
					     chunk_end,
					     &nibble_value,
					     error ) != 1 )
					{
						libcerror_error_set(
						 error,
						 LIBCERROR_ERROR_DOMAIN_RUNTIME,
						 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
						 "%s: unable to read repetition nibble.",
						 function );

						return( -1 );
					}
					if( nibble_value != 0x0f )
					{
						libfsapfs_zbitmap_rewind_nibble(
						 &meta3_reader );
					}
					else
					{
						uint16_t total = 4;

						while( nibble_value == 0x0f )
						{
							if( libfsapfs_zbitmap_read_nibble(
							     &meta3_reader,
							     chunk_end,
							     &nibble_value,
							     error ) != 1 )
							{
								libcerror_error_set(
								 error,
								 LIBCERROR_ERROR_DOMAIN_RUNTIME,
								 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
								 "%s: unable to read repetition nibble value.",
								 function );

								return( -1 );
							}
							total += nibble_value;

							if( total < nibble_value )
							{
								libcerror_error_set(
								 error,
								 LIBCERROR_ERROR_DOMAIN_RUNTIME,
								 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
								 "%s: invalid repetition count value out of bounds.",
								 function );

								return( -1 );
							}
						}
						repeat_count = total;
					}
				}
				for( repeat_index = 0;
				     repeat_index < repeat_count;
				     repeat_index++ )
				{
					libfsapfs_zbitmap_bitmap_t bitmap;
					uint8_t bitmap_value       = 0;
					uint8_t period_bytecnt     = 0;

					if( bitmap_number == 0x0f )
					{
						libcerror_error_set(
						 error,
						 LIBCERROR_ERROR_DOMAIN_RUNTIME,
						 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
						 "%s: invalid bitmap number value out of bounds.",
						 function );

						return( -1 );
					}
					/* Bitmap numbers 0-2 read their bitmap from meta2, and use the
					 * bitmap number as the period byte count.
					 */
					if( bitmap_number <= LIBFSAPFS_ZBITMAP_MAX_PERIOD_BYTECNT )
					{
						if( meta2_pointer >= chunk_end )
						{
							libcerror_error_set(
							 error,
							 LIBCERROR_ERROR_DOMAIN_RUNTIME,
							 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
							 "%s: meta2 pointer out of bounds.",
							 function );

							return( -1 );
						}
						bitmap_value   = *meta2_pointer;
						meta2_pointer += 1;

						period_bytecnt = bitmap_number;
					}
					else
					{
						if( bitmap_number < LIBFSAPFS_ZBITMAP_BITMAP_BASE )
						{
							libcerror_error_set(
							 error,
							 LIBCERROR_ERROR_DOMAIN_RUNTIME,
							 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
							 "%s: invalid bitmap number value out of bounds.",
							 function );

							return( -1 );
						}
						bitmap_index = (int) bitmap_number - LIBFSAPFS_ZBITMAP_BITMAP_BASE;

						if( ( bitmap_index < 0 )
						 || ( bitmap_index >= LIBFSAPFS_ZBITMAP_BITMAP_COUNT ) )
						{
							libcerror_error_set(
							 error,
							 LIBCERROR_ERROR_DOMAIN_RUNTIME,
							 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
							 "%s: invalid bitmap index value out of bounds.",
							 function );

							return( -1 );
						}
						bitmap = bitmaps[ bitmap_index ];

						bitmap_value   = bitmap.bitmap;
						period_bytecnt = bitmap.period_bytecnt;
					}
					/* Apply the bitmap.
					 */
					if( period_bytecnt != 0 )
					{
						period = 0;

						for( bit_index = 0;
						     bit_index < period_bytecnt;
						     bit_index++ )
						{
							if( meta1_pointer >= chunk_end )
							{
								libcerror_error_set(
								 error,
								 LIBCERROR_ERROR_DOMAIN_RUNTIME,
								 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
								 "%s: meta1 pointer out of bounds.",
								 function );

								return( -1 );
							}
							period |= (uint32_t) ( *meta1_pointer ) << ( bit_index * 8 );
							meta1_pointer++;
						}
						if( period == 0 )
						{
							libcerror_error_set(
							 error,
							 LIBCERROR_ERROR_DOMAIN_RUNTIME,
							 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
							 "%s: invalid repetition period value out of bounds.",
							 function );

							return( -1 );
						}
					}
					for( bit_index = 0;
					     bit_index < 8;
					     bit_index++ )
					{
						if( chunk_written == chunk_decompressed_length )
						{
							break;
						}
						if( bitmap_value & ( 1U << bit_index ) )
						{
							if( data_pointer >= chunk_end )
							{
								libcerror_error_set(
								 error,
								 LIBCERROR_ERROR_DOMAIN_RUNTIME,
								 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
								 "%s: data pointer out of bounds.",
								 function );

								return( -1 );
							}
							uncompressed_data[ output_offset ] = *data_pointer;
							data_pointer++;
						}
						else
						{
							if( output_offset < period )
							{
								libcerror_error_set(
								 error,
								 LIBCERROR_ERROR_DOMAIN_RUNTIME,
								 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
								 "%s: invalid repetition period exceeds output size.",
								 function );

								return( -1 );
							}
							uncompressed_data[ output_offset ] = uncompressed_data[ output_offset - period ];
						}
						output_offset++;
						chunk_written++;
					}
				}
			}
		}
		current_offset += chunk_length;
	}
	*uncompressed_data_size = output_offset;

	return( 1 );
}

