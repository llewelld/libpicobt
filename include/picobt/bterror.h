/**
 * @file bterror.h
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
 * License along with Libpicobt. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * @brief Defines the error type and values used by Libpicobt.
 * 
 * Note: no associated C file.
 * Defines the error type bt_err_t and associated values.
 */

#ifndef __BTERROR_H__
#define __BTERROR_H__

/** Enumeration of error values that functions may return. */
enum _bt_err_t {
	/// Indicates no error.
	BT_SUCCESS = 0,
	
	// --- general ---
	
	/// Generic failure where no further information is available. Use sparingly.
	BT_ERR_UNKNOWN,
	/// For the impossible cases.
	BT_ERR_WTF,
	/// You passed an illegal argument value.
	BT_ERR_BAD_PARAM,
	
	// --- un-something ---
	
	/// No Bluetooth interface on this device.
	BT_ERR_UNSUPPORTED,
	/// You forgot to call {@link bt_init}.
	BT_ERR_UNINITIALISED,
	
	// --- inquiry ---
	
	/// No devices found in inquiry, or could not connect to device.
	BT_ERR_DEVICE_NOT_FOUND,
	/// No services found in inquiry, or could not connect to service.
	BT_ERR_SERVICE_NOT_FOUND,
	/// Signals the end of inquiry enumeration (so not really an error).
	BT_ERR_END_OF_ENUM,
	
	// --- SDP ---
	
	/// SDP parsing failed
	BT_ERR_SDP_BAD_RECORD,
	
	// --- device list ---
	
	/// You tried to load a file that doesn't exist.
	BT_ERR_FILE_NOT_FOUND,

	// --- connection ---

	/// During a connection attempt the socket couldn't be created.
	BT_ERR_ALLOCATING_SOCKET,

	/// The connection was attempted but could not be made.
	BT_ERR_CONNECTION_FAILURE,
	
// --- sockets ---
	BT_SOCKET_CLOSED,

	BT_ERR_TIMEOUT,


};

/**
 * This type is used to return error values by most Libpicobt functions.
 * @see enum _bt_err_t
 */
typedef enum _bt_err_t bt_err_t;

/**
 * ERRNO provides a platform-independant way to obtain an error number if
 * something fails. On Windows, it uses `WSAGetLastError`; on Linux, it uses
 * `errno`.
 */
#ifdef WINDOWS
#define ERRNO WSAGetLastError()
#else
#define ERRNO errno
#endif

#endif //__BTERROR_H__
