/*
 * (Extended) attribute functions
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
#include <types.h>

#include "libfsapfs_attribute_values.h"
#include "libfsapfs_attributes.h"
#include "libfsapfs_data_stream.h"
#include "libfsapfs_definitions.h"
#include "libfsapfs_encryption_context.h"
#include "libfsapfs_file_extent.h"
#include "libfsapfs_file_system_btree.h"
#include "libfsapfs_io_handle.h"
#include "libfsapfs_libbfio.h"
#include "libfsapfs_libcdata.h"
#include "libfsapfs_libcerror.h"
#include "libfsapfs_libfdata.h"

/* Retrieves the attribute value data file extents
 * Returns 1 if successful or -1 on error
 */
int libfsapfs_attributes_get_file_extents(
     libfsapfs_attribute_values_t *attribute_values,
     libbfio_handle_t *file_io_handle,
     libfsapfs_file_system_btree_t *file_system_btree,
     uint64_t transaction_identifier,
     libcerror_error_t **error )
{
	static char *function = "libfsapfs_attributes_get_file_extents";
	int result            = 0;

	if( attribute_values == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid attribute values.",
		 function );

		return( -1 );
	}
	if( attribute_values->value_data_file_extents != NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_ALREADY_SET,
		 "%s: invalid attribute values - value data file extents value already set.",
		 function );

		return( -1 );
	}
	if( libcdata_array_initialize(
	     &( attribute_values->value_data_file_extents ),
	     0,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to create value data file extents array.",
		 function );

		goto on_error;
	}
	result = libfsapfs_file_system_btree_get_file_extents(
		  file_system_btree,
		  file_io_handle,
		  attribute_values->value_data_stream_identifier,
		  transaction_identifier,
		  attribute_values->value_data_file_extents,
		  error );

	if( result == -1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve value data file extents from file system B-tree.",
		 function );

		goto on_error;
	}
	return( 1 );

on_error:
	if( attribute_values->value_data_file_extents != NULL )
	{
		libcdata_array_free(
		 &( attribute_values->value_data_file_extents ),
		 (int (*)(intptr_t **, libcerror_error_t **)) &libfsapfs_file_extent_free,
		 NULL );
	}
	return( -1 );
}

/* Retrieves the attribute value data stream
 * Returns 1 if successful or -1 on error
 */
int libfsapfs_attributes_get_data_stream(
     libfsapfs_attribute_values_t *attribute_values,
     libfsapfs_io_handle_t *io_handle,
     libbfio_handle_t *file_io_handle,
     libfsapfs_encryption_context_t *encryption_context,
     libfsapfs_file_system_btree_t *file_system_btree,
     uint64_t transaction_identifier,
     libfdata_stream_t **data_stream,
     libcerror_error_t **error )
{
	libfsapfs_file_extent_t *file_extent = NULL;
	libfsapfs_sealed_extent_tree_t *sealed_extent_tree = NULL;
	uint64_t data_stream_size            = 0;
	int extent_index                     = 0;
	int number_of_extents                = 0;
	int requires_sealed_extent_tree      = 0;
	int result                           = 0;

	static char *function = "libfsapfs_attributes_get_data_stream";

	if( attribute_values == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid attribute values.",
		 function );

		return( -1 );
	}
	if( ( attribute_values->flags & 0x0001 ) != 0 )
	{
		if( attribute_values->value_data_file_extents == NULL )
		{
			if( libfsapfs_attributes_get_file_extents(
			     attribute_values,
			     file_io_handle,
			     file_system_btree,
			     transaction_identifier,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
				 "%s: unable to retrieve attribute value data file extents.",
				 function );

				return( -1 );
			}
		}
		data_stream_size = attribute_values->value_data_size;

		if( libcdata_array_get_number_of_entries(
		     attribute_values->value_data_file_extents,
		     &number_of_extents,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve number of attribute value data file extents.",
			 function );

			return( -1 );
		}
		for( extent_index = 0;
		     extent_index < number_of_extents;
		     extent_index++ )
		{
			if( libcdata_array_get_entry_by_index(
			     attribute_values->value_data_file_extents,
			     extent_index,
			     (intptr_t **) &file_extent,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
				 "%s: unable to retrieve attribute value data file extent: %d.",
				 function,
				 extent_index );

				return( -1 );
			}
			if( file_extent == NULL )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
				 "%s: missing attribute value data file extent: %d.",
				 function,
				 extent_index );

				return( -1 );
			}
			if( file_extent->file_system_data_type == LIBFSAPFS_FILE_SYSTEM_DATA_TYPE_FILE_EXTENT2 )
			{
				requires_sealed_extent_tree = 1;
			}
			if( extent_index < ( number_of_extents - 1 ) )
			{
				libfsapfs_file_extent_t *next_file_extent = NULL;

				if( libcdata_array_get_entry_by_index(
				     attribute_values->value_data_file_extents,
				     extent_index + 1,
				     (intptr_t **) &next_file_extent,
				     error ) != 1 )
				{
					libcerror_error_set(
					 error,
					 LIBCERROR_ERROR_DOMAIN_RUNTIME,
					 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
					 "%s: unable to retrieve attribute value data file extent: %d.",
					 function,
					 extent_index + 1 );

					return( -1 );
				}
				if( next_file_extent == NULL )
				{
					libcerror_error_set(
					 error,
					 LIBCERROR_ERROR_DOMAIN_RUNTIME,
					 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
					 "%s: missing attribute value data file extent: %d.",
					 function,
					 extent_index + 1 );

					return( -1 );
				}
				if( next_file_extent->logical_offset < file_extent->logical_offset )
				{
					libcerror_error_set(
					 error,
					 LIBCERROR_ERROR_DOMAIN_RUNTIME,
					 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
					 "%s: invalid attribute value data file extents ordering: %" PRIu64 " < %" PRIu64 ".",
					 function,
					 next_file_extent->logical_offset,
					 file_extent->logical_offset );

					return( -1 );
				}
				file_extent->data_size = next_file_extent->logical_offset - file_extent->logical_offset;
			}
				else
				{
					if( file_extent->logical_offset > data_stream_size )
					{
						/* On sealed volumes (FILE_EXTENT2), the attribute value data stream "used size"
						 * can be smaller than the last extent logical offset (e.g. resource fork backed
						 * compressed files). Defer sizing until extent2 resolution.
						 */
							if( file_extent->file_system_data_type == LIBFSAPFS_FILE_SYSTEM_DATA_TYPE_FILE_EXTENT2 )
							{
								file_extent->data_size = 0;
							}
						else
						{
							libcerror_error_set(
							 error,
							 LIBCERROR_ERROR_DOMAIN_RUNTIME,
							 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
							 "%s: invalid last attribute value data file extent logical offset value out of bounds (logical offset: %" PRIu64 ", data size: %" PRIu64 ", type: 0x%02" PRIx8 ", flags: 0x%02" PRIx8 ").",
							 function,
							 file_extent->logical_offset,
							 data_stream_size,
							 file_extent->file_system_data_type,
							 file_extent->logical_address_flags );

							return( -1 );
						}
					}
					else
					{
						file_extent->data_size = data_stream_size - file_extent->logical_offset;
					}
				}
				if( file_extent->logical_offset >= data_stream_size )
				{
					file_extent->data_size = 0;
				}
				else if( file_extent->data_size > ( data_stream_size - file_extent->logical_offset ) )
				{
					file_extent->data_size = data_stream_size - file_extent->logical_offset;
				}
			}

			/* Remove unused/preallocated extents that are beyond the xattr used size. */
			for( extent_index = number_of_extents - 1;
			     extent_index >= 0;
			     extent_index-- )
			{
				libfsapfs_file_extent_t *removed_file_extent = NULL;

				if( libcdata_array_get_entry_by_index(
				     attribute_values->value_data_file_extents,
				     extent_index,
				     (intptr_t **) &file_extent,
				     error ) != 1 )
				{
					libcerror_error_set(
					 error,
					 LIBCERROR_ERROR_DOMAIN_RUNTIME,
					 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
					 "%s: unable to retrieve attribute value data file extent: %d.",
					 function,
					 extent_index );

					return( -1 );
				}
				if( ( file_extent != NULL )
				 && ( file_extent->data_size == 0 ) )
				{
					if( libcdata_array_remove_entry(
					     attribute_values->value_data_file_extents,
					     extent_index,
					     (intptr_t **) &removed_file_extent,
					     error ) != 1 )
					{
						libcerror_error_set(
						 error,
						 LIBCERROR_ERROR_DOMAIN_RUNTIME,
						 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
						 "%s: unable to remove unused attribute value data file extent: %d.",
						 function,
						 extent_index );

						return( -1 );
					}
					if( removed_file_extent != NULL )
					{
						libfsapfs_file_extent_free(
						 &removed_file_extent,
						 NULL );
					}
				}
			}
			if( libcdata_array_get_number_of_entries(
			     attribute_values->value_data_file_extents,
			     &number_of_extents,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
				 "%s: unable to retrieve number of attribute value data file extents.",
				 function );

				return( -1 );
			}
			if( requires_sealed_extent_tree != 0 )
			{
			result = libfsapfs_file_system_btree_get_sealed_extent_tree(
			          file_system_btree,
			          file_io_handle,
			          &sealed_extent_tree,
			          error );

			if( result != 1 )
			{
				if( result == 0 )
				{
					libcerror_error_set(
					 error,
					 LIBCERROR_ERROR_DOMAIN_RUNTIME,
					 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
					 "%s: missing sealed extent tree root node block number.",
					 function );
				}
				else
				{
					libcerror_error_set(
					 error,
					 LIBCERROR_ERROR_DOMAIN_RUNTIME,
					 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
					 "%s: unable to retrieve sealed extent tree.",
					 function );
				}
				return( -1 );
			}
			for( extent_index = 0;
			     extent_index < number_of_extents;
			     extent_index++ )
			{
				uint64_t physical_block_number = 0;
				uint64_t maximum_data_size     = 0;

				if( libcdata_array_get_entry_by_index(
				     attribute_values->value_data_file_extents,
				     extent_index,
				     (intptr_t **) &file_extent,
				     error ) != 1 )
				{
					libcerror_error_set(
					 error,
					 LIBCERROR_ERROR_DOMAIN_RUNTIME,
					 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
					 "%s: unable to retrieve attribute value data file extent: %d.",
					 function,
					 extent_index );

					return( -1 );
				}
				if( ( file_extent == NULL )
				 || ( file_extent->file_system_data_type != LIBFSAPFS_FILE_SYSTEM_DATA_TYPE_FILE_EXTENT2 )
				 || ( file_extent->data_size == 0 ) )
				{
					continue;
				}
				result = libfsapfs_sealed_extent_tree_lookup(
				          sealed_extent_tree,
				          file_extent->identifier,
				          file_extent->logical_offset,
				          &physical_block_number,
				          &maximum_data_size,
				          error );

				if( result != 1 )
				{
					if( result == 0 )
					{
						libcerror_error_set(
						 error,
						 LIBCERROR_ERROR_DOMAIN_RUNTIME,
						 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
						 "%s: sealed extent mapping not found for attribute value data file extent2: %d (identifier: %" PRIu64 ", logical offset: %" PRIu64 ").",
						 function,
						 extent_index,
						 file_extent->identifier,
						 file_extent->logical_offset );
					}
					else
					{
						libcerror_error_set(
						 error,
						 LIBCERROR_ERROR_DOMAIN_RUNTIME,
						 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
						 "%s: unable to look up sealed extent mapping for attribute value data file extent2: %d.",
						 function,
						 extent_index );
					}
					return( -1 );
				}
				file_extent->physical_block_number = physical_block_number;

				if( maximum_data_size < file_extent->data_size )
				{
					file_extent->data_size = maximum_data_size;
				}
			}
			}
			if( libfsapfs_data_stream_initialize_from_file_extents(
			     data_stream,
			     io_handle,
			     encryption_context,
			     attribute_values->value_data_file_extents,
			     attribute_values->value_data_size,
			     0,
			     error ) != 1 )
			{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
			 "%s: unable to create value data stream from file extents.",
			 function );

			return( -1 );
			}
		}
		else if( ( attribute_values->flags & 0x0002 ) != 0 )
	{
		if( libfsapfs_data_stream_initialize_from_data(
		     data_stream,
		     attribute_values->value_data,
		     (size_t) attribute_values->value_data_size,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
			 "%s: unable to create value data stream from data.",
			 function );

			return( -1 );
		}
	}
	return( 1 );
}
