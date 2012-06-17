#ifndef _CHERRY_WRAPPER_OPENCL_HPP_INCLUDED_
#define _CHERRY_WRAPPER_OPENCL_HPP_INCLUDED_

#ifndef _CHERRY_WRAPPER_WRAPPER_HPP_INCLUDED_
#	error "#include <cherry/wrapper/wrapper.hpp> instead"
#endif

#ifdef __APPLE__
#	include <OpenCL/cl.h>
#else
#	include <CL/cl.h>
#endif

#include <cherry/except/OpenCLError.hpp>

#define CHECK_OPENCL_RETURN(returnCode) \
	do { \
		cl_int value = (returnCode); \
		if (copy != CL_SUCCESS) { \
			RAISE(OpenCLError, value); \
		} \
	} while (false)


namespace cherry { namespace wrapper {

inline cl_program clCreateProgramWithSource_(cl_context context,
		cl_uint count, const char** strings, const size_t* lengths) {
	cl_int returnCode = 0;
	cl_program program = clCreateProgramWithSource(context, count, strings,
			lengths, &returnCode);
	CHECK_OPENCL_RETURN(returnCode);
	return program;
}

}} // namespace cherry::wrapper

#endif // _CHERRY_WRAPPER_OPENCL_HPP_INCLUDED_
