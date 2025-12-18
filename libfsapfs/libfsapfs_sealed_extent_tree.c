/*
 * Sealed extent tree functions
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

#include "libfsapfs_btree_entry.h"
#include "libfsapfs_btree_node.h"
#include "libfsapfs_definitions.h"
#include "libfsapfs_io_handle.h"
#include "libfsapfs_libbfio.h"
#include "libfsapfs_libcerror.h"
#include "libfsapfs_libcnotify.h"
#include "libfsapfs_sealed_extent_tree.h"

#include "fsapfs_sealed_extent_tree.h"

typedef struct libfsapfs_sealed_extent_tree_entry libfsapfs_sealed_extent_tree_entry_t;

struct libfsapfs_sealed_extent_tree_entry
{
	uint64_t identifier;
	uint64_t logical_offset;
	uint32_t data_size;
	uint64_t physical_block_number;
};

struct libfsapfs_sealed_extent_tree
{
	libfsapfs_io_handle_t *io_handle;
	uint64_t root_node_block_number;

	libfsapfs_sealed_extent_tree_entry_t *entries;
	size_t number_of_entries;
	size_t entries_capacity;

	int is_built;
};

static int libfsapfs_sealed_extent_tree_entry_compare(
            const void *first,
            const void *second )
{
	const libfsapfs_sealed_extent_tree_entry_t *entry1 = (const libfsapfs_sealed_extent_tree_entry_t *) first;
	const libfsapfs_sealed_extent_tree_entry_t *entry2 = (const libfsapfs_sealed_extent_tree_entry_t *) second;

	if( entry1->identifier < entry2->identifier )
	{
		return( -1 );
	}
	else if( entry1->identifier > entry2->identifier )
	{
		return( 1 );
	}
	if( entry1->logical_offset < entry2->logical_offset )
	{
		return( -1 );
	}
	else if( entry1->logical_offset > entry2->logical_offset )
	{
		return( 1 );
	}
	return( 0 );
}

static int libfsapfs_sealed_extent_tree_append_entry(
            libfsapfs_sealed_extent_tree_t *sealed_extent_tree,
            uint64_t identifier,
            uint64_t logical_offset,
            uint32_t data_size,
            uint64_t physical_block_number,
            libcerror_error_t **error )
{
	static char *function = "libfsapfs_sealed_extent_tree_append_entry";
	libfsapfs_sealed_extent_tree_entry_t *new_entries = NULL;
	size_t new_capacity                              = 0;

	if( sealed_extent_tree == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid sealed extent tree.",
		 function );

		return( -1 );
	}
	if( sealed_extent_tree->number_of_entries >= sealed_extent_tree->entries_capacity )
	{
		new_capacity = sealed_extent_tree->entries_capacity;

		if( new_capacity == 0 )
		{
			new_capacity = 1024;
		}
		else if( new_capacity > (size_t) ( (size_t) SSIZE_MAX / 2 ) )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
			 "%s: invalid entries capacity value out of bounds.",
			 function );

			return( -1 );
		}
		else
		{
			new_capacity *= 2;
		}
		new_entries = (libfsapfs_sealed_extent_tree_entry_t *) memory_reallocate(
		                                                           sealed_extent_tree->entries,
		                                                           sizeof( libfsapfs_sealed_extent_tree_entry_t ) * new_capacity );

		if( new_entries == NULL )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_MEMORY,
			 LIBCERROR_MEMORY_ERROR_INSUFFICIENT,
			 "%s: unable to resize entries.",
			 function );

			return( -1 );
		}
		sealed_extent_tree->entries          = new_entries;
		sealed_extent_tree->entries_capacity = new_capacity;
	}
	sealed_extent_tree->entries[ sealed_extent_tree->number_of_entries ].identifier            = identifier;
	sealed_extent_tree->entries[ sealed_extent_tree->number_of_entries ].logical_offset        = logical_offset;
	sealed_extent_tree->entries[ sealed_extent_tree->number_of_entries ].data_size             = data_size;
	sealed_extent_tree->entries[ sealed_extent_tree->number_of_entries ].physical_block_number = physical_block_number;

	sealed_extent_tree->number_of_entries++;

	return( 1 );
}

static int libfsapfs_sealed_extent_tree_read_node(
            libfsapfs_sealed_extent_tree_t *sealed_extent_tree,
            libbfio_handle_t *file_io_handle,
            uint64_t node_block_number,
            int recursion_depth,
            libcerror_error_t **error )
{
	uint8_t *node_data                   = NULL;
	libfsapfs_btree_node_t *node         = NULL;
	libfsapfs_btree_entry_t *entry       = NULL;
	static char *function                = "libfsapfs_sealed_extent_tree_read_node";
	size_t node_data_size                = 0;
	ssize_t read_count                   = 0;
	uint64_t child_block_number          = 0;
	uint64_t identifier                  = 0;
	uint64_t logical_offset              = 0;
	uint64_t physical_block_number       = 0;
	uint32_t data_size                   = 0;
	int entry_index                      = 0;
	int is_leaf_node                     = 0;
	int number_of_entries                = 0;

	if( sealed_extent_tree == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid sealed extent tree.",
		 function );

		return( -1 );
	}
	if( sealed_extent_tree->io_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
		 "%s: invalid sealed extent tree - missing IO handle.",
		 function );

		return( -1 );
	}
	if( recursion_depth > LIBFSAPFS_MAXIMUM_BTREE_NODE_RECURSION_DEPTH )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: maximum recursion depth exceeded.",
		 function );

		return( -1 );
	}
	if( sealed_extent_tree->io_handle->block_size == 0 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid block size value out of bounds.",
		 function );

		return( -1 );
	}
	node_data_size = (size_t) sealed_extent_tree->io_handle->block_size;

	node_data = (uint8_t *) memory_allocate(
	                         sizeof( uint8_t ) * node_data_size );

	if( node_data == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_INSUFFICIENT,
		 "%s: unable to create node data buffer.",
		 function );

		return( -1 );
	}
	read_count = libbfio_handle_read_buffer_at_offset(
	              file_io_handle,
	              node_data,
	              node_data_size,
	              (off64_t) ( node_block_number * sealed_extent_tree->io_handle->block_size ),
	              error );

	if( read_count != (ssize_t) node_data_size )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_READ_FAILED,
		 "%s: unable to read node data at block: %" PRIu64 ".",
		 function,
		 node_block_number );

		goto on_error;
	}
	if( libfsapfs_btree_node_initialize(
	     &node,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_INITIALIZE_FAILED,
		 "%s: unable to create B-tree node.",
		 function );

		goto on_error;
	}
	if( libfsapfs_btree_node_read_data(
	     node,
	     node_data,
	     node_data_size,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_READ_FAILED,
		 "%s: unable to read B-tree node data at block: %" PRIu64 ".",
		 function,
		 node_block_number );

		goto on_error;
	}
	is_leaf_node = libfsapfs_btree_node_is_leaf_node(
	                node,
	                error );

	if( is_leaf_node == -1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to determine if node is a leaf node.",
		 function );

		goto on_error;
	}
	if( libfsapfs_btree_node_get_number_of_entries(
	     node,
	     &number_of_entries,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve number of entries from node.",
		 function );

		goto on_error;
	}
	for( entry_index = 0;
	     entry_index < number_of_entries;
	     entry_index++ )
	{
		if( libfsapfs_btree_node_get_entry_by_index(
		     node,
		     entry_index,
		     &entry,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve entry: %d from node.",
			 function,
			 entry_index );

			goto on_error;
		}
		if( entry == NULL )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
			 "%s: missing entry: %d from node.",
			 function,
			 entry_index );

			goto on_error;
		}
		if( entry->key_data == NULL )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
			 "%s: missing key data in entry: %d.",
			 function,
			 entry_index );

			goto on_error;
		}
		if( entry->key_data_size < sizeof( fsapfs_sealed_extent_btree_key_t ) )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
			 "%s: invalid key data size value out of bounds.",
			 function );

			goto on_error;
		}
		byte_stream_copy_to_uint64_little_endian(
		 ( (fsapfs_sealed_extent_btree_key_t *) entry->key_data )->identifier,
		 identifier );

		byte_stream_copy_to_uint64_little_endian(
		 ( (fsapfs_sealed_extent_btree_key_t *) entry->key_data )->logical_offset,
		 logical_offset );

		if( is_leaf_node == 0 )
		{
			if( entry->value_data == NULL )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
				 "%s: missing value data in branch entry: %d.",
				 function,
				 entry_index );

				goto on_error;
			}
			if( entry->value_data_size < 8 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
				 "%s: invalid value data size value out of bounds.",
				 function );

				goto on_error;
			}
			byte_stream_copy_to_uint64_little_endian(
			 entry->value_data,
			 child_block_number );

			if( libfsapfs_sealed_extent_tree_read_node(
			     sealed_extent_tree,
			     file_io_handle,
			     child_block_number,
			     recursion_depth + 1,
			     error ) != 1 )
			{
				libcerror_error_set(
				 error,
				 LIBCERROR_ERROR_DOMAIN_IO,
				 LIBCERROR_IO_ERROR_READ_FAILED,
				 "%s: unable to read sub node: %" PRIu64 ".",
				 function,
				 child_block_number );

				goto on_error;
			}
			continue;
		}
		if( entry->value_data == NULL )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
			 "%s: missing value data in leaf entry: %d.",
			 function,
			 entry_index );

			goto on_error;
		}
		if( entry->value_data_size < sizeof( fsapfs_sealed_extent_btree_value_t ) )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
			 "%s: invalid value data size value out of bounds.",
			 function );

			goto on_error;
		}
		byte_stream_copy_to_uint32_little_endian(
		 ( (fsapfs_sealed_extent_btree_value_t *) entry->value_data )->size,
		 data_size );

		byte_stream_copy_to_uint64_little_endian(
		 ( (fsapfs_sealed_extent_btree_value_t *) entry->value_data )->physical_block_number,
		 physical_block_number );

		if( libfsapfs_sealed_extent_tree_append_entry(
		     sealed_extent_tree,
		     identifier,
		     logical_offset,
		     data_size,
		     physical_block_number,
		     error ) != 1 )
		{
			libcerror_error_set(
			 error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_APPEND_FAILED,
			 "%s: unable to append entry.",
			 function );

			goto on_error;
		}
	}
	libfsapfs_btree_node_free(
	 &node,
	 NULL );

	memory_free(
	 node_data );

	return( 1 );

on_error:
	if( node != NULL )
	{
		libfsapfs_btree_node_free(
		 &node,
		 NULL );
	}
	if( node_data != NULL )
	{
		memory_free(
		 node_data );
	}
	return( -1 );
}

/* Creates a sealed extent tree
 * Make sure the value sealed_extent_tree is referencing, is set to NULL
 * Returns 1 if successful or -1 on error
 */
int libfsapfs_sealed_extent_tree_initialize(
     libfsapfs_sealed_extent_tree_t **sealed_extent_tree,
     libfsapfs_io_handle_t *io_handle,
     uint64_t root_node_block_number,
     libcerror_error_t **error )
{
	static char *function = "libfsapfs_sealed_extent_tree_initialize";

	if( sealed_extent_tree == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid sealed extent tree.",
		 function );

		return( -1 );
	}
	if( *sealed_extent_tree != NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_ALREADY_SET,
		 "%s: invalid sealed extent tree value already set.",
		 function );

		return( -1 );
	}
	if( io_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid IO handle.",
		 function );

		return( -1 );
	}
	*sealed_extent_tree = memory_allocate_structure(
	                      libfsapfs_sealed_extent_tree_t );

	if( *sealed_extent_tree == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_INSUFFICIENT,
		 "%s: unable to create sealed extent tree.",
		 function );

		goto on_error;
	}
	if( memory_set(
	     *sealed_extent_tree,
	     0,
	     sizeof( libfsapfs_sealed_extent_tree_t ) ) == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_SET_FAILED,
		 "%s: unable to clear sealed extent tree.",
		 function );

		goto on_error;
	}
	( *sealed_extent_tree )->io_handle             = io_handle;
	( *sealed_extent_tree )->root_node_block_number = root_node_block_number;
	( *sealed_extent_tree )->is_built              = 0;

	return( 1 );

on_error:
	if( *sealed_extent_tree != NULL )
	{
		memory_free(
		 *sealed_extent_tree );

		*sealed_extent_tree = NULL;
	}
	return( -1 );
}

/* Frees a sealed extent tree
 * Returns 1 if successful or -1 on error
 */
int libfsapfs_sealed_extent_tree_free(
     libfsapfs_sealed_extent_tree_t **sealed_extent_tree,
     libcerror_error_t **error )
{
	static char *function = "libfsapfs_sealed_extent_tree_free";

	if( sealed_extent_tree == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid sealed extent tree.",
		 function );

		return( -1 );
	}
	if( *sealed_extent_tree != NULL )
	{
		if( ( *sealed_extent_tree )->entries != NULL )
		{
			memory_free(
			 ( *sealed_extent_tree )->entries );

			( *sealed_extent_tree )->entries = NULL;
		}
		memory_free(
		 *sealed_extent_tree );

		*sealed_extent_tree = NULL;
	}
	return( 1 );
}

/* Builds the sealed extent tree index
 * Returns 1 if successful or -1 on error
 */
int libfsapfs_sealed_extent_tree_build(
     libfsapfs_sealed_extent_tree_t *sealed_extent_tree,
     libbfio_handle_t *file_io_handle,
     libcerror_error_t **error )
{
	static char *function = "libfsapfs_sealed_extent_tree_build";

	if( sealed_extent_tree == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid sealed extent tree.",
		 function );

		return( -1 );
	}
	if( sealed_extent_tree->is_built != 0 )
	{
		return( 1 );
	}
	if( sealed_extent_tree->root_node_block_number == 0 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
		 "%s: missing sealed extent tree root node block number.",
		 function );

		return( -1 );
	}
	if( libfsapfs_sealed_extent_tree_read_node(
	     sealed_extent_tree,
	     file_io_handle,
	     sealed_extent_tree->root_node_block_number,
	     0,
	     error ) != 1 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_READ_FAILED,
		 "%s: unable to read sealed extent tree root node.",
		 function );

		return( -1 );
	}
	if( sealed_extent_tree->number_of_entries > 1 )
	{
		qsort(
		 sealed_extent_tree->entries,
		 sealed_extent_tree->number_of_entries,
		 sizeof( libfsapfs_sealed_extent_tree_entry_t ),
		 libfsapfs_sealed_extent_tree_entry_compare );
	}
	sealed_extent_tree->is_built = 1;

	return( 1 );
}

/* Looks up the sealed extent mapping for the specified data stream identifier and logical offset.
 * Returns 1 if successful, 0 if not found or -1 on error.
 */
int libfsapfs_sealed_extent_tree_lookup(
     libfsapfs_sealed_extent_tree_t *sealed_extent_tree,
     uint64_t identifier,
     uint64_t logical_offset,
     uint64_t *physical_block_number,
     uint64_t *maximum_data_size,
     libcerror_error_t **error )
{
	static char *function = "libfsapfs_sealed_extent_tree_lookup";
	size_t lower          = 0;
	size_t upper          = 0;
	size_t index          = 0;
	uint64_t extent_offset = 0;
	uint64_t extent_size   = 0;
	uint64_t extent_phys   = 0;

	if( sealed_extent_tree == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid sealed extent tree.",
		 function );

		return( -1 );
	}
	if( sealed_extent_tree->is_built == 0 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
		 "%s: sealed extent tree is not built.",
		 function );

		return( -1 );
	}
	if( physical_block_number == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid physical block number.",
		 function );

		return( -1 );
	}
	if( maximum_data_size == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid maximum data size.",
		 function );

		return( -1 );
	}
	*physical_block_number = 0;
	*maximum_data_size     = 0;

	if( sealed_extent_tree->number_of_entries == 0 )
	{
#if defined( HAVE_DEBUG_OUTPUT )
		if( libcnotify_verbose != 0 )
		{
			libcnotify_printf(
			 "%s: no entries (lookup identifier: %" PRIu64 ", logical offset: %" PRIu64 ").\n",
			 function,
			 identifier,
			 logical_offset );
		}
#endif
		return( 0 );
	}
	/* Find the first entry greater than the lookup key (identifier, logical_offset). */
	lower = 0;
	upper = sealed_extent_tree->number_of_entries;

	while( lower < upper )
	{
		index = lower + ( ( upper - lower ) / 2 );

		if( ( sealed_extent_tree->entries[ index ].identifier < identifier )
		 || ( ( sealed_extent_tree->entries[ index ].identifier == identifier )
		  &&  ( sealed_extent_tree->entries[ index ].logical_offset <= logical_offset ) ) )
		{
			lower = index + 1;
		}
		else
		{
			upper = index;
		}
	}
	if( lower == 0 )
	{
#if defined( HAVE_DEBUG_OUTPUT )
		if( libcnotify_verbose != 0 )
		{
			libcnotify_printf(
			 "%s: lookup miss before first entry (lookup identifier: %" PRIu64 ", logical offset: %" PRIu64 ").\n",
			 function,
			 identifier,
			 logical_offset );
		}
#endif
		return( 0 );
	}
	index = lower - 1;

	if( sealed_extent_tree->entries[ index ].identifier != identifier )
	{
#if defined( HAVE_DEBUG_OUTPUT )
		if( libcnotify_verbose != 0 )
		{
			libcnotify_printf(
			 "%s: lookup miss (nearest identifier: %" PRIu64 ", lookup identifier: %" PRIu64 ", logical offset: %" PRIu64 ").\n",
			 function,
			 sealed_extent_tree->entries[ index ].identifier,
			 identifier,
			 logical_offset );
		}
#endif
		return( 0 );
	}
	extent_offset = sealed_extent_tree->entries[ index ].logical_offset;
	extent_size   = (uint64_t) sealed_extent_tree->entries[ index ].data_size;
	extent_phys   = sealed_extent_tree->entries[ index ].physical_block_number;

	if( ( logical_offset < extent_offset )
	 || ( ( logical_offset - extent_offset ) >= extent_size ) )
	{
#if defined( HAVE_DEBUG_OUTPUT )
		if( libcnotify_verbose != 0 )
		{
			libcnotify_printf(
			 "%s: lookup miss within identifier %" PRIu64 " (extent offset: %" PRIu64 ", extent size: %" PRIu64 ", lookup logical offset: %" PRIu64 ").\n",
			 function,
			 identifier,
			 extent_offset,
			 extent_size,
			 logical_offset );
		}
#endif
		return( 0 );
	}
	if( sealed_extent_tree->io_handle == NULL )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_MISSING,
		 "%s: invalid sealed extent tree - missing IO handle.",
		 function );

		return( -1 );
	}
	if( sealed_extent_tree->io_handle->block_size == 0 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid block size value out of bounds.",
		 function );

		return( -1 );
	}
	if( ( ( logical_offset - extent_offset ) % sealed_extent_tree->io_handle->block_size ) != 0 )
	{
		libcerror_error_set(
		 error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid logical offset alignment.",
		 function );

		return( -1 );
	}
	*physical_block_number = extent_phys + ( ( logical_offset - extent_offset ) / sealed_extent_tree->io_handle->block_size );
	*maximum_data_size     = extent_size - ( logical_offset - extent_offset );

	return( 1 );
}
