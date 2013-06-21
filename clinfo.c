/* Copyright (c) 2013 Simon Leblanc

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <OpenCL/cl.h>


#define assert(assertion, ...) if (!(assertion)) { \
    fprintf(stderr, "Fatal: " __VA_ARGS__); \
    putc('\n', stderr); \
    exit(EXIT_FAILURE); \
}

const size_t indent_size = 2;
const size_t column_size = 40;


#define printHeader(indent, format, ...) do { \
    for (size_t n = 0; n < (indent) * indent_size; ++n) putchar(' '); \
    printf("\x1b[1m" format ":\x1b[0m\n\n", __VA_ARGS__); \
} while(0)


void print(size_t indent, const char* key, const char* value)
{
    size_t n;
    for (n = 0; n < indent * indent_size; ++n)
    {
        putchar(' ');
    }
    for (n += key ? printf("%s:", key) : 0; n < column_size; ++n)
    {
        putchar(' ');
    }
    printf(" %s\n", value);
}


typedef void (*Action)(size_t indent, const char* name, void* value, size_t size);


void printPlatformInfo(cl_platform_id platform, cl_platform_info param, const char* name, size_t indent, Action action)
{
    size_t buffer_size;
    cl_int status = clGetPlatformInfo(platform, param, 0, NULL, &buffer_size);
    assert(status == CL_SUCCESS, "Cannot get the size of the '%s' platform parameter.", name);
    
    char* buffer = malloc(buffer_size);
    status = clGetPlatformInfo(platform, param, buffer_size, buffer, NULL);
    assert(status == CL_SUCCESS, "Cannot get the '%s' platform parameter.", name);
    
    action(indent, name, buffer, buffer_size);
    
    free(buffer);
}


void printDeviceInfo(cl_device_id device, cl_device_info param, const char* name, size_t indent, Action action)
{
    size_t buffer_size;
    cl_int status = clGetDeviceInfo(device, param, 0, NULL, &buffer_size);
    assert(status == CL_SUCCESS, "Cannot get the size of the '%s' device parameter.", name);
    
    char* buffer = malloc(buffer_size);
    status = clGetDeviceInfo(device, param, buffer_size, buffer, NULL);
    assert(status == CL_SUCCESS, "Cannot get the '%s' device parameter.", name);
    
    action(indent, name, buffer, buffer_size);
    
    free(buffer);
}


void printString(size_t indent, const char* key, void* value, size_t size)
{
    print(indent, key, value);
}

void printDeviceType(size_t indent, const char* key, void* value, size_t size)
{
    const cl_device_type type = *((cl_device_type*)value);
	struct { cl_device_type type; const char* name; } list[] = {
		{ CL_DEVICE_TYPE_CPU, "CPU"},
		{ CL_DEVICE_TYPE_GPU, "GPU"},
		{ CL_DEVICE_TYPE_ACCELERATOR, "Accelerator"},
		{ CL_DEVICE_TYPE_DEFAULT, "Default"}
	};
	char buffer[32], *p = buffer;
	for (size_t i = 0; i < sizeof list / sizeof(list[0]); ++i)
	{
		if (type & list[i].type)
		{
			p += sprintf(p, "%s | ", list[i].name);
		}
	}
	if (p == buffer)
	{
		print(indent, key, "Unknown");
	}
	else
	{
		*(p - 3) = '\0';
		print(indent, key, buffer);
	}
}

void printBool(size_t indent, const char* key, void* value, size_t size)
{
    if (*((cl_bool*)value))
        print(indent, key, "Yes");
    else
        print(indent, key, "No");
}

void printUint(size_t indent, const char* key, void* value, size_t size)
{
    const cl_uint num = *((cl_uint*)value);
    char buffer[(num > 0 ? lrint(log10(num)) + 1 : 1) + 1];
    sprintf(buffer, "%u", num);
    print(indent, key, buffer);
}

void printUlong(size_t indent, const char* key, void* value, size_t size)
{
    const cl_ulong num = *((cl_ulong*)value);
    char buffer[(num > 0 ? lrint(log10(num)) + 1 : 1) + 1];
    sprintf(buffer, "%llu", num);
    print(indent, key, buffer);
}

void printSize(size_t indent, const char* key, void* value, size_t size)
{
    const size_t num = *((size_t*)value);
    char buffer[(num > 0 ? lrint(log10(num)) + 1 : 1) + 1];
    sprintf(buffer, "%zu", num);
    print(indent, key, buffer);
}

void printMemSize(size_t indent, const char* key, void* value, size_t size)
{
    const cl_ulong mem_size = *((cl_ulong*)value);
    if (mem_size == 0)
    {
        print(indent, key, "0 B");
        return;
    }
    char buffer[((mem_size >> 40) > 0 ? lrint(log10(mem_size >> 40)) + 1 : 1) + 38];
    int num, n = 0;
    if ((num = mem_size >> 40)) n += sprintf(&buffer[n], "%d TB ", num);
    if ((num = mem_size >> 30 & 1023)) n += sprintf(&buffer[n], "%d GB ", num);
    if ((num = mem_size >> 20 & 1023)) n += sprintf(&buffer[n], "%d MB ", num);
    if ((num = mem_size >> 10 & 1023)) n += sprintf(&buffer[n], "%d kB ", num);
    if ((num = mem_size & 1023)) n+= sprintf(&buffer[n], "%d B", num);
    print(indent, key, buffer);
}

void printDimensions(size_t indent, const char* key, void* value, size_t size)
{
    const size_t ndims = size / sizeof(size_t);
    const size_t *dims = *((size_t(*)[])value);
	size_t buffer_size = 1;
	for (size_t i = 0; i < ndims; ++i)
	{
		buffer_size += (dims[i] > 0 ? lrint(log10(dims[i])) + 1 : 1) + 2;
	}
	char buffer[buffer_size], *p = buffer;
	*p++ = '(';
	for (size_t i = 0; i < ndims; ++i)
	{
		p += sprintf(p, "%zu, ", dims[i]);
	}
	if (p + 1 > buffer)
	{
		*(p - 2) = ')';
		*(p - 1) = '\0';
	}
	else
	{
		*p++ = ')';
		*p++ = '\0';
	}
	print(indent, key, buffer);
}

void printExtensions(size_t indent, const char* key, void* value, size_t size)
{
    char* item = strtok(value, " ");
    print(indent, key, item);
    while ((item = strtok(NULL, " ")))
    {
        print(indent, NULL, item);
    }
}

void printFPConfig(size_t indent, const char* key, void* value, size_t size)
{
    const cl_device_fp_config config = *((cl_device_type*)value);
	struct { cl_device_fp_config flag; const char* name; } list[] = {
		{ CL_FP_DENORM, "Denorms" },
		{ CL_FP_INF_NAN, "Inf and NaNs" },
		{ CL_FP_ROUND_TO_NEAREST, "Round to nearest even rounding mode" },
		{ CL_FP_ROUND_TO_ZERO, "Round to zero rounding mode" },
		{ CL_FP_ROUND_TO_INF, "Round to +ve and -ve infinity rounding modes" },
		{ CL_FP_FMA, "IEEE754-2008 fused multiply-add" },
		{ CL_FP_SOFT_FLOAT, "Basic floating-point operations implemented in software" }
	};
	for (size_t i = 0; i < sizeof list / sizeof(list[0]); ++i)
	{
		if (config & list[i].flag)
		{
			print(indent, key, list[i].name);
			key = NULL;
		}
	}
	if (key)
	{
		print(indent, key, "Not supported");
	}
}

void printQueueProperties(size_t indent, const char* key, void* value, size_t size)
{
    const cl_command_queue_properties props = *((cl_command_queue_properties*)value);
	struct { cl_command_queue_properties flag; const char* name; } list[] = {
		{ CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, "Out of order execution" },
		{ CL_QUEUE_PROFILING_ENABLE, "Profiling" }
	};
	for (size_t i = 0; i < sizeof list / sizeof(list[0]); ++i)
	{
		if (props & list[i].flag)
		{
			print(indent, key, list[i].name);
			key = NULL;
		}
	}
	if (key)
	{
		print(indent, key, "Not supported");
	}
}

void printLocalMemType(size_t indent, const char* key, void* value, size_t size)
{
    if (*((cl_device_local_mem_type*)value) == CL_LOCAL)
        print(indent, key, "Local");
    else
        print(indent, key, "Global");
}

void printGlobalMemCacheType(size_t indent, const char* key, void* value, size_t size)
{
    switch (*((cl_device_mem_cache_type*)value))
	{
	case CL_NONE:
		print(indent, key, "None");
		break;
	case CL_READ_ONLY_CACHE:
		print(indent, key, "Read only");
		break;
	case CL_READ_WRITE_CACHE:
		print(indent, key, "Read write");
		break;
	default:
        print(indent, key, "Unknown");
	}
}

void printExecutionCapabilities(size_t indent, const char* key, void* value, size_t size)
{
    const cl_device_exec_capabilities props = *((cl_device_exec_capabilities*)value);
	struct { cl_device_exec_capabilities flag; const char* name; } list[] = {
		{ CL_EXEC_KERNEL, "OpenCL kernels" },
		{ CL_EXEC_NATIVE_KERNEL, "Native kernels" }
	};
	for (size_t i = 0; i < sizeof list / sizeof(list[0]); ++i)
	{
		if (props & list[i].flag)
		{
			print(indent, key, list[i].name);
			key = NULL;
		}
	}
	if (key)
	{
		print(indent, key, "Not supported");
	}
}


int main (int argc, char* argv[])
{
    cl_int status;

    cl_uint num_platforms;
    status = clGetPlatformIDs(0, NULL, &num_platforms);
    assert(status == CL_SUCCESS, "Cannot get the number of OpenCL platforms available.");

    cl_platform_id platforms[num_platforms];
    status = clGetPlatformIDs(num_platforms, platforms, NULL);
    assert(status == CL_SUCCESS, "Cannot get the list of OpenCL platforms.");

    size_t indent = 0;
    printHeader(indent, "%d OpenCL platform%s found", num_platforms, num_platforms > 1 ? "s" : "");
    ++indent;
    for (size_t i = 0; i < num_platforms; ++i)
    {
        printHeader(indent, "Platform #%zu", i);
        ++indent;

        printPlatformInfo(platforms[i], CL_PLATFORM_NAME, "Name", indent, printString);
        printPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, "Vendor", indent, printString);
        printPlatformInfo(platforms[i], CL_PLATFORM_VERSION, "Version", indent, printString);
        printPlatformInfo(platforms[i], CL_PLATFORM_PROFILE, "Profile", indent, printString);
        printPlatformInfo(platforms[i], CL_PLATFORM_EXTENSIONS, "Extensions", indent, printExtensions);

        putchar('\n');
        
        cl_uint num_devices;
        status = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices);
        assert(status == CL_SUCCESS, "Cannot get the number of OpenCL devices available on this platform.");

        cl_device_id devices[num_devices];
        status = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, num_devices, devices, NULL);
        assert(status == CL_SUCCESS, "Cannot get the list of OpenCL devices.");

        printHeader(indent, "%d OpenCL device%s found", num_devices, num_devices > 1 ? "s" : "");
        ++indent;
        
        for (size_t j = 0; j < num_devices; ++j)
        {
            printHeader(indent, "Device #%zu", j);
            ++indent;

            printDeviceInfo(devices[j], CL_DEVICE_NAME, "Name", indent, printString);
            printDeviceInfo(devices[j], CL_DEVICE_TYPE, "Type", indent, printDeviceType);
            printDeviceInfo(devices[j], CL_DEVICE_VENDOR, "Vendor", indent, printString);
            printDeviceInfo(devices[j], CL_DEVICE_VENDOR_ID, "Vendor ID", indent, printUint);
            printDeviceInfo(devices[j], CL_DEVICE_PROFILE, "Profile", indent, printString);
            printDeviceInfo(devices[j], CL_DEVICE_AVAILABLE, "Available", indent, printBool);
            printDeviceInfo(devices[j], CL_DEVICE_VERSION, "Version", indent, printString);
            printDeviceInfo(devices[j], CL_DRIVER_VERSION, "Driver version", indent, printString);
            
            // Compiler
            printDeviceInfo(devices[j], CL_DEVICE_COMPILER_AVAILABLE, "Compiler available", indent, printBool);
            printDeviceInfo(devices[j], CL_DEVICE_OPENCL_C_VERSION, "OpenCL C version", indent, printString);
            
            // Misc
            printDeviceInfo(devices[j], CL_DEVICE_ADDRESS_BITS, "Address space size", indent, printUint);
            printDeviceInfo(devices[j], CL_DEVICE_ENDIAN_LITTLE, "Little endian", indent, printBool);
            printDeviceInfo(devices[j], CL_DEVICE_ERROR_CORRECTION_SUPPORT, "Error correction support", indent, printBool);
            printDeviceInfo(devices[j], CL_DEVICE_HOST_UNIFIED_MEMORY, "Unified memory", indent, printBool);

            printDeviceInfo(devices[j], CL_DEVICE_MEM_BASE_ADDR_ALIGN, "Address alignment (bits)", indent, printUint);
            printDeviceInfo(devices[j], CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE, "Smallest alignment (bytes)", indent, printUint);
            printDeviceInfo(devices[j], CL_DEVICE_PROFILING_TIMER_RESOLUTION, "Resolution of timer (ns)", indent, printSize);
            printDeviceInfo(devices[j], CL_DEVICE_MAX_CLOCK_FREQUENCY, "Max clock frequency (MHz)", indent, printUint);
            printDeviceInfo(devices[j], CL_DEVICE_MAX_COMPUTE_UNITS, "Max compute units", indent, printUint);
            printDeviceInfo(devices[j], CL_DEVICE_MAX_CONSTANT_ARGS, "Max constant args", indent, printUint);
            printDeviceInfo(devices[j], CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, "Max constant buffer size", indent, printMemSize);
            printDeviceInfo(devices[j], CL_DEVICE_MAX_MEM_ALLOC_SIZE, "Max mem alloc size", indent, printMemSize);
            printDeviceInfo(devices[j], CL_DEVICE_MAX_PARAMETER_SIZE, "Max parameter size", indent, printSize);
			printDeviceInfo(devices[j], CL_DEVICE_QUEUE_PROPERTIES, "Command-queue supported props", indent, printQueueProperties);
			printDeviceInfo(devices[j], CL_DEVICE_EXECUTION_CAPABILITIES, "Execution capabilities", indent, printExecutionCapabilities);
            
            // Memory
            printDeviceInfo(devices[j], CL_DEVICE_GLOBAL_MEM_SIZE, "Global memory size", indent, printMemSize);
            printDeviceInfo(devices[j], CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, "Global memory cache size", indent, printMemSize);
            printDeviceInfo(devices[j], CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, "Global memory line cache size", indent, printUint);
            printDeviceInfo(devices[j], CL_DEVICE_LOCAL_MEM_SIZE, "Local memory size", indent, printMemSize);
			printDeviceInfo(devices[j], CL_DEVICE_LOCAL_MEM_TYPE, "Local memory type", indent, printLocalMemType);
			printDeviceInfo(devices[j], CL_DEVICE_GLOBAL_MEM_CACHE_TYPE, "Global memory cache type", indent, printGlobalMemCacheType);
            
            // Work group
            printDeviceInfo(devices[j], CL_DEVICE_MAX_WORK_GROUP_SIZE, "Max work group size", indent, printSize);
            printDeviceInfo(devices[j], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, "Max work item dimensions", indent, printUint);
			printDeviceInfo(devices[j], CL_DEVICE_MAX_WORK_ITEM_SIZES, "Max work item sizes", indent, printDimensions);
            
            // Images
            printDeviceInfo(devices[j], CL_DEVICE_IMAGE_SUPPORT, "Image support", indent, printBool);
            printDeviceInfo(devices[j], CL_DEVICE_IMAGE2D_MAX_HEIGHT, "Max 2D image height", indent, printSize);
            printDeviceInfo(devices[j], CL_DEVICE_IMAGE2D_MAX_WIDTH, "Max 2D image width", indent, printSize);
            printDeviceInfo(devices[j], CL_DEVICE_IMAGE3D_MAX_DEPTH, "Max 3D image depth", indent, printSize);
            printDeviceInfo(devices[j], CL_DEVICE_IMAGE3D_MAX_HEIGHT, "Max 3D image height", indent, printSize);
            printDeviceInfo(devices[j], CL_DEVICE_IMAGE3D_MAX_WIDTH, "Max 3D image width", indent, printSize);
            printDeviceInfo(devices[j], CL_DEVICE_MAX_READ_IMAGE_ARGS, "Max read image args", indent, printUint);
            printDeviceInfo(devices[j], CL_DEVICE_MAX_WRITE_IMAGE_ARGS, "Max write image args", indent, printUint);
            printDeviceInfo(devices[j], CL_DEVICE_MAX_SAMPLERS, "Max samplers", indent, printUint);
            
            // Vectors
            printDeviceInfo(devices[j], CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR, "Native vector width char", indent, printUint);
            printDeviceInfo(devices[j], CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT, "Native vector width short", indent, printUint);
            printDeviceInfo(devices[j], CL_DEVICE_NATIVE_VECTOR_WIDTH_INT, "Native vector width int", indent, printUint);
            printDeviceInfo(devices[j], CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG, "Native vector width long", indent, printUint);
            printDeviceInfo(devices[j], CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF, "Native vector width half", indent, printUint);
            printDeviceInfo(devices[j], CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT, "Native vector width float", indent, printUint);
            printDeviceInfo(devices[j], CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE, "Native vector width double", indent, printUint);
            printDeviceInfo(devices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, "Preferred vector width char", indent, printUint);
            printDeviceInfo(devices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, "Preferred vector width short", indent, printUint);
            printDeviceInfo(devices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, "Preferred vector width int", indent, printUint);
            printDeviceInfo(devices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, "Preferred vector width long", indent, printUint);
            printDeviceInfo(devices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF, "Preferred vector width half", indent, printUint);
            printDeviceInfo(devices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, "Preferred vector width float", indent, printUint);
            printDeviceInfo(devices[j], CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, "Preferred vector width double", indent, printUint);
            
			// Floating-points
#ifdef CL_DEVICE_HALF_FP_CONFIG
			printDeviceInfo(devices[j], CL_DEVICE_HALF_FP_CONFIG, "Half precision fp capability", indent, printFPConfig);
#endif
			printDeviceInfo(devices[j], CL_DEVICE_SINGLE_FP_CONFIG, "Single precision fp capability", indent, printFPConfig);
			printDeviceInfo(devices[j], CL_DEVICE_DOUBLE_FP_CONFIG, "Double precision fp capability", indent, printFPConfig);
			
            // Extensions
            printDeviceInfo(devices[j], CL_DEVICE_EXTENSIONS, "Extensions", indent, printExtensions);
            
            putchar('\n');
            indent -= 2;
        }
        indent -= 2;
    }

    return EXIT_SUCCESS;
}


// TODO: Support OpenCL 1.1
//
// http://www.khronos.org/registry/cl/sdk/1.1/docs/man/xhtml/
//
