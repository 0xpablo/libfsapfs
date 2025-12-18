/*
 * File extent functions
 *
 * Copyright (C) 2018-2026, Joachim Metz <joachim.metz@gmail.com>
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

#include "libfsapfs_debug.h"
#include "libfsapfs_definitions.h"
#include "libfsapfs_file_extent.h"
#include "libfsapfs_libcerror.h"
#include "libfsapfs_libcnotify.h"
#include "libfsapfs_libfdatetime.h"
#include "libfsapfs_libuna.h"

#include "fsapfs_file_system.h"

/* Creates a file_extent
 * Make sure the value file_extent is referencing, is set to NULL
 * Returns 1 if successful or -1 on error
 */
int libfsapfs_file_extent_initialize(
     libfsapfs_file_extent_t **file_extent,
     libcerror_error_t **error )
{
	static char *function = "libfsapfs_file_extent_initialize";

	if( file_extent == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file extent.",
		 function );

		return( -1 );
	}
	if( *file_extent != NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_ALREADY_SET,
		 "%s: invalid file extent value already set.",
		 function );

		return( -1 );
	}
	*file_extent = memory_allocate_structure(
	                libfsapfs_file_extent_t );

	if( *file_extent == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_INSUFFICIENT,
		 "%s: unable to create file extent.",
		 function );

		goto on_error;
	}
	if( memory_set(
	     *file_extent,
	     0,
	     sizeof( libfsapfs_file_extent_t ) ) == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_SET_FAILED,
		 "%s: unable to clear file extent.",
		 function );

		goto on_error;
	}
	return( 1 );

on_error:
	if( *file_extent != NULL )
	{
		memory_free(
		 *file_extent );

		*file_extent = NULL;
	}
	return( -1 );
}

/* Frees a file extent
 * Returns 1 if successful or -1 on error
 */
int libfsapfs_file_extent_free(
     libfsapfs_file_extent_t **file_extent,
     libcerror_error_t **error )
{
	static char *function = "libfsapfs_file_extent_free";

	if( file_extent == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file extent.",
		 function );

		return( -1 );
	}
	if( *file_extent != NULL )
	{
		memory_free(
		 *file_extent );

		*file_extent = NULL;
	}
	return( 1 );
}

/* Reads the file extent key data
 * Returns 1 if successful or -1 on error
 */
int libfsapfs_file_extent_read_key_data(
     libfsapfs_file_extent_t *file_extent,
     uint8_t file_system_data_type,
     const uint8_t *data,
     size_t data_size,
     libcerror_error_t **error )
{
	static char *function = "libfsapfs_file_extent_read_key_data";

#if defined( HAVE_DEBUG_OUTPUT )
	uint64_t value_64bit  = 0;
#endif
	uint64_t logical_address = 0;
	uint64_t file_system_identifier = 0;

	if( file_extent == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file extent.",
		 function );

		return( -1 );
	}
	if( data == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
		 "%s: invalid data.",
		 function );

		return( -1 );
	}
	if( ( data_size < sizeof( fsapfs_file_system_btree_key_file_extent_t ) )
	 || ( data_size > (size_t) SSIZE_MAX ) )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid data size value out of bounds.",
		 function );

		return( -1 );
	}
#if defined( HAVE_DEBUG_OUTPUT )
	if( libcnotify_verbose != 0 )
	{
		libcnotify_printf(
		 "%s: file extent key data:\n",
		 function );
		libcnotify_print_data(
		 data,
		 data_size,
		 LIBCNOTIFY_PRINT_DATA_FLAG_GROUP_DATA );
	}
#endif
	file_extent->file_system_data_type = file_system_data_type;

	byte_stream_copy_to_uint64_little_endian(
	 ( (fsapfs_file_system_btree_key_file_extent_t *) data )->file_system_identifier,
	 file_system_identifier );

	file_extent->identifier = file_system_identifier & 0x0fffffffffffffffUL;

	byte_stream_copy_to_uint64_little_endian(
	 ( (fsapfs_file_system_btree_key_file_extent_t *) data )->logical_address,
	 logical_address );

	file_extent->logical_offset        = logical_address & 0x00ffffffffffffffUL;
	file_extent->logical_address_flags = (uint8_t) ( logical_address >> 56 );

#if defined( HAVE_DEBUG_OUTPUT )
	if( libcnotify_verbose != 0 )
	{
		byte_stream_copy_to_uint64_little_endian(
		 ( (fsapfs_file_system_btree_key_file_extent_t *) data )->file_system_identifier,
		 value_64bit );
		libcnotify_printf(
		 "%s: identifier\t\t\t\t: 0x%08" PRIx64 "\n",
		 function,
		 value_64bit );

		libcnotify_printf(
		 "%s: logical address\t\t\t: 0x%08" PRIx64 "\n",
		 function,
		 logical_address );

		if( file_extent->logical_address_flags != 0 )
		{
			libcnotify_printf(
			 "%s: logical offset\t\t\t: 0x%08" PRIx64 " (flags: 0x%02" PRIx8 ")\n",
			 function,
			 file_extent->logical_offset,
			 file_extent->logical_address_flags );
		}

		libcnotify_printf(
		 "\n" );
	}
#endif /* defined( HAVE_DEBUG_OUTPUT ) */

	return( 1 );
}

/* Reads the file extent value data
 * Returns 1 if successful or -1 on error
 */
int libfsapfs_file_extent_read_value_data(
     libfsapfs_file_extent_t *file_extent,
     uint8_t file_system_data_type,
     const uint8_t *data,
     size_t data_size,
     libcerror_error_t **error )
{
	static char *function = "libfsapfs_file_extent_read_value_data";

#if defined( HAVE_DEBUG_OUTPUT )
	uint64_t value_64bit  = 0;
#endif

	if( file_extent == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file extent.",
		 function );

		return( -1 );
	}
	if( data == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
		 "%s: invalid data.",
		 function );

		return( -1 );
	}
	if( ( data_size > (size_t) SSIZE_MAX ) )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid data size value out of bounds.",
		 function );

		return( -1 );
	}
#if defined( HAVE_DEBUG_OUTPUT )
	if( libcnotify_verbose != 0 )
	{
		libcnotify_printf(
		 "%s: file extent value data:\n",
		 function );
		libcnotify_print_data(
		 data,
		 data_size,
		 LIBCNOTIFY_PRINT_DATA_FLAG_GROUP_DATA );
	}
#endif
	file_extent->file_system_data_type = file_system_data_type;

	if( file_system_data_type == LIBFSAPFS_FILE_SYSTEM_DATA_TYPE_FILE_EXTENT2 )
	{
		if( ( data_size < 35 )
		 || ( data_size > (size_t) SSIZE_MAX ) )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
			 "%s: invalid data size value out of bounds.",
			 function );

			return( -1 );
		}
		file_extent->data_size           = 0;
		file_extent->physical_block_number = 0;
		file_extent->encryption_identifier = 0;
		file_extent->has_sha256_digest     = 0;

		/* Observed format:
		 *   uint8_t type    = 0x04
		 *   uint8_t unknown = 0x00
		 *   uint8_t length  = 0x20 (32)
		 *   uint8_t digest[32] (SHA-256)
		 */
		if( ( data[ 0 ] == 0x04 )
		 && ( data[ 1 ] == 0x00 )
		 && ( data[ 2 ] == 0x20 ) )
		{
			if( memory_copy(
			     file_extent->sha256_digest,
			     &( data[ 3 ] ),
			     32 ) == NULL )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_MEMORY,
				 LIBCERROR_MEMORY_ERROR_COPY_FAILED,
				 "%s: unable to copy SHA-256 digest.",
				 function );

				return( -1 );
			}
			file_extent->has_sha256_digest = 1;
		}
		return( 1 );
	}
	if( ( data_size < sizeof( fsapfs_file_system_btree_value_file_extent_t ) )
	 || ( data_size > (size_t) SSIZE_MAX ) )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid data size value out of bounds.",
		 function );

		return( -1 );
	}
	byte_stream_copy_to_uint64_little_endian(
	 ( (fsapfs_file_system_btree_value_file_extent_t *) data )->data_size_and_flags,
	 file_extent->data_size );

	file_extent->data_size &= 0x00ffffffffffffffUL;

	byte_stream_copy_to_uint64_little_endian(
	 ( (fsapfs_file_system_btree_value_file_extent_t *) data )->physical_block_number,
	 file_extent->physical_block_number );

	byte_stream_copy_to_uint64_little_endian(
	 ( (fsapfs_file_system_btree_value_file_extent_t *) data )->encryption_identifier,
	 file_extent->encryption_identifier );

#if defined( HAVE_DEBUG_OUTPUT )
	if( libcnotify_verbose != 0 )
	{
		byte_stream_copy_to_uint64_little_endian(
		 ( (fsapfs_file_system_btree_value_file_extent_t *) data )->data_size_and_flags,
		 value_64bit );
		libcnotify_printf(
		 "%s: data size and flags\t\t: 0x%08" PRIx64 " (data size: %" PRIu64 ", flags: 0x%02" PRIx64 ")\n",
		 function,
		 value_64bit,
		 file_extent->data_size,
		 value_64bit >> 56 );

		libcnotify_printf(
		 "%s: physical block number\t\t: %" PRIu64 "\n",
		 function,
		 file_extent->physical_block_number );

		libcnotify_printf(
		 "%s: encryption identifier\t\t: %" PRIu64 "\n",
		 function,
		 file_extent->encryption_identifier );

		libcnotify_printf(
		 "\n" );
	}
#endif /* defined( HAVE_DEBUG_OUTPUT ) */

	return( 1 );
}
