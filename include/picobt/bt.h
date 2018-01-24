/**
 * @file bt.h
 * 
 * @section LICENSE
 *
 * (C) Copyright Cambridge Authentication Ltd, 2017
 *
 * This file is part of libtt.
 *
 * Libpicobt is free software: you can redistribute it and\/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Libpicobt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public
 * License along with libpicobt. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * @brief Main header for libpicobt that includes everything else.
 * 
 * Include this to get all the Bluetooth functions in libpicobt.
 * This header also defines `WINDOWS` if it is not already defined and the
 * target platform is Windows, which is subsequently used to determine which
 * Bluetooth headers to include (see bttypes.h) and which libpicobt implementation
 * gets compiled.
 */

#ifndef __BT_H__
#define __BT_H__

/**
 * Defined if the compilation platform is Windows.
 * @see http://stackoverflow.com/questions/3213037/determine-if-linux-or-windows-in-c
 */
#if !defined(WINDOWS) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__))
#define WINDOWS
#endif

#include "btmain.h"
#include "btutil.h"
#include "btsdp.h"

#endif //__BT_H__
