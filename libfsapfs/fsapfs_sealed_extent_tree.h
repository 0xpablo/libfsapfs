/*
 * The APFS sealed extent tree definitions
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

#if !defined( _FSAPFS_SEALED_EXTENT_TREE_H )
#define _FSAPFS_SEALED_EXTENT_TREE_H

#include <common.h>
#include <types.h>

#if defined( __cplusplus )
extern "C" {
#endif

typedef struct fsapfs_sealed_extent_btree_key fsapfs_sealed_extent_btree_key_t;

struct fsapfs_sealed_extent_btree_key
{
	/* The data stream identifier
	 * Consists of 8 bytes
	 */
	uint8_t identifier[ 8 ];

	/* The logical offset
	 * Consists of 8 bytes
	 */
	uint8_t logical_offset[ 8 ];
};

typedef struct fsapfs_sealed_extent_btree_value fsapfs_sealed_extent_btree_value_t;

struct fsapfs_sealed_extent_btree_value
{
	/* The size
	 * Consists of 4 bytes
	 */
	uint8_t size[ 4 ];

	/* Unknown
	 * Consists of 4 bytes
	 */
	uint8_t unknown1[ 4 ];

	/* The physical block number
	 * Consists of 8 bytes
	 */
	uint8_t physical_block_number[ 8 ];
};

#if defined( __cplusplus )
}
#endif

#endif /* !defined( _FSAPFS_SEALED_EXTENT_TREE_H ) */

