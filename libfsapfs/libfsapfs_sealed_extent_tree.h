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

#if !defined( _LIBFSAPFS_SEALED_EXTENT_TREE_H )
#define _LIBFSAPFS_SEALED_EXTENT_TREE_H

#include <common.h>
#include <types.h>

#include "libfsapfs_io_handle.h"
#include "libfsapfs_libbfio.h"
#include "libfsapfs_libcerror.h"

#if defined( __cplusplus )
extern "C" {
#endif

typedef struct libfsapfs_sealed_extent_tree libfsapfs_sealed_extent_tree_t;

int libfsapfs_sealed_extent_tree_initialize(
     libfsapfs_sealed_extent_tree_t **sealed_extent_tree,
     libfsapfs_io_handle_t *io_handle,
     uint64_t root_node_block_number,
     libcerror_error_t **error );

int libfsapfs_sealed_extent_tree_free(
     libfsapfs_sealed_extent_tree_t **sealed_extent_tree,
     libcerror_error_t **error );

int libfsapfs_sealed_extent_tree_build(
     libfsapfs_sealed_extent_tree_t *sealed_extent_tree,
     libbfio_handle_t *file_io_handle,
     libcerror_error_t **error );

/* Looks up the sealed extent mapping for the specified data stream identifier and logical offset.
 * Returns 1 if successful, 0 if not found or -1 on error.
 */
int libfsapfs_sealed_extent_tree_lookup(
     libfsapfs_sealed_extent_tree_t *sealed_extent_tree,
     uint64_t identifier,
     uint64_t logical_offset,
     uint64_t *physical_block_number,
     uint64_t *maximum_data_size,
     libcerror_error_t **error );

#if defined( __cplusplus )
}
#endif

#endif /* !defined( _LIBFSAPFS_SEALED_EXTENT_TREE_H ) */
