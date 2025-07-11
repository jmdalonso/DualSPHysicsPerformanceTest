#include <hip/hip_runtime.h>
// Set of macros pointing CUDA functions to their analogous HIP function.
#define hipDeviceSynchronize hipDeviceSynchronize
#define hipError_t hipError_t
#define hipError_t hipError_t
#define hipErrorInsufficientDriver hipErrorInsufficientDriver
#define hipErrorNoDevice hipErrorNoDevice
#define hipEvent_t hipEvent_t
#define hipEventCreate hipEventCreate
