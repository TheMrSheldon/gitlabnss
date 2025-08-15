#ifndef ERROR_HPP
#define ERROR_HPP

enum class Error {
	Ok = 0,
	NotFound,
	AuthenticationError,
	ServerError,
	ResponseFormatError,
	GenericError,
};

#endif